#include "config.h"
#include "Imlib2_Loader.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define DBG_PFX "LDR-pnm"

static const char  *const _formats[] = { "pnm", "ppm", "pgm", "pbm", "pam" };

typedef enum {
   BW_RAW_PACKED, BW_RAW, BW_PLAIN, GRAY_RAW, GRAY_PLAIN, RGB_RAW, RGB_PLAIN,
   XV332
} px_type;

static struct {
   const unsigned char *data, *dptr;
   unsigned int        size;
} mdata;

static void
mm_init(const void *src, unsigned int size)
{
   mdata.data = mdata.dptr = src;
   mdata.size = size;
}

static int
mm_read(void *dst, unsigned int len)
{
   if (mdata.dptr + len > mdata.data + mdata.size)
      return 1;                 /* Out of data */

   memcpy(dst, mdata.dptr, len);
   mdata.dptr += len;

   return 0;
}

static int
mm_getc(void)
{
   unsigned char       ch;

   if (mdata.dptr + 1 > mdata.data + mdata.size)
      return -1;                /* Out of data */

   ch = *mdata.dptr++;

   return ch;
}

static int
mm_get01(void)
{
   int                 ch;

   for (;;)
     {
        ch = mm_getc();
        switch (ch)
          {
          case '0':
             return 0;
          case '1':
             return 1;
          case ' ':
          case '\t':
          case '\r':
          case '\n':
             continue;
          default:
             return -1;
          }
     }
}

static int
mm_getu(unsigned int *pui)
{
   int                 ch;
   int                 uval;
   bool                comment;

   /* Strip whitespace and comments */

   for (comment = false;;)
     {
        ch = mm_getc();
        if (ch < 0)
           return ch;
        if (comment)
          {
             if (ch == '\n')
                comment = false;
             continue;
          }
        if (isspace(ch))
           continue;
        if (ch != '#')
           break;
        comment = true;
     }

   if (!isdigit(ch))
      return -1;

   /* Parse number */
   for (uval = 0;;)
     {
        uval = 10 * uval + ch - '0';
        ch = mm_getc();
        if (ch < 0)
           return ch;
        if (!isdigit(ch))
           break;
     }

   *pui = uval;
   return 0;                    /* Ok */
}

