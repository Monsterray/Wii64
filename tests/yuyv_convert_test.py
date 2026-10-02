"""glN64's fast XFB-to-RGBA5551 copy against the original per-pixel version."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "YUYVConvert.h"

// The original gDPUpdateColorImage loop (Wii64 1.6.8), with its inputs as arguments.
static inline u16 YUYV_to_RGBA5551(u8 y, u8 u, u8 v)
{
    int C = y - 16;
    int D = u - 128;
    int E = v - 128;
    int R = (298 * C + 409 * E + 128) >> 8;
    int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
    int B = (298 * C + 516 * D + 128) >> 8;
    if (R < 0) R = 0; else if (R > 255) R = 255;
    if (G < 0) G = 0; else if (G > 255) G = 255;
    if (B < 0) B = 0; else if (B > 255) B = 255;
    u16 r5 = R >> 3;
    u16 g5 = G >> 3;
    u16 b5 = B >> 3;
    return (r5 << 11) | (g5 << 6) | (b5 << 1) | 1;
}

static void reference(u16 *dst, const u32 *xfb, u32 width, u32 height, float scaleX, float scaleY, u32 GXheight)
{
    const u32 w = width * scaleX;
    u32 i = 0;
    for (u32 y = 0; y < height; y++)
    {
        u32 frameY = GXheight < 480 ? y : (y * scaleY);
        for (u32 x = 0; x < width; x++)
        {
            u32 frameX = (u32)(x * scaleX);
            u32 px = frameY * w + frameX;
            u32 yuyv = xfb[px >> 1];
            u8 y0 = (yuyv >> 24) & 0xFF;
            u8 u  = (yuyv >> 16) & 0xFF;
            u8 y1 = (yuyv >>  8) & 0xFF;
            u8 v  = (yuyv >>  0) & 0xFF;
            u8 yval = (px & 1) ? y1 : y0;
            dst[i ^ 1] = YUYV_to_RGBA5551(yval, u, v);
            i++;
        }
    }
}

int main()
{
    // Every Y/U/V triple, through both the even (Y0) and odd (Y1) pixel.
    for (u32 y = 0; y < 256; y++)
        for (u32 u = 0; u < 256; u++)
            for (u32 v = 0; v < 256; v++) {
                const u32 word[2] = { y << 24 | u << 16 | (255 - y) << 8 | v, 0 };
                u16 got[2];
                YUYV_ToRGBA5551(got, word, 2, 1, 2, 1.0f, 1.0f, true);
                assert(got[1] == YUYV_to_RGBA5551(y, u, v));
                assert(got[0] == YUYV_to_RGBA5551(255 - y, u, v));
            }
    // Whole frames: integer and fractional scales, scaled and unscaled rows,
    // and widths past the 640-entry column table.
    std::vector<u32> xfb(640 * 576 * 4);
    srand(1);
    for (auto &word : xfb) word = (u32)rand() << 16 ^ (u32)rand();
    struct { u32 width, height; float sx, sy; u32 gxh; } cases[] = {
        {320, 240, 2.0f, 2.0f, 480}, {320, 240, 2.0f, 2.0f, 240}, {320, 240, 2.0f, 2.2f, 528},
        {320, 240, 2.25f, 2.0f, 480}, {640, 480, 1.0f, 1.0f, 480}, {640, 240, 1.0f, 2.0f, 480},
        {700, 1, 2.0f, 2.0f, 480}, {321, 240, 2.0f, 2.0f, 480}, {16, 3, 2.0f, 2.0f, 480}, {1024, 1, 1.0f, 1.0f, 480}, {1, 1, 2.0f, 2.0f, 480},
        {0, 240, 2.0f, 2.0f, 480}, {297, 211, 640 / 297.0f, 480 / 211.0f, 480},
    };
    for (auto &c : cases)
        for (u32 skew = 0; skew < 32; skew += 3) { // guard words on both sides of dst
            std::vector<u16> want(c.width * c.height + 96, 0xBEEF), got(want);
            reference(want.data() + 32 + skew, xfb.data(), c.width, c.height, c.sx, c.sy, c.gxh);
            YUYV_ToRGBA5551(got.data() + 32 + skew, xfb.data(), c.width, c.height,
                            (u32)(c.width * c.sx), c.sx, c.sy, c.gxh < 480);
            assert(got == want);
        }
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='wii64-yuyv-') as directory:
    source = Path(directory) / 'test.cpp'
    source.write_text(fixture)
    binary = Path(directory) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-function', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-I', str(root / 'glN64_GX'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('YUYV to RGBA5551: every Y/U/V value and whole frames match the original: ok')
