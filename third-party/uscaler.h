#ifndef USC__INCLUDE_GUARD
#define USC__INCLUDE_GUARD

///////////////////////////////////////////////////////////////////////////
///
/// uscaler - minimalistic image resize library
/// <https://codeberg.org/NRK/slashtmp/src/branch/master/media/uscaler>
///
/// Single-header C library for resizing images with a separable
/// bicubic upscaler. Catmull-Rom weights are used by default.
/// When downscaling, uses box/area scaling instead.
///
/// Usage:
///
/// Embed the implementation in *one* C file of your program:
///
///    #define USC_IMPLEMENTATION
///    #include "uscaler.h"
///
/// Then fill out the input and output `UscImageInfo` structs and call `usc_resize()`.
///
///    UscImageInfo in  = { .width = src_w, .height = src_h, .pixel_type = USC_PIX_ARGB32 };
///    UscImageInfo out = { .width = dst_w, .height = dst_h };
///    if (usc_resize(src_ptr, in, dst_ptr, out, NULL) < 0) {
///        fprintf(stderr, "resize failed\n");
///    }
///
/// See the extended api section below for the extended api documentation.
///
/// Optional build configuration:
///
///    USC_API        custom linkage/visibility for the public functions
///    USC_ASSERT     override the internal assert macro
///    USC_NO_STDLIB  disable the built-in malloc/free fallback
///    USC_NO_SIMD    force the scalar implementation
///    USC_NO_SSE     disable SSE path specifically
///
/////////////////////////////////////////////////////////////////////////////

#ifndef USC_API
	#define USC_API extern
#endif
#ifndef USC_ASSERT
	#define USC_ASSERT(X) ((X) ? (void)0 : __builtin_unreachable())
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

/// NOTE: currently only supports obvious formats. more format support can be added on request.
typedef enum {
	USC_PIX_NONE,    /// can be specified on output image to inherit the same type as input
	USC_PIX_ARGB32,  /// uint32_t, packed 0xAARRGGBB
	USC_PIX_BGRA8,   /// bytes in B,G,R,A order
	USC_PIX_ARGB8,   /// bytes in A,R,G,B order
	USC_PIX_END,     /// internal, should not be used
} UscPixelType;

typedef enum {
	USC_IF_PREMULTIPLIED = 1 << 0,  /// the rgb channels should be read/written as alpha premultiplied
	USC_IF_IGNORE_ALPHA  = 1 << 1,  /// ignore alpha; input reads and output writes fully opaque alpha
	                                /// NOTE: PREMULTIPLIED is redundant if IGNORE_ALPHA is set
} UscImageFlags;

typedef struct {
	int64_t width, height;
	ptrdiff_t stride_bytes;  /// bytes to the next row. optional, can be set to 0 for tightly packed data.
	UscPixelType pixel_type;
	UscImageFlags flags;
} UscImageInfo;

typedef struct {
	/// allocation routine, should ensure multiplication doesn't overflow
	void *(*alloc)(void *, ptrdiff_t, ptrdiff_t);
	void  (*dealloc)(void *, void *, ptrdiff_t);
	/// userdata that will be passed to the alloc/dealloc function as first argument
	void *userdata;
	/// bicubic weight table. size is fixed at 256. taps are fixed at 4.
	/// can be built with `usc_build_bicubic_weight_table()`
	float (*bicubic_weights)[4];
} UscExtra;

/// returns 0 on success negative on error.
/// `extra` is optional, can be null to use the defaults.
USC_API int usc_resize(
	const void *restrict input_data, UscImageInfo input_info,
	void *restrict output_data, UscImageInfo output_info,
	UscExtra *extra
);

/// ==== Extended API ====

/// NOTE: everything in this struct is private and should not be accessed directly by the user.
typedef struct UscContext UscContext;

/// Initialize a resizer context.
/// Once initialized, can be reused to resize multiple images of the same dimensions.
/// Use `usc_ctx_set_input_buffer` to set the input image buffer.
/// Use `usc_ctx_release` to free any allocated resources once done.
USC_API int usc_ctx_init(
	UscContext *ctx,
	int64_t input_w, int64_t input_h,
	int64_t output_w, int64_t output_h,
	UscExtra *extra
);

/// reset the context and call dealloc on all allocated buffers.
USC_API void usc_ctx_release(UscContext *ctx);

/// Same as `UscImageInfo`, except width/height come from the context.
/// `stride_bytes == 0` means tightly packed rows: full input width for bound
/// input buffers, or the currently configured output subrectangle width for
/// output buffers.
typedef struct UscBuffer {
	union { const void *restrict cdata; void *restrict data; };
	ptrdiff_t stride_bytes;
	UscPixelType pixel_type;
	UscImageFlags flags;
} UscBuffer;

/// Bind the input buffer used by `usc_resize_extended()`.
/// For performance reasons, some of decoded the input data may be cached.
/// If the input image has been mutated in-place, you must call this again to
/// flush the old cache before the next resize.
USC_API void usc_ctx_set_input_buffer(UscContext *ctx, UscBuffer input);

/// Only output the specified subrectangle.
/// The subrectangle must not extend beyond the conceptual output dimensions
/// specified in `usc_ctx_init()`. If not set, defaults to the full output.
/// This only configures the subrectangle to be written via `usc_resize_extended()`.
/// Scaling is still based on the full conceptual output geometry.
USC_API void usc_ctx_set_output_subrect(
	UscContext *ctx,
	int64_t x, int64_t y, int64_t w, int64_t h
);

