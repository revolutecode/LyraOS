// SPDX-License-Identifier: 0BSD
// LyraOS — Framebuffer console (implementation)

#include "Framebuffer.hpp"
#include "Font8x8.hpp"

namespace LyraOS::Video {

namespace {

// Framebuffer state (module-local)
uint8_t*  g_fbBase       = nullptr;
uint64_t  g_fbWidth      = 0;
uint64_t  g_fbHeight     = 0;
uint64_t  g_fbPitch      = 0;  // bytes per scanline
uint16_t  g_fbBpp        = 0;
uint32_t  g_pixelWhite   = 0;  // pre-computed white pixel value
uint32_t  g_pixelBg      = 0;  // background colour (black)

// Cursor position (in character cells)
uint64_t  g_cursorCol    = 0;
uint64_t  g_cursorRow    = 0;

// Derived limits (in character cells)
uint64_t  g_maxCols      = 0;
uint64_t  g_maxRows      = 0;

// Simple scrolling: copy rows up by one character height.
void ScrollUp() noexcept
{
    const uint64_t rowBytes = g_fbPitch * FontGlyphHeight;
    const uint64_t totalRows = g_maxRows;

    // Move rows 1..N-1 → 0..N-2
    for (uint64_t y = 0; y < (totalRows - 1) * FontGlyphHeight; ++y) {
        uint8_t* dst = g_fbBase + y * g_fbPitch;
        const uint8_t* src = g_fbBase + (y + FontGlyphHeight) * g_fbPitch;
        for (uint64_t x = 0; x < g_fbPitch; ++x) {
            dst[x] = src[x];
        }
    }
    // Clear the last row
    for (uint64_t y = (totalRows - 1) * FontGlyphHeight;
         y < totalRows * FontGlyphHeight && y < g_fbHeight; ++y) {
        uint8_t* row = g_fbBase + y * g_fbPitch;
        for (uint64_t x = 0; x < g_fbPitch; ++x) {
            row[x] = 0;
        }
    }
}

void PutPixel(uint64_t x, uint64_t y, uint32_t colour) noexcept
{
    if (x >= g_fbWidth || y >= g_fbHeight) return;

    const uint64_t bytesPerPixel = g_fbBpp / 8;
    uint8_t* pixel = g_fbBase + y * g_fbPitch + x * bytesPerPixel;

    // We only support 32-bpp and 24-bpp
    if (bytesPerPixel == 4) {
        *reinterpret_cast<volatile uint32_t*>(pixel) = colour;
    } else if (bytesPerPixel == 3) {
        pixel[0] = static_cast<uint8_t>(colour);
        pixel[1] = static_cast<uint8_t>(colour >> 8);
        pixel[2] = static_cast<uint8_t>(colour >> 16);
    }
}

void DrawGlyph(uint64_t col, uint64_t row, char c, uint32_t fg) noexcept
{
    const int idx = static_cast<int>(c) - FontFirstChar;
    if (idx < 0 || idx > (FontLastChar - FontFirstChar)) return;

    const uint64_t baseX = col * FontGlyphWidth;
    const uint64_t baseY = row * FontGlyphHeight;

    for (int gy = 0; gy < FontGlyphHeight; ++gy) {
        const uint8_t bits = Font8x8Data[idx][gy];
        for (int gx = 0; gx < FontGlyphWidth; ++gx) {
            const uint32_t colour = (bits & (0x80 >> gx)) ? fg : g_pixelBg;
            PutPixel(baseX + gx, baseY + gy, colour);
        }
    }
}

constexpr uint32_t FontScale = 3;

void DrawScaledPixel(
    uint64_t x,
    uint64_t y,
    uint32_t colour
) noexcept
{
    for (uint32_t offsetY = 0; offsetY < FontScale; ++offsetY) {
        for (uint32_t offsetX = 0; offsetX < FontScale; ++offsetX) {
            PutPixel(
                x * FontScale + offsetX,
                y * FontScale + offsetY,
                colour
            );
        }
    }
}

} // anonymous namespace

bool FramebufferInit(void* base, uint64_t width, uint64_t height,
                     uint64_t pitch, uint16_t bpp,
                     uint8_t redMaskSize,   uint8_t redMaskShift,
                     uint8_t greenMaskSize, uint8_t greenMaskShift,
                     uint8_t blueMaskSize,  uint8_t blueMaskShift) noexcept
{
    if (!base || width == 0 || height == 0 || pitch == 0) return false;
    if (bpp != 32 && bpp != 24) return false;

    g_fbBase   = static_cast<uint8_t*>(base);
    g_fbWidth  = width;
    g_fbHeight = height;
    g_fbPitch  = pitch;
    g_fbBpp    = bpp;

    // Compute white pixel using the actual mask layout
    g_pixelWhite = (((1u << redMaskSize)   - 1u) << redMaskShift)
                 | (((1u << greenMaskSize) - 1u) << greenMaskShift)
                 | (((1u << blueMaskSize)  - 1u) << blueMaskShift);
    g_pixelBg = 0x00000000;  // black

    g_cursorCol = 0;
    g_cursorRow = 0;
    g_maxCols   = width  / FontGlyphWidth;
    g_maxRows   = height / FontGlyphHeight;

    if (g_maxCols == 0 || g_maxRows == 0) return false;

    return true;
}

void FramebufferPutChar(char c) noexcept
{
    if (!g_fbBase) return;

    if (c == '\n') {
        g_cursorCol = 0;
        ++g_cursorRow;
        if (g_cursorRow >= g_maxRows) {
            ScrollUp();
            g_cursorRow = g_maxRows - 1;
        }
        return;
    }

    // Skip non-printable characters
    if (c < FontFirstChar || c > FontLastChar) return;

    DrawGlyph(g_cursorCol, g_cursorRow, c, g_pixelWhite);
    ++g_cursorCol;

    if (g_cursorCol >= g_maxCols) {
        g_cursorCol = 0;
        ++g_cursorRow;
        if (g_cursorRow >= g_maxRows) {
            ScrollUp();
            g_cursorRow = g_maxRows - 1;
        }
    }
}

void FramebufferPuts(const char* str) noexcept
{
    if (!str) return;
    while (*str) {
        FramebufferPutChar(*str);
        ++str;
    }
}

} // namespace LyraOS::Video
