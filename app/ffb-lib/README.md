# Force Feedback Library

A standalone C++ force-feedback library extracted from the
[OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) firmware. The
effect math is bit-for-bit identical to the original; the project-specific
glue (STM32 HAL, TinyUSB, FreeRTOS, flash storage, configurator CLI) has
been stripped so the library drops into any bare-metal microcontroller
project.

## What it does

The library:

- Decodes USB HID PID reports from the host (DirectInput force-feedback
  effects: Constant, Ramp, Periodic, Spring, Damper, Friction, Inertia,
  Envelope shaping, conditions).
- Maintains a fixed-size pool of effect slots.
- Computes torque per axis every tick.
- Ships a ready-to-use HID Report Descriptor (1-axis and 2-axis variants).

It does **not** care about:

- Which USB stack you use (TinyUSB, ST USB, LUFA, anything).
- Which motor driver you use (PWM, BLDC controller, CAN, anything).
- Which encoder you use.
- Whether you have an RTOS.

## In 60 seconds

```cpp
#include "ffb/ffb.h"

// 1. One-time setup
ffb::Library lib(/*axis_count=*/1, { my_millis, my_micros });
lib.setSendReportCallback(my_usb_send);

// 2. Hand the descriptor to your USB stack
uint16_t len;
const uint8_t* desc = ffb::Library::descriptor1Axis(&len);
my_usb_set_hid_descriptor(desc, len);

// 3. In your USB Set/Get Report callbacks
void on_usb_set_report(uint8_t id, const uint8_t* buf, uint16_t n) { lib.hidOut(id, buf, n); }
uint16_t on_usb_get_report(uint8_t id, uint8_t* buf, uint16_t n)   { return lib.hidGet(id, buf, n); }

// 4. Every FFB tick (1 kHz default)
lib.setAxisState(0, { wheel_pos_int16, wheel_speed_dps, wheel_accel_dpss });
lib.calculate();
int32_t torque = lib.getAxisTorque(0);   // -0x7fff .. 0x7fff
```

## Library layout

```
ffb-lib/
├── include/ffb/
│   ├── ffb.h              Main facade (the only header most users need)
│   ├── ffb_c.h            Plain-C wrapper
│   ├── ffb_config.h       Compile-time knobs (FFB_MAX_AXIS, FFB_MAX_EFFECTS)
│   ├── ffb_descriptor.h   Pre-built HID descriptor + assembly macros
│   ├── ffb_calculator.h   The math engine (advanced)
│   ├── ffb_parser.h       The USB report decoder (advanced)
│   ├── ffb_effect.h       Internal Effect struct
│   ├── ffb_biquad.h       Biquad filter
│   ├── ffb_metrics.h      Optional: derives speed/accel from raw position
│   ├── ffb_axis_local.h   Optional: idle spring, endstop, axis-local effects
│   ├── ffb_metrics_c.h    Optional: C wrapper for ffb_metrics.h
│   └── ffb_axis_local_c.h Optional: C wrapper for ffb_axis_local.h
├── src/                   Implementations
├── examples/
│   ├── minimal_cpp.cpp    Smallest C++ integration
│   ├── minimal_c.c        Same, using the C API
│   ├── c_wrappers.c       C API + metrics/axis-local helpers + tuning
│   ├── tinyusb_glue.cpp   TinyUSB report-callback wiring
│   └── usb_descriptors_tinyusb.c  Device/config/string descriptors you supply
└── CMakeLists.txt
```

## Compile-time configuration

Override these by defining them before including any `ffb/*` header, or
on the compiler command line (`-DFFB_MAX_AXIS=1`):

| Macro | Default | Purpose |
|---|---|---|
| `FFB_MAX_AXIS` | `2` | Number of physical axes (1, 2, or 3) |
| `FFB_MAX_EFFECTS` | `40` | Effect-slot pool size |
| `FFB_DEFAULT_SAMPLERATE_HZ` | `1000.0f` | Initial calculation rate |
| `FFB_ID_OFFSET` | `0` | Added to every report ID (for composite HID stacks) |
| `FFB_LOG(msg)` | no-op | Hook for debug logging |

## Platform requirements

You must supply two functions:

```c
uint32_t millis(void);  // free-running millisecond counter
uint32_t micros(void);  // free-running microsecond counter
```

Both can wrap around — the library only uses deltas. If your MCU only
has `millis()`, derive `micros()` from any hardware timer.

That's it. No heap is needed at runtime: the library allocates its
effect pool and biquad filters statically. The memory footprint with
defaults (`FFB_MAX_EFFECTS=40`, `FFB_MAX_AXIS=2`) is about 9 KB BSS.

## Building

```sh
cmake -B build
cmake --build build
# produces libffb.a
```

Or just add the `src/*.cpp` files to your existing build and put `include/`
on the include path.

## What the host sees

The shipped HID **report** descriptor enumerates as a standard HID joystick
with the PID (Physical Input Device) usage page, which Windows' DirectInput
stack recognises as a force-feedback device. You can verify by:

1. Plugging in the device.
2. Running `joy.cpl` -> Properties -> Test. The "Forces" tab should appear.
3. Launching any DirectInput-aware game.

> **You still write your own USB configuration descriptor.** The library only
> provides the HID *report* descriptor; the USB *device*, *configuration*, and
> *string* descriptors (VID/PID, endpoints, product name) are yours to supply.
> The HID interface must expose an **OUT** endpoint as well as an IN endpoint
> (FFB is host-driven), e.g. TinyUSB's `TUD_HID_INOUT_DESCRIPTOR`. See
> `examples/usb_descriptors_tinyusb.c` and DOCUMENTATION.md §2.3.

## Supported effects

All standard DirectInput PID effects:

| Effect | Implementation |
|---|---|
| Constant Force | Direct magnitude, optional low-pass filter |
| Ramp | Linear interpolation start → end |
| Sine, Square, Triangle, Saw Up/Down | Periodic generators with phase + offset |
| Spring | Coefficient × (position − cpOffset), deadband + saturation |
| Damper | Same but on filtered velocity |
| Inertia | Same but on filtered acceleration |
| Friction | Velocity-based with sinusoidal ramp-up below a configurable speed threshold |
| Envelope | Attack/sustain/fade modulation applied on top of magnitude |
| Conditions | Single or per-axis condition parameter blocks |

The math is copied byte-for-byte from OpenFFBoard's `EffectsCalculator.cpp`.

## Opt-in helpers

`ffb_metrics.h` derives filtered speed and acceleration from a raw
position stream, if you don't already have those. Same math as the
original `Axis::updateMetrics()`.

`ffb_axis_local.h` provides the "feel" effects that are NOT requested
by the host (idle self-centring spring when FFB is off, software endstop,
always-on damper/friction/inertia). Same math as `Axis::calculateAxisEffects()`.

Both are independent — include only what you want. If you skip them,
neither file is compiled into your binary.

## License

Same as the upstream OpenFFBoard project. See LICENSE in the parent
repository.

## Credits

The FFB effect math, the HID descriptor layout, and the underlying parsing
logic come from [OpenFFBoard](https://github.com/Ultrawipf/OpenFFBoard) by
Yannick Richter and contributors (Jon Lidgard, Vincent Manoukian, and others).
This library is a re-packaging for embedded use; all credit for the design
goes upstream.
