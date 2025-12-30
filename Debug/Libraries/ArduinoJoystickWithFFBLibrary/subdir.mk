################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.cpp \
../Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.cpp \
../Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.cpp 

OBJS += \
./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.o \
./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.o \
./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.o 

CPP_DEPS += \
./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.d \
./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.d \
./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/ArduinoJoystickWithFFBLibrary/%.o Libraries/ArduinoJoystickWithFFBLibrary/%.su Libraries/ArduinoJoystickWithFFBLibrary/%.cyclo: ../Libraries/ArduinoJoystickWithFFBLibrary/%.cpp Libraries/ArduinoJoystickWithFFBLibrary/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CustomHID/Inc -I../Wheel -I../Wheel/common_templates -I../Libraries/ArduinoJoystickWithFFBLibrary -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Libraries-2f-ArduinoJoystickWithFFBLibrary

clean-Libraries-2f-ArduinoJoystickWithFFBLibrary:
	-$(RM) ./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.cyclo ./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.d ./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.o ./Libraries/ArduinoJoystickWithFFBLibrary/FfbEngine.su ./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.cyclo ./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.d ./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.o ./Libraries/ArduinoJoystickWithFFBLibrary/PIDReportHandler.su ./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.cyclo ./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.d ./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.o ./Libraries/ArduinoJoystickWithFFBLibrary/ffb_library.su

.PHONY: clean-Libraries-2f-ArduinoJoystickWithFFBLibrary

