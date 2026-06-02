# FFB Library — Full Documentation

A standalone C++ force-feedback engine extracted from the
[OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) firmware. The effect
math is bit-for-bit identical to the original; everything project-specific
(STM32 HAL, TinyUSB, FreeRTOS, flash storage, the configurator CLI) has been
removed so the engine drops into any bare-metal microcontroller project with
no RTOS and no heap.

This document has four parts:

1. [How to use the interface](#1-how-to-use-the-interface)
2. [User-dependent modifications and modules](#2-user-dependent-modifications-and-modules)
3. [Summary of the internal functioning](#3-summary-of-the-internal-functioning)
4. [Full explanation of how the library works internally](#4-full-explanation-of-how-the-library-works-internally)

---

## 1. How to use the interface

### 1.1 The mental model

The library sits between your **USB stack** and your **motor driver**:

```
  Host PC (game / DirectInput)
        │  USB HID PID reports
        ▼
  Your USB stack  ──hidOut()/hidGet()──►  ffb::Library  ──getAxisTorque()──►  Your motor driver
        ▲                                      ▲
        │ status reports (optional)            │ setAxisState()  (wheel position/speed/accel)
        └────── SendReportFn ──────────────────┘
```

You feed it two things every control cycle:

- inbound **USB reports** from the host (forwarded verbatim), and
- the current **axis state** (where the wheel is, how fast it moves).

It gives you back one number per axis: the **torque** the host wants applied,
in the range `-0x7fff .. 0x7fff` (`-32767 .. 32767`). You scale that to your
motor's PWM/current and drive the hardware. That is the entire contract.

### 1.2 The five things you call

| Phase | Call | When |
|---|---|---|
| Setup | `ffb::Library lib(axis_count, ts)` | Once, at boot |
| Setup | `lib.setSendReportCallback(cb)` | Once (optional) |
| Enumeration | `ffb::Library::descriptor1Axis(&len)` | When the USB stack asks for the HID report descriptor |
| USB RX | `lib.hidOut(id, buf, len)` / `lib.hidGet(id, buf, len)` | From your Set/Get Report callbacks |
| Per tick | `setAxisState()` → `calculate()` → `getAxisTorque()` | Every control cycle (e.g. 1 kHz) |

### 1.3 Construction

```cpp
#include "ffb/ffb.h"

ffb::Library lib(/*axis_count=*/1, ffb::TimeSource(my_millis, my_micros));
```

- `axis_count` — how many physical axes you drive (1 for a wheel, 2 for a
  joystick, etc.). It must be `<= FFB_MAX_AXIS` (compile-time, default 2).
- `ffb::TimeSource(millis, micros)` — two function pointers to free-running
  counters. See [§2.1](#21-the-two-functions-you-must-supply).

The constructor allocates nothing on the heap. The effect pool and all biquad
filters live inside the `Library` object (static / stack / BSS — wherever you
place the instance).

### 1.4 Giving the host the descriptor

At enumeration your USB stack needs the HID **report descriptor**. The library
ships a pre-built one (byte-identical to upstream OpenFFBoard):

```cpp
uint16_t desc_len;
const uint8_t* desc = ffb::Library::descriptor1Axis(&desc_len);  // 1196 bytes
// const uint8_t* desc = ffb::Library::descriptor2Axis(&desc_len); // 1215 bytes
my_usb_provide_hid_report_descriptor(desc, desc_len);
```

Use `descriptor1Axis` for a single-axis device, `descriptor2Axis` for two
axes. These are `static` and may be called before the `Library` is even
constructed (handy because some USB stacks ask for the descriptor very early).

### 1.5 Forwarding USB traffic

The host sends FFB commands as HID **Output reports** (effect parameters,
start/stop) and **Feature reports** (create-effect handshake). Your USB stack
hands those to its Set/Get Report callbacks; forward them straight through:

```cpp
// Host → device (Set Report): effect definitions, start/stop, gain, etc.
void on_usb_set_report(uint8_t report_id, const uint8_t* buf, uint16_t len) {
    lib.hidOut(report_id, buf, len);
}

// Device → host (Get Report): block-load and pool replies the host polls for.
uint16_t on_usb_get_report(uint8_t report_id, uint8_t* buf, uint16_t maxlen) {
    return lib.hidGet(report_id, buf, maxlen);   // returns #bytes written
}
```

Pass the report ID exactly as the host sent it. You do **not** need to know
what any report means — the library decodes all of them. `hidGet` returns the
number of bytes written into `buf` (0 if the report ID isn't a feature reply).

### 1.6 The per-tick loop

This is the hot path. Run it at a fixed rate — the default math is tuned for
1 kHz (`FFB_DEFAULT_SAMPLERATE_HZ`):

```cpp
// 1. Tell the library where the wheel is right now.
lib.setAxisState(0, ffb::AxisState(
    wheel_pos_scaled,   // int32: -0x7fff..0x7fff over the full rotation range
    wheel_speed_dps,    // float: degrees / second
    wheel_accel_dpss)); // float: degrees / second^2

// 2. Run the effect engine.
lib.calculate();

// 3. Read the torque the host is asking for and drive the motor.
int32_t torque = lib.getAxisTorque(0);   // -0x7fff .. 0x7fff
motor_set_output(torque);
```

**Units and conventions (important):**

- **Position** `pos_scaled_16b` is an integer mapped so that the two ends of
  your wheel's travel are `-0x7fff` and `+0x7fff`, with center at `0`. This is
  what spring effects read. If you only have raw encoder counts or degrees, the
  optional [`MetricsBuilder`](#26-optional-derive-speedaccel-from-position)
  helper does the scaling and filtering for you.
- **Speed** is in **degrees per second**, **acceleration** in **degrees per
  second²**. They drive damper, inertia and friction effects. They should be
  reasonably smooth — derivatives of a raw encoder are noisy, so filter them
  (again, `MetricsBuilder` does this).
- **Torque** out is `-0x7fff .. 0x7fff`. Sign is direction; magnitude is
  strength. Map it linearly to your motor command.

If `setActive(false)` (the default until the host enables FFB), `calculate()`
zeroes every axis torque and returns immediately.

### 1.7 Control and settings

```cpp
bool on  = lib.isActive();            // is FFB currently enabled by the host?
lib.setActive(true);                  // force-enable (host normally does this)
lib.resetAllEffects();                // wipe the effect pool (host can also)
lib.setGlobalGain(200);               // 0..255 master strength scaler
uint8_t g = lib.getGlobalGain();
lib.setSamplerate(2000.0f);           // tell the engine your real tick rate (Hz)
float hz = lib.getSamplerate();
lib.setDirectionEnableMask(0x02);     // advanced: see §4.4
```

`setSamplerate()` matters if you do **not** run at 1 kHz: the biquad filter
coefficients are derived from it, so a wrong value detunes damper/friction/
inertia smoothing. Call it once at startup with your actual loop rate.

### 1.8 Status reports back to the host (optional)

DirectInput expects the device to publish a "PID State" input report (report
ID 2) when effects are created or started/stopped. If you want that, register
a sender:

```cpp
lib.setSendReportCallback([](const uint8_t* buf, uint16_t len) -> bool {
    return my_usb_send_input_report(buf, len);   // true if accepted
});
```

If you don't register one, status reports are simply not emitted. Most games
work fine without them, but providing it is more spec-compliant.

### 1.9 Complete minimal example

```cpp
#include "ffb/ffb.h"

extern uint32_t board_millis(void);
extern uint32_t board_micros(void);
extern bool     usb_send_in_report(const uint8_t*, uint16_t);
extern void     motor_set(int32_t torque);
extern int32_t  encoder_pos_scaled(void);
extern float    encoder_speed_dps(void);
extern float    encoder_accel_dpss(void);

static ffb::Library lib(1, ffb::TimeSource(board_millis, board_micros));

void ffb_init(void) {
    lib.setSendReportCallback(usb_send_in_report);
    uint16_t len;
    const uint8_t* d = ffb::Library::descriptor1Axis(&len);
    usb_register_hid_descriptor(d, len);
}

// from USB callbacks
void usb_on_set_report(uint8_t id, const uint8_t* b, uint16_t n) { lib.hidOut(id, b, n); }
uint16_t usb_on_get_report(uint8_t id, uint8_t* b, uint16_t n)   { return lib.hidGet(id, b, n); }

// 1 kHz timer ISR or main-loop tick
void ffb_tick(void) {
    lib.setAxisState(0, ffb::AxisState(encoder_pos_scaled(),
                                       encoder_speed_dps(),
                                       encoder_accel_dpss()));
    lib.calculate();
    motor_set(lib.getAxisTorque(0));
}
```

### 1.10 The C interface

If your firmware is C (or you need an FFI boundary), include `ffb/ffb_c.h`
instead. It wraps one static `ffb::Library`, so `ffb_create()` is called once:

```c
#include "ffb/ffb_c.h"

ffb_lib_t* lib = ffb_create(1, board_millis, board_micros);
ffb_set_send_report_callback(lib, usb_send_in_report);

uint16_t len;
const uint8_t* desc = ffb_descriptor_1axis(&len);

/* USB callbacks */
ffb_hid_out(lib, id, buf, n);
uint16_t got = ffb_hid_get(lib, id, buf, n);

/* per tick */
ffb_set_axis_state(lib, 0, pos_scaled, speed_dps, accel_dpss);
ffb_calculate(lib);
int32_t torque = ffb_get_axis_torque(lib, 0);

/* control */
ffb_set_active(lib, true);
ffb_set_global_gain(lib, 255);
ffb_set_samplerate(lib, 1000.0f);
ffb_reset_all_effects(lib);
```

Every core C function mirrors a C++ method one-to-one. Because the instance is a
single file-static object, `ffb_create()` may be called only once per program;
the returned handle stays valid for the program's lifetime.

Beyond the basics, the C API also exposes the calculator tuning knobs that
otherwise need `getCalculator()` in C++:

```c
ffb_get_global_gain(lib);  ffb_get_samplerate(lib);  ffb_get_axis_count(lib);
ffb_set_friction_rampup_pct(lib, 25);   ffb_get_friction_rampup_pct(lib);
ffb_set_filter_profile_id(lib, 1);      ffb_get_filter_profile_id(lib);

ffb_effect_gain_t g;   ffb_get_effect_gains(lib, &g);   /* spring/damper/inertia/friction */
ffb_set_effect_gains(lib, &g);
ffb_effect_scaler_t s; ffb_get_effect_scalers(lib, &s); ffb_set_effect_scalers(lib, &s);

ffb_effect_filter_preset_t p; ffb_get_filter_preset(lib, 1, &p);
ffb_set_filter_preset(lib, 1, &p);      ffb_update_filters_for_type(lib, FFB_EFFECT_DAMPER);
```

The two opt-in helpers have their own C headers — `ffb/ffb_metrics_c.h` and
`ffb/ffb_axis_local_c.h` (see [§2.6](#26-optional-derive-speedaccel-from-position)
and [§2.7](#27-optional-module-axis-local-feel-effects)). Their instances come
from a small static pool (one slot per `FFB_MAX_AXIS`, no heap), so each
`ffb_*_create()` may be called up to `FFB_MAX_AXIS` times.

### 1.11 Complete C example: metrics + feel effects

This is the full C counterpart to §1.9, additionally using the two opt-in
helper wrappers (`ffb_metrics_c.h`, `ffb_axis_local_c.h`) and a tuning call.
The complete file ships as `examples/c_wrappers.c`; build it with
`cmake -B build -DFFB_BUILD_EXAMPLES=ON`.

```c
#include "ffb/ffb_c.h"
#include "ffb/ffb_metrics_c.h"
#include "ffb/ffb_axis_local_c.h"
#include <math.h>
#include <stdio.h>

static uint32_t g_ms = 0;
static uint32_t platform_millis(void) { return g_ms; }
static uint32_t platform_micros(void) { return g_ms * 1000u; }
static bool platform_send(const uint8_t* r, uint16_t n) { (void)r; (void)n; return true; }
static void platform_set_motor(int32_t t) { (void)t; }   /* map to PWM / current */

int main(void) {
    /* 1. Core engine + descriptor. */
    ffb_lib_t* lib = ffb_create(/*axis_count=*/1, platform_millis, platform_micros);
    ffb_set_send_report_callback(lib, platform_send);
    uint16_t desc_len = 0;
    const uint8_t* desc = ffb_descriptor_1axis(&desc_len);   /* hand to USB stack */
    (void)desc;

    /* 2. Metrics helper: raw wheel degrees -> scaled pos + filtered speed/accel.
     *    900 deg total travel, 1 kHz control loop. */
    const float DOR = 900.0f;
    ffb_metrics_t* metrics = ffb_metrics_create(DOR, 1000.0f);

    /* 3. Axis-local "feel" effects the host does NOT request. */
    ffb_axis_local_config_t cfg;
    ffb_axis_local_config_default(&cfg);
    cfg.degrees_of_rotation  = DOR;
    cfg.idle_spring_strength = 40;     /* auto-center when FFB off  */
    cfg.endstop_strength     = 127;    /* wall at the travel limit  */
    cfg.damper_intensity     = 30;     /* always-on damping         */
    ffb_axis_local_t* local = ffb_axis_local_create(&cfg);

    /* 4. (Optional) tune the host-effect gain table. Defaults match upstream. */
    ffb_effect_gain_t gains;
    ffb_get_effect_gains(lib, &gains);
    gains.spring = 80;
    ffb_set_effect_gains(lib, &gains);

    ffb_set_active(lib, true);          /* normally the host enables FFB */

    /* 5. Control loop. Update metrics ONCE per tick and reuse the result. */
    for (int tick = 0; tick < 5; ++tick, g_ms += 1) {
        float raw_deg = (DOR * 0.5f) * sinf((float)tick * 0.05f);

        ffb_axis_state_t st = ffb_metrics_update(metrics, raw_deg);
        ffb_set_axis_state_s(lib, 0, &st);

        ffb_calculate(lib);
        int32_t host_torque = ffb_get_axis_torque(lib, 0);

        /* Add the local feel torque on top, then clamp and drive the motor. */
        int32_t local_torque = ffb_axis_local_compute(local, &st, raw_deg,
                                                       ffb_is_active(lib));
        int32_t total = host_torque + local_torque;
        if (total >  0x7fff) total =  0x7fff;
        if (total < -0x7fff) total = -0x7fff;
        platform_set_motor(total);
    }

    /* 6. Runtime feel adjustment (e.g. from a settings menu). */
    ffb_axis_local_set_idle_spring(local, 10);
    ffb_axis_local_set_intensities(local, /*endstop=*/200, /*damper=*/50,
                                   /*friction=*/0, /*inertia=*/0);
    return 0;
}
```

Notes:

- `ffb_metrics_update_and_set()` is the one-call convenience (update + feed);
  `ffb_metrics_update()` returns the `ffb_axis_state_t` if you want it yourself
  (as used above to feed the axis-local helper).
- The helper instances come from a static pool of `FFB_MAX_AXIS` slots, so
  call each `*_create()` once per axis.
- `ffb_axis_local_set_idle_spring()` exists separately from
  `ffb_axis_local_set_intensities()` because the idle-spring scale is cached and
  must be recomputed when it changes.

### 1.12 Quick interface reference

**C++ (`ffb::Library`, header `ffb/ffb.h`):**

| Method | Purpose |
|---|---|
| `Library(uint8_t axis_count, TimeSource ts)` | Construct |
| `void hidOut(id, buf, len)` | Feed an inbound Output/Feature report |
| `uint16_t hidGet(id, buf, reqlen)` | Produce a Feature reply; returns bytes written |
| `void setAxisState(axis, AxisState)` | Provide position/speed/accel |
| `void calculate()` | Run one engine tick |
| `int32_t getAxisTorque(axis)` | Read torque (`-0x7fff..0x7fff`) |
| `bool isActive()` / `void setActive(bool)` | Query/force FFB enable |
| `void resetAllEffects()` | Clear all effect slots |
| `void setGlobalGain(u8)` / `uint8_t getGlobalGain()` | Master gain |
| `void setSamplerate(float)` / `float getSamplerate()` | Engine rate (Hz) |
| `void setDirectionEnableMask(u8)` | Advanced direction-bit override |
| `void setSendReportCallback(SendReportFn)` | Status-report sender (optional) |
| `static const uint8_t* descriptor1Axis(uint16_t*)` | 1-axis HID descriptor |
| `static const uint8_t* descriptor2Axis(uint16_t*)` | 2-axis HID descriptor |
| `Calculator& getCalculator()` / `HidParser& getParser()` | Escape hatch to internals |

---

## 2. User-dependent modifications and modules

This section is everything you, the integrator, must or may provide. The core
philosophy: the library never touches hardware directly. Anything platform-
specific is reached through a function pointer or a compile-time macro.

### 2.1 The two functions you must supply

The library needs to know how much time has passed (for effect duration,
envelopes, periodic phase). You supply two free-running counters:

```cpp
uint32_t my_millis(void);   // milliseconds since boot
uint32_t my_micros(void);   // microseconds since boot
```

Wrap them in a `TimeSource` and pass to the constructor:

```cpp
ffb::Library lib(1, ffb::TimeSource(my_millis, my_micros));
```

Rules:

- Both may **wrap around** (overflow). Every internal use is delta-based
  (`now - startTime`), so wraparound is harmless.
- If your MCU only exposes `millis()`, derive `micros()` from any free-running
  hardware timer (e.g. a 1 MHz timer's count register). The periodic and ramp
  effects use `micros()` for sub-millisecond phase accuracy; a coarse `micros`
  still works, just with coarser waveform timing.
- They are read several times per tick, so keep them cheap (a register read or
  a counter, not a syscall).

> **Why function pointers and not virtual methods?** No vtable, no heap, and
> the optimizer can often inline through them on `-O2`. It also keeps the C
> wrapper trivial.

### 2.2 The status-report callback (optional)

```cpp
using SendReportFn = bool (*)(const uint8_t* report, uint16_t len);
lib.setSendReportCallback(my_sender);
```

Called when the engine wants to push a PID State input report (ID 2) to the
host. Return `true` if your USB stack accepted it. Leaving it unset disables
status reporting (acceptable for most games).

### 2.3 Wiring to your USB stack

The library is USB-stack agnostic. You wire three touch points:

| USB event | Library call |
|---|---|
| Host requests HID report descriptor | return `descriptor1Axis()/2Axis()` bytes |
| Set Report (OUT data / control SET_REPORT) | `lib.hidOut(id, buf, len)` |
| Get Report (control GET_REPORT) | `return lib.hidGet(id, buf, maxlen)` |

**TinyUSB** is the most common target. The mapping is:

```cpp
// Descriptor
uint8_t const* tud_hid_descriptor_report_cb(uint8_t) {
    uint16_t len; return ffb::Library::descriptor1Axis(&len);
}

// Host → device
void tud_hid_set_report_cb(uint8_t, uint8_t report_id,
                           hid_report_type_t /*type*/,
                           uint8_t const* buffer, uint16_t bufsize) {
    lib.hidOut(report_id, buffer, bufsize);
}

// Device → host
uint16_t tud_hid_get_report_cb(uint8_t, uint8_t report_id,
                               hid_report_type_t /*type*/,
                               uint8_t* buffer, uint16_t reqlen) {
    return lib.hidGet(report_id, buffer, reqlen);
}
```

`examples/tinyusb_glue.cpp` is a ready-to-copy version. For ST USB, LUFA, or a
hand-rolled stack the idea is identical: locate the "set report" and "get
report" entry points and forward.

> **Note on report IDs with TinyUSB:** when TinyUSB strips the leading report-ID
> byte from the OUT buffer it passes the ID separately — which is exactly what
> `hidOut` expects. If your stack leaves the ID *in* the buffer, pass
> `buffer+1`/`bufsize-1` and the parsed ID byte yourself.

#### The USB descriptors you must supply (report vs. configuration)

This trips people up, so it is worth stating plainly. There are **two different
descriptors** and the library only owns one of them:

| Descriptor | What it is | Who provides it |
|---|---|---|
| **HID report descriptor** | The ~1.2 KB blob describing the FFB reports | **The library** (`descriptor1Axis()/2Axis()`) |
| **USB device / configuration / string descriptors** | VID/PID, interfaces, endpoints, product name | **You** |

The library is not a full USB stack — it never sees enumeration. It cannot ship
your **configuration descriptor**, because that describes your *whole* device
(its VID/PID, how many interfaces, which endpoints, whether you also expose a
CDC serial port, etc.). You must write it yourself and return it from your USB
stack's configuration-descriptor callback (`tud_descriptor_configuration_cb` on
TinyUSB). Two things are mandatory for FFB to work:

1. **The HID interface must have an OUT endpoint as well as an IN endpoint.**
   Force feedback is driven by the host sending HID *Output* reports, so an
   IN-only HID interface cannot receive effects. On TinyUSB use
   `TUD_HID_INOUT_DESCRIPTOR` (not the IN-only `TUD_HID_DESCRIPTOR`).
2. **The report-descriptor length in the config must match the bytes the
   library returns** — `1196` for `descriptor1Axis()`, `1215` for
   `descriptor2Axis()`. (The report descriptor itself travels over the control
   endpoint, so its size does not constrain the interrupt endpoint buffer.)

A complete, copy-pasteable HID-only configuration (device + config + string
descriptors and the three callbacks) is in
**`examples/usb_descriptors_tinyusb.c`**. The minimal config descriptor is just:

```c
enum { ITF_NUM_HID = 0, ITF_NUM_TOTAL };
#define EPNUM_HID_OUT 0x01   /* host -> device: effect reports */
#define EPNUM_HID_IN  0x81   /* device -> host: PID state      */
#define FFB_HID_REPORT_DESC_LEN 1196          /* == descriptor1Axis() length */
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    /* itf, str, boot-proto, report-len, EP-OUT, EP-IN, EP-size, interval(ms) */
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                             FFB_HID_REPORT_DESC_LEN,
                             EPNUM_HID_OUT, EPNUM_HID_IN,
                             CFG_TUD_HID_EP_BUFSIZE, 1 /* 1 ms = 1000 Hz */),
};
```

> Upstream OpenFFBoard ships a **composite CDC + HID** device (the CDC channel
> is its configurator serial port, which this library drops). Its config places
> the HID on interface 2 with IN EP `0x83` / OUT EP `0x02`. If you keep your own
> CDC/MIDI/etc. interface, model it on that; for a pure FFB wheel the HID-only
> config above is all you need. The original macro is reproduced at the bottom
> of `examples/usb_descriptors_tinyusb.c`.

#### `tusb_config.h` settings for TinyUSB

A pure FFB device needs the HID class enabled and an endpoint buffer large
enough for the FFB reports (all are well under 64 bytes):

```c
#define CFG_TUD_ENDPOINT0_SIZE   64

#define CFG_TUD_HID              1     /* one FFB HID interface          */
#define CFG_TUD_HID_EP_BUFSIZE   64    /* >= largest FFB report (it is)  */

/* Classes you are not using - keep them off: */
#define CFG_TUD_CDC              0
#define CFG_TUD_MSC              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0
```

Plus the usual board-level lines TinyUSB needs regardless of class:
`CFG_TUSB_MCU` (e.g. `OPT_MCU_STM32F4`, `OPT_MCU_RP2040`, `OPT_MCU_ESP32S3`),
`CFG_TUSB_OS` (`OPT_OS_NONE` for a bare polled loop, `OPT_OS_FREERTOS` if you
run one — the FFB library itself is RTOS-agnostic either way), and the RHPort /
speed defines for your part. `CFG_TUD_HID_EP_BUFSIZE` of 64 is sufficient: the
1.2 KB report descriptor is sent over the control endpoint, not this one.

> If you keep OpenFFBoard's composite layout, also enable `CFG_TUD_CDC 1`
> (and `CFG_TUD_MIDI 1` if used) and size `CFG_TUD_CDC_RX/TX_BUFSIZE` as the
> upstream `tusb_config.h` does.

### 2.4 Wiring to your motor driver

`getAxisTorque(axis)` returns a signed value in `-0x7fff .. 0x7fff`. You own
the mapping to hardware. Typical patterns:

```cpp
// PWM H-bridge, 0..1000 duty, sign = direction pin
int32_t t = lib.getAxisTorque(0);
gpio_set_dir(t >= 0);
pwm_set_duty( (abs(t) * MAX_DUTY) / 0x7fff );

// Bipolar current target for a BLDC FOC loop
float amps = (lib.getAxisTorque(0) / 32767.0f) * MAX_CURRENT_A;
foc_set_iq_target(amps);
```

Add your own safety clamps (current limit, slew-rate limit, thermal fold-back)
**outside** the library — it deliberately knows nothing about your motor.

### 2.5 Compile-time configuration

Override any of these by defining them before including any `ffb/*` header, or
on the compiler command line (`-DFFB_MAX_AXIS=1`). They live in
`include/ffb/ffb_config.h`:

| Macro | Default | Purpose | When to change |
|---|---|---|---|
| `FFB_MAX_AXIS` | `2` | Max physical axes (1–3) | Set to 1 for a wheel/pedal to shrink RAM |
| `FFB_MAX_EFFECTS` | `40` | Effect-slot pool size | Lower on tiny MCUs; the host's pool report advertises this |
| `FFB_DEFAULT_SAMPLERATE_HZ` | `1000.0f` | Initial filter tuning rate | Match your loop rate (or call `setSamplerate`) |
| `FFB_ID_OFFSET` | `0` | Added to every report ID | Composite HID device that shares report IDs |
| `FFB_LOG(msg)` | no-op | Debug log hook | Define to your logger to trace effect lifecycle |

Memory scales as roughly `FFB_MAX_EFFECTS × (sizeof(Effect) + FFB_MAX_AXIS ×
sizeof(Biquad))`. With defaults that's about **9 KB** of static RAM — fine for
a Cortex-M0+.

`FFB_LOG` example:

```cpp
#define FFB_LOG(msg) my_uart_print(msg)   // before including ffb headers
```

### 2.6 Optional module: derive speed/accel from position

If you only have a raw wheel position and don't want to compute filtered speed
and acceleration yourself, include `ffb/ffb_metrics.h`:

```cpp
#include "ffb/ffb_metrics.h"

ffb::MetricsBuilder metrics(/*degrees_of_rotation=*/900.0f,
                            /*samplerate_hz=*/1000.0f);

void tick(float raw_wheel_degrees) {
    ffb::AxisState st = metrics.update(raw_wheel_degrees);  // scales + filters
    lib.setAxisState(0, st);
    lib.calculate();
    motor_set(lib.getAxisTorque(0));
}
```

`update()` does exactly what OpenFFBoard's `Axis::updateMetrics()` did:

```
speed = (new_pos - prev_pos) * samplerate;  speed = speedFilter.process(speed);
accel = (speed - last_speed) * samplerate;  accel = accelFilter.process(accel);
```

and scales degrees into the `-0x7fff..0x7fff` position range from the
`degrees_of_rotation` you pass. Call `metrics.setSamplerate(hz)` if your rate
changes, `metrics.reset(pos)` to clear history. **Skip this module entirely if
you already produce filtered speed/accel** — fill `AxisState` directly.

From C, include `ffb/ffb_metrics_c.h` and compile `src/ffb_metrics_c.cpp`:

```c
ffb_metrics_t* m = ffb_metrics_create(900.0f, 1000.0f);
ffb_metrics_update_and_set(m, lib, 0, raw_wheel_degrees);  /* updates + feeds engine */
/* or: ffb_axis_state_t s = ffb_metrics_update(m, raw_wheel_degrees); */
```

### 2.7 Optional module: axis-local "feel" effects

Some effects are *not* sent by the host — they're things a wheel does on its
own: self-centering when FFB is off, a software end-stop at the rotation limit,
and an always-on damper/friction/inertia for mechanical feel. Include
`ffb/ffb_axis_local.h` if you want them:

```cpp
#include "ffb/ffb_axis_local.h"

ffb::AxisLocalConfig cfg;
cfg.idle_spring_strength = 40;     // auto-center when FFB disabled
cfg.endstop_strength     = 127;    // wall at the rotation limit
cfg.damper_intensity     = 30;     // always-on damping
cfg.degrees_of_rotation  = 900.0f;
ffb::AxisLocalEffects local(cfg);

void tick(...) {
    lib.setAxisState(0, st);
    lib.calculate();
    int32_t host_t  = lib.getAxisTorque(0);
    int32_t local_t = local.compute(st, raw_wheel_degrees, lib.isActive());
    motor_set(host_t + local_t);   // you sum and clamp
}
```

The math mirrors `Axis::calculateAxisEffects()` and `Axis::updateEndstop()`.
**Most host-driven devices don't need this** — the game provides spring/damper
itself. Use it for standalone "feel" or as a safety auto-center.

From C, include `ffb/ffb_axis_local_c.h` and compile `src/ffb_axis_local_c.cpp`:

```c
ffb_axis_local_config_t cfg;
ffb_axis_local_config_default(&cfg);
cfg.idle_spring_strength = 40;
ffb_axis_local_t* local = ffb_axis_local_create(&cfg);

ffb_axis_state_t st = ffb_metrics_update(m, raw_wheel_degrees);
ffb_set_axis_state_s(lib, 0, &st);
ffb_calculate(lib);
int32_t total = ffb_get_axis_torque(lib, 0) +
                ffb_axis_local_compute(local, &st, raw_wheel_degrees, ffb_is_active(lib));
```

Runtime tuning: `ffb_axis_local_set_idle_spring()`,
`ffb_axis_local_set_intensities()`, `ffb_axis_local_set_samplerate()`.

Both optional modules are in separate translation units: if you never include
their header and never link their `.cpp`, they contribute **zero** bytes.

### 2.8 Advanced: a custom HID descriptor

The pre-built descriptors cover the common cases. If you need a different
report layout (extra buttons, different axis count, composite device), include
`ffb/ffb_descriptor.h` — it re-exports the `HIDDESC_FFB_*` building-block macros
so you can assemble your own `static const uint8_t[]`. If you move report IDs,
also set `FFB_ID_OFFSET` (and/or `setDirectionEnableMask`, see [§4.4](#44-the-set-effect-report-and-direction-projection))
so the parser stays in sync with your bytes.

### 2.9 Build integration

**With the provided CMake:**

```sh
cmake -B build -DFFB_BUILD_TESTS=ON
cmake --build build          # produces libffb.a, runs the smoke test target
```

CMake options: `FFB_BUILD_EXAMPLES`, `FFB_BUILD_TESTS`, `FFB_BUILD_C_WRAPPER`
(on by default). The target requires C++11 with extensions off.

**Without CMake (drop into an existing firmware build):**

1. Add `include/` to your include path.
2. Compile these always: `src/ffb_biquad.cpp`, `src/ffb_calculator.cpp`,
   `src/ffb_parser.cpp`, `src/ffb_descriptor.cpp`.
3. Compile `src/ffb_c.cpp` only if you use the C API.
4. Compile `src/ffb_metrics.cpp` / `src/ffb_axis_local.cpp` only if you include
   their headers. From C, also compile the matching wrapper
   (`src/ffb_metrics_c.cpp` / `src/ffb_axis_local_c.cpp`).

No external libraries, no RTOS, no dynamic allocation. C++11 is the minimum
standard; C++14/17 also build clean.

---

## 3. Summary of the internal functioning

### 3.1 Layered architecture

```
┌──────────────────────────────────────────────────────────┐
│  ffb::Library  (facade, header-only)                     │  ffb.h
│  thin pass-through to the two components below            │
└───────────────┬───────────────────────┬──────────────────┘
                │                        │
                ▼                        ▼
┌───────────────────────────┐  ┌───────────────────────────┐
│  ffb::HidParser           │  │  ffb::Calculator          │
│  decodes USB reports into │  │  reads AxisState[],       │
│  Effect mutations         │◄─┤  runs the force math,     │
│                           │  │  writes axis_torque[]     │
└───────────────┬───────────┘  └───────────┬───────────────┘
                │  share the same           │ uses
                ▼  Effect array             ▼
┌───────────────────────────┐  ┌───────────────────────────┐
│  std::array<Effect, N>    │  │  ffb::Biquad (low-pass)    │
│  the effect pool          │  │  per-effect, per-axis      │
└───────────────────────────┘  └───────────────────────────┘

Static, no-dependency support files:
  ffb_defs.h        wire-format report structs + report-ID/effect-type constants
  ffb_descriptor.*  pre-built HID report descriptor byte arrays
  ffb_config.h      compile-time knobs

Opt-in, never pulled in unless you include them:
  ffb_metrics.*     raw position → filtered speed/accel
  ffb_axis_local.*  idle spring / endstop / always-on damper-friction-inertia
```

### 3.2 Data flow in one breath

The host defines an effect through a sequence of HID reports → `HidParser`
decodes each report and writes the relevant fields into a shared `Effect` slot
→ once per tick `Calculator::calculate()` walks every active slot, computes a
raw force, projects it onto each axis, applies condition/envelope/filter shaping,
sums per axis, clamps to `±0x7fff`, and stores it → you read it with
`getAxisTorque()` and drive the motor.

### 3.3 Each module in one line

- **`Library`** — a header-only facade; it owns a `Calculator` and a `HidParser`
  and forwards calls. Pure convenience.
- **`HidParser`** (was `HidFFB`) — the USB decoder. One big switch on report ID
  turns incoming bytes into effect-pool mutations and produces feature replies.
- **`Calculator`** (was `EffectsCalculator`) — the math. Per tick it turns the
  effect pool plus the current axis state into per-axis torque.
- **`Effect`** — a plain struct holding one effect's parameters and its biquad
  filter slots. The pool is a fixed `std::array`.
- **`Biquad`** — a direct-form-I low-pass filter used to smooth constant force
  and the condition (damper/friction/inertia) effects.
- **`ffb_defs.h`** — the exact `__attribute__((packed))` structs matching the
  USB wire format, plus all the report-ID and effect-type constants.
- **`ffb_descriptor`** — the HID report descriptor bytes the host reads at
  enumeration to learn the device is a force-feedback joystick.

### 3.4 What changed from upstream OpenFFBoard

The effect **math is unchanged** (verified bit-identical). The structural
edits are only at the boundaries:

| Upstream dependency | Replaced with |
|---|---|
| FreeRTOS `Thread`/`Delay` | Removed — engine is polled |
| `PersistentStorage` / flash | Removed — coefficients are runtime defaults + setters |
| `CommandHandler` CLI | Removed |
| `UsbHidHandler` singleton + `tud_hid_report()` | `hidOut/hidGet` + `SendReportFn` |
| `HAL_GetTick()` / `micros()` | `TimeSource` function pointers |
| `std::vector<unique_ptr<Axis>>` | internal `AxisState[]` in / `axis_torque[]` out |
| `unique_ptr<Biquad> filter[]` (heap) | inline `Biquad filter[]` + `bool filter_active[]` (static) |

---

## 4. Full explanation of how the library works internally

This section walks the whole pipeline in detail, from a report arriving to a
torque coming out.

### 4.1 The effect pool and an effect's lifecycle

All effects live in one fixed array inside the `Calculator`:

```cpp
std::array<Effect, FFB_MAX_EFFECTS> effects;   // default 40 slots
```

A slot is "free" when its `type == FFB_EFFECT_NONE`. An `Effect` is a flat
struct (see `ffb_effect.h`) holding every parameter the math needs: `type`,
`state`, `gain`, `magnitude`, `offset`, `startLevel`/`endLevel` (ramp),
`phase`/`period` (periodic), `duration`, envelope levels/times, an
`axisMagnitudes[]` projection vector, a `conditions[]` block per axis, and —
critically for the static design — an inline `Biquad filter[FFB_MAX_AXIS]` plus
a `bool filter_active[FFB_MAX_AXIS]` flag instead of the upstream heap pointer.

A DirectInput effect is born and dies through this sequence (all driven by the
host; the parser implements each step):

1. **Create New Effect** (Feature report `0x11`, `HID_ID_NEWEFREP`) — the host
   says "I want a CONSTANT effect." `HidParser::newEffect()` calls
   `Calculator::findFreeEffect(type)`, which returns the first slot whose
   `type == NONE`. The parser records the block index and bumps `used_effects`.
2. **Block Load poll** (Feature GET `0x12`, `HID_ID_BLKLDREP`) — the host polls
   "did that allocation succeed and what index?" `HidParser::hidGet()` fills a
   `FFB_BlockLoad_Feature_Data_t` with the `effectBlockIndex` (1-based) and
   `loadStatus = 1` (success) / pool size.
3. **Set Effect** (Output `0x01`) and the type-specific parameter reports
   (constant `0x05`, ramp `0x06`, periodic `0x04`, envelope `0x02`, condition
   `0x03`) — the host fills in the actual numbers. Each maps to a `set_*`
   handler that writes fields into the slot.
4. **Effect Operation** (Output `0x0A`, `HID_ID_EFOPREP`) — start/stop/stop-all.
   On start the parser stamps `startTime = millisNow() + startDelay` and sets
   `state = 1`.
5. **Block Free** (Output `0x0B`) — `Calculator::freeEffect(idx)` resets the
   slot to a default `Effect()` and clears `filter_active[]`, returning it to
   the pool.

The host can also poll the **PID Pool** feature report (`0x13`,
`HID_ID_POOLREP`) to learn `FFB_MAX_EFFECTS`, and send **Device Control**
(`0x0C`) to enable/disable/reset all FFB, and **Device Gain** (`0x0D`) to set
the master gain.

### 4.2 HidParser — decoding inbound reports

`hidOut(report_id, buffer, bufsize)` is a direct port of upstream
`HidFFB::hidOut`. It subtracts `FFB_ID_OFFSET`, then switches on the report ID
and reinterpret-casts the buffer to the matching packed struct:

| Report ID | Constant | Handler | Writes |
|---|---|---|---|
| `0x01` | `EFFREP` | `setEffect` | type, gain, duration, direction → `axisMagnitudes[]`, envelope/condition flags |
| `0x02` | `ENVREP` | `setEnvelope` | attack/fade levels + times, `useEnvelope=true` |
| `0x03` | `CONDREP` | `setCondition` | one `conditions[]` block (cpOffset, coeffs, saturations, deadband) |
| `0x04` | `PRIDREP` | `setPeriodic` | magnitude, offset, phase, period |
| `0x05` | `CONSTREP` | `setConstantForce` | magnitude |
| `0x06` | `RAMPREP` | `setRamp` | startLevel, endLevel |
| `0x0A` | `EFOPREP` | `setEffectOperation` | start (stamp startTime, state=1) / stop |
| `0x0B` | `BLKFRREP` | `freeEffect` | frees the slot |
| `0x0C` | `CTRLREP` | `controlCmd` | enable/disable/reset-all FFB |
| `0x0D` | `GAINREP` | `setGain` | global gain |
| `0x11` | `NEWEFREP` | `newEffect` | allocate a slot (this one arrives as a Feature report) |

Because the structs are `__attribute__((packed))` and laid out exactly like the
USB wire format, decoding is a cast and a few field copies — no byte-by-byte
parsing. The packed structs are validated implicitly by the descriptor: the
host computes offsets from the same report descriptor the library ships, so the
layouts line up by construction.

`hidGet(report_id, reply, reqlen)` handles the two **Feature GET** replies the
host polls: Block Load (`0x12`) and PID Pool (`0x13`). It memcpys the prepared
reply struct into `reply` and returns its size; for any other ID it returns 0.

When an effect is created or operated on and a `SendReportFn` is registered,
the parser also assembles a `reportFFB_status_t` (PID State, IN report `0x02`)
and calls the callback so the host learns the current effect-block state.

### 4.3 Calculator::calculate() — the per-tick loop

The whole tick (ported verbatim, with the time source swapped in):

```cpp
void Calculator::calculate() {
    for (axis) axis_torque[axis] = 0;            // start from zero
    if (!isActive()) return;                     // FFB disabled → all zero

    int32_t forces[FFB_MAX_AXIS] = {0};
    uint32_t now_ms = time_source.millis();

    for (each effect slot) {
        // Expiry: finite-duration effects past startTime+duration go inactive;
        // effects still inside their startDelay are skipped this tick.
        if (active && finite_duration) {
            if (now_ms < startTime) continue;                 // start delay
            if (now_ms - startTime > duration) state = INACTIVE;
        }
        if (state == INACTIVE) continue;

        int32_t force = calcNonConditionEffectForce(effect);  // base scalar
        for (axis) forces[axis] += calcComponentForce(effect, force, axis);
    }

    for (axis) axis_torque[axis] = clip(forces[axis], -0x7fff, 0x7fff);
}
```

So each tick: zero, bail if inactive, walk every slot handling expiry, compute a
base force, fan it out across axes, accumulate, clamp. Note that condition
effects (spring/damper/friction/inertia) ignore the `force` argument and read
the axis state directly inside `calcComponentForce`.

### 4.4 calcNonConditionEffectForce() — the waveform generators

For constant/ramp/periodic effects this returns a single scalar "force vector"
before per-axis projection. If `useEnvelope`, the magnitude is first reshaped by
`getEnvelopeMagnitude()`. Then a switch on `type`:

- **Constant** — `force = magnitude`.
- **Ramp** — linear interpolation from `startLevel` to `endLevel` across
  `duration`, using elapsed time from `micros()`.
- **Square** — `±magnitude` depending on whether `(elapsed+phase) mod period`
  is in the first or second half, plus `offset`.
- **Triangle / Sawtooth Up / Sawtooth Down** — compute a `remainder` within the
  period from `micros()` and `phase`, then a linear slope between
  `offset±magnitude`.
- **Sine** — `offset + sin(2π·(t·freq + phase))·magnitude`, with `freq =
  1/period` and `phase` normalized by 35999.

The result is finally scaled by the effect's own gain: `return force_vector *
effect->gain / 255`.

### 4.5 calcComponentForce() — projection, conditions, filtering

This routes the base force (or the axis state, for conditions) onto one axis and
applies the per-effect filter. `angle_ratio = effect->axisMagnitudes[axis]` is
the projection coefficient the parser computed from the host's direction field.

- **Constant** — if a filter is active and tuned (`Fc < 0.5`), the force is
  low-pass filtered first; then (shared with all periodic/ramp types)
  `result = -forceVector * angle_ratio`. The sign flip matches DirectInput's
  convention (a +X effect pushes toward −X torque).
- **Spring** — `result -= calcConditionEffectForce(pos, gain.spring, ...,
  scaler.spring, angle_ratio)`, i.e. force proportional to displacement from
  `cpOffset`, outside the deadband, clamped to saturation.
- **Damper** — same condition formula but the metric is `speed *
  INTERNAL_SCALER_DAMPER (40)`, then low-pass filtered.
- **Inertia** — metric is `accel * INTERNAL_SCALER_INERTIA (4)`, filtered.
- **Friction** — metric is `speed * INTERNAL_SCALER_FRICTION (45)`; below a
  configurable speed threshold (`frictionPctSpeedToRampup`) the output is eased
  in with a half-sine ramp so friction doesn't snap on at zero crossing;
  result is gained, scaled, projected and filtered.

Finally `return result_torque * global_gain / 255` applies the master gain.

The condition core, `calcConditionEffectForce()`, is the classic DirectInput
condition law:

```
if |metric - cpOffset| > deadBand:
    coefficient = (metric > cpOffset ? positiveCoefficient : negativeCoefficient) / 0x7fff
    metric -= cpOffset + deadBand·sign
    force   = clip(coefficient · gainfactor · scale · metric,
                   -negativeSaturation, +positiveSaturation)
return force · angle_ratio
```

`gainfactor = (gain_val + 1) / 256`. The per-type `scale` values
(`scaler.spring=16`, `damper=4`, `inertia=2`, `friction=1`) and gains
(`gain.spring=64`, `damper=64`, `inertia=127`, `friction=254`) are the upstream
defaults, exposed for tuning through `getCalculator().gains()/scalers()`.

> **`directionEnableMask` / `axisMagnitudes`.** When the host sends Set Effect,
> it gives a direction (either a per-axis enable + angle, or a polar direction).
> The parser turns that into the projection vector `axisMagnitudes[axis]`. For
> the common 1-axis case with `directionX = 0`, the X unit vector is `-1`, so a
> positive constant force becomes a positive torque after the sign flip in
> `calcComponentForce`. `setDirectionEnableMask()` lets you tell the parser
> which bit in `enableAxis` selects polar vs. per-axis mode if you ship a custom
> descriptor that places it differently.

### 4.6 getEnvelopeMagnitude() — attack/sustain/fade

For finite-duration effects with an envelope, the magnitude is reshaped over
time: during `attackTime` it ramps from `attackLevel` up to the full magnitude;
during the final `fadeTime` it ramps down to `fadeLevel`; in between it holds.
Infinite-duration effects bypass the envelope. The sign of the original
magnitude is preserved.

### 4.7 Biquad filters and the static-allocation design

`ffb::Biquad` is a direct-form-I low-pass (the upstream `Filters` class with the
one external `clip<>` inlined). Each effect that needs smoothing (constant,
damper, friction, inertia) gets one biquad **per axis**, stored *inline* in the
`Effect`:

```cpp
struct Effect {
    ...
    Biquad filter[FFB_MAX_AXIS];
    bool   filter_active[FFB_MAX_AXIS] = { false };
};
```

This is the single structural rewrite versus upstream, which used
`std::unique_ptr<Biquad> filter[MAX_AXIS]` (heap). Three call sites changed:

1. `setFilters(effect)` — instead of `make_unique`, it calls
   `effect->filter[i].setBiquad(lowpass, fc/calcfrequency, q·0.01, 0)` and sets
   `filter_active[i] = true`. The cutoff frequencies per effect type come from
   the `EffectFilterPreset` table (constant `{500,70}`, friction `{50,20}`,
   damper `{30,40}`, inertia `{15,20}` by default), divided by the sample rate.
2. `calcComponentForce()` — every `filter != nullptr` check became
   `filter_active[axis]`, and `filter->process()` became `filter.process()`.
3. `freeEffect()` — clears `filter_active[]` instead of resetting smart
   pointers.

Because the cutoff is `freq / calcfrequency`, the filters are only correct if
`calcfrequency` matches your real loop rate — hence `setSamplerate()` rebuilds
the coefficients of every active filter (`updateFiltersForType` /
`setFilterProfileId` do the same for a profile change).

The upshot: **no heap is ever touched at runtime**. The entire effect pool,
including all filters, is part of the `Library` object's storage. On a 2-axis,
40-effect default that's ~9 KB of BSS, fixed at compile time.

### 4.8 Time handling and overflow

Everything time-related flows through the two user counters via
`Calculator::millisNow()` / `microsNow()`:

- Effect **duration / expiry** and **envelope** use `millis()`.
- **Ramp** and **periodic** waveforms use `micros()` for fine phase resolution.
- Effect **start time** is `millis() + startDelay`, stamped when the host starts
  the effect.

All comparisons are deltas (`now - startTime`), so a 32-bit counter wrapping
roughly every 49 days (millis) or every ~71 minutes (micros) causes no glitch —
the subtraction wraps consistently in unsigned arithmetic.

### 4.9 The HID report descriptor

At enumeration the host reads the report descriptor to discover the device is a
PID-class force-feedback joystick and to learn the byte layout of every report.
`ffb_descriptor.cpp` holds the fully pre-assembled byte arrays
(`hid_1ffb_desc_bytes[]` = 1196 bytes, `hid_2ffb_desc_bytes[]` = 1215 bytes),
byte-identical to upstream OpenFFBoard's generated descriptors. `descriptor1Axis`
/ `descriptor2Axis` just return the pointer and length.

The descriptor and the packed structs in `ffb_defs.h` are two views of the same
contract: the host parses the descriptor to know where, say, `magnitude` sits in
the Set Constant Force report, and the library casts the incoming bytes to a
struct with that exact layout. Keep them in sync — if you build a custom
descriptor (`ffb_descriptor.h` macros), the report structs must still match.

### 4.10 Putting it together — a constant force, end to end

This is the path the smoke test exercises:

1. Host: Create New Effect (CONSTANT) → `newEffect` grabs slot 1.
2. Host polls Block Load → `hidGet` replies index 1, status 1.
3. Host: Set Effect with `enableAxis=0x01`, `directionX=0`, gain 255 →
   `setEffect` sets `axisMagnitudes[0] = -1` (linear X projection).
4. Host: Set Constant Force `magnitude=5000` → `setConstantForce`.
5. Host: Effect Operation = start → `startTime` stamped, `state=1`.
6. Device tick: `calculate()` →
   - `calcNonConditionEffectForce`: `forceVector = 5000 · gain(255)/255 = 5000`.
   - `calcComponentForce`: `axisforce = -5000 · (-1) = 5000`, then
     `· global_gain(255)/255 = 5000`.
   - clamp → `axis_torque[0] = 5000`.
7. `getAxisTorque(0)` returns **5000**. On stop, `state` goes inactive and the
   next `calculate()` returns **0**.

That is the complete trip from a host command to a motor torque.

---

## Appendix: report IDs and effect types

**Report IDs** (`ffb_defs.h`):

| ID | Name | Dir | Meaning |
|---|---|---|---|
| `0x01` | `HID_ID_EFFREP` | OUT | Set Effect |
| `0x02` | `HID_ID_STATE` / `ENVREP` | IN / OUT | PID State (in) / Set Envelope (out) |
| `0x03` | `HID_ID_CONDREP` | OUT | Set Condition |
| `0x04` | `HID_ID_PRIDREP` | OUT | Set Periodic |
| `0x05` | `HID_ID_CONSTREP` | OUT | Set Constant Force |
| `0x06` | `HID_ID_RAMPREP` | OUT | Set Ramp |
| `0x0A` | `HID_ID_EFOPREP` | OUT | Effect Operation (start/stop) |
| `0x0B` | `HID_ID_BLKFRREP` | OUT | Block Free |
| `0x0C` | `HID_ID_CTRLREP` | OUT | Device Control (enable/disable/reset) |
| `0x0D` | `HID_ID_GAINREP` | OUT | Device Gain |
| `0x11` | `HID_ID_NEWEFREP` | FEAT | Create New Effect |
| `0x12` | `HID_ID_BLKLDREP` | FEAT | Block Load reply |
| `0x13` | `HID_ID_POOLREP` | FEAT | PID Pool reply |

**Effect types** (`ffb_defs.h`): `CONSTANT 0x01`, `RAMP 0x02`, `SQUARE 0x03`,
`SINE 0x04`, `TRIANGLE 0x05`, `SAWTOOTHUP 0x06`, `SAWTOOTHDOWN 0x07`,
`SPRING 0x08`, `DAMPER 0x09`, `INERTIA 0x0A`, `FRICTION 0x0B`, `CUSTOM 0x0C`.

---

*The FFB effect math, HID descriptor layout, and parsing logic originate from
[OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) by Yannick Richter and
contributors. This library is a re-packaging for embedded use; design credit is
upstream.*
