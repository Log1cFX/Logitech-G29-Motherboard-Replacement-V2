################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ffb-lib/src/ffb_axis_local.cpp \
../ffb-lib/src/ffb_axis_local_c.cpp \
../ffb-lib/src/ffb_biquad.cpp \
../ffb-lib/src/ffb_c.cpp \
../ffb-lib/src/ffb_calculator.cpp \
../ffb-lib/src/ffb_descriptor.cpp \
../ffb-lib/src/ffb_metrics.cpp \
../ffb-lib/src/ffb_metrics_c.cpp \
../ffb-lib/src/ffb_parser.cpp 

OBJS += \
./ffb-lib/src/ffb_axis_local.o \
./ffb-lib/src/ffb_axis_local_c.o \
./ffb-lib/src/ffb_biquad.o \
./ffb-lib/src/ffb_c.o \
./ffb-lib/src/ffb_calculator.o \
./ffb-lib/src/ffb_descriptor.o \
./ffb-lib/src/ffb_metrics.o \
./ffb-lib/src/ffb_metrics_c.o \
./ffb-lib/src/ffb_parser.o 

CPP_DEPS += \
./ffb-lib/src/ffb_axis_local.d \
./ffb-lib/src/ffb_axis_local_c.d \
./ffb-lib/src/ffb_biquad.d \
./ffb-lib/src/ffb_c.d \
./ffb-lib/src/ffb_calculator.d \
./ffb-lib/src/ffb_descriptor.d \
./ffb-lib/src/ffb_metrics.d \
./ffb-lib/src/ffb_metrics_c.d \
./ffb-lib/src/ffb_parser.d 


# Each subdirectory must supply rules for building sources it contributes
ffb-lib/src/%.o ffb-lib/src/%.su ffb-lib/src/%.cyclo: ../ffb-lib/src/%.cpp ffb-lib/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DRELEASE -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/core_definitions" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/hardware_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/software_modules" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/Wheel/usb" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/class/hid" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/common" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/device" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/host" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/osal" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/portable/st/stm32_fsdev" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/tinyusb/src/typec" -I"C:/Users/raffi/STM32CubeIDE/workspace_1.16.0/g29-MB-Replacement/ffb-lib/include" -Og -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-ffb-2d-lib-2f-src

clean-ffb-2d-lib-2f-src:
	-$(RM) ./ffb-lib/src/ffb_axis_local.cyclo ./ffb-lib/src/ffb_axis_local.d ./ffb-lib/src/ffb_axis_local.o ./ffb-lib/src/ffb_axis_local.su ./ffb-lib/src/ffb_axis_local_c.cyclo ./ffb-lib/src/ffb_axis_local_c.d ./ffb-lib/src/ffb_axis_local_c.o ./ffb-lib/src/ffb_axis_local_c.su ./ffb-lib/src/ffb_biquad.cyclo ./ffb-lib/src/ffb_biquad.d ./ffb-lib/src/ffb_biquad.o ./ffb-lib/src/ffb_biquad.su ./ffb-lib/src/ffb_c.cyclo ./ffb-lib/src/ffb_c.d ./ffb-lib/src/ffb_c.o ./ffb-lib/src/ffb_c.su ./ffb-lib/src/ffb_calculator.cyclo ./ffb-lib/src/ffb_calculator.d ./ffb-lib/src/ffb_calculator.o ./ffb-lib/src/ffb_calculator.su ./ffb-lib/src/ffb_descriptor.cyclo ./ffb-lib/src/ffb_descriptor.d ./ffb-lib/src/ffb_descriptor.o ./ffb-lib/src/ffb_descriptor.su ./ffb-lib/src/ffb_metrics.cyclo ./ffb-lib/src/ffb_metrics.d ./ffb-lib/src/ffb_metrics.o ./ffb-lib/src/ffb_metrics.su ./ffb-lib/src/ffb_metrics_c.cyclo ./ffb-lib/src/ffb_metrics_c.d ./ffb-lib/src/ffb_metrics_c.o ./ffb-lib/src/ffb_metrics_c.su ./ffb-lib/src/ffb_parser.cyclo ./ffb-lib/src/ffb_parser.d ./ffb-lib/src/ffb_parser.o ./ffb-lib/src/ffb_parser.su

.PHONY: clean-ffb-2d-lib-2f-src

