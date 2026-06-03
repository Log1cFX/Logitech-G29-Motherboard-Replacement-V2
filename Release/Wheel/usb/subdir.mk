################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/usb/usb_callbacks.c \
../Wheel/usb/usb_descriptors.c \
../Wheel/usb/usb_processing.c 

C_DEPS += \
./Wheel/usb/usb_callbacks.d \
./Wheel/usb/usb_descriptors.d \
./Wheel/usb/usb_processing.d 

OBJS += \
./Wheel/usb/usb_callbacks.o \
./Wheel/usb/usb_descriptors.o \
./Wheel/usb/usb_processing.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/usb/%.o Wheel/usb/%.su Wheel/usb/%.cyclo: ../Wheel/usb/%.c Wheel/usb/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DRELEASE -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/core_definitions" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/hardware_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/software_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/usb" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/class/hid" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/common" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/device" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/host" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/osal" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/portable/st/stm32_fsdev" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/typec" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/ffb-lib/include" -Ofast -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-usb

clean-Wheel-2f-usb:
	-$(RM) ./Wheel/usb/usb_callbacks.cyclo ./Wheel/usb/usb_callbacks.d ./Wheel/usb/usb_callbacks.o ./Wheel/usb/usb_callbacks.su ./Wheel/usb/usb_descriptors.cyclo ./Wheel/usb/usb_descriptors.d ./Wheel/usb/usb_descriptors.o ./Wheel/usb/usb_descriptors.su ./Wheel/usb/usb_processing.cyclo ./Wheel/usb/usb_processing.d ./Wheel/usb/usb_processing.o ./Wheel/usb/usb_processing.su

.PHONY: clean-Wheel-2f-usb

