#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

typedef struct {
    const char     *file;
    unsigned int    crcs[7][7];
} td_t;

/**INDENT-OFF**/
static const td_t   td[] = {
   { FILE_PFX1,
     {{ 3046815695, 1454483539, 3370318971, 2874830719, 1214541623, 2844236007, 3032447904 },
      { 1815586576,  263711685, 4047593408, 4057849886, 1594726180, 3131159967, 4057202128 },
      { 1933095809, 3648317153, 1395541195, 1249762085, 2722829659, 2165791240, 2585747393 },
      {  782842457, 1932282766, 3880282857, 1636116234, 3315918683, 1193211817, 1942266510 },
      { 2805205078, 2946349443, 1606463093, 1492088580, 2786516861, 2237182866,  302757948 },
      { 2297885998,  225209835, 2293509423,  328002836, 1089112719,  213167177, 4131082479 },
      { 3901515440, 2796460889,  276806743, 2171015539, 3822837555, 3885475666, 3980468705 }},
   },
   { FILE_PFX2,
     {{  554670403, 1763261605, 1241877634, 3706125386,  228665406, 2072030794, 3839855691 },
      { 1157605892, 2885229835, 1503938172, 4232506104, 3207078945,  313559559, 3094452170 },
      { 1909682519, 1936648972, 2054562341, 2497475393, 3018205552,  256976836, 1601946914 },
      { 3951351362, 2316414843,  278674581,  169859126, 3218891235, 3891167455, 1719701818 },
      { 3843504277, 2214512526, 3952889006, 1898581468, 1778618945, 3013587113,  992972279 },
      { 1923529742, 1813557735, 3545258330, 1990082212, 1565152381, 2575962696,  166753393 },
      { 2676965940, 3119276040, 2124579026, 1860240013, 1452807565, 2552669637, 3314226629 }},
   },
};
/**INDENT-ON**/

static void
test_scale_1(int alpha)
{
    const td_t     *ptd;
    char            filei[256];
    char            fileo[256];
    int             w, h, i, j;
    unsigned int    crc, crc_exp;
    Imlib_Image     imi, imo;
    int             err;

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

            pr_info("AA=%d Alpha=%d: %dx%d -> %dx%d (%d,%d)",
                    imlib_context_get_anti_alias(), alpha,
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
test_scale_2a(int alpha, int w0, int h0, int w1, int h1, int w2, int h2)
{
    int             w, h;
    Imlib_Image     imi, imo;

    pr_info("AA=%d Alpha=%d: %dx%d -> %d:%dx%d:%d",
            imlib_context_get_anti_alias(), alpha, w0, h0, w1, w2, h1, h2);

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

static void
test_scale_2(int alpha, int w0, int h0, int w1, int h1, int w2, int h2)
{
    imlib_context_set_anti_alias(0);
    test_scale_2a(alpha, w0, h0, w1, h1, w2, h2);
    imlib_context_set_anti_alias(1);
    test_scale_2a(alpha, w0, h0, w1, h1, w2, h2);
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
