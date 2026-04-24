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
#ifdef ENABLE_USCALER
#ifdef __i386__
   { FILE_PFX1,
     {{ 4245706519, 3295934506, 3853037928, 1035864029, 4097991351, 1776246123, 2062727270 },
      {  594450109, 3641167648,  568456092, 1080681146, 4278457098, 2950543919, 3237268015 },
      {  782809892, 4170550056, 1498130354,  332125572, 3794250706, 2047656766, 3475883439 },
      { 3574290725, 3263195472,  902396807, 1636116234, 3022228585, 3002900033, 3243488735 },
      { 4055614643,  639948824, 2340626515, 2806013467, 1100585327,  314570666, 1575575341 },
      {  313450508, 3818428321, 3256973761, 2179059908, 1557278094, 2435958984, 4078999882 },
      {  418740142,  516651517, 1893623646, 3377068498, 2110113462, 2317857015, 1077900994 }},
   },
   { FILE_PFX2,
     {{  313469993, 1621090873, 4249868193, 2383629981, 1553332517,  920968377, 1352832716 },
      { 1896718558, 3003301525,  262241770,  649806784, 2938064112, 3151158197,  598633591 },
      {  125732388, 3842647967, 2753917548, 2608246900, 3235173505, 2969915209, 1628812114 },
      { 1820113830, 3577403172, 1533710162,  169859126,  323536092, 1607734957, 3336685779 },
      { 3295285340, 1846604070,  528506442,  539963535, 2442878685, 1625121910, 2800888738 },
      { 2061876226, 2937524483, 3888339165, 3758672483,  253960209, 2026819422, 1357604943 },
      { 1947848084, 2724858217, 3094261808, 3353880260,  109704387, 4129758123, 3777273844 }},
   },
#else
   { FILE_PFX1,
     {{ 4245706519, 3295934506,  464852744,  285361659, 4097991351,  548115439, 2828138628 },
      { 2352460102,  604360091, 1602241567, 4081740353, 4278457098, 2950543919, 3237268015 },
      {  782809892, 4170550056,  255429293,  746639253, 2578989385, 1171307755, 1522512343 },
      { 2612297179, 1687815195,  902396807, 1636116234, 3022228585, 3002900033, 3175835115 },
      { 4055614643,  639948824, 2473142129, 2806013467, 1100585327,  314570666, 1154755109 },
      {  326793140, 3818428321, 3256973761, 2179059908, 1557278094, 2435958984, 1781293855 },
      { 1641695958, 2430413301, 2104951014, 4201245956, 2511182337, 2555745137,   13922641 }},
   },
   { FILE_PFX2,
     {{  313469993, 1621090873, 4249868193, 4017023367, 1553332517,  920968377,  459497127 },
      { 1638353602, 3003301525,  262241770, 1900319509, 2938064112, 3151158197, 2457366973 },
      {  125732388, 3356188817, 3758191728, 1364522439, 2122595240, 4193206814,  771186262 },
      { 1158510312,  489611070, 4077343918,  169859126,  323536092, 1607734957, 2599269797 },
      { 2536694293, 1846604070,  528506442,  539963535, 2442878685, 1625121910, 1507120069 },
      { 2061876226, 2937524483, 3888339165, 3758672483,  253960209, 2026819422, 3702572770 },
      {  399054270, 1515045668,  561525589, 3347871545,  245027576, 3700271664, 1209421927 }},
   },
#endif
#else
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
#endif
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
