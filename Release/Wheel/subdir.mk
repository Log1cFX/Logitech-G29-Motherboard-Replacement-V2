################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/steeringwheel.c \
../Wheel/util.c 

C_DEPS += \
./Wheel/steeringwheel.d \
./Wheel/util.d 

OBJS += \
./Wheel/steeringwheel.o \
./Wheel/util.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/%.o Wheel/%.su Wheel/%.cyclo: ../Wheel/%.c Wheel/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DRELEASE -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/core_definitions" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/hardware_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/software_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/usb" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/class/hid" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/common" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/device" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/host" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/osal" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/portable/st/stm32_fsdev" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/typec" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/ffb-lib/include" -Ofast -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel

clean-Wheel:
	-$(RM) ./Wheel/steeringwheel.cyclo ./Wheel/steeringwheel.d ./Wheel/steeringwheel.o ./Wheel/steeringwheel.su ./Wheel/util.cyclo ./Wheel/util.d ./Wheel/util.o ./Wheel/util.su

.PHONY: clean-Wheel

