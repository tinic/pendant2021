cmake_minimum_required(VERSION 3.10)

# M48x M480 M482 M482ZGCAE

project(pendant2021-mainfirmware C CXX ASM)

set(CC_FLAGS
    -flto
    -mcpu=cortex-m4 
    -Wnull-dereference
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wno-strict-aliasing 
    -Wno-format
    -ffast-math)

set(CXX_FLAGS 
    -Wall 
    -Wextra
    -Wdouble-promotion
    -Wshadow 
    -Wfloat-conversion 
    -Wformat=2 
    -std=c++17 
    -fno-rtti 
    -fno-exceptions)

set(LD_FLAGS
    -T${CMAKE_SOURCE_DIR}/Library/Device/Nuvoton/M480/Source/GCC/gcc_arm_128k.ld
    -flto
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -specs=nano.specs
    -lc
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections)

set(DEBUG_FLAGS
    -Og)

set(RELEASE_FLAGS   
    -Os)

set(DEFINITIONS ARM_MATH_CM4 __FPU_PRESENT)

set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

include_directories(.)
include_directories(Library/CMSIS/Include)
include_directories(Library/Device/Nuvoton/M480/Include)
include_directories(Library/UsbHostLib/inc)
include_directories(Library/UsbHostLib/src_msc)
include_directories(Library/UsbHostLib/src_uac)
include_directories(Library/StdDriver/inc)

file(GLOB_RECURSE DSP0_SRC ${CMAKE_SOURCE_DIR}/Library/CMSIS/DSP_Lib/Source/CommonTables/*.c)
file(GLOB_RECURSE DSP1_SRC ${CMAKE_SOURCE_DIR}/Library/CMSIS/DSP_Lib/Source/SupportFunctions/*.c)
file(GLOB_RECURSE DSP2_SRC ${CMAKE_SOURCE_DIR}/Library/CMSIS/DSP_Lib/Source/FastMathFunctions/*.c)
file(GLOB_RECURSE DSP3_SRC ${CMAKE_SOURCE_DIR}/Library/CMSIS/DSP_Lib/Source/BasicMathFunctions/*.c)

file(GLOB_RECURSE STDDRV_SRC ${CMAKE_SOURCE_DIR}/Library/StdDriver/src/*.c)

file(GLOB_RECURSE USB_CORE_SRC ${CMAKE_SOURCE_DIR}/Library/UsbHostLib/src_core/*.c)
file(GLOB_RECURSE USB_CDC_SRC ${CMAKE_SOURCE_DIR}/Library/UsbHostLib/src_cdc/*.c)
file(GLOB_RECURSE USB_MSC_SRC ${CMAKE_SOURCE_DIR}/Library/UsbHostLib/src_msc/*.c)
file(GLOB_RECURSE USB_UAC_SRC ${CMAKE_SOURCE_DIR}/Library/UsbHostLib/src_uac/*.c)
file(GLOB_RECURSE USB_HID_SRC ${CMAKE_SOURCE_DIR}/Library/UsbHostLib/src_hid/*.c)

file(GLOB_RECURSE NUVOTON0_SRC ${CMAKE_SOURCE_DIR}/Library/Device/Nuvoton/GCC/*.c)
file(GLOB_RECURSE NUVOTON1_SRC ${CMAKE_SOURCE_DIR}/Library/Device/Nuvoton/M480/Source/*.c)

add_executable(${PROJECT_NAME}.elf 
    ${CMAKE_SOURCE_DIR}/main.c
    ${DSP0_SRC}
    ${DSP1_SRC}
    ${DSP2_SRC}
    ${DSP3_SRC}
    ${DSP3_SRC}
    ${STDDRV_SRC}
    ${USB_CORE_SRC}
#    ${USB_CDC_SRC}
#    ${USB_MSC_SRC}
#    ${USB_UAC_SRC}
#    ${USB_HID_SRC}
    ${NUVOTON0_SRC}
    ${NUVOTON1_SRC}
    ${CMAKE_SOURCE_DIR}/Library/Device/Nuvoton/M480/Source/GCC/startup_M480.S)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${DEFINITIONS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CC_FLAGS})
target_link_options(${PROJECT_NAME}.elf PRIVATE ${LD_FLAGS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