/// Write the configured output subrectangle into `output` using the input bound
/// with `usc_ctx_set_input_buffer()`.
USC_API void usc_resize_extended(UscContext *ctx, UscBuffer output);

/// can be used to build custom bicubic bc weights
/// the default weight table uses B = 0.0f, C = 0.5f (ie. catmull-rom)
USC_API void usc_build_bicubic_weight_table(float (*wt)[4], float B, float C);

////// API end

//////////////////////////// private struct definition

typedef int64_t Usc__Fx64;

typedef struct {
	int left_idx, right_idx;
	float left_weight, right_weight;
} usc__BoxInfo;

struct UscContext {
	int64_t input_w, input_h;
	int64_t output_w, output_h;
	int64_t output_subrect_x, output_subrect_y;
	int64_t output_subrect_w, output_subrect_h;
	UscBuffer input_buffer;
	UscExtra extra;
	float *decode_row;
	ptrdiff_t decode_row_size;
	int64_t decode_row_y;
	float *prefixsum_row;
	ptrdiff_t prefixsum_row_size;
	usc__BoxInfo *box_x;
	ptrdiff_t box_x_size;
	float box_x_range_weight;
	float *acc_row;
	ptrdiff_t acc_row_size;
	float *ring_buf;
	ptrdiff_t ring_buf_size;
	float *ring[4];
	int ring_head;
	int ring_tail;
	ptrdiff_t bicubic_weights_size;
	int use_box_x, use_box_y;
	Usc__Fx64 map_x_start, map_delta_x;
	Usc__Fx64 map_y_start, map_delta_y;
	double inv_map_delta_x, inv_map_delta_y;
};


//////////////////////////
#ifdef USC_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
	#define USC__FLATTEN __attribute((flatten))
#else
	#define USC__FLATTEN
#endif

#if !defined(USC_NO_STDLIB)
	#include <stdlib.h>
	static void *
	usc__alloc(void *userdata, ptrdiff_t n, ptrdiff_t size)
	{
		(void)userdata;
		if (size && (PTRDIFF_MAX/size) < n)
			return NULL;
		return malloc(n * size);
	}
	static void
	usc__dealloc(void *userdata, void *ptr, ptrdiff_t size)
	{
		(void)userdata; (void)size;
		free(ptr);
	}
#else
	static void *
	usc__alloc(void *userdata, ptrdiff_t n, ptrdiff_t size)
	{
		(void)userdata; (void)n; (void)size;
		return 0;
	}
	static void
	usc__dealloc(void *userdata, void *ptr, ptrdiff_t size)
	{
		(void)userdata; (void)ptr; (void)size;
	}
#endif

////////////////////////////

