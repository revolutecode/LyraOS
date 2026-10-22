# LyraOS

LyraOS is an experimental, educational microkernel project. The initial target is x86-64, developed natively on Windows with Clang/LLD, CMake and Ninja. Limine is planned as the bootloader and QEMU as the first test machine.

## Current milestone: foundation

This starter establishes the project layout and a freestanding x86-64 kernel entry point. It is **not yet a bootable image**: the Limine boot files, linker layout, and first boot test are the next milestone.

## Design rules

- Microkernel direction: keep the privileged kernel small; plan drivers and system services for user space.
- Keep architecture-specific code under `kernel/Arch/<architecture>`.
- Keep architecture-independent code under `kernel/Base`.
- Start with x86-64; add AArch64 and RISC-V only after the first target boots.
- No hosted C++ runtime, exceptions, RTTI, or standard-library dependencies in kernel code.

## Toolchain target

The intended compiler target is `x86_64-unknown-elf`. On Windows, Clang and LLD can emit ELF objects, but the exact linker script and boot image packaging still need to be completed and verified.
