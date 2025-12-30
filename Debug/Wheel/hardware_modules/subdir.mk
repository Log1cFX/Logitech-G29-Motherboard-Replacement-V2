################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/hardware_modules/hw_analog_input.c \
../Wheel/hardware_modules/hw_digital_input.c \
../Wheel/hardware_modules/hw_magnetometer.c \
../Wheel/hardware_modules/hw_motor_driver.c 

C_DEPS += \
./Wheel/hardware_modules/hw_analog_input.d \
./Wheel/hardware_modules/hw_digital_input.d \
./Wheel/hardware_modules/hw_magnetometer.d \
./Wheel/hardware_modules/hw_motor_driver.d 

OBJS += \
./Wheel/hardware_modules/hw_analog_input.o \
./Wheel/hardware_modules/hw_digital_input.o \
./Wheel/hardware_modules/hw_magnetometer.o \
./Wheel/hardware_modules/hw_motor_driver.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/hardware_modules/%.o Wheel/hardware_modules/%.su Wheel/hardware_modules/%.cyclo: ../Wheel/hardware_modules/%.c Wheel/hardware_modules/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel/common_templates -I../Libraries/ArduinoJoystickWithFFBLibrary -I../Wheel -Og -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-hardware_modules

clean-Wheel-2f-hardware_modules:
	-$(RM) ./Wheel/hardware_modules/hw_analog_input.cyclo ./Wheel/hardware_modules/hw_analog_input.d ./Wheel/hardware_modules/hw_analog_input.o ./Wheel/hardware_modules/hw_analog_input.su ./Wheel/hardware_modules/hw_digital_input.cyclo ./Wheel/hardware_modules/hw_digital_input.d ./Wheel/hardware_modules/hw_digital_input.o ./Wheel/hardware_modules/hw_digital_input.su ./Wheel/hardware_modules/hw_magnetometer.cyclo ./Wheel/hardware_modules/hw_magnetometer.d ./Wheel/hardware_modules/hw_magnetometer.o ./Wheel/hardware_modules/hw_magnetometer.su ./Wheel/hardware_modules/hw_motor_driver.cyclo ./Wheel/hardware_modules/hw_motor_driver.d ./Wheel/hardware_modules/hw_motor_driver.o ./Wheel/hardware_modules/hw_motor_driver.su

.PHONY: clean-Wheel-2f-hardware_modules

