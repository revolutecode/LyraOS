// SPDX-License-Identifier: 0BSD
// LyraOS — Framebuffer console (header)

#pragma once

#include <stdint.h>

namespace LyraOS::Video {

/// Initialise the framebuffer console with the given linear framebuffer.
/// Returns false if any parameter is invalid.
bool FramebufferInit(void* base, uint64_t width, uint64_t height,
                     uint64_t pitch, uint16_t bpp,
                     uint8_t redMaskSize,   uint8_t redMaskShift,
                     uint8_t greenMaskSize, uint8_t greenMaskShift,
                     uint8_t blueMaskSize,  uint8_t blueMaskShift) noexcept;

/// Write a null-terminated string to the framebuffer console.
/// Handles '\n' for newline. Non-printable characters are skipped.
void FramebufferPuts(const char* str) noexcept;

/// Write a single character at the current cursor position and advance.
void FramebufferPutChar(char c) noexcept;

} // namespace LyraOS::Video