static int64_t
usc__clamp(int64_t v, int64_t lo, int64_t hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

static void
usc__memzero(void *sv, ptrdiff_t n)
{
	uint8_t *s = sv;
	for (ptrdiff_t i = 0; i < n; ++i) s[i] = 0;
}

static void *
usc__alloc_array(UscExtra extra, ptrdiff_t n, ptrdiff_t size, ptrdiff_t *size_out)
{
	void *ptr = extra.alloc(extra.userdata, n, size);
	*size_out = ptr != NULL ? n * size : 0;
	return ptr;
}

static int
usc__need_premultiply(UscImageFlags flags)
{
	return !(flags & USC_IF_IGNORE_ALPHA) && !(flags & USC_IF_PREMULTIPLIED);
}

static uint32_t
usc__read_as_bgra32(const void *p, UscPixelType type, UscImageFlags flags)
{
	uint32_t bgra32;
	switch (type) {
	case USC_PIX_ARGB32: {
		uint32_t argb32 = *(const uint32_t *)p;
		bgra32 = __builtin_bswap32(argb32);
	} break;
	case USC_PIX_ARGB8: {
		const uint8_t *p8 = p; enum { A, R, G, B };
		bgra32 = ((uint32_t)p8[B] << 24) | ((uint32_t)p8[G] << 16) |
		         ((uint32_t)p8[R] <<  8) | ((uint32_t)p8[A] <<  0);
	} break;
	case USC_PIX_BGRA8: {
		const uint8_t *p8 = p; enum { B, G, R, A };
		bgra32 = ((uint32_t)p8[B] << 24) | ((uint32_t)p8[G] << 16) |
		         ((uint32_t)p8[R] <<  8) | ((uint32_t)p8[A] <<  0);
	} break;
	default: USC_ASSERT(!"unreachable");
	}
	if (flags & USC_IF_IGNORE_ALPHA)
		bgra32 |= 0xFF;
	return bgra32;
}

static void
usc__write_encoded(void *dst_row, int64_t x, uint32_t pixels[4], UscPixelType write_type)
{
	enum { A, R, G, B };
	switch (write_type) {
	case USC_PIX_ARGB32: {
		uint32_t *dst = (uint32_t *)dst_row + x;
		*dst = (pixels[A] << 24) | (pixels[R] << 16) |
		       (pixels[G] <<  8) | (pixels[B] <<  0);
	} break;
	case USC_PIX_ARGB8: {
		uint8_t *dst = (uint8_t *)dst_row + x*4;
		dst[0] = pixels[A];
		dst[1] = pixels[R];
		dst[2] = pixels[G];
		dst[3] = pixels[B];
	} break;
	case USC_PIX_BGRA8: {
		uint8_t *dst = (uint8_t *)dst_row + x*4;
		dst[0] = pixels[B];
		dst[1] = pixels[G];
		dst[2] = pixels[R];
		dst[3] = pixels[A];
	} break;
	default: USC_ASSERT(!"unreachable"); break;
	}
}

////////////////////////////

#if !defined(USC_NO_SIMD) && !defined(USC_NO_SSE) && defined(__SSE4_1__)

#include <smmintrin.h>

typedef __m128 usc__pix4;

static usc__pix4 usc__pix4_set1(float v) { return _mm_set1_ps(v); }
static usc__pix4 usc__pix4_load(const float *p) { return _mm_loadu_ps(p); }
static void usc__pix4_store(float *p, usc__pix4 v) { _mm_storeu_ps(p, v); }
static usc__pix4 usc__pix4_add(usc__pix4 a, usc__pix4 b) { return _mm_add_ps(a, b); }
static usc__pix4 usc__pix4_sub(usc__pix4 a, usc__pix4 b) { return _mm_sub_ps(a, b); }
static usc__pix4 usc__pix4_mul(usc__pix4 a, usc__pix4 b) { return _mm_mul_ps(a, b); }

static usc__pix4
usc__pix4_decode(const void *p, UscPixelType type, UscImageFlags flags)
{
	uint32_t bgra32 = usc__read_as_bgra32(p, type, flags);
	__m128i packed8  = _mm_cvtsi32_si128(bgra32);
	__m128i packed32 = _mm_cvtepu8_epi32(packed8);
	float norm = 1.0f / 255.0f;
	float norm_pm = usc__need_premultiply(flags) ? ((bgra32 & 0xFF) * (norm * norm)) : norm;
	__m128 f = _mm_cvtepi32_ps(packed32);
	__m128 m = _mm_set_ps(norm_pm, norm_pm, norm_pm, norm);
	return _mm_mul_ps(f, m);
}

static void
usc__pix4_encode(void *dst_row, int64_t x, usc__pix4 pix, UscPixelType pixel_type, UscImageFlags flags)
{
	__m128 denorm = _mm_mul_ps(pix, _mm_set1_ps(255.0f));
	__m128i outv = _mm_cvtps_epi32(denorm);
	int32_t a = (flags & USC_IF_IGNORE_ALPHA) ? 255 : _mm_extract_epi32(outv, 0);
	if (usc__need_premultiply(flags) && a < 255) {
		// TODO: if the alpha is 0, then the rgb values will also end
		// up as 0 after un-premultiply. is this correct?
		float smallfloat = 1e-6f;
		__m128 alpha = _mm_shuffle_ps(pix, pix, _MM_SHUFFLE(0,0,0,0));
		__m128 nonzero = _mm_cmpgt_ps(alpha, _mm_set1_ps(smallfloat));
		__m128 recip = _mm_rcp_ps(alpha);
		recip = _mm_mul_ps(recip, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(alpha, recip)));
		recip = _mm_and_ps(recip, nonzero);
		denorm = _mm_mul_ps(denorm, recip);
		outv = _mm_cvtps_epi32(denorm);
	}
	outv = _mm_insert_epi32(outv, a, 0);
	outv = _mm_max_epi32(_mm_setzero_si128(), outv);
	outv = _mm_min_epi32(outv, _mm_set1_epi32(255));
	uint32_t outbuf[4];
	_mm_storeu_si128((__m128i *)outbuf, outv);
	usc__write_encoded(dst_row, x, outbuf, pixel_type);
}

#else // SCALER

static int
usc__ftoi(float x)
{
	float nudge = x < 0.0f ? -0.5f : 0.5f;
	int n = (int)(x + nudge);
	return n;
}

typedef struct { float lane[4]; } usc__pix4;

static usc__pix4 usc__pix4_set1(float v) { return (usc__pix4){{v, v, v, v}}; }
static usc__pix4 usc__pix4_load(const float *p) { return (usc__pix4){{p[0], p[1], p[2], p[3]}}; }

static void
usc__pix4_store(float *p, usc__pix4 v)
{
	p[0] = v.lane[0];
	p[1] = v.lane[1];
	p[2] = v.lane[2];
	p[3] = v.lane[3];
}

static usc__pix4
usc__pix4_add(usc__pix4 a, usc__pix4 b)
{
	return (usc__pix4){{
		a.lane[0] + b.lane[0],
		a.lane[1] + b.lane[1],
		a.lane[2] + b.lane[2],
		a.lane[3] + b.lane[3],
	}};
}

static usc__pix4
usc__pix4_sub(usc__pix4 a, usc__pix4 b)
{
	return (usc__pix4){{
		a.lane[0] - b.lane[0],
		a.lane[1] - b.lane[1],
		a.lane[2] - b.lane[2],
		a.lane[3] - b.lane[3],
	}};
}

static usc__pix4
usc__pix4_mul(usc__pix4 a, usc__pix4 b)
{
	return (usc__pix4){{
		a.lane[0] * b.lane[0],
		a.lane[1] * b.lane[1],
		a.lane[2] * b.lane[2],
		a.lane[3] * b.lane[3],
	}};
}

static usc__pix4
usc__pix4_decode(const void *p, UscPixelType type, UscImageFlags flags)
{
	uint32_t bgra32 = usc__read_as_bgra32(p, type, flags);
	float a = (bgra32 >>  0) & 0xFF;
	float r = (bgra32 >>  8) & 0xFF;
	float g = (bgra32 >> 16) & 0xFF;
	float b = (bgra32 >> 24) & 0xFF;
	float norm = 1.0f / 255.0f;
	float norm_pm = usc__need_premultiply(flags) ? (a * (norm * norm)) : norm;
	usc__pix4 pix = {{ a, r, g, b }};
	usc__pix4 normv = {{ norm, norm_pm, norm_pm, norm_pm }};
	return usc__pix4_mul(pix, normv);
}

