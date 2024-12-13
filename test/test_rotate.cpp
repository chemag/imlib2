#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

typedef struct {
    double          ang;
    unsigned int    crc[4];
} tv_t;

typedef struct {
    const char     *file;
    const tv_t     *tv;         // Test values
} td_t;

/**INDENT-OFF**/
static const tv_t tv1[] = {
//                  No MMX                   MMX
//              -aa         +aa         -aa         +aa
  {   0., { 1365529130, 1916136761, 1365529130,  222684661 }},
  {  45., { 1614407402,  640217638, 1614407402, 3104606551 }},
  { -45., { 1942980583,  835187300, 1942980583, 3173650180 }},
};
static const tv_t tv2[] = {
//                  No MMX                   MMX
//              -aa         +aa         -aa         +aa
  {   0., { 3144863184, 2786274943, 3144863184,  388522726 }},
  {  45., {   55353012, 2307392419,   55353012, 2115485479 }},
  { -45., { 1693950105, 4078578263, 1693950105, 2826635782 }},
};
#define N_VAL (sizeof(tv1) / sizeof(tv_t))

static const td_t   td[] = {
   { FILE_PFX1, tv1 },
   { FILE_PFX2, tv2 },
};
/**INDENT-ON**/

static void
test_rotate(int no, int aa)
{
    const td_t     *ptd;
    char            filei[256];
    char            fileo[256];

// int                 wi, hi;
    int             wo, ho;
    unsigned int    i, ic, crc;
    Imlib_Image     imi, imo;
    int             err;

    ptd = &td[no];

    ic = aa;                    // CRC index
#ifdef DO_MMX_ASM
    // Hmm.. MMX functions appear to produce a slightly different result
    if (!getenv("IMLIB2_ASM_OFF"))
        ic += 2;
#endif

    snprintf(filei, sizeof(filei), "%s/%s.png", IMG_SRC, ptd->file);
    D("Load '%s'\n", filei);
    imi = imlib_load_image(filei);
    ASSERT_TRUE(imi);

    imlib_context_set_anti_alias(aa);

    for (i = 0; i < N_VAL; i++)
    {
        imlib_context_set_image(imi);
        //wi = imlib_image_get_width();
        //hi = imlib_image_get_height();

        D("Rotate %f\n", ptd->tv[i].ang);
        imo = imlib_create_rotated_image(ptd->tv[i].ang);
        ASSERT_TRUE(imo);

        imlib_context_set_image(imo);

        wo = imlib_image_get_width();
        ho = imlib_image_get_height();

        snprintf(fileo, sizeof(fileo), "%s/rotate-%s-%dx%d-%d.%s",
                 IMG_GEN, ptd->file, wo, ho, i, "png");
        imlib_image_set_format("png");
        D("Save '%s'\n", fileo);
        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
            D("Error %d saving '%s'\n", err, fileo);

        crc = image_get_crc32(imo);
        EXPECT_EQ(crc, ptd->tv[i].crc[ic]);

        imlib_context_set_image(imo);
        imlib_free_image_and_decache();
    }

    imlib_context_set_image(imi);
    imlib_free_image_and_decache();
}

TEST(ROTAT, rotate_1_aa)
{
    test_rotate(0, 1);
}

TEST(ROTAT, rotate_1_noaa)
{
    test_rotate(0, 0);
}

TEST(ROTAT, rotate_2_aa)
{
    test_rotate(1, 1);
}

TEST(ROTAT, rotate_2_noaa)
{
    test_rotate(1, 0);
}
