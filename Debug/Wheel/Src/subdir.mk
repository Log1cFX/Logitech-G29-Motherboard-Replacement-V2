################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/Src/app_hid_desc.c \
../Wheel/Src/app_usb_hid.c \
../Wheel/Src/hw_analog_input.c \
../Wheel/Src/hw_digital_input.c \
../Wheel/Src/hw_magnetometer.c \
../Wheel/Src/hw_motor_driver.c \
../Wheel/Src/steeringwheel.c \
../Wheel/Src/sw_actuator.c \
../Wheel/Src/sw_buttons.c \
../Wheel/Src/sw_sensor.c \
../Wheel/Src/sw_shifter.c \
../Wheel/Src/util.c 

C_DEPS += \
./Wheel/Src/app_hid_desc.d \
./Wheel/Src/app_usb_hid.d \
./Wheel/Src/hw_analog_input.d \
./Wheel/Src/hw_digital_input.d \
./Wheel/Src/hw_magnetometer.d \
./Wheel/Src/hw_motor_driver.d \
./Wheel/Src/steeringwheel.d \
./Wheel/Src/sw_actuator.d \
./Wheel/Src/sw_buttons.d \
./Wheel/Src/sw_sensor.d \
./Wheel/Src/sw_shifter.d \
./Wheel/Src/util.d 

OBJS += \
./Wheel/Src/app_hid_desc.o \
./Wheel/Src/app_usb_hid.o \
./Wheel/Src/hw_analog_input.o \
./Wheel/Src/hw_digital_input.o \
./Wheel/Src/hw_magnetometer.o \
./Wheel/Src/hw_motor_driver.o \
./Wheel/Src/steeringwheel.o \
./Wheel/Src/sw_actuator.o \
./Wheel/Src/sw_buttons.o \
./Wheel/Src/sw_sensor.o \
./Wheel/Src/sw_shifter.o \
./Wheel/Src/util.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/Src/%.o Wheel/Src/%.su Wheel/Src/%.cyclo: ../Wheel/Src/%.c Wheel/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel/Inc -I../Libraries/ArduinoJoystickWithFFBLibrary -Og -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-Src

clean-Wheel-2f-Src:
	-$(RM) ./Wheel/Src/app_hid_desc.cyclo ./Wheel/Src/app_hid_desc.d ./Wheel/Src/app_hid_desc.o ./Wheel/Src/app_hid_desc.su ./Wheel/Src/app_usb_hid.cyclo ./Wheel/Src/app_usb_hid.d ./Wheel/Src/app_usb_hid.o ./Wheel/Src/app_usb_hid.su ./Wheel/Src/hw_analog_input.cyclo ./Wheel/Src/hw_analog_input.d ./Wheel/Src/hw_analog_input.o ./Wheel/Src/hw_analog_input.su ./Wheel/Src/hw_digital_input.cyclo ./Wheel/Src/hw_digital_input.d ./Wheel/Src/hw_digital_input.o ./Wheel/Src/hw_digital_input.su ./Wheel/Src/hw_magnetometer.cyclo ./Wheel/Src/hw_magnetometer.d ./Wheel/Src/hw_magnetometer.o ./Wheel/Src/hw_magnetometer.su ./Wheel/Src/hw_motor_driver.cyclo ./Wheel/Src/hw_motor_driver.d ./Wheel/Src/hw_motor_driver.o ./Wheel/Src/hw_motor_driver.su ./Wheel/Src/steeringwheel.cyclo ./Wheel/Src/steeringwheel.d ./Wheel/Src/steeringwheel.o ./Wheel/Src/steeringwheel.su ./Wheel/Src/sw_actuator.cyclo ./Wheel/Src/sw_actuator.d ./Wheel/Src/sw_actuator.o ./Wheel/Src/sw_actuator.su ./Wheel/Src/sw_buttons.cyclo ./Wheel/Src/sw_buttons.d ./Wheel/Src/sw_buttons.o ./Wheel/Src/sw_buttons.su ./Wheel/Src/sw_sensor.cyclo ./Wheel/Src/sw_sensor.d ./Wheel/Src/sw_sensor.o ./Wheel/Src/sw_sensor.su ./Wheel/Src/sw_shifter.cyclo ./Wheel/Src/sw_shifter.d ./Wheel/Src/sw_shifter.o ./Wheel/Src/sw_shifter.su ./Wheel/Src/util.cyclo ./Wheel/Src/util.d ./Wheel/Src/util.o ./Wheel/Src/util.su

.PHONY: clean-Wheel-2f-Src

