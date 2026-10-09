/*
 * usb_processing.c
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 *
 *  The core of the USB module: the state machine, the two mailboxes, and the
 *  only place of the firmware that sends reports to the host.
 *  See usb_processing.h for the interface and the states
 */

#include "usb_internal.h"
#include "main.h"
#include <string.h>

// See the table in wheel_def.h
#define USB_HP_IRQ_PRIORITY 0
#define USB_LP_IRQ_PRIORITY 1

// Biggest report the ffb library can queue. So far it only sends the PID state
// report, which is 2 bytes
#define FFB_REPORT_MAX_SIZE 8

/*
 * Everything in this module runs in main, usb_callbacks.c included: TinyUSB
 * only calls back from tud_task(), which only usb_task() calls. So nothing is
 * volatile or protected.
 * The exception is usb_irq_handler(), which only passes the interrupt to TinyUSB.
 */
static struct {
  usb_state_t state;
  ffb_lib_t *ffb;
  void (*custom_task)(usb_state_t *state);

  // Mailbox of usb_set_input(). Only the latest input is kept
  wheel_input_t input;
  bool input_pending;

  // Mailbox of the ffb library: a report that already starts with its report
  // id. Only the latest one is kept
  uint8_t ffb_report[FFB_REPORT_MAX_SIZE];
  uint16_t ffb_report_len; // 0 when nothing is waiting
} usb;

static void init_peripheral(void) {
  __HAL_RCC_USB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = USB_DM_Pin | USB_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(USB_DM_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, USB_HP_IRQ_PRIORITY, 0);
  HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, USB_LP_IRQ_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
  HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

// Send callback of the ffb library, which calls it while it handles something
// the host sent, so inside usb_task(). The endpoint may be busy: the report is
// kept and usb_task() sends it as soon as it can
static bool queue_ffb_report(const uint8_t *report, uint16_t len) {
  if (len == 0 || len > FFB_REPORT_MAX_SIZE) {
    return false;
  }
  memcpy(usb.ffb_report, report, len);
  usb.ffb_report_len = len;
  return true;
}

// Hat switch value of the d-pad, whose 4 bits are the upper half of byte:
// 0 is up, then clockwise up to 7, 8 is released
static uint8_t hat_switch_from_msb(uint8_t byte) {
  uint8_t down = (byte & 0x10u) ? 1u : 0u;
  uint8_t left = (byte & 0x20u) ? 1u : 0u;
  uint8_t up = (byte & 0x40u) ? 1u : 0u;
  uint8_t right = (byte & 0x80u) ? 1u : 0u;

  // -1, 0 or +1 on each axis. Two opposite directions cancel each other
  int8_t dpadX = (int8_t)right - (int8_t)left;
  int8_t dpadY = (int8_t)down - (int8_t)up;

  if ((right && left)) dpadX = 0;
  if ((up && down)) dpadY = 0;

  // Rows: up, centered, down. Columns: left, centered, right
  // @formatter:off
  static const uint8_t hat_table[3][3] = {
    {7, 0, 1},
    {6, 8, 2},
    {5, 4, 3}
  };
    // @formatter:on
  return hat_table[dpadY + 1][dpadX + 1];
}

// Builds the input report from the mailbox and gives it to TinyUSB.
// Its layout is HIDDESC_G29_TEMPLATE (ffb_descriptor.h)
static bool send_input_report(void) {
  const wheel_input_t *input = &usb.input;

  uint8_t tx[REPORT_SIZE] = {0};
  // The d-pad (bits 4 to 7 of buttons) as a hat switch, then bits 0 to 3
  tx[0] |= 0x0F & hat_switch_from_msb((uint8_t)input->buttons);
  tx[0] |= 0xF0 & (input->buttons << 4);
  tx[1] = input->buttons >> 8;
  tx[2] = input->buttons >> 16;
  // One button per gear. Gear 0 sets bit 0, which is padding in the report
  tx[3] = 1U << input->gear;
  tx[4] = input->steering;
  tx[5] = input->steering >> 8;
  tx[6] = input->clutch;
  tx[7] = input->brake;
  tx[8] = input->throttle;

  return tud_hid_report(JOYSTICK_REPORT_ID, tx, REPORT_SIZE);
}

// Asks TinyUSB where the bus is instead of relying on its callbacks: none of
// them is called when the bus is reset (the host enumerating again, or the
// cable plugged back in)
static void update_state(void) {
  if (!tud_mounted()) {
    usb.state = USB_DETACHED;
  } else if (tud_suspended()) {
    usb.state = USB_SUSPENDED;
  } else if (usb.state == USB_DETACHED || usb.state == USB_SUSPENDED) {
    // Just configured or just resumed: the driver of the host has to read a
    // report before it is counted on
    usb.state = USB_WAIT_HOST;
  }
}

void usb_init(ffb_lib_t *ffb, void (*custom_task)(usb_state_t *state)) {
  memset(&usb, 0, sizeof(usb));
  usb.state = USB_DETACHED;
  usb.ffb = ffb;
  usb.custom_task = custom_task;
  ffb_set_send_report_callback(ffb, queue_ffb_report);

  init_peripheral();
  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

void usb_task(void) {
  // Handles what the host sent since the last call. The callbacks of
  // usb_callbacks.c all run inside this call
  tud_task();

  update_state();

  void (*custom_task)(usb_state_t *state) = usb.custom_task;
  if (custom_task != NULL) {
    custom_task(&usb.state);
  }

  // Not configured, suspended, or the last report is not read yet
  if (!tud_hid_ready()) {
    return;
  }

  switch (usb.state) {
  case USB_WAIT_HOST:
    // Being configured does not mean that the HID driver of the host runs.
    // Leave an input report on the endpoint: usb_on_report_sent() is called
    // once the host reads it
    send_input_report();
    break;
  case USB_READY:
    // One report at a time on the endpoint, the ffb library first
    if (usb.ffb_report_len > 0) {
      // Id 0: the report already starts with its id
      if (tud_hid_report(0, usb.ffb_report, usb.ffb_report_len)) {
        usb.ffb_report_len = 0;
      }
    } else if (usb.input_pending) {
      if (send_input_report()) {
        usb.input_pending = false;
      }
    }
    break;
  default:
    break;
  }
}

usb_state_t usb_state(void) {
  return usb.state;
}

void usb_set_input(const wheel_input_t *input) {
  usb.input = *input;
  usb.input_pending = true;
}

// Runs in the USB_HP and USB_LP interrupts (priority 0 and 1). TinyUSB only
// records the event, tud_task() does the work later
void usb_irq_handler(void) {
  tud_int_handler(BOARD_TUD_RHPORT);
}

/* FOR usb_callbacks.c */

void usb_on_report_sent(void) {
  // The host read a report: its HID driver is there
  if (usb.state == USB_WAIT_HOST) {
    usb.state = USB_READY;
  }
}

ffb_lib_t* usb_ffb(void) {
  return usb.ffb;
}
