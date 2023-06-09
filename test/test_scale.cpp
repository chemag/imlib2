#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

#define FILE_REF1	"icon-64"       // RGB
#define FILE_REF2	"xeyes" // ARGB (shaped)

typedef struct {
   const char         *file;
   unsigned int        crcs[4];
} td_t;

/**INDENT-OFF**/
static const td_t   td[] = {
   { FILE_REF1, { 1153555547, 1208851425, 1464496753, 4181395999 }},
   { FILE_REF2, { 2937827957, 1356142132, 3061068732,  830163639 }},
   { FILE_REF1, { 1153555547, 1813415566, 2513294192, 1184904601 }},	// mmx
   { FILE_REF2, { 2937827957,  199400762, 1969395327, 3756282520 }},	// mmx
};
/**INDENT-ON**/

static void
test_scale(int no)
{
   const td_t         *ptd;
   char                filei[256];
   char                fileo[256];
   int                 w, h;
   unsigned int        i, crc;
   Imlib_Image         imi, imo;
   int                 err;

#ifdef DO_MMX_ASM
   // Hmm.. MMX functions appear to produce a slightly different result
   if (!getenv("IMLIB2_ASM_OFF"))
      no += 2;
#endif
   ptd = &td[no];

   snprintf(filei, sizeof(filei), "%s/%s.png", IMG_SRC, ptd->file);
   D("Load '%s'\n", filei);
   imi = imlib_load_image(filei);
   ASSERT_TRUE(imi);

   crc = image_get_crc32(imi);
   EXPECT_EQ(crc, ptd->crcs[0]);

   for (i = 0; i < 4; i++)
     {
        imlib_context_set_image(imi);
        w = imlib_image_get_width();
        h = imlib_image_get_height();

        imo = imlib_create_cropped_scaled_image(0, 0, w, h, w + i, h + i);
        ASSERT_TRUE(imo);
        imlib_context_set_image(imo);

        w = imlib_image_get_width();
        h = imlib_image_get_height();

        crc = image_get_crc32(imo);
        EXPECT_EQ(crc, ptd->crcs[i]);

        snprintf(fileo, sizeof(fileo), "%s/scale-%s-%dx%d.%s",
                 IMG_GEN, ptd->file, w, h, "png");
        imlib_image_set_format("png");
        D("Save '%s'\n", fileo);
        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
           D("Error %d saving '%s'\n", err, fileo);

        imlib_context_set_image(imo);
        imlib_free_image_and_decache();
     }

   imlib_context_set_image(imi);
   imlib_free_image_and_decache();
}

TEST(SCALE, scale_1_rgb)
{
   test_scale(0);
}

TEST(SCALE, scale_1_argb)
{
   test_scale(1);
}
