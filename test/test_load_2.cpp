#include <gtest/gtest.h>

#include "config.h"
#define IMLIB2_DEPRECATED       // Suppress deprecation warnings
#include <Imlib2.h>

#include <fcntl.h>
#include <zlib.h>

#include "test.h"

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

typedef struct {
   const char         *name;
   unsigned int        crc;
} tii_t;

static tii_t        tii[] = {
/**INDENT-OFF**/
   { "icon-64.argb",		1153555547 },
   { "icon-64.bmp",		1153555547 },
   { "icon-64.ff",		1153555547 },
#ifdef BUILD_BZ2_LOADER
   { "icon-64.ff.bz2",		1153555547 },
#endif
#ifdef BUILD_ZLIB_LOADER
   { "icon-64.ff.gz",		1153555547 },
#endif
#ifdef BUILD_LZMA_LOADER
   { "icon-64.ff.xz",		1153555547 },
#endif
   { "icon-64.gif",		4016720483 },
#ifdef BUILD_HEIF_LOADER
   { "icon-64.heif",		 174609659 },
#endif
   { "icon-64.ico",		1153555547 },
   { "icon-64.ilbm",		1153555547 },
   { "icon-64.jpg",		4132154843 },
   { "icon-64.jpg.mp3",		4132154843 },
#ifdef BUILD_J2K_LOADER
   { "icon-64.jp2",		 451428725 },
   { "icon-64.j2k",		 451428725 },
   { "xeyes.jp2",		2937827957 },
   { "xeyes.j2k",		2937827957 },
   { "icon-64-gray.jp2",	2437152898 },
   { "icon-64-gray.j2k",	2437152898 },
   { "xeyes-gray.jp2",		3377113384 },
   { "xeyes-gray.j2k",		3377113384 },
#endif
#ifdef BUILD_JXL_LOADER
   { "icon-64.jxl",		 712907299 },
#endif
   { "icon-64.png",		1153555547 },
   { "xeyes-gray.png",		3493264608 },
   { "icon-64.ppm",		1153555547 },
   { "icon-64.pgm",		 140949526 },
   { "icon-64.pbm",		2153856013 },
   { "icon-64-P3.ppm",		1153555547 },
   { "icon-64-P2.pgm",		 140949526 },
   { "icon-64-P1.pbm",		2153856013 },
   { "icon-64.tga",		1153555547 },
   { "icon-64.tiff",		1153555547 },
   { "icon-64.webp",		1698406918 },
   { "icon-64.xbm",		2153856013 },
   { "icon-64.xpm",		4016720483 },

   { "icon-128.ico",		 218415319 },
   { "icon-128-d1.ico",		3776822558 },
   { "icon-128-d4.ico",		1822311162 },
   { "icon-128-d8.ico",		2584400446 },
/**INDENT-ON**/
};
#define NT3_IMGS (sizeof(tii) / sizeof(tii_t))

TEST(LOAD2, load_1)
{
   unsigned int        i, crc, w, h;
   const char         *fn;
   char                buf[256];
   Imlib_Image         im;
   unsigned char      *data;

   for (i = 0; i < NT3_IMGS; i++)
     {
        fn = tii[i].name;
        if (*fn != '/')
          {
             snprintf(buf, sizeof(buf), "%s/%s", IMG_SRC, fn);
             fn = buf;
          }
        D("Load '%s'\n", fn);
        im = imlib_load_image(fn);
        ASSERT_TRUE(im);
        imlib_context_set_image(im);
        data = (unsigned char *)imlib_image_get_data();
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        crc = crc32(0, data, w * h * sizeof(uint32_t));
        EXPECT_EQ(crc, tii[i].crc);
     }
}
