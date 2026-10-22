cmake_minimum_required(VERSION 3.25)
project(LyraOS LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Kernel executable
# ---------------------------------------------------------------------------
add_executable(LyraOS
    kernel/KernelMain.cpp
    kernel/Arch/X86_64/Video/Framebuffer.cpp
    kernel/Arch/X86_64/Serial.cpp
)

set_target_properties(LyraOS PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    OUTPUT_NAME "LyraOS"
    SUFFIX ".elf"
)

# Include path: kernel/ so that #include "limine.h" and
# #include "Arch/X86_64/..." resolve from the kernel directory.
target_include_directories(LyraOS PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel
)

# ---------------------------------------------------------------------------
# Compiler flags for freestanding x86-64 ELF
# ---------------------------------------------------------------------------
target_compile_options(LyraOS PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:
        --target=x86_64-unknown-elf
        -ffreestanding
        -fno-exceptions
        -fno-rtti
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -mno-red-zone
        -mcmodel=kernel
        -mno-sse
        -mno-sse2
        -mno-mmx
        -mno-80387
        -Wall
        -Wextra
        -Wpedantic
    >
)

# ---------------------------------------------------------------------------
# Linker flags — produce ELF via LLD with our linker script
# ---------------------------------------------------------------------------
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/kernel/Linker.ld")

target_link_options(LyraOS PRIVATE
    --target=x86_64-unknown-elf
    -fuse-ld=lld
    -nostdlib
    -static
    -Wl,--build-id=none
    -Wl,-z,max-page-size=0x1000
    -T ${LINKER_SCRIPT}
)

# Make CMake aware the linker script is a dependency
set_target_properties(LyraOS PROPERTIES
    LINK_DEPENDS "${LINKER_SCRIPT}"
)
