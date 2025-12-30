################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/app_hid_desc.c \
../Wheel/app_usb_hid.c \
../Wheel/steeringwheel.c \
../Wheel/util.c 

C_DEPS += \
./Wheel/app_hid_desc.d \
./Wheel/app_usb_hid.d \
./Wheel/steeringwheel.d \
./Wheel/util.d 

OBJS += \
./Wheel/app_hid_desc.o \
./Wheel/app_usb_hid.o \
./Wheel/steeringwheel.o \
./Wheel/util.o 


# Each subdirectory must supply rules for building sources it contributes
Wheel/%.o Wheel/%.su Wheel/%.cyclo: ../Wheel/%.c Wheel/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel/common_templates -I../Libraries/ArduinoJoystickWithFFBLibrary -I../Wheel -Og -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel

clean-Wheel:
	-$(RM) ./Wheel/app_hid_desc.cyclo ./Wheel/app_hid_desc.d ./Wheel/app_hid_desc.o ./Wheel/app_hid_desc.su ./Wheel/app_usb_hid.cyclo ./Wheel/app_usb_hid.d ./Wheel/app_usb_hid.o ./Wheel/app_usb_hid.su ./Wheel/steeringwheel.cyclo ./Wheel/steeringwheel.d ./Wheel/steeringwheel.o ./Wheel/steeringwheel.su ./Wheel/util.cyclo ./Wheel/util.d ./Wheel/util.o ./Wheel/util.su

.PHONY: clean-Wheel

