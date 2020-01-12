//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "core/types.h"
#include "gfx/blender.h"
#include "util/wgt2allg.h"

extern "C" {
    unsigned long _blender_trans16(unsigned long x, unsigned long y, unsigned long n);
    unsigned long _blender_trans15(unsigned long x, unsigned long y, unsigned long n);
}

// the allegro "inline" ones are not actually inline, so #define
// over them to speed it up
#define getr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define getg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define getb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define geta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define makeacol32(r,g,b,a) ((r << _rgb_r_shift_32) | (g << _rgb_g_shift_32) | (b << _rgb_b_shift_32) | (a << _rgb_a_shift_32))

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color15_light(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
    rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol15(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
// n is the last parameter passed to draw_lit_sprite
unsigned long _myblender_color16_light(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
    rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol16(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color32_light(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr32(x), getg32(x), getb32(x), &xh, &xs, &xv);
    rgb_to_hsv(getr32(y), getg32(y), getb32(y), &yh, &ys, &yv);

    // adjust luminance
    yv -= (1.0 - ((float)n / 250.0));
    if (yv < 0.0) yv = 0.0;

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makeacol32(r, g, b, geta32(y));
}

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color15(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
    rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol15(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color16(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
    rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makecol16(r, g, b);
}

// Take hue and saturation of blend colour, luminance of image
unsigned long _myblender_color32(unsigned long x, unsigned long y, unsigned long n)
{
    float xh, xs, xv;
    float yh, ys, yv;
    int r, g, b;

    rgb_to_hsv(getr32(x), getg32(x), getb32(x), &xh, &xs, &xv);
    rgb_to_hsv(getr32(y), getg32(y), getb32(y), &yh, &ys, &yv);

    hsv_to_rgb(xh, xs, yv, &r, &g, &b);

    return makeacol32(r, g, b, geta32(y));
}

// trans24 blender, but preserve alpha channel from image
unsigned long _myblender_alpha_trans24(unsigned long x, unsigned long y, unsigned long n)
{
    unsigned long res, g, alph;

    if (n)
        n++;

    alph = y & 0xff000000;
    y &= 0x00ffffff;

    res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
    y &= 0xFF00;
    x &= 0xFF00;
    g = (x - y) * n / 256 + y;

    res &= 0xFF00FF;
    g &= 0xFF00;

    return res | g | alph;
}

void set_my_trans_blender(int r, int g, int b, int a)
{
    // use standard allegro 15 and 16 bit blenders, but customize
    // the 32-bit one to preserve the alpha channel
    set_blender_mode(_blender_trans15, _blender_trans16, _myblender_alpha_trans24, r, g, b, a);
}

// plain copy source to destination
// assign new alpha value as a summ of alphas.
unsigned long _additive_alpha_copysrc_blender(unsigned long x, unsigned long y, unsigned long n)
{
    unsigned long newAlpha = ((x & 0xff000000) >> 24) + ((y & 0xff000000) >> 24);

    if (newAlpha > 0xff) newAlpha = 0xff;

    return (newAlpha << 24) | (x & 0x00ffffff);
}

FORCEINLINE static unsigned long argb2argb_blend_core_fast_float(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
	union UNPACKED {
		uint8_t rgba[4];
		struct { uint8_t r,g,b,a; } components;
		unsigned long col;
	};

	UNPACKED u_src_col, u_dst_col;
	u_src_col.col = src_col;
	u_dst_col.col = dst_col;

	// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
	// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;

	float front_alpha = u_src_col.components.a / 255.0f;
	float back_alpha = u_src_col.components.a / 255.0f;

	float combined_alpha = front_alpha + back_alpha * (1.0f - front_alpha);

	for(int i=0;i<3;i++)
	{
		float front_rgb = u_src_col.rgba[i] / 255.0f;
		float back_rgb = u_dst_col.rgba[i] / 255.0f;
		float combined_rgb = (front_rgb * front_alpha + back_rgb * (1 - front_alpha) * back_alpha) / combined_alpha;
		u_dst_col.rgba[i] = (int)(combined_rgb*255);
	}
	
	u_dst_col.components.a = (int)(combined_alpha*255);
	
	return u_dst_col.col;
}

#include <assert.h>
FORCEINLINE static unsigned long argb2argb_blend_core_fast(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
	unsigned long dst_g, dst_alpha;
	src_alpha++;
	dst_alpha = geta32(dst_col);
	if (dst_alpha)
		dst_alpha++;

	// dst_g now contains the green hue from destination color
	dst_g   = (dst_col & 0x00FF00) * dst_alpha / 256;
	// dst_col now contains the red & blue hues from destination color
	dst_col = (dst_col & 0xFF00FF) * dst_alpha / 256;

	// res_g now contains the green hue of the pre-final color
	dst_g   = (((src_col & 0x00FF00) - (dst_g   & 0x00FF00)) * src_alpha / 256 + dst_g)   & 0x00FF00;
	// res_rb now contains the red & blue hues of the pre-final color
	dst_col = (((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col) & 0xFF00FF;

	// dst_alpha now contains the final alpha
	// we assume that final alpha will never be zero
	dst_alpha  = 256 - (256 - src_alpha) * (256 - dst_alpha) / 256;
	// src_alpha is now the final alpha factor made for being multiplied by,
	// instead of divided by: this makes it possible to use it in faster
	// calculation below
	src_alpha  = /* 256 * 256 == */ 0x10000 / dst_alpha;
	//or... 
	static uint32_t table[257] = {
		0, 65536, 32768, 21845, 16384, 13107, 10922, 9362, 8192, 7281, 6553, 5957, 5461, 5041, 4681, 4369, 4096, 3855, 3640, 3449, 3276, 3120, 2978, 2849, 2730, 2621, 2520, 2427, 2340, 2259, 2184, 2114, 2048, 1985, 1927, 1872, 1820, 1771, 1724, 1680, 1638, 1598, 1560, 1524, 1489, 1456, 1424, 1394, 1365, 1337, 1310, 1285, 1260, 1236, 1213, 1191, 1170, 1149, 1129, 1110, 1092, 1074, 1057, 1040, 1024, 1008, 992, 978, 963, 949, 936, 923, 910, 897, 885, 873, 862, 851, 840, 829, 819, 809, 799, 789, 780, 771, 762, 753, 744, 736, 728, 720, 712, 704, 697, 689, 682, 675, 668, 661, 655, 648, 642, 636, 630, 624, 618, 612, 606, 601, 595, 590, 585, 579, 574, 569, 564, 560, 555, 550, 546, 541, 537, 532, 528, 524, 520, 516, 512, 508, 504, 500, 496, 492, 489, 485, 481, 478, 474, 471, 468, 464, 461, 458, 455, 451, 448, 445, 442, 439, 436, 434, 431, 428, 425, 422, 420, 417, 414, 412, 409, 407, 404, 402, 399, 397, 394, 392, 390, 387, 385, 383, 381, 378, 376, 374, 372, 370, 368, 366, 364, 362, 360, 358, 356, 354, 352, 350, 348, 346, 344, 343, 341, 339, 337, 336, 334, 332, 330, 329, 327, 326, 324, 322, 321, 319, 318, 316, 315, 313, 312, 310, 309, 307, 306, 304, 303, 302, 300, 299, 297, 296, 295, 293, 292, 291, 289, 288, 287, 286, 284, 283, 282, 281, 280, 278, 277, 276, 275, 274, 273, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257, 256
	};
	assert(dst_alpha<=256);
	src_alpha = table[dst_alpha];

	// setting up final color hues
	dst_g   = (dst_g   * src_alpha / 256) & 0x00FF00;
	dst_col = (dst_col * src_alpha / 256) & 0xFF00FF;
	return dst_col | dst_g | (--dst_alpha << 24);
}

FORCEINLINE static unsigned long argb2argb_blend_core(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
    unsigned long dst_g, dst_alpha;
    src_alpha++;
    dst_alpha = geta32(dst_col);
    if (dst_alpha)
        dst_alpha++;

    // dst_g now contains the green hue from destination color
    dst_g   = (dst_col & 0x00FF00) * dst_alpha / 256;
    // dst_col now contains the red & blue hues from destination color
    dst_col = (dst_col & 0xFF00FF) * dst_alpha / 256;

    // res_g now contains the green hue of the pre-final color
    dst_g   = (((src_col & 0x00FF00) - (dst_g   & 0x00FF00)) * src_alpha / 256 + dst_g)   & 0x00FF00;
    // res_rb now contains the red & blue hues of the pre-final color
    dst_col = (((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col) & 0xFF00FF;

    // dst_alpha now contains the final alpha
    // we assume that final alpha will never be zero
    dst_alpha  = 256 - (256 - src_alpha) * (256 - dst_alpha) / 256;
    // src_alpha is now the final alpha factor made for being multiplied by,
    // instead of divided by: this makes it possible to use it in faster
    // calculation below
    src_alpha  = /* 256 * 256 == */ 0x10000 / dst_alpha;

    // setting up final color hues
    dst_g   = (dst_g   * src_alpha / 256) & 0x00FF00;
    dst_col = (dst_col * src_alpha / 256) & 0xFF00FF;
    return dst_col | dst_g | (--dst_alpha << 24);
}

// blend source to destination with respect to source and destination alphas;
// assign new alpha value as a multiplication of translucenses.
// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;
FORCEINLINE unsigned long _inline_argb2argb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
	//src_alpha (factor) of 0 is a special value indicating 1.0
    if (src_alpha > 0)
        src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;
    else
        src_alpha = geta32(src_col);
		//if the modulated (texture alpha * alpha factor) is transparent, return early
    if (src_alpha == 0)
        return dst_col;
		//go into more detailed blender
    return argb2argb_blend_core(src_col, dst_col, src_alpha);
}


// blend source to destination with respect to source and destination alphas;
// assign new alpha value as a multiplication of translucenses.
// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;
template<int KIND> unsigned long _inline_argb2argb_blender_fast(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
	if(KIND == 0) 
	{
		//weird special case
		src_alpha = geta32(src_col);
	}
	else if(KIND == 255)
	{
		//common case
		src_alpha = geta32(src_col);
	}
	else 
		src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;

	if (src_alpha == 0)
		return dst_col;

	//IS THIS SAFE? NOT SURE. BUT IT'S LOGICAL
	if(src_alpha == 0xFF)
		return src_col;

	return argb2argb_blend_core_fast(src_col, dst_col, src_alpha);
}

// blend source to destination with respect to source and destination alphas;
// assign new alpha value as a multiplication of translucenses.
// combined_alpha = front.alpha + back.alpha * (1 - front.alpha);
// combined_rgb = (front.rgb * front.alpha + back.rgb * (1 - front.alpha) * back.alpha) / combined_alpha;
unsigned long _argb2argb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
	return _inline_argb2argb_blender(src_col, dst_col, src_alpha);
}

unsigned long _rgb2argb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
    if (src_alpha == 0 || src_alpha == 0xFF)
        return src_col | 0xFF000000;
    return argb2argb_blend_core(src_col | 0xFF000000, dst_col, src_alpha);
}

unsigned long _argb2rgb_blender(unsigned long src_col, unsigned long dst_col, unsigned long src_alpha)
{
   unsigned long res, g;

   if (src_alpha > 0)
        src_alpha = geta32(src_col) * ((src_alpha & 0xFF) + 1) / 256;
    else
        src_alpha = geta32(src_col);
   if (src_alpha)
      src_alpha++;

   res = ((src_col & 0xFF00FF) - (dst_col & 0xFF00FF)) * src_alpha / 256 + dst_col;
   dst_col &= 0xFF00;
   src_col &= 0xFF00;
   g = (src_col - dst_col) * src_alpha / 256 + dst_col;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}

void set_additive_alpha_blender()
{
    set_blender_mode(nullptr, nullptr, _additive_alpha_copysrc_blender, 0, 0, 0, 0);
}

// sets the alpha channel to opaque. used when drawing a non-alpha sprite onto an alpha-sprite
unsigned long _opaque_alpha_blender(unsigned long x, unsigned long y, unsigned long n)
{
    return x | 0xff000000;
}

void set_opaque_alpha_blender()
{
    set_blender_mode(nullptr, nullptr, _opaque_alpha_blender, 0, 0, 0, 0);
}


#ifdef ALLEGRO_RATA_HACKS

#include "allegro/internal/aintern.h"
#include "allegro/../../src/c/cdefs32.h"

template<int KIND> void __linear_draw_trans_sprite32_WITH_argb2argb_blender(BITMAP *dst, BITMAP *src, int dx, int dy)
{
	int x, y, w, h;
	int dxbeg, dybeg;
	int sxbeg, sybeg;

	ASSERT(dst);
	ASSERT(src);

	if (dst->clip) {
		int tmp;

		tmp = dst->cl - dx;
		sxbeg = ((tmp < 0) ? 0 : tmp);
		dxbeg = sxbeg + dx;

		tmp = dst->cr - dx;
		w = ((tmp > src->w) ? src->w : tmp) - sxbeg;
		if (w <= 0)
			return;

		tmp = dst->ct - dy;
		sybeg = ((tmp < 0) ? 0 : tmp);
		dybeg = sybeg + dy;

		tmp = dst->cb - dy;
		h = ((tmp > src->h) ? src->h : tmp) - sybeg;
		if (h <= 0)
			return;
	}
	else {
		w = src->w;
		h = src->h;
		sxbeg = 0;
		sybeg = 0;
		dxbeg = dx;
		dybeg = dy;
	}

	if ((src->vtable->color_depth == 8) && (dst->vtable->color_depth != 8)) {
		bmp_select(dst);

		for (y = 0; y < h; y++) {
			unsigned char *s = src->line[sybeg + y] + sxbeg;
			PIXEL_PTR ds = OFFSET_PIXEL_PTR(bmp_read_line(dst, dybeg + y), dxbeg);
			PIXEL_PTR dd = OFFSET_PIXEL_PTR(bmp_write_line(dst, dybeg + y), dxbeg);

			for (x = w - 1; x >= 0; s++, INC_PIXEL_PTR(ds), INC_PIXEL_PTR(dd), x--) {
				unsigned long c = *s;
				#if PP_DEPTH == 8
				c = DTS_BLEND(blender, GET_PIXEL(ds), c);
				PUT_PIXEL(dd, c);
				#else
				if (!IS_SPRITE_MASK(src, c)) {
					c = _inline_argb2argb_blender_fast<KIND>(c, GET_PIXEL(ds), _blender_alpha);
					PUT_PIXEL(dd, c);
				}
				#endif
			}
		}

		bmp_unwrite_line(dst);
	}
	else if (dst->id & (BMP_ID_VIDEO | BMP_ID_SYSTEM)) {
		bmp_select(dst);

		for (y = 0; y < h; y++) {
			PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
			PIXEL_PTR ds = OFFSET_PIXEL_PTR(bmp_read_line(dst, dybeg + y), dxbeg);
			PIXEL_PTR dd = OFFSET_PIXEL_PTR(bmp_write_line(dst, dybeg + y), dxbeg);

			for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(ds), INC_PIXEL_PTR(dd), x--) {
				unsigned long c = GET_MEMORY_PIXEL(s);
				#if PP_DEPTH == 8
				c = DTS_BLEND(blender, GET_PIXEL(ds), c);
				PUT_PIXEL(dd, c);
				#else
				if (!IS_SPRITE_MASK(src, c)) {
					c = _inline_argb2argb_blender_fast<KIND>(c, GET_PIXEL(ds), _blender_alpha);
					PUT_PIXEL(dd, c);
				}
				#endif
			}
		}

		bmp_unwrite_line(dst);
	}
	else {
		for (y = 0; y < h; y++) {
			PIXEL_PTR s = OFFSET_PIXEL_PTR(src->line[sybeg + y], sxbeg);
			PIXEL_PTR d = OFFSET_PIXEL_PTR(dst->line[dybeg + y], dxbeg);

			for (x = w - 1; x >= 0; INC_PIXEL_PTR(s), INC_PIXEL_PTR(d), x--) {
				unsigned long c = GET_MEMORY_PIXEL(s);
				#if PP_DEPTH == 8
				c = DTS_BLEND(blender, GET_MEMORY_PIXEL(d), c);
				PUT_MEMORY_PIXEL(d, c);
				#else
				if (!IS_SPRITE_MASK(src, c)) {
					c = _inline_argb2argb_blender_fast<KIND>(c, GET_MEMORY_PIXEL(d), _blender_alpha);
					PUT_MEMORY_PIXEL(d, c);
				}
				#endif
			}
		}
	}
}

void _linear_draw_trans_sprite32_WITH_argb2argb_blender(BITMAP *dst, BITMAP *src, int dx, int dy)
{
	if(_blender_alpha == 255)
		__linear_draw_trans_sprite32_WITH_argb2argb_blender<255>(dst,src,dx,dy);
	else if(_blender_alpha == 0)
		__linear_draw_trans_sprite32_WITH_argb2argb_blender<0>(dst,src,dx,dy);
	else 
		__linear_draw_trans_sprite32_WITH_argb2argb_blender<-1>(dst,src,dx,dy);
}

#endif //ALLEGRO_RATA_HACKS
