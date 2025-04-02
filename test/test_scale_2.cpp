#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

typedef struct {
    int             sw, sh, ml, mr, mt, mb;
    unsigned int    crc;
} tv_t;

typedef struct {
    const char     *file;
    const tv_t     *tv;         // Test values
} td_t;

#define FILE_NOALP FILE_PFX1 ".png"
//#define FILE_NOALP "../../duo_light.jpg"

/**INDENT-OFF**/
static const tv_t tv1[] = {
//    sw    sh    ml    mr    mt    mb         crc
  { 1000, 1000,    0,    0,    0,    0, 1636116234 },

  {  700,  700,   10,   10,   13,   13, 3382478357 },
  { 1300, 1300,  -11,  -11,   12,   12,  652769865 },
  {  700, 1300,   12,   12,  -11,  -11, 1748211482 },
  { 1300,  700,  -13,  -13,  -10,  -10,  840350556 },

  { 1200,  800,   10,    9,    8,    7, 3331650595 },
  { 1200,  800,   10,    9,    8,   -7,  925666147 },
  { 1200,  800,   10,    9,   -8,    7, 2572401557 },
  { 1200,  800,   10,    9,   -8,   -7, 2544040289 },

  { 1200,  800,   10,   -9,    8,    7, 2676605133 },
  { 1200,  800,   10,   -9,    8,   -7,  976709471 },
  { 1200,  800,   10,   -9,   -8,    7, 4041364061 },
  { 1200,  800,   10,   -9,   -8,   -7,  722787591 },

  { 1200,  800,  -10,    9,    8,    7, 2577148194 },
  { 1200,  800,  -10,    9,    8,   -7, 2018936290 },
  { 1200,  800,  -10,    9,   -8,    7, 1972929354 },
  { 1200,  800,  -10,    9,   -8,   -7, 1989666416 },

  { 1200,  800,  -10,   -9,    8,    7, 4020329861 },
  { 1200,  800,  -10,   -9,    8,   -7,  929087062 },
  { 1200,  800,  -10,   -9,   -8,    7, 3718636657 },
  { 1200,  800,  -10,   -9,   -8,   -7, 3098957003 },
  {  }
};
/**INDENT-ON**/

static const td_t td[] = {
    { FILE_NOALP, tv1 },
};

static void
_test_scale2(Imlib_Image im, const char *image,
             int sw, int sh, int ml, int mr, int mt, int mb,
             unsigned int crc_exp)
{
    static int      seqn = 0;
    char            buf[64];
    int             w, h, wo, ho, xs, ys, ws, hs;
    unsigned int    crc;
    Imlib_Image     im2;

    imlib_context_set_image(im);
    w = imlib_image_get_width();
    h = imlib_image_get_height();
    wo = (w * sw) / 1000;
    ho = (h * sh) / 1000;

    xs = ml;
    ys = mt;
    ws = wo - (ml + mr);
    hs = ho - (mt + mb);

    pr_info("%s: %s: s=%4d,%4d, m=%d,%d,%d,%d: %dx%d -> %dx%d",
            __func__, image, sw, sh, ml, mr, mt, mb, w, h, ws, hs);

    im2 = imlib_create_image(wo, ho);
    ASSERT_TRUE(im2);
    imlib_context_set_image(im2);

    imlib_context_set_color(0x00, 0x00, 0xff, 0xff);
    imlib_context_set_blend(0);
    imlib_image_set_has_alpha(1);
    imlib_image_fill_rectangle(0, 0, wo, ho);

    imlib_context_set_blend(1);
    imlib_blend_image_onto_image(im, 0, 0, 0, w, h, xs, ys, ws, hs);

    snprintf(buf, sizeof(buf), "%s/scale2-%02d-%dx%d.%s",
             IMG_GEN, ++seqn, wo, ho, "png");
    imlib_save_image(buf);

    crc = image_get_crc32(im2);
    EXPECT_EQ(crc, crc_exp);

    imlib_context_set_image(im2);
    imlib_free_image_and_decache();
}

TEST(SCALE2, scale2_a)
{
    char            buf[128];
    Imlib_Image     im;
    const td_t     *ptd = &td[0];
    const tv_t     *ptv;
    int             i;

    snprintf(buf, sizeof(buf), "%s/%s", IMG_SRC, ptd->file);
    im = imlib_load_image(buf);
    ASSERT_TRUE(im);

    for (i = 0;; i++)
    {
        ptv = &ptd->tv[i];
        if (ptv->sw == 0)
            break;
        _test_scale2(im, ptd->file, ptv->sw, ptv->sh,
                     ptv->ml, ptv->mr, ptv->mt, ptv->mb, ptv->crc);
    }

    imlib_context_set_image(im);
    imlib_free_image_and_decache();
}
