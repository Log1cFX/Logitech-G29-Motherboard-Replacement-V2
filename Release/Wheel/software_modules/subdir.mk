################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/software_modules/sw_actuator.c \
../Wheel/software_modules/sw_buttons.c \
../Wheel/software_modules/sw_sensor.c \
../Wheel/software_modules/sw_shifter.c 

C_DEPS += \
./Wheel/software_modules/sw_actuator.d \
./Wheel/software_modules/sw_buttons.d \
./Wheel/software_modules/sw_sensor.d \
./Wheel/software_modules/sw_shifter.d 

OBJS += \
./Wheel/software_modules/sw_actuator.o \
./Wheel/software_modules/sw_buttons.o \
./Wheel/software_modules/sw_sensor.o \
./Wheel/software_modules/sw_shifter.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/software_modules/%.o Wheel/software_modules/%.su Wheel/software_modules/%.cyclo: ../Wheel/software_modules/%.c Wheel/software_modules/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DRELEASE -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/core_definitions" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/hardware_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/software_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/usb" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/class/hid" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/common" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/device" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/host" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/osal" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/portable/st/stm32_fsdev" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/typec" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/ffb-lib/include" -Ofast -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-software_modules

clean-Wheel-2f-software_modules:
	-$(RM) ./Wheel/software_modules/sw_actuator.cyclo ./Wheel/software_modules/sw_actuator.d ./Wheel/software_modules/sw_actuator.o ./Wheel/software_modules/sw_actuator.su ./Wheel/software_modules/sw_buttons.cyclo ./Wheel/software_modules/sw_buttons.d ./Wheel/software_modules/sw_buttons.o ./Wheel/software_modules/sw_buttons.su ./Wheel/software_modules/sw_sensor.cyclo ./Wheel/software_modules/sw_sensor.d ./Wheel/software_modules/sw_sensor.o ./Wheel/software_modules/sw_sensor.su ./Wheel/software_modules/sw_shifter.cyclo ./Wheel/software_modules/sw_shifter.d ./Wheel/software_modules/sw_shifter.o ./Wheel/software_modules/sw_shifter.su

.PHONY: clean-Wheel-2f-software_modules

