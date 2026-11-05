# The firmware of the G29 motherboard replacement: STM32F103CB, HAL, TinyUSB
# and ffb-lib. The sources, flags, pre-build and post-build steps are the ones
# of the STM32CubeIDE project it replaces. Configure with a preset
# (CMakePresets.json): Debug or Release
cmake_minimum_required(VERSION 3.22)

project(g29-MB-Replacement C CXX ASM)

if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
  message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Debug or Release, not \"${CMAKE_BUILD_TYPE}\"")
endif()

# CMake adds flags of its own per build type (-g, -O3 -DNDEBUG...): none here,
# the flags of each build type are set below
foreach(lang C CXX ASM)
  set(CMAKE_${lang}_FLAGS_DEBUG "")
  set(CMAKE_${lang}_FLAGS_RELEASE "")
endforeach()

# SOURCES
# Every source of these folders, as STM32CubeIDE built them. A new file is
# picked up at the next build
file(GLOB_RECURSE FIRMWARE_SOURCES CONFIGURE_DEPENDS
  Core/*.c Core/*.s
  Drivers/*.c
  Wheel/*.c Wheel/*.cpp
  ffb-lib/src/*.c ffb-lib/src/*.cpp
  tinyusb/src/*.c)

# Debug builds link rdimon, which brings the system calls (semihosting)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(REMOVE_ITEM FIRMWARE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/syscalls.c)
endif()

add_executable(${PROJECT_NAME} ${FIRMWARE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
  Core/Inc
  Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
  Drivers/STM32F1xx_HAL_Driver/Inc
  Drivers/CMSIS/Device/ST/STM32F1xx/Include
  Drivers/CMSIS/Include
  Wheel
  Wheel/core_definitions
  Wheel/hardware_modules
  Wheel/software_modules
  Wheel/usb
  tinyusb/src
  tinyusb/src/class/hid
  tinyusb/src/common
  tinyusb/src/device
  tinyusb/src/host
  tinyusb/src/osal
  tinyusb/src/portable/st/stm32_fsdev
  tinyusb/src/typec
  ffb-lib/include
  Wheel/ffb
  Wheel/error_handling
  Wheel/file_system
  Wheel/file_system/file_types
  Wheel/settings
  Wheel/command)

# The assembler only gets DEBUG
target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<COMPILE_LANGUAGE:C,CXX>:USE_HAL_DRIVER>
  $<$<COMPILE_LANGUAGE:C,CXX>:STM32F103xB>
  $<$<CONFIG:Debug>:DEBUG>)

# FLAGS
# The core flags come from the toolchain file. Per language, for both build
# types
set(C_FLAGS -std=gnu11 -ffunction-sections -fdata-sections -mslow-flash-data
  -Wall -fstack-usage -fcyclomatic-complexity --specs=nano.specs)
set(CXX_FLAGS -std=gnu++14 -ffunction-sections -fdata-sections
  -fno-exceptions -fno-rtti -fno-use-cxa-atexit
  -Wall -fstack-usage -fcyclomatic-complexity --specs=nano.specs)
# The startup file goes through the preprocessor
set(ASM_FLAGS -x assembler-with-cpp --specs=nano.specs)

# Per build type, for every language
set(DEBUG_FLAGS -Og -g3)
set(RELEASE_FLAGS -Ofast)

target_compile_options(${PROJECT_NAME} PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>"
  "$<$<COMPILE_LANGUAGE:ASM>:${ASM_FLAGS}>"
  "$<$<CONFIG:Debug>:${DEBUG_FLAGS}>"
  "$<$<CONFIG:Release>:${RELEASE_FLAGS}>")

# A warning of GCC on TinyUSB's FIFO that is not a bug
set_source_files_properties(tinyusb/src/common/tusb_fifo.c
  PROPERTIES COMPILE_OPTIONS -Wno-stringop-overflow)

# LINK
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F103CBTX_FLASH.ld)
target_link_options(${PROJECT_NAME} PRIVATE
  -T${LINKER_SCRIPT}
  -Wl,-Map=${PROJECT_NAME}.map
  -Wl,--gc-sections
  -static
  --specs=nano.specs
  $<$<CONFIG:Debug>:-specs=rdimon.specs>
  $<$<CONFIG:Release>:--specs=nosys.specs>)
target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:rdimon>
  -Wl,--start-group c m stdc++ supc++ -Wl,--end-group)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# BOOTLOADER
# Built before the firmware by its own Makefile, with the configuration of
# this build type: see cmake/bootloader.cmake. Its Makefile needs GNU make and
# a POSIX shell (MSYS2), and arm-none-eabi-gcc on the PATH
set(BOOTLOADER_CONFIG_Debug "-DWINUSB_SUPPORT -DENABLE_CHECKSUM -DENABLE_PINRST_DFU_BOOT")
set(BOOTLOADER_CONFIG_Release "${BOOTLOADER_CONFIG_Debug} -DENABLE_WATCHDOG=20")
find_program(BOOTLOADER_MAKE NAMES make
  HINTS "C:/msys64/usr/bin"
  DOC "GNU make with a POSIX shell, for the bootloader")
if(NOT BOOTLOADER_MAKE)
  message(FATAL_ERROR "No make for the bootloader: set BOOTLOADER_MAKE, or put MSYS2's make on the PATH")
endif()

add_custom_target(bootloader
  COMMAND ${CMAKE_COMMAND}
    -DMAKE=${BOOTLOADER_MAKE}
    "-DCONFIG=${BOOTLOADER_CONFIG_${CMAKE_BUILD_TYPE}}"
    -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/../bootloader
    -DSTAMP=${CMAKE_CURRENT_SOURCE_DIR}/.bootloader.config.stamp
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/bootloader.cmake
  COMMENT "Building the bootloader"
  VERBATIM)
add_dependencies(${PROJECT_NAME} bootloader)

# POST-BUILD
# The checksum and the settings CRCs patched into the ELF, the .bin, the size
# and the listing: see cmake/postbuild.cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    -DPYTHON=${Python3_EXECUTABLE}
    -DCHECKSUM=${CMAKE_CURRENT_SOURCE_DIR}/tools/checksum.py
    -DELF=$<TARGET_FILE:${PROJECT_NAME}>
    -DBIN=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    -DLIST=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.list
    -DSIZE=${CMAKE_SIZE}
    -DOBJDUMP=${CMAKE_OBJDUMP}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/postbuild.cmake
  VERBATIM)

# FLASH
# The target "flash" builds, then writes the bootloader and the firmware
# through the ST-LINK with STM32CubeProgrammer, without a debugger. The final
# reset is a software one: a reset by the NRST pin alone keeps the bootloader
# in DFU mode (ENABLE_PINRST_DFU_BOOT)
file(GLOB PROGRAMMER_HINTS "$ENV{LOCALAPPDATA}/stm32cube/bundles/programmer/*/bin")
find_program(STM32_PROGRAMMER NAMES STM32_Programmer_CLI HINTS ${PROGRAMMER_HINTS}
  DOC "STM32CubeProgrammer command line, for the flash target")
if(STM32_PROGRAMMER)
  add_custom_target(flash
    COMMAND ${STM32_PROGRAMMER} -c port=SWD mode=UR
      -w ${CMAKE_CURRENT_SOURCE_DIR}/../bootloader/bootloader-dfu-fw.elf -v
      -w $<TARGET_FILE:${PROJECT_NAME}> -v
      -rst
    USES_TERMINAL
    VERBATIM)
  add_dependencies(flash ${PROJECT_NAME})
endif()
