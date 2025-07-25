#include "config.h"
#include "Imlib2_Loader.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <libyuv.h>

#define DBG_PFX "LDR-y4m"

static const char *const _formats[] = { "y4m" };

// Parser code taken from (commit 05ce1e4):
// https://codeberg.org/NRK/slashtmp/src/branch/master/parsers/y4m.c
//
// only parses the format, doesn't do any colorspace conversion.
// comments and frame parameters are ignored.
//
// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org/>
#define Y4M_PARSE_API static
#if IMLIB2_DEBUG
#define Y4M_PARSE_ASSERT(X)   do { if (!(X)) D("%d: %s\n", __LINE__, #X); } while (0)
#else
#define Y4M_PARSE_ASSERT(X)
#endif

enum Y4mParseStatus {
    Y4M_PARSE_OK,
    Y4M_PARSE_NOT_Y4M,
    Y4M_PARSE_EOF,
    Y4M_PARSE_CORRUPTED,
    Y4M_PARSE_UNSUPPORTED,
};

typedef struct {
    ptrdiff_t       w, h;
    int64_t         frametime_us;       /* frametime in micro-seconds */
    enum {
        Y4M_PARSE_CS_UNSUPPORTED = -1,
        Y4M_PARSE_CS_420,       /* default picked from ffmpeg */
        Y4M_PARSE_CS_420JPEG,
        Y4M_PARSE_CS_420MPEG2,
        Y4M_PARSE_CS_420PALDV,
        Y4M_PARSE_CS_422,
        Y4M_PARSE_CS_444,
        Y4M_PARSE_CS_MONO,
        Y4M_PARSE_CS_MONO10,
        Y4M_PARSE_CS_420P10,
        Y4M_PARSE_CS_422P10,
        Y4M_PARSE_CS_444P10,
        Y4M_PARSE_CS_MONO12,
        Y4M_PARSE_CS_MONO14,
        Y4M_PARSE_CS_MONO16,
    } colour_space;
    enum {
        Y4M_PARSE_IL_PROGRESSIVE,
        Y4M_PARSE_IL_TOP,
        Y4M_PARSE_IL_BOTTOM,
        Y4M_PARSE_IL_MIXED,
    } interlacing;
    int             pixel_aspect_w, pixel_aspect_h;
    enum {
        Y4M_PARSE_RANGE_UNSPECIFIED,
        Y4M_PARSE_RANGE_LIMITED,
        Y4M_PARSE_RANGE_FULL,
    } range;
    uint8_t         depth;      // Bit depth  (8, 10, 12, 14, 16)

    const void     *frame_data;
    ptrdiff_t       frame_data_len;
    const void     *y, *u, *v;
    ptrdiff_t       y_stride, u_stride, v_stride;

    // private
    const uint8_t  *p, *end;
} Y4mParse;

Y4M_PARSE_API enum Y4mParseStatus y4m_parse_init(Y4mParse * res,
                                                 const void *buffer,
                                                 ptrdiff_t size);
Y4M_PARSE_API enum Y4mParseStatus y4m_parse_frame(Y4mParse * res);

// implementation

static int
y4m__int(ptrdiff_t *out, const uint8_t **p, const uint8_t *end)
{
    uint64_t        n = 0;
    const uint8_t  *s = *p;

    for (; s < end && *s >= '0' && *s <= '9'; ++s)
    {
        n = (n * 10) + (*s - '0');
        if (n > INT_MAX)
        {
            return 0;
        }
    }
    *out = n;
    *p = s;
    return 1;
}

static int
y4m__match(const char *match, ptrdiff_t mlen,
           const uint8_t **p, const uint8_t *end)
{
    if (end - *p >= mlen && memcmp(match, *p, mlen) == 0)
    {
        *p += mlen;
        return 1;
    }
    return 0;
}

