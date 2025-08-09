################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Wheel/Core/Src/g29_buttons.c \
../Wheel/Core/Src/g29_mlx90363.c 

OBJS += \
./Wheel/Core/Src/g29_buttons.o \
./Wheel/Core/Src/g29_mlx90363.o 

C_DEPS += \
./Wheel/Core/Src/g29_buttons.d \
./Wheel/Core/Src/g29_mlx90363.d 


# Each subdirectory must supply rules for building sources it contributes
Wheel/Core/Src/%.o Wheel/Core/Src/%.su Wheel/Core/Src/%.cyclo: ../Wheel/Core/Src/%.c Wheel/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel/Core/Inc -I../Wheel/App/Inc -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Wheel-2f-Core-2f-Src

clean-Wheel-2f-Core-2f-Src:
	-$(RM) ./Wheel/Core/Src/g29_buttons.cyclo ./Wheel/Core/Src/g29_buttons.d ./Wheel/Core/Src/g29_buttons.o ./Wheel/Core/Src/g29_buttons.su ./Wheel/Core/Src/g29_mlx90363.cyclo ./Wheel/Core/Src/g29_mlx90363.d ./Wheel/Core/Src/g29_mlx90363.o ./Wheel/Core/Src/g29_mlx90363.su

.PHONY: clean-Wheel-2f-Core-2f-Src