static void
usc__pix4_encode(void *dst_row, int64_t x, usc__pix4 pix, UscPixelType pixel_type, UscImageFlags flags)
{
	float alpha = pix.lane[0];
	usc__pix4 denorm = {{ 255.0f, 255.0f, 255.0f, 255.0f }};
	float opaque_thresh = 0.99805f;
	if (usc__need_premultiply(flags) && alpha < opaque_thresh) {
		float smallfloat = 1e-6f;
		float recip = alpha > smallfloat ? 1.0f / alpha : 0.0f;
		denorm.lane[1] *= recip;
		denorm.lane[2] *= recip;
		denorm.lane[3] *= recip;
	}
	usc__pix4 outv = usc__pix4_mul(pix, denorm);

	uint32_t outbuf[4];
	outbuf[0] = (flags & USC_IF_IGNORE_ALPHA) ? 255 :
	            usc__clamp(usc__ftoi(outv.lane[0]), 0, 255);
	outbuf[1] = usc__clamp(usc__ftoi(outv.lane[1]), 0, 255);
	outbuf[2] = usc__clamp(usc__ftoi(outv.lane[2]), 0, 255);
	outbuf[3] = usc__clamp(usc__ftoi(outv.lane[3]), 0, 255);
	usc__write_encoded(dst_row, x, outbuf, pixel_type);
}

#endif // SCALER

////////////////////

static usc__pix4
usc__pix4_mul_scalar(usc__pix4 a, float b)
{
	return usc__pix4_mul(a, usc__pix4_set1(b));
}

static int
usc__pixel_width(UscPixelType type)
{
	USC_ASSERT(type > USC_PIX_NONE && type < USC_PIX_END);
	return 4;
}

static float *
usc__decode_row(
	float *decode_row, int64_t *decode_row_y, float *prefixsum_row,
	int64_t decode_xoff, int64_t decode_xend, int64_t y,
	const void *input_data, UscImageInfo input_info
)
{
	if (y == *decode_row_y)
		return decode_row;

	USC_ASSERT(decode_xoff >= 0 && decode_xoff <= input_info.width);
	USC_ASSERT(decode_xend >= decode_xoff && decode_xend <= input_info.width);

	const char *src_row = (const char *)input_data + (y * input_info.stride_bytes);
	ptrdiff_t pixel_width = usc__pixel_width(input_info.pixel_type);
	ptrdiff_t channels = 4;
	usc__pix4 ps_acc = usc__pix4_set1(0.0f);
	for (int64_t x = decode_xoff; x < decode_xend; ++x) {
		const void *src = src_row + (x * pixel_width);
		float *dst = decode_row + (x * channels);
		usc__pix4 p = usc__pix4_decode(src, input_info.pixel_type, input_info.flags);
		usc__pix4_store(dst, p);
		if (prefixsum_row) {
			usc__pix4_store(prefixsum_row + x*4, ps_acc);
			ps_acc = usc__pix4_add(ps_acc, p);
		}
	}
	if (prefixsum_row)
		usc__pix4_store(prefixsum_row + decode_xend*4, ps_acc);
	*decode_row_y = y;
	return decode_row;
}

static usc__pix4
usc__bicubic_x(float *src_row, const float *wt, int64_t base_x, int64_t sw)
{
	int64_t sx0 = usc__clamp(base_x + 0, 0, sw - 1);
	int64_t sx1 = usc__clamp(base_x + 1, 0, sw - 1);
	int64_t sx2 = usc__clamp(base_x + 2, 0, sw - 1);
	int64_t sx3 = usc__clamp(base_x + 3, 0, sw - 1);

	usc__pix4 w0 = usc__pix4_set1(wt[0]);
	usc__pix4 w1 = usc__pix4_set1(wt[1]);
	usc__pix4 w2 = usc__pix4_set1(wt[2]);
	usc__pix4 w3 = usc__pix4_set1(wt[3]);

	usc__pix4 p0 = usc__pix4_load(src_row + sx0*4);
	usc__pix4 p1 = usc__pix4_load(src_row + sx1*4);
	usc__pix4 p2 = usc__pix4_load(src_row + sx2*4);
	usc__pix4 p3 = usc__pix4_load(src_row + sx3*4);

	usc__pix4 pix =          usc__pix4_mul(w0, p0);
	pix = usc__pix4_add(pix, usc__pix4_mul(w1, p1));
	pix = usc__pix4_add(pix, usc__pix4_mul(w2, p2));
	pix = usc__pix4_add(pix, usc__pix4_mul(w3, p3));

	return pix;
}

static float
usc__bicubic_bc(float x, float B, float C)
{
	if (x < 0) x = -x;
	if (x < 1.0f) {
		return (((12.0f - 9.0f*B - 6.0f*C) * x*x*x)
			+ ((-18.0f + 12.0f*B + 6.0f*C) * x*x)
			+ (6.0f - 2.0f*B)) * (1.0f / 6.0f);
	} else if (x < 2.0f) {
		return (((-B - 6.0f*C) * x*x*x)
			+ ((6.0f*B + 30.0f*C) * x*x)
			+ ((-12.0f*B - 48.0f*C) * x)
			+ (8.0f*B + 24.0f*C)) * (1.0f / 6.0f);
	} else {
		return 0.0f;
	}
}

