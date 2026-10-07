/* XFB (YUYV) to N64 RGBA5551 for gDPUpdateColorImage, Mario Kart's billboard.
   The same output as the original per-pixel version: the same float column and
   row arithmetic, done once per column instead of once per pixel, and the
   same integer YUV formula with table clamps instead of branches.
   tests/yuyv_convert_test.py compares the two. dcbz/dcbt prefetching made it
   slower on the Wii: 2.85 against 2.55 ms per Mario Kart conversion. */
#ifndef YUYV_CONVERT_H
#define YUYV_CONVERT_H

#ifndef GLN64_YUYV_PRODUCTS
#define GLN64_YUYV_PRODUCTS 1 // Exact lookup; 0 retains the benchmark reference.
#endif

// width x height N64 pixels into dst (16-bit words swapped, as in RDRAM), from
// an XFB of w YUYV pixels per row. Rows are scaled by scaleY unless unscaledRows.
static void YUYV_ToRGBA5551(u16 *dst, const u32 *xfb, u32 width, u32 height, u32 w,
                            float scaleX, float scaleY, bool unscaledRows)
{
	// clamp(v, 0, 255) >> 3 at [v + 512]; the formula gives -277 <= v <= 534.
	static u8 clamp5[1280];
#if GLN64_YUYV_PRODUCTS
	static int products[5][256];
#endif
	if (!clamp5[1279])
	{
		for (int i = 0; i < 1280; i++)
			clamp5[i] = (i < 512 ? 0 : i > 767 ? 255 : i - 512) >> 3;
#if GLN64_YUYV_PRODUCTS
		for (int i = 0; i < 256; i++) {
			products[0][i] = 298 * (i - 16) + 128;
			products[1][i] = 409 * (i - 128);
			products[2][i] = 100 * (i - 128);
			products[3][i] = 208 * (i - 128);
			products[4][i] = 516 * (i - 128);
		}
#endif
	}

	u16 col[640];
	const u32 cols = width < 640 ? width : 640;
	for (u32 x = 0; x < cols; x++)
		col[x] = (u32)(x * scaleX);

	u32 i = 0;
	for (u32 y = 0; y < height; y++)
	{
		const u32 row = (unscaledRows ? y : (u32)(y * scaleY)) * w;
		for (u32 x = 0; x < width; x++)
		{
			const u32 px = row + (x < 640 ? col[x] : (u32)(x * scaleX));
			const u32 yuyv = xfb[px >> 1];
#if GLN64_YUYV_PRODUCTS
			const u32 Y = ((px & 1) ? yuyv >> 8 : yuyv >> 24) & 0xFF;
			const u32 U = (yuyv >> 16) & 0xFF, V = yuyv & 0xFF;
			const int C = products[0][Y];
			dst[i ^ 1] = (clamp5[((C + products[1][V]) >> 8) + 512] << 11) |
			             (clamp5[((C - products[2][U] - products[3][V]) >> 8) + 512] << 6) |
			             (clamp5[((C + products[4][U]) >> 8) + 512] << 1) | 1;
#else
			const int C = 298 * (int)(((px & 1) ? yuyv >> 8 : yuyv >> 24) & 0xFF) - 298 * 16 + 128;
			const int D = (int)((yuyv >> 16) & 0xFF) - 128;
			const int E = (int)(yuyv & 0xFF) - 128;
			dst[i ^ 1] = (clamp5[((C + 409 * E) >> 8) + 512] << 11) |
			             (clamp5[((C - 100 * D - 208 * E) >> 8) + 512] << 6) |
			             (clamp5[((C + 516 * D) >> 8) + 512] << 1) | 1;
#endif
			i++;
		}
	}
}

#endif
