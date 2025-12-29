The project is unfinished. In this current state, it is usable but not perfect, it has no tutorial yet neither.

The goal of this project is to make a replacement for the g29 motherboard (doesn't include buttons' moderboard), requiring only an stm32f103c8t3 (bluepill) and a BTS7960 motor driver (and soldering skills but I can try to make a custom PCB).

What has been done:
- Reading and calculating wheel angle
- Buttons + debounce logic
- Pedals
- Shifter
- HID sending info to PC
- PID, yes force feedback (with an asterix)
- Motor control

What needs to be done:
- Fix the force feedback as it isn't very reliable and some effects don't work at all
- Make a custom PCB to make it accessible to everyone

This project already has taken an enormous amount of time, probably +1000 Hours for research learning experimenting and everything that comes with it. The V2 in the name is because this is a direct try to improve propos' version of making a motherboard replacement for g29. It is also [his project](https://github.com/popos123/Logitech-G29-Motherboard-Replacement) that gave me inspiration to create my own version from litteraly scratch.

This project was created in STM32CubeIDE and configured inside STM32CubeMX. It uses HAL for peripheral control and the "USB device" library from ST (I think?). The library has been heavily modified and improved in some regions so don't rerun the configurator when setting up the project. The force feedback library is for now a modified version of YukiMingLaw's code, taken from his [ArduinoJoystickWithFFBLibrary](https://github.com/YukMingLaw/ArduinoJoystickWithFFBLibrary). The force feedback functinnality wouldn't be possible without his, JakaSimonic's and hoantv's work. I am well aware that openFFB-like projects exist and until this project isn't finished there's no reason to not use them, however I am confident that once this project will be in its final state, there will be no reason to not use it solely because of its simplicity (which is the end goal).

If you want to contribute to this project (I would be impressed) and you have any question about the code or how to set it up, open up an issue and I will answer.