static int
mm_parse_pam_header(unsigned *w, unsigned *h, unsigned *v, px_type * pxt,
                    char *alpha)
{
   char                tuple_type[32] = { 0 };
   unsigned            tti = 0, d;

   for (int ch;;)
     {
        if ((ch = mm_getc()) == -1)
           return -1;
        if (ch == '#')
          {
             do
               {
                  if ((ch = mm_getc()) == -1)
                     return -1;
               }
             while (ch != '\n');
          }
        else
          {
             char                key[9] = { 0 };        /* max key size per spec: 8 */
             for (unsigned ki = 0; !isspace(ch) && ki < sizeof(key) - 1; ++ki)
               {
                  key[ki] = (char)ch;
                  if ((ch = mm_getc()) == -1)
                     return -1;
               }
             if (!strcmp(key, "ENDHDR"))
                break;

             unsigned int       *p = NULL;

             if (!strcmp(key, "HEIGHT"))
                p = h;
             else if (!strcmp(key, "WIDTH"))
                p = w;
             else if (!strcmp(key, "DEPTH"))
                p = &d;
             else if (!strcmp(key, "MAXVAL"))
                p = v;
             else if (!strcmp(key, "TUPLTYPE"))
               {
                  while (isspace(ch))
                    {
                       if ((ch = mm_getc()) == -1)
                          return -1;
                    }
                  if (tti != 0) /* not the first TUPLE_TYPE header */
                    {
                       if (tti < sizeof(tuple_type) - 1)
                          tuple_type[tti++] = ' ';
                    }
                  while (ch != '\n' && tti < sizeof(tuple_type) - 1)
                    {
                       tuple_type[tti++] = ch;
                       if ((ch = mm_getc()) == -1)
                          return -1;
                    }
               }
             else               /* unknown header */
               {

               }

             if (p)
               {
                  if (mm_getu(p) == -1)
                     return -1;
               }
          }
     }

   *alpha = (tti >= 6 && !strcmp(tuple_type + tti - 6, "_ALPHA"));

   if (!strncmp(tuple_type, "BLACKANDWHITE", 13))
     {
        *pxt = BW_RAW;
        /* assert(d == *alpha + 1);
         * assert(*v == 1); */
     }
   else if (!strncmp(tuple_type, "GRAYSCALE", 9))
     {
        *pxt = GRAY_RAW;
        /* assert(d == *alpha + 1); */
     }
   else if (!strncmp(tuple_type, "RGB", 3))
     {
        *pxt = RGB_RAW;
        /* assert(d == *alpha + 3); */
     }
   else                         /* unknown tuple type */
     {
        return -1;
     }
   return 0;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   int                 p;
   unsigned int        w, h, v, hlen;
   uint8_t            *data = NULL;     /* for the binary versions */
   uint8_t            *ptr = NULL;
   int                *idata = NULL;    /* for the ASCII versions */
   uint32_t           *ptr2, rval, gval, bval;
   unsigned            i, j, x, y;
   px_type             pxt;

   rc = LOAD_FAIL;

   mm_init(im->fi->fdata, im->fi->fsize);

   /* read the header info */
   if (mm_getc() != 'P')
      goto quit;

   p = mm_getc();
   hlen = 3;
   switch (p)
     {
     case '1':
        pxt = BW_PLAIN;
        hlen = 2;
        break;
     case '2':
        pxt = GRAY_PLAIN;
        break;
     case '3':
        pxt = RGB_PLAIN;
        break;
     case '4':
        pxt = BW_RAW_PACKED;
        hlen = 2;
        break;
     case '5':
        pxt = GRAY_RAW;
        break;
     case '6':
        pxt = RGB_RAW;
        break;
     case '7':
        if (mm_getc() != '\n')
          {
             if (mm_getu(&gval) || gval != 332) /* XV thumbnail format */
                goto quit;
             pxt = XV332;
          }
        else
          {
             if (mm_parse_pam_header(&w, &h, &v, &pxt, &im->has_alpha))
                goto quit;
             hlen = 0;
          }
        break;
     case '8':                 /* not in netpbm, unknown format provenance (Imlib2 specific?) */
        pxt = RGB_RAW;
        im->has_alpha = 1;
        break;
     default:
        goto quit;
     }

   /* read header for non-PAM formats */
   if (hlen != 0)
     {
        w = h = 0;
        v = 255;
        for (i = 0; i < hlen; i++)
          {
             if (mm_getu(&gval))
                goto quit;

             switch (i)
               {
               case 0:         /* width */
                  w = gval;
                  break;
               case 1:         /* height */
                  h = gval;
                  break;
               case 2:         /* max value, only for color and greyscale */
                  v = gval;
                  break;
               }
          }
     }

   if (v > 255)
      goto quit;

   D("P%c: pxtype=%d WxH=%ux%u V=%u A=%s\n",
     p, pxt, w, h, v, im->has_alpha ? "YES" : "NO");

   rc = LOAD_BADIMAGE;          /* Format accepted */

   im->w = w;
   im->h = h;
   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto quit;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   ptr2 = __imlib_AllocateData(im);
   if (!ptr2)
      QUIT_WITH_RC(LOAD_OOM);

   /* start reading the data */
   switch (pxt)
     {
     case BW_PLAIN:            /* ASCII monochrome */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  int                 px = mm_get01();

                  if (px < 0)
                     goto quit;

                  *ptr2++ = px ? 0xff000000 : 0xffffffff;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     case GRAY_PLAIN:          /* ASCII greyscale */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  if (mm_getu(&gval))
                     goto quit;

                  if (v == 0 || v == 255)
                    {
                       *ptr2++ = 0xff000000 | (gval << 16) | (gval << 8) | gval;
                    }
                  else
                    {
                       *ptr2++ =
                          0xff000000 |
                          (((gval * 255) / v) << 16) |
                          (((gval * 255) / v) << 8) | ((gval * 255) / v);
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     case RGB_PLAIN:           /* ASCII RGB */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  if (mm_getu(&rval))
                     goto quit;
                  if (mm_getu(&gval))
                     goto quit;
                  if (mm_getu(&bval))
                     goto quit;

                  if (v == 0 || v == 255)
                    {
                       *ptr2++ = 0xff000000 | (rval << 16) | (gval << 8) | bval;
                    }
                  else
                    {
                       *ptr2++ =
                          0xff000000 |
                          (((rval * 255) / v) << 16) |
                          (((gval * 255) / v) << 8) | ((bval * 255) / v);
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     case BW_RAW_PACKED:       /* binary 1bit monochrome */
        data = malloc((w + 7) / 8 * sizeof(uint8_t));
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, (w + 7) / 8))
                goto quit;

             ptr = data;
             for (x = 0; x < w; x += 8)
               {
                  j = (w - x >= 8) ? 8 : w - x;
                  for (i = 0; i < j; i++)
                    {
                       if (ptr[0] & (0x80 >> i))
                          *ptr2 = 0xff000000;
                       else
                          *ptr2 = 0xffffffff;
                       ptr2++;
                    }
                  ptr++;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     case BW_RAW:              /* binary 1byte monochrome */
        if (im->has_alpha)
          {
             data = malloc(2 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 2))
                     goto quit;

                  ptr = data;
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          (ptr[1] ? 0xff000000 : 0) | (ptr[0] ? 0xffffff : 0);
                       ptr2++;
                       ptr += 2;
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        else
          {
             data = malloc(1 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 1))
                     goto quit;

                  ptr = data;
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 = 0xff000000 | (ptr[0] ? 0xffffff : 0);
                       ptr2++;
                       ptr++;
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     case GRAY_RAW:            /* binary 8bit grayscale GGGGGGGG */
        if (im->has_alpha)
          {
             data = malloc(2 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 2))
                     goto quit;

                  ptr = data;
                  if (v == 0 || v == 255)
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               ((uint32_t)ptr[1] << 24) | (ptr[0] << 16) |
                               (ptr[0] << 8) | ptr[0];
                            ptr2++;
                            ptr += 2;
                         }
                    }
                  else
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               ((uint32_t)((ptr[1] * 255) / v) << 24) |
                               (((ptr[0] * 255) / v) << 16) |
                               (((ptr[0] * 255) / v) << 8) | ((ptr[0] * 255) /
                                                              v);
                            ptr2++;
                            ptr += 2;
                         }
                    }

                  if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                     goto quit_progress;
               }
          }
        else
          {
             data = malloc(1 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 1))
                     goto quit;

                  ptr = data;
                  if (v == 0 || v == 255)
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               0xff000000 | (ptr[0] << 16) | (ptr[0] << 8) |
                               ptr[0];
                            ptr2++;
                            ptr++;
                         }
                    }
                  else
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               0xff000000 |
                               (((ptr[0] * 255) / v) << 16) |
                               (((ptr[0] * 255) / v) << 8) | ((ptr[0] * 255) /
                                                              v);
                            ptr2++;
                            ptr++;
                         }
                    }

                  if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                     goto quit_progress;
               }
          }
        break;

     case RGB_RAW:             /* 24bit binary RGBRGBRGB */
        if (im->has_alpha)
          {
             data = malloc(4 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 4))
                     goto quit;

                  ptr = data;
                  if (v == 0 || v == 255)
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 = PIXEL_ARGB(ptr[3], ptr[0], ptr[1], ptr[2]);
                            ptr2++;
                            ptr += 4;
                         }
                    }
                  else
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               PIXEL_ARGB((ptr[3] * 255) / v,
                                          (ptr[0] * 255) / v,
                                          (ptr[1] * 255) / v,
                                          (ptr[2] * 255) / v);
                            ptr2++;
                            ptr += 4;
                         }
                    }

                  if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                     goto quit_progress;
               }
          }
        else
          {
             data = malloc(3 * sizeof(uint8_t) * w);
             if (!data)
                QUIT_WITH_RC(LOAD_OOM);

             ptr2 = im->data;
             for (y = 0; y < h; y++)
               {
                  if (mm_read(data, w * 3))
                     goto quit;

                  ptr = data;
                  if (v == 0 || v == 255)
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               0xff000000 | (ptr[0] << 16) | (ptr[1] << 8) |
                               ptr[2];
                            ptr2++;
                            ptr += 3;
                         }
                    }
                  else
                    {
                       for (x = 0; x < w; x++)
                         {
                            *ptr2 =
                               0xff000000 |
                               (((ptr[0] * 255) / v) << 16) |
                               (((ptr[1] * 255) / v) << 8) | ((ptr[2] * 255) /
                                                              v);
                            ptr2++;
                            ptr += 3;
                         }
                    }

                  if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                     goto quit_progress;
               }
          }
        break;

     case XV332:               /* XV's 8bit 332 format */
        data = malloc(1 * sizeof(uint8_t) * w);
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, w * 1))
                goto quit;

             ptr = data;
             for (x = 0; x < w; x++)
               {
                  int                 r, g, b;

                  r = (*ptr >> 5) & 0x7;
                  g = (*ptr >> 2) & 0x7;
                  b = (*ptr) & 0x3;
                  *ptr2 =
                     0xff000000 |
                     (((r << 21) | (r << 18) | (r << 15)) & 0xff0000) |
                     (((g << 13) | (g << 10) | (g << 7)) & 0xff00) |
                     ((b << 6) | (b << 4) | (b << 2) | (b << 0));
                  ptr2++;
                  ptr++;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;

     default:
        goto quit;
      quit_progress:
        rc = LOAD_BREAK;
        goto quit;
     }

   rc = LOAD_SUCCESS;

 quit:
   free(idata);
   free(data);

   return rc;
}

