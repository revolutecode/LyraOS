// SPDX-License-Identifier: 0BSD
// LyraOS — Minimal COM1 serial output for diagnostics

#pragma once

#include <stdint.h>

namespace LyraOS::Serial {

/// Initialise COM1 at 115200 baud (8N1). No-op if already initialised.
void Init() noexcept;

/// Write a single byte to COM1, blocking until the transmit buffer is ready.
void PutChar(char c) noexcept;

/// Write a null-terminated string to COM1.
void Puts(const char* str) noexcept;

} // namespace LyraOS::Serial
