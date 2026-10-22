// SPDX-License-Identifier: 0BSD
// LyraOS — Kernel entry point (Limine boot protocol)

#include <stdint.h>
#include <stddef.h>

#include "Arch/X86_64/Video/Framebuffer.hpp"
#include "Arch/X86_64/Serial.hpp"

// ---------------------------------------------------------------------------
// Limine protocol header (vendored from the official limine.h)
// ---------------------------------------------------------------------------
#include "limine.h"

// ---------------------------------------------------------------------------
// Limine requests — placed in special sections scanned by the bootloader
// ---------------------------------------------------------------------------

// Request start marker
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

// Base revision: we require revision 3 (Limine ≥ v8.x)
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// Framebuffer request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id       = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr,
};

// Request end marker
__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// ---------------------------------------------------------------------------
// Kernel helpers
// ---------------------------------------------------------------------------

namespace LyraOS::Kernel {

[[noreturn]] void Halt() noexcept
{
    for (;;) {
        __asm__ volatile("cli; hlt" ::: "memory");
    }
}

} // namespace LyraOS::Kernel

// ---------------------------------------------------------------------------
// Kernel entry — called by Limine
// ---------------------------------------------------------------------------

extern "C" [[noreturn]] void _start() noexcept
{
    // 1. Initialise serial output for diagnostics
    LyraOS::Serial::Init();

    // 2. Check that the bootloader supports our requested base revision
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        LyraOS::Serial::Puts("[LyraOS] FATAL: Limine base revision not supported\n");
        LyraOS::Kernel::Halt();
    }

    LyraOS::Serial::Puts("[LyraOS] Boot: base revision OK\n");

    // 3. Check framebuffer response
    if (framebuffer_request.response == nullptr ||
        framebuffer_request.response->framebuffer_count < 1) {
        LyraOS::Serial::Puts("[LyraOS] FATAL: No framebuffer available\n");
        LyraOS::Kernel::Halt();
    }

    struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];

    LyraOS::Serial::Puts("[LyraOS] Framebuffer acquired\n");

    // 4. Initialise our framebuffer console
    bool fbOk = LyraOS::Video::FramebufferInit(
        fb->address,
        fb->width, fb->height, fb->pitch, fb->bpp,
        fb->red_mask_size,   fb->red_mask_shift,
        fb->green_mask_size, fb->green_mask_shift,
        fb->blue_mask_size,  fb->blue_mask_shift
    );

    if (!fbOk) {
        LyraOS::Serial::Puts("[LyraOS] FATAL: Framebuffer init failed (bad params)\n");
        LyraOS::Kernel::Halt();
    }

    // 5. Print "Hello, world!" on the framebuffer
    LyraOS::Video::FramebufferPuts("Hello, world!\n");
    LyraOS::Video::FramebufferPuts("\n");
    LyraOS::Video::FramebufferPuts("LyraOS kernel booted successfully.\n");
    LyraOS::Video::FramebufferPuts("Microkernel foundation (L4-inspired).\n");

    LyraOS::Serial::Puts("[LyraOS] Hello, world! displayed on framebuffer\n");
    LyraOS::Serial::Puts("[LyraOS] Entering halt loop\n");

    // 6. Halt — wait for interrupts in a safe loop
    LyraOS::Kernel::Halt();
}
