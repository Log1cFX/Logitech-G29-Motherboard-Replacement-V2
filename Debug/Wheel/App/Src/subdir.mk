################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/App/Src/axis.c \
../Wheel/App/Src/wheel.c \
../Wheel/App/Src/wheel_buttons.c 

OBJS += \
./Wheel/App/Src/axis.o \
./Wheel/App/Src/wheel.o \
./Wheel/App/Src/wheel_buttons.o 

C_DEPS += \
./Wheel/App/Src/axis.d \
./Wheel/App/Src/wheel.d \
./Wheel/App/Src/wheel_buttons.d 


# Each subdirectory must supply rules for building sources it contributes
Wheel/App/Src/%.o Wheel/App/Src/%.su Wheel/App/Src/%.cyclo: ../Wheel/App/Src/%.c Wheel/App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel/Core/Inc -I../Wheel/App/Inc -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-App-2f-Src

clean-Wheel-2f-App-2f-Src:
	-$(RM) ./Wheel/App/Src/axis.cyclo ./Wheel/App/Src/axis.d ./Wheel/App/Src/axis.o ./Wheel/App/Src/axis.su ./Wheel/App/Src/wheel.cyclo ./Wheel/App/Src/wheel.d ./Wheel/App/Src/wheel.o ./Wheel/App/Src/wheel.su ./Wheel/App/Src/wheel_buttons.cyclo ./Wheel/App/Src/wheel_buttons.d ./Wheel/App/Src/wheel_buttons.o ./Wheel/App/Src/wheel_buttons.su

.PHONY: clean-Wheel-2f-App-2f-Src