USC_API void
usc_build_bicubic_weight_table(float (*wt)[4], float B, float C)
{
	enum { TABLE_SIZE = 256 };
	for (int i = 0; i < TABLE_SIZE; ++i) {
		float f = (float)i / (float)TABLE_SIZE;
		float w0 = usc__bicubic_bc(-1.0f - f, B, C);
		float w1 = usc__bicubic_bc(-0.0f - f, B, C);
		float w2 = usc__bicubic_bc(+1.0f - f, B, C);
		float w3 = usc__bicubic_bc(+2.0f - f, B, C);
		float sum = w0 + w1 + w2 + w3;
		float inv_sum = (sum == 0.0f) ? 1.0f : (1.0f / sum);
		wt[i][0] = w0 * inv_sum;
		wt[i][1] = w1 * inv_sum;
		wt[i][2] = w2 * inv_sum;
		wt[i][3] = w3 * inv_sum;
	}
}

static void
usc__ctx_reset_decode_cache(UscContext *ctx)
{
	ctx->decode_row_y = -1;
	ctx->ring_head = 0;
	ctx->ring_tail = 0;
}

USC_API int
usc_ctx_init(
	UscContext *ctx,
	int64_t input_w, int64_t input_h,
	int64_t output_w, int64_t output_h,
	UscExtra *extra
)
{
	int rc = -1;
	ptrdiff_t channels = 4;

	USC_ASSERT(ctx != NULL);
	USC_ASSERT(input_w >= 0 && input_h >= 0);
	USC_ASSERT(output_w >= 0 && output_h >= 0);

	usc__memzero(ctx, sizeof *ctx);

	UscExtra default_extra = {
		usc__alloc, usc__dealloc, NULL, NULL,
	};
	ctx->extra = extra ? *extra : default_extra;
	USC_ASSERT(!!ctx->extra.alloc == !!ctx->extra.dealloc && "Both alloc and dealloc must be defined");
	if (ctx->extra.alloc == NULL) {
		ctx->extra.alloc = usc__alloc;
		ctx->extra.dealloc = usc__dealloc;
	}

	/* ensures w*h doesn't overflow `int`, safety precaution */
	if (input_w && (INT_MAX / input_w) < input_h)
		return -2;
	if (output_w && (INT_MAX / output_w) < output_h)
		return -2;

	ctx->input_w = input_w;
	ctx->input_h = input_h;
	ctx->output_w = output_w;
	ctx->output_h = output_h;
	ctx->output_subrect_x = 0;
	ctx->output_subrect_y = 0;
	ctx->output_subrect_w = output_w;
	ctx->output_subrect_h = output_h;
	ctx->input_buffer.pixel_type = USC_PIX_NONE;
	usc__ctx_reset_decode_cache(ctx);

	if (input_w == 0 || input_h == 0 || output_w == 0 || output_h == 0) {
		rc = 0;
		goto out;
	}

	ctx->use_box_x = output_w < input_w;
	ctx->use_box_y = output_h < input_h;

	ctx->decode_row = usc__alloc_array(
		ctx->extra, input_w, channels * sizeof *ctx->decode_row,
		&ctx->decode_row_size
	);
	if (ctx->decode_row == NULL)
		goto out;

	if ((!ctx->use_box_x || !ctx->use_box_y) && ctx->extra.bicubic_weights == NULL) {
		ctx->extra.bicubic_weights = usc__alloc_array(
			ctx->extra, 256, sizeof *ctx->extra.bicubic_weights,
			&ctx->bicubic_weights_size
		);
		if (ctx->extra.bicubic_weights == NULL)
			goto out;
		usc_build_bicubic_weight_table(ctx->extra.bicubic_weights, 0.0f, 0.5f);
	}

	if (ctx->use_box_x) {
		ctx->prefixsum_row = usc__alloc_array(
			ctx->extra, input_w + 1,
			channels * sizeof *ctx->prefixsum_row,
			&ctx->prefixsum_row_size
		);
		if (ctx->prefixsum_row == NULL)
			goto out;

		ctx->box_x = usc__alloc_array(
			ctx->extra, output_w, sizeof *ctx->box_x,
			&ctx->box_x_size
		);
		if (ctx->box_x == NULL)
			goto out;
	}

	if (ctx->use_box_y) {
		ctx->acc_row = usc__alloc_array(
			ctx->extra, output_w,
			channels * sizeof *ctx->acc_row,
			&ctx->acc_row_size
		);
		if (ctx->acc_row == NULL)
			goto out;
	}

	enum { RING_ROWS = 4 };
	if (!ctx->use_box_y) {
		ctx->ring_buf = usc__alloc_array(
			ctx->extra, output_w,
			channels * RING_ROWS * sizeof *ctx->ring_buf,
			&ctx->ring_buf_size
		);
		if (ctx->ring_buf == NULL)
			goto out;
		for (int i = 0; i < RING_ROWS; ++i)
			ctx->ring[i] = ctx->ring_buf + (i * channels * output_w);
	}

	int64_t FX_FRAC = 24;
	Usc__Fx64 FX_ONE = (Usc__Fx64)1 << FX_FRAC;

	float scale_x = (float)input_w / output_w;
	float source_x0 = 0.5f * scale_x - 0.5f;
	float source_x1 = 1.5f * scale_x - 0.5f;
	float delta_x = source_x1 - source_x0;
	ctx->map_delta_x = (Usc__Fx64)(delta_x * (float)FX_ONE + 0.5f);
	ctx->inv_map_delta_x = 1.0 / (double)ctx->map_delta_x;
	ctx->map_x_start = (Usc__Fx64)(source_x0 * (float)FX_ONE + 0.5f);

	float scale_y = (float)input_h / output_h;
	float source_y0 = 0.5f * scale_y - 0.5f;
	float source_y1 = 1.5f * scale_y - 0.5f;
	float delta_y = source_y1 - source_y0;
	ctx->map_delta_y = (Usc__Fx64)(delta_y * (float)FX_ONE + 0.5f);
	ctx->inv_map_delta_y = 1.0 / (double)ctx->map_delta_y;
	ctx->map_y_start = (Usc__Fx64)(source_y0 * (float)FX_ONE + 0.5f);

	ctx->box_x_range_weight = 0.0f;
	if (ctx->use_box_x) {
		USC_ASSERT(ctx->map_delta_x > 0);
		Usc__Fx64 row_hi = input_w * FX_ONE;
		Usc__Fx64 map_x = ctx->map_x_start;
		ctx->box_x_range_weight = (float)FX_ONE * ctx->inv_map_delta_x;
		for (int64_t dx = 0; dx < output_w; ++dx, map_x += ctx->map_delta_x) {
			Usc__Fx64 left_box = map_x - (ctx->map_delta_x >> 1);
			Usc__Fx64 right_box = map_x + (ctx->map_delta_x >> 1);
			if (left_box < 0) left_box = 0;
			if (right_box > row_hi) right_box = row_hi;
			Usc__Fx64 left_idx = left_box >> FX_FRAC;
			/* `right_idx` is the last source pixel touched by [left_box, right_box). */
			Usc__Fx64 right_idx = right_box ? (right_box - 1) >> FX_FRAC : 0;
			USC_ASSERT(left_idx >= 0 && left_idx < input_w);
			USC_ASSERT(right_idx >= 0 && right_idx < input_w);
			USC_ASSERT(left_idx <= right_idx);
			ctx->box_x[dx].left_idx = left_idx;
			ctx->box_x[dx].right_idx = right_idx;
			ctx->box_x[dx].left_weight = (left_box - (left_idx << FX_FRAC)) * ctx->inv_map_delta_x;
			ctx->box_x[dx].right_weight = (((right_idx + 1) << FX_FRAC) - right_box) * ctx->inv_map_delta_x;
		}
	}

	rc = 0;
out:
	if (rc != 0)
		usc_ctx_release(ctx);
	return rc;
}

