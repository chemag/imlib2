#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

#define FILE_REF1	"icon-64"       // RGB
#define FILE_REF2	"xeyes" // ARGB (shaped)

typedef struct {
   const char         *file;
   unsigned int        crcs[7][7];
} td_t;

/**INDENT-OFF**/
static const td_t   td[] = {
   { FILE_REF1,
     {{ 2162077252, 4214091794,  412865864, 2208852583, 4008806248, 2557426026, 3819956603 },
      {  595275359,  283954495, 3441282729, 3333662483, 2515534288, 2423654221, 1122712545 },
      { 4233001257, 1723457820, 1156039624, 3037040985, 3626247407, 2613094123, 2184173847 },
      { 3309372516,  984665754, 3767976262, 1153555547, 1698491060, 4110238991, 1891961102 },
      { 2847337837,  794693659, 1876625640, 3678611634, 1208851425, 3813154232, 2783573400 },
      {  764823695, 3982871911, 1209331970, 4264374244, 1817759116, 1464496753, 3881173833 },
      { 3864856322,  990950723,  372177638, 1857970971, 1641112498, 4095443635, 4181395999 }},
   },
   { FILE_REF2,
     {{  555768304, 1646920612, 2050816676, 3770476584, 2825645535, 3113513304,  913084799 },
      {  220865185,  862759789, 4159187644, 2770777978, 3461933485, 2827786048, 2219120417 },
      { 2540897236, 2887022398,   65635572,  916230131, 3952902516,   62808115, 2932301957 },
      { 3434262573, 3693633821,  137945133, 2937827957,  521763255,  411503651, 2049615039 },
      {  239430986,  332364389, 1260118846,  554129137, 1356142132, 2853343843, 1297241144 },
      { 1175312809, 3451222972, 3395731656,  295362128, 1881386666, 3061068732, 3976437623 },
      { 1755782477,  704181442, 2272158502,  325667026, 1654128943,  901365279,  830163639 }},
   },
   { FILE_REF1,	// mmx
     {{ 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 },
      { 0, 0, 0, 1153555547, 1813415566, 2513294192, 1184904601 }},
   },
   { FILE_REF2,	// mmx
     {{ 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 },
      { 0, 0, 0, 2937827957,  199400762, 1969395327, 3756282520 }},
   },
};
/**INDENT-ON**/

static void
test_scale_1(int alpha)
{
   const td_t         *ptd;
   char                filei[256];
   char                fileo[256];
   int                 w, h, i, j;
   unsigned int        crc, crc_exp;
   Imlib_Image         imi, imo;
   int                 err;

#ifdef DO_MMX_ASM
   // Hmm.. MMX functions appear to produce a slightly different result
   if (!getenv("IMLIB2_ASM_OFF"))
      alpha += 2;
#endif
   ptd = &td[alpha];

   snprintf(filei, sizeof(filei), "%s/%s.png", IMG_SRC, ptd->file);
   D("Load '%s'\n", filei);
   imi = imlib_load_image(filei);
   ASSERT_TRUE(imi);

   crc = image_get_crc32(imi);
   crc_exp = ptd->crcs[3][3];
   EXPECT_EQ(crc, crc_exp);

   for (i = -3; i <= 3; i++)
     {
        for (j = -3; j <= 3; j++)
          {
             imlib_context_set_image(imi);
             w = imlib_image_get_width();
             h = imlib_image_get_height();

             pr_info("Alpha=%d: %dx%d -> %dx%d (%d,%d)", alpha,
                     w, h, w + i, h + j, i, j);

             imo = imlib_create_cropped_scaled_image(0, 0, w, h, w + i, h + j);
             ASSERT_TRUE(imo);
             imlib_context_set_image(imo);

             w = imlib_image_get_width();
             h = imlib_image_get_height();

             crc = image_get_crc32(imo);
             crc_exp = ptd->crcs[3 + j][3 + i];
             EXPECT_EQ(crc, crc_exp);

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
     }

   imlib_context_set_image(imi);
   imlib_free_image_and_decache();
}

static void
test_scale_2(int alpha, int w0, int h0, int w1, int h1, int w2, int h2)
{
   int                 w, h;
   Imlib_Image         imi, imo;

   pr_info("Alpha=%d: %dx%d -> %d:%dx%d:%d", alpha, w0, h0, w1, w2, h1, h2);

   imi = imlib_create_image(w0, h0);
   ASSERT_TRUE(imi);

   imlib_context_set_image(imi);
   imlib_image_set_has_alpha(alpha);
   imlib_context_set_blend(0);

   imlib_context_set_color(0x22, 0x44, 0x66, 0x88);
   imlib_image_fill_rectangle(0, 0, w0, h0);

   for (w = w1; w <= w2; w++)
     {
        for (h = h1; h <= h2; h++)
          {
//           pr_info("Alpha=%d: %dx%d -> %dx%d", alpha, w0, h0, w, h);

             imlib_context_set_image(imi);

             imo = imlib_create_cropped_scaled_image(0, 0, w0, h0, w, h);
             if (w <= 0 || h <= 0)
               {
                  ASSERT_FALSE(imo);
                  continue;
               }
             ASSERT_TRUE(imo);

             imlib_context_set_image(imo);
             imlib_free_image_and_decache();
          }
     }

   imlib_context_set_image(imi);
   imlib_free_image_and_decache();
}

TEST(SCALE, scale_1_rgb)
{
   test_scale_1(0);
}

TEST(SCALE, scale_1_argb)
{
   test_scale_1(1);
}

TEST(SCALE, scale_2_rgb_0)
{
   test_scale_2(0, 4, 4, 0, 0, 0, 0);
}

TEST(SCALE, scale_2_rgb_1_x)
{
   test_scale_2(0, 1, 1, 1, 1, 10, 10);
}

TEST(SCALE, scale_2_rgb_2_x)
{
   test_scale_2(0, 2, 2, 1, 1, 10, 10);
}

TEST(SCALE, scale_2_rgb_up)
{
   test_scale_2(0, 7, 30, 7, 30, 7, 100);
   test_scale_2(0, 30, 9, 30, 9, 100, 9);
}

TEST(SCALE, scale_2_rgb_down)
{
   test_scale_2(0, 7, 100, 7, 40, 7, 100);
   test_scale_2(0, 100, 9, 40, 9, 100, 9);
}

TEST(SCALE, scale_2_argb_0)
{
   test_scale_2(1, 4, 4, 0, 0, 0, 0);
}

TEST(SCALE, scale_2_argb_1_x)
{
   test_scale_2(1, 1, 1, 1, 1, 10, 10);
}

TEST(SCALE, scale_2_argb_2_x)
{
   test_scale_2(1, 2, 2, 1, 1, 10, 10);
}

TEST(SCALE, scale_2_argb_up)
{
   test_scale_2(1, 7, 30, 7, 30, 7, 100);
   test_scale_2(1, 30, 9, 30, 9, 100, 9);
}

TEST(SCALE, scale_2_argb_down)
{
   test_scale_2(1, 7, 100, 7, 40, 7, 100);
   test_scale_2(1, 100, 9, 40, 9, 100, 9);
}
