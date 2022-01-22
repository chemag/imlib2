#include "config.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <math.h>
#include <string.h>
#include <sys/types.h>

#include "blend.h"
#include "colormod.h"
#include "common.h"
#include "font.h"
#include "image.h"
#include "rgbadraw.h"
#include "rotate.h"

FT_Library          ft_lib;

void
__imlib_font_init(void)
{
   static int          initialised = 0;
   int                 error;

   if (initialised)
      return;
   error = FT_Init_FreeType(&ft_lib);
   if (error)
      return;
   initialised = 1;
}

int
__imlib_font_ascent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->ascender;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_descent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = -(int)fn->ft.face->descender;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_max_ascent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->bbox.yMax;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_max_descent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->bbox.yMin;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_get_line_advance(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->height;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_utf8_get_next(unsigned char *buf, int *iindex)
{
   /* Reads UTF8 bytes from @buf, starting at *@index and returns the code
    * point of the next valid code point. @index is updated ready for the
    * next call.
    * 
    * * Returns 0 to indicate an error (e.g. invalid UTF8) */

   int                 index = *iindex, r;
   unsigned char       d = buf[index++], d2, d3, d4;

   if (!d)
      return 0;
   if (d < 0x80)
     {
        *iindex = index;
        return d;
     }
   if ((d & 0xe0) == 0xc0)
     {
        /* 2 byte */
        d2 = buf[index++];
        if ((d2 & 0xc0) != 0x80)
           return 0;
        r = d & 0x1f;           /* copy lower 5 */
        r <<= 6;
        r |= (d2 & 0x3f);       /* copy lower 6 */
     }
   else if ((d & 0xf0) == 0xe0)
     {
        /* 3 byte */
        d2 = buf[index++];
        d3 = buf[index++];
        if ((d2 & 0xc0) != 0x80 || (d3 & 0xc0) != 0x80)
           return 0;
        r = d & 0x0f;           /* copy lower 4 */
        r <<= 6;
        r |= (d2 & 0x3f);
        r <<= 6;
        r |= (d3 & 0x3f);
     }
   else
     {
        /* 4 byte */
        d2 = buf[index++];
        d3 = buf[index++];
        d4 = buf[index++];
        if ((d2 & 0xc0) != 0x80 || (d3 & 0xc0) != 0x80 || (d4 & 0xc0) != 0x80)
           return 0;
        r = d & 0x0f;           /* copy lower 4 */
        r <<= 6;
        r |= (d2 & 0x3f);
        r <<= 6;
        r |= (d3 & 0x3f);
        r <<= 6;
        r |= (d4 & 0x3f);

     }
   *iindex = index;
   return r;
}
