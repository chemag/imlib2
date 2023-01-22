#include <gtest/gtest.h>

#include "config.h"
#include "test.h"

int                 debug = 0;

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
      again:
        switch (*s++)
          {
          case 'd':
             debug++;
             goto again;
          }
     }

   // Required by some tests
   mkdir(IMG_GEN, 0755);

   return RUN_ALL_TESTS();
}

#include <Imlib2.h>
#include <zlib.h>

unsigned int
image_get_crc32(Imlib_Image im)
{
   const unsigned char *data;
   unsigned int        crc, w, h;

   imlib_context_set_image(im);
   w = imlib_image_get_width();
   h = imlib_image_get_height();
   data = (const unsigned char *)imlib_image_get_data_for_reading_only();
   crc = crc32(0, data, w * h * sizeof(uint32_t));

   return crc;
}
