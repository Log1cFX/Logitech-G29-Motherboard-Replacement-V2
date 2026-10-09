/*
 * usb_processing.c
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 *
 *  The core of the USB module : the state machine, the two mailboxes, and the
 *  only place of the firmware that sends reports to the host.
 *  The interface and the meaning of the states are in usb_processing.h
 */

#include "usb_internal.h"
#include "main.h"
#include <string.h>

// priorities of the two USB interrupts (see the table in wheel_def.h)
#define USB_HP_IRQ_PRIORITY 0
#define USB_LP_IRQ_PRIORITY 1

// biggest report the ffb library can give us to send. As of today, it only
// sends the PID state report, which is 2 bytes
#define FFB_REPORT_MAX_SIZE 8

/*
 * CONTEXTS (see the table in wheel_def.h)
 * Everything in this module runs in the main thread, the TinyUSB callbacks of
 * usb_callbacks.c included : TinyUSB only calls them from inside tud_task(),
 * and usb_task() is the only place that calls tud_task().
 * So nothing here is volatile and nothing has to be protected.
 * The only exception is usb_irq_handler(), which runs in the USB interrupts
 * and does nothing but pass the interrupt to TinyUSB.
 */
static struct {
  usb_state_t state;
  ffb_lib_t *ffb;
  // custom task set at initialization
  void (*custom_task)(usb_state_t *state);

  // the state of the controls, filled by usb_set_input().
  // Only the latest one is kept
  wheel_input_t input;
  bool input_pending;

  // a report that the ffb library wants to send. It already
  // starts with its report id. Only the latest one is kept
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

// Given to the ffb library, which calls it when it has a report for the host
// (it does it while it handles something the host sent, so inside usb_task).
// The report isn't sent from here : the endpoint may be busy. It is kept and
// usb_task() sends it as soon as it can
static bool queue_ffb_report(const uint8_t *report, uint16_t len) {
  if (len == 0 || len > FFB_REPORT_MAX_SIZE) {
    return false;
  }
  memcpy(usb.ffb_report, report, len);
  usb.ffb_report_len = len;
  return true;
}

/*
 * returns the encoded direction of the d_pad with the 4 most important bits
 * of the parameter byte
 */
static uint8_t hat_switch_from_msb(uint8_t byte) {
  /* Extract individual direction bits (boolean 0 / 1) */
  uint8_t down = (byte & 0x10u) ? 1u : 0u; // bit 4
  uint8_t left = (byte & 0x20u) ? 1u : 0u; // bit 5
  uint8_t up = (byte & 0x40u) ? 1u : 0u; // bit 6
  uint8_t right = (byte & 0x80u) ? 1u : 0u; // bit 7

  // Convert to signed axis values −1 / 0 / +1
  int8_t dpadX = (int8_t)right - (int8_t)left; // +1 = Right, −1 = Left
  int8_t dpadY = (int8_t)down - (int8_t)up; // +1 = Down,  −1 = Up

  // If both opposite directions are pressed, cancel the axis
  if ((right && left)) dpadX = 0;
  if ((up && down)) dpadY = 0;

  // Map (dpadX, dpadY) to the hat-switch look-up table
  static const uint8_t hat_table[3][3] = {
  /**/{7, 0, 1},/**/
  /**/{6, 8, 2},/**/
  /**/{5, 4, 3}/**/
  };
  return hat_table[dpadY + 1][dpadX + 1];
}

// builds the input report from the mailbox and gives it to TinyUSB
static bool send_input_report(void) {
  const wheel_input_t *input = &usb.input;

  uint8_t tx[REPORT_SIZE] = {0};
  // fill the first byte with d_pad buttons and the first 4 buttons
  tx[0] |= 0x0F & hat_switch_from_msb((uint8_t)input->buttons);
  tx[0] |= 0xF0 & (input->buttons << 4);
  // set the next buttons
  tx[1] = input->buttons >> 8;
  tx[2] = input->buttons >> 16;
  // activate one of 7 buttons depending on the shifter's speed
  tx[3] = 1U << input->gear;
  // set the steering axis
  tx[4] = input->steering;
  tx[5] = input->steering >> 8;
  // set the pedals
  tx[6] = input->throttle;
  tx[7] = input->brake;
  tx[8] = input->clutch;

  return tud_hid_report(JOYSTICK_REPORT_ID, tx, REPORT_SIZE);
}

// Follows what happens on the bus.
// TinyUSB has callbacks for this, but none of them is called when the bus is
// reset (the host enumerating again, or the cable plugged back in), so the
// module asks TinyUSB where it is instead of waiting to be told
static void update_state(void) {
  if (!tud_mounted()) {
    usb.state = USB_DETACHED;
  } else if (tud_suspended()) {
    usb.state = USB_SUSPENDED;
  } else if (usb.state == USB_DETACHED || usb.state == USB_SUSPENDED) {
    // just got configured, or just woke up : the driver of the host has
    // to show that it is reading before we count on it
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
  // handles everything the host sent since the last call.
  // All the callbacks of usb_callbacks.c run inside this call
  tud_task();

  update_state();

  void (*custom_task)(usb_state_t *state) = usb.custom_task;
  if (custom_task != NULL) {
    custom_task(&usb.state);
  }

  // not configured, suspended, or the last report hasn't been read yet
  if (!tud_hid_ready()) {
    return;
  }

  switch (usb.state) {
  case USB_WAIT_HOST:
    // The device being configured doesn't mean that the HID driver of the
    // host is running. Put the current input report on the endpoint :
    // usb_on_report_sent() is called the day the host reads it
    send_input_report();
    break;
  case USB_READY:
    // one report at a time on the endpoint, the ffb library goes first
    if (usb.ffb_report_len > 0) {
      // the report already contains its id, so use id 0
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

// context: USB_HP and USB_LP interrupts (priority 0 and 1)
// TinyUSB only notes what happened, the work is done later by tud_task()
void usb_irq_handler(void) {
  tud_int_handler(BOARD_TUD_RHPORT);
}

/*		FOR usb_callbacks.c 		*/

void usb_on_report_sent(void) {
  // the host read a report : its HID driver is there
  if (usb.state == USB_WAIT_HOST) {
    usb.state = USB_READY;
  }
}

ffb_lib_t* usb_ffb(void) {
  return usb.ffb;
}