USC_API void
usc_ctx_release(UscContext *ctx)
{
	if (ctx->bicubic_weights_size)
		ctx->extra.dealloc(ctx->extra.userdata, ctx->extra.bicubic_weights, ctx->bicubic_weights_size);
	ctx->extra.dealloc(ctx->extra.userdata, ctx->ring_buf, ctx->ring_buf_size);
	ctx->extra.dealloc(ctx->extra.userdata, ctx->acc_row, ctx->acc_row_size);
	ctx->extra.dealloc(ctx->extra.userdata, ctx->box_x, ctx->box_x_size);
	ctx->extra.dealloc(ctx->extra.userdata, ctx->prefixsum_row, ctx->prefixsum_row_size);
	ctx->extra.dealloc(ctx->extra.userdata, ctx->decode_row, ctx->decode_row_size);
	usc__memzero(ctx, sizeof *ctx);
}

USC_API void
usc_ctx_set_input_buffer(UscContext *ctx, UscBuffer input)
{
	USC_ASSERT(ctx != NULL);
	USC_ASSERT(input.pixel_type > USC_PIX_NONE && input.pixel_type < USC_PIX_END);
	USC_ASSERT(input.stride_bytes >= 0);
	USC_ASSERT((input.flags & ~(USC_IF_PREMULTIPLIED | USC_IF_IGNORE_ALPHA)) == 0);
	if (input.stride_bytes == 0)
		input.stride_bytes = ctx->input_w * usc__pixel_width(input.pixel_type);
	ctx->input_buffer = input;
	usc__ctx_reset_decode_cache(ctx);
}

USC_API void
usc_ctx_set_output_subrect(
	UscContext *ctx,
	int64_t x, int64_t y, int64_t w, int64_t h
)
{
	USC_ASSERT(ctx != NULL);
	USC_ASSERT(x >= 0 && y >= 0 && w >= 0 && h >= 0);
	USC_ASSERT(x <= ctx->output_w && y <= ctx->output_h);
	USC_ASSERT(w <= ctx->output_w - x);
	USC_ASSERT(h <= ctx->output_h - y);
	if (ctx->output_subrect_x != x || ctx->output_subrect_w != w)
		usc__ctx_reset_decode_cache(ctx);
	ctx->output_subrect_x = x;
	ctx->output_subrect_y = y;
	ctx->output_subrect_w = w;
	ctx->output_subrect_h = h;
}