static enum Y4mParseStatus
y4m__parse_params(Y4mParse *res, const uint8_t **start, const uint8_t *end)
{
    ptrdiff_t       number_of_frames, seconds;
    ptrdiff_t       tmp_w, tmp_h;
    const uint8_t  *p = *start;
    const uint8_t  *peek;

    // default values
    res->range = Y4M_PARSE_RANGE_UNSPECIFIED;
    res->depth = 8;
    res->frametime_us = 1000000;        /* 1 fps */

    for (;;)
    {
#if IMLIB2_DEBUG
        const char     *pp = (const char *)p;
#endif

        if (end - p <= 0)
            return Y4M_PARSE_CORRUPTED;

        switch (*p++)
        {
        case ' ':
            break;
        case '\n':
            *start = p;
            if (res->w <= 0 || res->h <= 0 || res->frametime_us <= 0)
                return Y4M_PARSE_CORRUPTED;
            return Y4M_PARSE_OK;
        case 'W':
            if (!y4m__int(&res->w, &p, end))
                return Y4M_PARSE_CORRUPTED;
            break;
        case 'H':
            if (!y4m__int(&res->h, &p, end))
                return Y4M_PARSE_CORRUPTED;
            break;
        case 'F':
            if (!y4m__int(&number_of_frames, &p, end) ||
                !y4m__match(":", 1, &p, end) ||
                !y4m__int(&seconds, &p, end) ||
                number_of_frames == 0 || seconds == 0)
            {
#if IMLIB2_DEBUG
                char            str[1024];
                sscanf(pp, "%s", str);
                D("%s: unknown frame rate: '%s'\n", __func__, str);
#endif
                return Y4M_PARSE_CORRUPTED;
            }
            res->frametime_us =
                ((int64_t) seconds * 1000000) / number_of_frames;
            break;
        case 'I':
            if (y4m__match("p", 1, &p, end))
                res->interlacing = Y4M_PARSE_IL_PROGRESSIVE;
            else if (y4m__match("t", 1, &p, end))
                res->interlacing = Y4M_PARSE_IL_TOP;
            else if (y4m__match("b", 1, &p, end))
                res->interlacing = Y4M_PARSE_IL_BOTTOM;
            else if (y4m__match("m", 1, &p, end))
                res->interlacing = Y4M_PARSE_IL_MIXED;
            else
            {
#if IMLIB2_DEBUG
                char            str[1024];
                sscanf(pp, "%s", str);
                D("%s: unknown interlace type: '%s'\n", __func__, str);
#endif
                return Y4M_PARSE_CORRUPTED;
            }
            break;
        case 'C':
            res->colour_space = Y4M_PARSE_CS_UNSUPPORTED;
            if (y4m__match("mono10", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_MONO10;
                res->depth = 10;
            }
            else if (y4m__match("mono12", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_MONO12;
                res->depth = 12;
            }
            else if (y4m__match("mono14", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_MONO14;
                res->depth = 14;
            }
            else if (y4m__match("mono16", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_MONO16;
                res->depth = 16;
            }
            else if (y4m__match("mono", 4, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_MONO;
                res->depth = 8;
            }
            else if (y4m__match("420jpeg", 7, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_420JPEG;
                res->depth = 8;
            }
            else if (y4m__match("420mpeg2", 8, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_420MPEG2;
                res->depth = 8;
            }
            else if (y4m__match("420paldv", 8, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_420PALDV;
                res->depth = 8;
            }
            else if (y4m__match("420p10", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_420P10;
                res->depth = 10;
            }
            else if (y4m__match("420", 3, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_420;
                res->depth = 8;
            }
            else if (y4m__match("422p10", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_422P10;
                res->depth = 10;
            }
            else if (y4m__match("422", 3, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_422;
                res->depth = 8;
            }
            else if (y4m__match("444p10", 6, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_444P10;
                res->depth = 10;
            }
            else if (y4m__match("444", 3, &p, end))
            {
                res->colour_space = Y4M_PARSE_CS_444;
                res->depth = 8;
            }

            peek = p;           /* peek to avoid falsly matching things like "420p16" */
            if (res->colour_space == Y4M_PARSE_CS_UNSUPPORTED ||
                (!y4m__match(" ", 1, &peek, end) &&
                 !y4m__match("\n", 1, &peek, end)))
            {
#if IMLIB2_DEBUG
                char            str[1024];
                sscanf(pp, "%s", str);
                D("%s: unknown color type: '%s'\n", __func__, str);
#endif
                return Y4M_PARSE_UNSUPPORTED;
            }

            break;
        case 'A':
            if (!y4m__int(&tmp_w, &p, end) ||
                !y4m__match(":", 1, &p, end) || !y4m__int(&tmp_h, &p, end))
            {
                return Y4M_PARSE_CORRUPTED;
            }
            res->pixel_aspect_w = (int)tmp_w;
            res->pixel_aspect_h = (int)tmp_h;
            break;
        case 'X':
            if (y4m__match("COLORRANGE=LIMITED", 18, &p, end))
                res->range = Y4M_PARSE_RANGE_LIMITED;
            else if (y4m__match("COLORRANGE=FULL", 15, &p, end))
                res->range = Y4M_PARSE_RANGE_FULL;
            else if (y4m__match("COLORRANGE=", 11, &p, end))
            {
#if IMLIB2_DEBUG
                char            str[1024];
                sscanf(pp, "%s", str);
                D("%s: unknown color range: '%s'\n", __func__, str);
#endif
                return Y4M_PARSE_UNSUPPORTED;
            }
            else
            {
                /* other comments ignored */
                for (; p < end && *p != ' ' && *p != '\n'; ++p)
                    ;
            }
            break;
        default:
            return Y4M_PARSE_CORRUPTED;
            break;
        }
    }
    Y4M_PARSE_ASSERT(!"unreachable");
    return -1;                  // silence warning
}

Y4M_PARSE_API enum Y4mParseStatus
y4m_parse_init(Y4mParse *res, const void *buffer, ptrdiff_t size)
{
    static const char magic[] = "YUV4MPEG2 ";

    memset(res, 0x0, sizeof(*res));
    res->w = res->h = -1;
    res->p = buffer;
    res->end = res->p + size;

    if (!y4m__match(magic, sizeof(magic) - 1, &res->p, res->end))
    {
        return Y4M_PARSE_NOT_Y4M;
    }
    return y4m__parse_params(res, &res->p, res->end);
}

Y4M_PARSE_API enum Y4mParseStatus
y4m_parse_frame(Y4mParse *res)
{
    ptrdiff_t       ypixels, cpixels, cstride_pixels, voff;
    const uint8_t  *p = res->p, *end = res->end;

    Y4M_PARSE_ASSERT(p <= end);
    if (p == end)
    {
        return Y4M_PARSE_EOF;
    }
    if (!y4m__match("FRAME", 5, &p, end))
    {
        return Y4M_PARSE_CORRUPTED;
    }
    // NOTE: skip frame params, ffmpeg seems to do the same...
    for (; p < end && *p != '\n'; ++p)
        ;
    if (p == end)
        return Y4M_PARSE_CORRUPTED;
    ++p;                        /* skip '\n' */

    res->frame_data = p;
    ypixels = res->w * res->h;
    switch (res->colour_space)
    {
    case Y4M_PARSE_CS_420JPEG:
    case Y4M_PARSE_CS_420MPEG2:
    case Y4M_PARSE_CS_420PALDV:
    case Y4M_PARSE_CS_420:
        cpixels = ((res->w + 1) / 2) * ((res->h + 1) / 2);
        res->frame_data_len = ypixels + 2 * cpixels;
        cstride_pixels = ((res->w + 1) / 2);
        break;
    case Y4M_PARSE_CS_420P10:
        cpixels = ((res->w + 1) / 2) * ((res->h + 1) / 2);
        res->frame_data_len = 2 * (ypixels + 2 * cpixels);
        cstride_pixels = ((res->w + 1) / 2);
        break;
    case Y4M_PARSE_CS_422:
        cpixels = ((res->w + 1) / 2) * res->h;
        res->frame_data_len = ypixels + 2 * cpixels;
        cstride_pixels = ((res->w + 1) / 2);
        break;
    case Y4M_PARSE_CS_422P10:
        cpixels = ((res->w + 1) / 2) * res->h;
        res->frame_data_len = 2 * (ypixels + 2 * cpixels);
        cstride_pixels = ((res->w + 1) / 2);
        break;
    case Y4M_PARSE_CS_444:
        cpixels = ypixels;
        res->frame_data_len = ypixels + 2 * cpixels;
        cstride_pixels = res->w;
        break;
    case Y4M_PARSE_CS_444P10:
        cpixels = ypixels;
        res->frame_data_len = 2 * (ypixels + 2 * cpixels);
        cstride_pixels = res->w;
        break;
    case Y4M_PARSE_CS_MONO10:
        cpixels = 0;
        res->frame_data_len = ypixels * 2;
        cstride_pixels = 0;     // silence bogus compiler warning
        break;
    case Y4M_PARSE_CS_MONO12:
        cpixels = 0;
        res->frame_data_len = ypixels * 2;
        cstride_pixels = 0;     // silence bogus compiler warning
        break;
    case Y4M_PARSE_CS_MONO14:
        cpixels = 0;
        res->frame_data_len = ypixels * 2;
        cstride_pixels = 0;     // silence bogus compiler warning
        break;
    case Y4M_PARSE_CS_MONO16:
        cpixels = 0;
        res->frame_data_len = ypixels * 2;
        cstride_pixels = 0;     // silence bogus compiler warning
        break;
    case Y4M_PARSE_CS_MONO:
        cpixels = 0;
        res->frame_data_len = ypixels;
        cstride_pixels = 0;     // silence bogus compiler warning
        break;
    default:
        return Y4M_PARSE_UNSUPPORTED;
        break;
    }
    voff = ypixels + cpixels;
    if (end - p < res->frame_data_len)
    {
        return Y4M_PARSE_CORRUPTED;
    }

    res->y = p;
    res->y_stride = res->w;
    if (res->colour_space == Y4M_PARSE_CS_MONO ||
        res->colour_space == Y4M_PARSE_CS_MONO10 ||
        res->colour_space == Y4M_PARSE_CS_MONO12 ||
        res->colour_space == Y4M_PARSE_CS_MONO14 ||
        res->colour_space == Y4M_PARSE_CS_MONO16)
    {
        res->u = res->v = NULL;
        res->u_stride = res->v_stride = 0;
    }
    else
    {
        if (res->depth == 10 || res->depth == 12 || res->depth == 14 ||
            res->depth == 16)
        {
            res->u = p + ypixels * 2;
            res->v = p + voff * 2;
        }
        else
        {
            res->u = p + ypixels;
            res->v = p + voff;
        }
        res->u_stride = res->v_stride = cstride_pixels;
    }

    res->p = p + res->frame_data_len;   /* advance to next potential frame */
    Y4M_PARSE_ASSERT(res->p <= end);

    return Y4M_PARSE_OK;
}

// END Y4mParse library

/* wrapper for mono colour space to match the signature of other yuv conversion
 * routines. */
static int
conv_mono(const uint8_t *y, int y_stride, const uint8_t *u, int u_stride,
          const uint8_t *v, int v_stride, uint8_t *dst, int dst_stride,
          int width, int height)
{
    return I400ToARGB(y, y_stride, dst, dst_stride, width, height);
}

static int
conv_mono_full(const uint8_t *y, int y_stride, const uint8_t *u, int u_stride,
               const uint8_t *v, int v_stride, uint8_t *dst, int dst_stride,
               int width, int height)
{
    return J400ToARGB(y, y_stride, dst, dst_stride, width, height);
}

static int
_load(ImlibImage *im, int load_data)
{
    Y4mParse        y4m;
    int             res, fcount, frame;
    ImlibImageFrame *pf = NULL;
    int             (*conv)(const uint8_t *, int, const uint8_t *, int,
                            const uint8_t *, int, uint8_t *, int, int, int);

    if (y4m_parse_init(&y4m, im->fi->fdata, im->fi->fsize) != Y4M_PARSE_OK)
        return LOAD_FAIL;

    frame = im->frame;
    if (frame > 0)
    {
        fcount = 0;
        // NOTE: this is fairly cheap since nothing is being decoded.
        for (Y4mParse tmp = y4m; y4m_parse_frame(&tmp) == Y4M_PARSE_OK;)
            ++fcount;
        if (fcount == 0)
            return LOAD_BADIMAGE;
        if (frame > fcount)
            return LOAD_BADFRAME;

        pf = __imlib_GetFrame(im);
        if (!pf)
            return LOAD_OOM;
        pf->frame_count = fcount;
        pf->loop_count = 0;     /* Loop forever */
        if (pf->frame_count > 1)
            pf->frame_flags |= FF_IMAGE_ANIMATED;
    }
    else
    {
        frame = 1;
    }

    for (int i = 0; i < frame; ++i)
    {
        if (y4m_parse_frame(&y4m) != Y4M_PARSE_OK)
            return LOAD_BADIMAGE;
    }

    if (!IMAGE_DIMENSIONS_OK(y4m.w, y4m.h))
        return LOAD_BADIMAGE;
    // no interlacing support
    if (y4m.interlacing != Y4M_PARSE_IL_PROGRESSIVE)
        return LOAD_BADIMAGE;

    if (y4m.pixel_aspect_w != y4m.pixel_aspect_h || y4m.pixel_aspect_w == 0)
    {
        DL("Non-square pixel aspect ratio: %d:%d\n",
           y4m.pixel_aspect_w, y4m.pixel_aspect_h);
    }

    im->w = y4m.w;
    im->h = y4m.h;
    im->has_alpha = 0;

    switch (y4m.colour_space)
    {
    case Y4M_PARSE_CS_MONO:
    case Y4M_PARSE_CS_MONO10:
    case Y4M_PARSE_CS_MONO12:
    case Y4M_PARSE_CS_MONO14:
    case Y4M_PARSE_CS_MONO16:
        if (y4m.range == Y4M_PARSE_RANGE_FULL)
            conv = conv_mono_full;
        else
            conv = conv_mono;
        break;
    case Y4M_PARSE_CS_422:
    case Y4M_PARSE_CS_422P10:
        if (y4m.range == Y4M_PARSE_RANGE_FULL)
            conv = J422ToARGB;
        else
            conv = I422ToARGB;
        break;
    case Y4M_PARSE_CS_444:
    case Y4M_PARSE_CS_444P10:
        if (y4m.range == Y4M_PARSE_RANGE_FULL)
            conv = J444ToARGB;
        else
            conv = I444ToARGB;
        break;
    case Y4M_PARSE_CS_420JPEG:
    case Y4M_PARSE_CS_420MPEG2:
    case Y4M_PARSE_CS_420PALDV:
    case Y4M_PARSE_CS_420:
    case Y4M_PARSE_CS_420P10:
        if (y4m.range == Y4M_PARSE_RANGE_FULL)
            conv = J420ToARGB;
        else
            conv = I420ToARGB;
        break;
    default:
        DL("colour_space: %d\n", y4m.colour_space);
        return LOAD_BADIMAGE;
        break;
    }

    if (pf)
    {
        pf->canvas_w = im->w;
        pf->canvas_h = im->h;
        pf->frame_delay = (y4m.frametime_us + 500) / 1000;
    }

    if (!load_data)
        return LOAD_SUCCESS;

    if (!__imlib_AllocateData(im))
        return LOAD_OOM;

    if (y4m.depth == 10 || y4m.depth == 12 || y4m.depth == 14 ||
        y4m.depth == 16)
    {
        /* convert y4m (10/12/14/16-bit YUV) to 8-bit YUV (same subsampling) */

        /* 1. allocate a small buffer to convert image data to 8-bit */
        int             ypixels = y4m.w * y4m.h;
        int             cpixels = 0;
        int             cstride_pixels = 0;
        if (y4m.colour_space == Y4M_PARSE_CS_420P10)
        {
            cpixels = ((y4m.w + 1) / 2) * ((y4m.h + 1) / 2);
            cstride_pixels = ((y4m.w + 1) / 2);
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_422P10)
        {
            cpixels = ((y4m.w + 1) / 2) * y4m.h;
            cstride_pixels = ((y4m.w + 1) / 2);
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_444P10)
        {
            cpixels = ypixels;
            cstride_pixels = y4m.w;
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_MONO10 ||
                 y4m.colour_space == Y4M_PARSE_CS_MONO12 ||
                 y4m.colour_space == Y4M_PARSE_CS_MONO14 ||
                 y4m.colour_space == Y4M_PARSE_CS_MONO16)
        {
            cpixels = 0;
            cstride_pixels = 0;
        }
        size_t          buf_size = ypixels + 2 * cpixels;
        uint8_t        *buf = calloc((buf_size), sizeof(uint8_t));
        if (!buf)
            return LOAD_OOM;

        uint8_t        *buf_y = buf;
        uint8_t        *buf_u = buf + ypixels;
        uint8_t        *buf_v = buf + ypixels + cpixels;
        int             buf_stride_y = y4m.w;
        ptrdiff_t       buf_stride_u = cstride_pixels;
        ptrdiff_t       buf_stride_v = cstride_pixels;

        /* 2. run the color conversion */
        if (y4m.colour_space == Y4M_PARSE_CS_420P10)
        {
            I010ToI420(y4m.y, y4m.y_stride, y4m.u, y4m.u_stride,
                       y4m.v, y4m.v_stride, buf_y, buf_stride_y,
                       buf_u, buf_stride_u, buf_v, buf_stride_v, y4m.w, y4m.h);
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_422P10)
        {
            I210ToI422(y4m.y, y4m.y_stride, y4m.u, y4m.u_stride,
                       y4m.v, y4m.v_stride, buf_y, buf_stride_y,
                       buf_u, buf_stride_u, buf_v, buf_stride_v, y4m.w, y4m.h);
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_444P10)
        {
            I410ToI444(y4m.y, y4m.y_stride, y4m.u, y4m.u_stride,
                       y4m.v, y4m.v_stride, buf_y, buf_stride_y,
                       buf_u, buf_stride_u, buf_v, buf_stride_v, y4m.w, y4m.h);
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_MONO10)
        {
            /* libyuv does not support 10-bit grayscale (Y10-like) color.
             * Let's downconvert to 8-bit before using the original mono
             * conversion function. */
            /* convert the 10-bit plane to an 8-bit plane */
            for (int i = 0; i < y4m.w * y4m.h; ++i)
            {
                /* convert 10-bit to 8-bit */
                buf_y[i] = (uint8_t) ((*((uint16_t *) (y4m.y) + i)) >> 2);
            }
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_MONO12)
        {
            /* libyuv does not support 12-bit grayscale (Y12-like) color.
             * Let's downconvert to 8-bit before using the original mono
             * conversion function. */
            /* convert the 12-bit plane to an 8-bit plane */
            for (int i = 0; i < y4m.w * y4m.h; ++i)
            {
                /* convert 12-bit to 8-bit */
                buf_y[i] = (uint8_t) ((*((uint16_t *) (y4m.y) + i)) >> 4);
            }
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_MONO14)
        {
            /* libyuv does not support 14-bit grayscale (Y14-like) color.
             * Let's downconvert to 8-bit before using the original mono
             * conversion function. */
            /* convert the 14-bit plane to an 8-bit plane */
            for (int i = 0; i < y4m.w * y4m.h; ++i)
            {
                /* convert 14-bit to 8-bit */
                buf_y[i] = (uint8_t) ((*((uint16_t *) (y4m.y) + i)) >> 6);
            }
        }
        else if (y4m.colour_space == Y4M_PARSE_CS_MONO16)
        {
            /* libyuv does not support 16-bit grayscale (Y16-like) color.
             * Let's downconvert to 8-bit before using the original mono
             * conversion function. */
            /* convert the 16-bit plane to an 8-bit plane */
            for (int i = 0; i < y4m.w * y4m.h; ++i)
            {
                /* convert 16-bit to 8-bit */
                buf_y[i] = (uint8_t) ((*((uint16_t *) (y4m.y) + i)) >> 8);
            }
        }

        /* 3. convert 8-bit YUV (original subsampling) to 8-bit ARGB */
        res = conv(buf_y, buf_stride_y, buf_u, buf_stride_u,
                   buf_v, buf_stride_v,
                   (uint8_t *) im->data, im->w * 4, im->w, im->h);

        free(buf);
    }
    else
    {
        res =
            conv(y4m.y, y4m.y_stride, y4m.u, y4m.u_stride, y4m.v, y4m.v_stride,
                 (uint8_t *) im->data, im->w * 4, im->w, im->h);
    }

    if (res != 0)
        return LOAD_BADIMAGE;

    if (im->lc)
        __imlib_LoadProgressRows(im, 0, im->h);

    return LOAD_SUCCESS;
}

IMLIB_LOADER(_formats, _load, NULL);
