// SPDX-License-Identifier: 0BSD
// LyraOS — COM1 serial output (x86-64 I/O ports)

#include "Serial.hpp"

namespace LyraOS::Serial {

namespace {

constexpr uint16_t COM1 = 0x3F8;

inline void OutByte(uint16_t port, uint8_t val) noexcept
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline uint8_t InByte(uint16_t port) noexcept
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

bool g_initialised = false;

} // anonymous namespace

void Init() noexcept
{
    if (g_initialised) return;

    OutByte(COM1 + 1, 0x00);  // Disable interrupts
    OutByte(COM1 + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    OutByte(COM1 + 0, 0x01);  // Divisor = 1 → 115200 baud
    OutByte(COM1 + 1, 0x00);
    OutByte(COM1 + 3, 0x03);  // 8 bits, no parity, 1 stop bit
    OutByte(COM1 + 2, 0xC7);  // Enable FIFO, clear, 14-byte threshold
    OutByte(COM1 + 4, 0x0B);  // IRQs enabled, RTS/DSR set

    g_initialised = true;
}

void PutChar(char c) noexcept
{
    // Wait for transmit buffer to be empty (bit 5 of LSR)
    while ((InByte(COM1 + 5) & 0x20) == 0) {
        __asm__ volatile("pause");
    }
    OutByte(COM1, static_cast<uint8_t>(c));
}

void Puts(const char* str) noexcept
{
    if (!str) return;
    while (*str) {
        if (*str == '\n') PutChar('\r');
        PutChar(*str);
        ++str;
    }
}

} // namespace LyraOS::Serial