USC_API void
usc_resize_extended(
	UscContext *ctx,
	UscBuffer output
)
{
	enum { RING_ROWS = 4 };
	ptrdiff_t channels = 4;
	int64_t FX_FRAC = 24;
	Usc__Fx64 FX_ONE = (Usc__Fx64)1 << FX_FRAC;
	Usc__Fx64 FX_MASK = FX_ONE - 1;
	int FX_TO_TABLE_SHIFT = FX_FRAC - 8;
	UscBuffer input = ctx->input_buffer;
	UscImageInfo input_info;
	UscBuffer output_buf = output;

	USC_ASSERT(ctx != NULL);
	USC_ASSERT(input.pixel_type > USC_PIX_NONE && input.pixel_type < USC_PIX_END);
	USC_ASSERT(output_buf.pixel_type == USC_PIX_NONE ||
	           (output_buf.pixel_type > USC_PIX_NONE && output_buf.pixel_type < USC_PIX_END));
	USC_ASSERT(input.stride_bytes >= 0 && output_buf.stride_bytes >= 0);
	USC_ASSERT((input.flags & ~(USC_IF_PREMULTIPLIED | USC_IF_IGNORE_ALPHA)) == 0);
	USC_ASSERT((output_buf.flags & ~(USC_IF_PREMULTIPLIED | USC_IF_IGNORE_ALPHA)) == 0);

	if (output_buf.pixel_type == USC_PIX_NONE)
		output_buf.pixel_type = input.pixel_type;
	/* if src is fully opaque, dst must be too */
	if (input.flags & USC_IF_IGNORE_ALPHA)
		output_buf.flags |= USC_IF_IGNORE_ALPHA;

	input_info.width = ctx->input_w;
	input_info.height = ctx->input_h;
	input_info.stride_bytes = input.stride_bytes ?
		input.stride_bytes : ctx->input_w * usc__pixel_width(input.pixel_type);
	input_info.pixel_type = input.pixel_type;
	input_info.flags = input.flags;

	if (output_buf.stride_bytes == 0)
		output_buf.stride_bytes = ctx->output_subrect_w * usc__pixel_width(output_buf.pixel_type);

	if (ctx->input_w == 0 || ctx->input_h == 0 ||
	    ctx->output_w == 0 || ctx->output_h == 0 ||
	    ctx->output_subrect_w == 0 || ctx->output_subrect_h == 0)
		return;

	int64_t decode_xoff = ctx->input_w;
	int64_t decode_xend = 0;
	if (ctx->use_box_x) {
		decode_xoff = ctx->box_x[ctx->output_subrect_x].left_idx;
		decode_xend = ctx->box_x[ctx->output_subrect_x + ctx->output_subrect_w - 1].right_idx + 1;
	} else {
		Usc__Fx64 map_x0 = ctx->map_x_start + ctx->output_subrect_x * ctx->map_delta_x;
		Usc__Fx64 map_x1 = map_x0 + (ctx->output_subrect_w - 1) * ctx->map_delta_x;
		decode_xoff = usc__clamp((map_x0 >> FX_FRAC) - 1, 0, ctx->input_w - 1);
		decode_xend = usc__clamp((map_x1 >> FX_FRAC) + 3, 0, ctx->input_w);
	}

	Usc__Fx64 map_y = ctx->map_y_start + ctx->output_subrect_y * ctx->map_delta_y;
	char *dst_row = output_buf.data;
	for (int64_t dy = 0; dy < ctx->output_subrect_h;
	     ++dy, map_y += ctx->map_delta_y,
	     dst_row += output_buf.stride_bytes)
	{
		int64_t base_y = (map_y >> FX_FRAC) - 1;
		int idx_y = (map_y & FX_MASK) >> FX_TO_TABLE_SHIFT;
		Usc__Fx64 top_box = map_y - (ctx->map_delta_y >> 1);
		Usc__Fx64 bot_box = map_y + (ctx->map_delta_y >> 1);
		int64_t start_y, end_y;

		if (ctx->use_box_y) {
			start_y = top_box >> FX_FRAC;
			end_y = ((bot_box + FX_ONE - 1) >> FX_FRAC) - 1;
			start_y = usc__clamp(start_y, 0, ctx->input_h - 1);
			end_y = usc__clamp(end_y, 0, ctx->input_h - 1);
			usc__memzero(ctx->acc_row, ctx->acc_row_size);
		} else {
			start_y = usc__clamp(base_y, 0, ctx->input_h - 1);
			end_y = usc__clamp(base_y + 3, 0, ctx->input_h - 1);
			if (start_y < ctx->ring_head || start_y > ctx->ring_tail) {
				ctx->ring_head = start_y;
				ctx->ring_tail = start_y;
			}
			start_y = end_y < ctx->ring_tail ? end_y + 1 : ctx->ring_tail;
		}

		for (int64_t sy = start_y; sy <= end_y; ++sy) {
			float *out_row = ctx->use_box_y ? ctx->acc_row :
			                 ctx->ring[ctx->ring_tail++ % RING_ROWS];
			float *src_row = usc__decode_row(
				ctx->decode_row, &ctx->decode_row_y, ctx->prefixsum_row,
				decode_xoff, decode_xend, sy,
				input.cdata, input_info
			);
			float wy = 0.0f;

			if (ctx->ring_tail - ctx->ring_head > RING_ROWS)
				ctx->ring_head = ctx->ring_tail - RING_ROWS;

			if (ctx->use_box_y) {
				Usc__Fx64 row_top = sy * FX_ONE;
				Usc__Fx64 row_bot = (sy + 1) * FX_ONE;
				Usc__Fx64 ovl_top = top_box > row_top ? top_box : row_top;
				Usc__Fx64 ovl_bot = bot_box < row_bot ? bot_box : row_bot;
				Usc__Fx64 overlap = ovl_bot - ovl_top;
				wy = overlap * ctx->inv_map_delta_y;
			}

			Usc__Fx64 map_x = ctx->map_x_start + ctx->output_subrect_x * ctx->map_delta_x;
			for (int64_t dx = 0; dx < ctx->output_subrect_w; ++dx, map_x += ctx->map_delta_x) {
				int64_t full_dx = ctx->output_subrect_x + dx;
				usc__pix4 pix;
				if (ctx->use_box_x) {
					usc__BoxInfo *bx = ctx->box_x + full_dx;
					ptrdiff_t lidx = bx->left_idx, ridx = bx->right_idx;
					usc__pix4 lsum = usc__pix4_load(ctx->prefixsum_row + lidx * 4);
					usc__pix4 rsum = usc__pix4_load(ctx->prefixsum_row + (ridx + 1) * 4);
					usc__pix4 sum = usc__pix4_sub(rsum, lsum);
					usc__pix4 acc = usc__pix4_mul_scalar(sum, ctx->box_x_range_weight);

					usc__pix4 lpix = usc__pix4_load(src_row + lidx * 4);
					usc__pix4 rpix = usc__pix4_load(src_row + ridx * 4);
					acc = usc__pix4_sub(acc, usc__pix4_mul_scalar(lpix, bx->left_weight));
					acc = usc__pix4_sub(acc, usc__pix4_mul_scalar(rpix, bx->right_weight));
					pix = acc;
				} else {
					Usc__Fx64 base_x = (map_x >> FX_FRAC) - 1;
					int idx_x = (map_x & FX_MASK) >> FX_TO_TABLE_SHIFT;
					const float *wtx = ctx->extra.bicubic_weights[idx_x];
					pix = usc__bicubic_x(src_row, wtx, base_x, ctx->input_w);
				}
				if (ctx->use_box_y) {
					pix = usc__pix4_mul_scalar(pix, wy);
					pix = usc__pix4_add(usc__pix4_load(ctx->acc_row + dx * channels), pix);
				}
				usc__pix4_store(out_row + dx * channels, pix);
			}
		}

		if (ctx->use_box_y) {
			for (int64_t dx = 0; dx < ctx->output_subrect_w; ++dx) {
				usc__pix4 outpix = usc__pix4_load(ctx->acc_row + dx * channels);
				usc__pix4_encode(
					dst_row, dx, outpix,
					output_buf.pixel_type, output_buf.flags
				);
			}
		} else {
			int64_t sy0 = usc__clamp(base_y + 0, 0, ctx->input_h - 1);
			int64_t sy1 = usc__clamp(base_y + 1, 0, ctx->input_h - 1);
			int64_t sy2 = usc__clamp(base_y + 2, 0, ctx->input_h - 1);
			int64_t sy3 = usc__clamp(base_y + 3, 0, ctx->input_h - 1);
			float *brow0 = ctx->ring[sy0 % RING_ROWS];
			float *brow1 = ctx->ring[sy1 % RING_ROWS];
			float *brow2 = ctx->ring[sy2 % RING_ROWS];
			float *brow3 = ctx->ring[sy3 % RING_ROWS];

			const float *wty = ctx->extra.bicubic_weights[idx_y];
			usc__pix4 w0 = usc__pix4_set1(wty[0]);
			usc__pix4 w1 = usc__pix4_set1(wty[1]);
			usc__pix4 w2 = usc__pix4_set1(wty[2]);
			usc__pix4 w3 = usc__pix4_set1(wty[3]);

			for (int64_t dx = 0; dx < ctx->output_subrect_w; ++dx) {
				usc__pix4 p0 = usc__pix4_load(brow0 + dx * channels);
				usc__pix4 p1 = usc__pix4_load(brow1 + dx * channels);
				usc__pix4 p2 = usc__pix4_load(brow2 + dx * channels);
				usc__pix4 p3 = usc__pix4_load(brow3 + dx * channels);

				usc__pix4 pix =          usc__pix4_mul(w0, p0);
				pix = usc__pix4_add(pix, usc__pix4_mul(w1, p1));
				pix = usc__pix4_add(pix, usc__pix4_mul(w2, p2));
				pix = usc__pix4_add(pix, usc__pix4_mul(w3, p3));
				usc__pix4_encode(dst_row, dx, pix, output_buf.pixel_type, output_buf.flags);
			}
		}
	}
}

