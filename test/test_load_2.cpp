#include <gtest/gtest.h>

#include <Imlib2.h>
#include <fcntl.h>
#include <zlib.h>

#include "config.h"
#include "test_common.h"

int                 debug = 0;

#define D(...) if (debug) printf(__VA_ARGS__)

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
   { "icon-64.gif",		1768448874 },
   { "icon-64.ico",		1153555547 },
   { "icon-64.ilbm",		1153555547 },
   { "icon-64.jpeg",		4132154843 },
   { "icon-64.jpg",		4132154843 },
   { "icon-64.jpg.mp3",		4132154843 },
   { "icon-64.pbm",		 907392323 },
   { "icon-64.png",		1153555547 },
   { "icon-64.ppm",		1153555547 },
   { "icon-64.tga",		1153555547 },
   { "icon-64.tiff",		1153555547 },
   { "icon-64.webp",		1698406918 },
   { "icon-64.xbm",		 907392323 },
   { "icon-64.xpm",		1768448874 },

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
        crc = crc32(0, data, w * h * sizeof(DATA32));
        EXPECT_EQ(crc, tii[i].crc);
     }
}

int
main(int argc, char **argv)
{
   const char         *s;

   ::testing::InitGoogleTest(&argc, argv);

   for (argc--, argv++; argc > 0; argc--, argv++)
     {
        s = argv[0];
        if (*s++ != '-')
           break;
        switch (*s)
          {
          case 'd':
             debug++;
             break;
          }
     }

   return RUN_ALL_TESTS();
}
