cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32-uwb)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_SOURCE_DIR}/Core/Src/uwb.c
    ${CMAKE_SOURCE_DIR}/Core/Src/util.c
    ${CMAKE_SOURCE_DIR}/Core/Src/uart.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
# 定义自定义目标：make download
set(OPENOCD_SCRIPTS "/home/budali11/xpack-openocd-0.11.0-5/scripts")
add_custom_target(download
    COMMAND echo "开始烧录程序..."
    COMMAND sudo openocd -s ${OPENOCD_SCRIPTS}
            -f ${OPENOCD_SCRIPTS}/interface/stlink.cfg 
            -f ${OPENOCD_SCRIPTS}/target/stm32f1x.cfg
            -c init
            -c "reset halt"
            -c "flash write_image erase ${CMAKE_PROJECT_NAME}.elf 0x00000000"
            -c reset
            -c shutdown
    COMMAND echo "烧录完成！"
    DEPENDS ${PROJECT_NAME}  # 依赖于主项目构建
    COMMENT "通过OpenOCD烧录固件到设备"
    USES_TERMINAL  # 允许在终端显示输出
)

add_custom_target(connect
    COMMAND sudo openocd -s ${OPENOCD_SCRIPTS}
            -f ${OPENOCD_SCRIPTS}/interface/stlink.cfg 
            -f ${OPENOCD_SCRIPTS}/target/stm32f1x.cfg
)