USC_API USC__FLATTEN int
usc_resize(
	const void *restrict input_data, UscImageInfo input_info,
	void *restrict output_data, UscImageInfo output_info,
	UscExtra *extra
)
{
	UscContext ctx[1];
	int rc = usc_ctx_init(ctx,
		input_info.width, input_info.height,
		output_info.width, output_info.height,
		extra
	);
	if (rc < 0)
		return rc;
	usc_ctx_set_input_buffer(ctx, (UscBuffer){
		.cdata = input_data,
		.stride_bytes = input_info.stride_bytes,
		.pixel_type = input_info.pixel_type,
		.flags = input_info.flags,
	});
	usc_resize_extended(ctx, (UscBuffer){
		.data = output_data,
		.stride_bytes = output_info.stride_bytes,
		.pixel_type = output_info.pixel_type,
		.flags = output_info.flags,
	});
	usc_ctx_release(ctx);
	return 0;
}

#endif // USC_IMPLEMENTATION

#endif // USC__INCLUDE_GUARD


// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute
// this software, either in source code form or as a compiled binary, for any
// purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of
// this software dedicate any and all copyright interest in the software to the
// public domain. We make this dedication for the benefit of the public at
// large and to the detriment of our heirs and successors. We intend this
// dedication to be an overt act of relinquishment in perpetuity of all present
// and future rights to this software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <https://unlicense.org/>