static int
_save(ImlibImage * im)
{
   int                 rc;
   FILE               *f;
   uint8_t            *buf, *bptr;
   uint32_t           *ptr;
   int                 x, y;

   f = fopen(im->fi->name, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_FAIL;

   /* allocate a small buffer to convert image data */
   buf = malloc(im->w * 4 * sizeof(uint8_t));
   if (!buf)
      goto quit;

   ptr = im->data;

   /* if the image has a useful alpha channel */
   if (im->has_alpha)
     {
        fprintf(f, "P8\n" "# PNM File written by Imlib2\n" "%i %i\n" "255\n",
                im->w, im->h);
        for (y = 0; y < im->h; y++)
          {
             bptr = buf;
             for (x = 0; x < im->w; x++)
               {
                  uint32_t            pixel = *ptr++;

                  bptr[0] = PIXEL_R(pixel);
                  bptr[1] = PIXEL_G(pixel);
                  bptr[2] = PIXEL_B(pixel);
                  bptr[3] = PIXEL_A(pixel);
                  bptr += 4;
               }
             fwrite(buf, im->w * 4, 1, f);

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
     }
   else
     {
        fprintf(f, "P6\n" "# PNM File written by Imlib2\n" "%i %i\n" "255\n",
                im->w, im->h);
        for (y = 0; y < im->h; y++)
          {
             bptr = buf;
             for (x = 0; x < im->w; x++)
               {
                  uint32_t            pixel = *ptr++;

                  bptr[0] = PIXEL_R(pixel);
                  bptr[1] = PIXEL_G(pixel);
                  bptr[2] = PIXEL_B(pixel);
                  bptr += 3;
               }
             fwrite(buf, im->w * 3, 1, f);

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
     }

   rc = LOAD_SUCCESS;

 quit:
   /* finish off */
   free(buf);
   fclose(f);

   return rc;

 quit_progress:
   rc = LOAD_BREAK;
   goto quit;
}

IMLIB_LOADER(_formats, _load, _save);
