################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.c \
../Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.c 

OBJS += \
./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.o \
./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.o 

C_DEPS += \
./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.d \
./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/%.o Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/%.su Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/%.cyclo: ../Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/%.c Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Middlewares/JakaSimonic/ForceFeedback-core-library-legacy -I../Wheel/Inc -Og -ffunction-sections -fdata-sections -mslow-flash-data -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-JakaSimonic-2f-ForceFeedback-2d-core-2d-library-2d-legacy

clean-Middlewares-2f-JakaSimonic-2f-ForceFeedback-2d-core-2d-library-2d-legacy:
	-$(RM) ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.cyclo ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.d ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.o ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb.su ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.cyclo ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.d ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.o ./Middlewares/JakaSimonic/ForceFeedback-core-library-legacy/ffb_math.su

.PHONY: clean-Middlewares-2f-JakaSimonic-2f-ForceFeedback-2d-core-2d-library-2d-legacy

