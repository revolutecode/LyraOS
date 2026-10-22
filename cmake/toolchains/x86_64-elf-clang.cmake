set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# O kernel não é uma aplicação Windows.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compilação cruzada para ELF x86-64.
set(CMAKE_C_COMPILER_TARGET x86_64-unknown-elf)
set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-elf)
set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-elf)

# Usar LLD, sem bibliotecas padrão do Windows.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")
