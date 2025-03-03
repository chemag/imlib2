#include <gtest/gtest.h>

#include "config.h"
#include <fcntl.h>
#include <Imlib2.h>
#ifdef BUILD_HEIF_LOADER
#include <libheif/heif.h>
#endif
#ifdef BUILD_JXL_LOADER
#include <jxl/version.h>
#endif

#include "test.h"

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

typedef struct {
    const char     *ext;
    unsigned int    crc[2];
} test_rec_t;

/**INDENT-OFF**/
static const test_rec_t exts[] = {
// { "ani",  { 0, 0 } },
   { "argb", { 1636116234,  169859126 } },
   { "bmp",  { 1636116234,  247945219 } },
// { "bz2",  { 0, 0 } },
   { "ff",   { 1636116234,  169859126 } },
// { "gif",  { 0, 0 } },
#ifdef BUILD_HEIF_LOADER
#ifndef __SANITIZE_ADDRESS__
#if LIBHEIF_HAVE_VERSION(1, 19, 0)
   { "avif", {  271316575,  632516970 } },
   { "heic", { 2429908023, 1720725349 } },
#else
   { "avif", {  458570512, 4237984766 } },
   { "heic", { 4046271191,  793249753 } },
#endif
#endif
#endif
// { "ico",  { 0, 0 } },
// { "id3",  { 0, 0 } },
// { "j2k",  { 0, 0 } },
#ifdef BUILD_JPEG_LOADER
   { "jpeg", {  631237512, 1120421740 } },
#endif
#ifdef BUILD_JXL_LOADER
#if JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0, 10, 0)
#ifdef __i386__
   { "jxl",  { 1626654164, 2937200044 } },
#else
   { "jxl",  { 2415497566, 2734910059 } },
#endif
#elif JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0, 8, 0)
#ifdef __i386__
   { "jxl",  { 4232124886, 2621968888 } },
#else
   { "jxl",  { 4232124886, 1010978750 } },
#endif
#else
#ifdef __i386__
   { "jxl",  { 2681286418, 3923017710 } },
#else
   { "jxl",  { 2681286418, 2516911844 } },
#endif
#endif
#endif // BUILD_JXL_LOADER
// { "lbm",  { 0, 0 } },
// { "lzma", { 0, 0 } },
   { "png",  { 1636116234,  169859126 } },
   { "pnm",  { 1636116234,  169859126 } },
// { "ps",   { 0, 0 } },
// { "svg",  { 0, 0 } },
   { "tga",  { 1636116234,  169859126 } },
   { "tiff", { 1636116234, 2443461544 } },
#ifdef BUILD_WEBP_LOADER
   { "webp", {  902703844, 3830549848 } },
#endif
   { "xbm",  { 2432994606, 2367802683 } },
// { "xpm",  { 0, 0 } },
// { "zlib", { 0, 0 } },
};
#define N_PFX (sizeof(exts) / sizeof(exts[0]))
/**INDENT-ON**/

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
    D("%s: %3d%% %4d,%4d %4dx%4d\n",
      __func__, percent, update_x, update_y, update_w, update_h);

    return 1;                   /* Continue */
}

static void
test_save_1(const char *file, int prog, int check = 0)
{
    char            filei[256];
    char            fileo[256];
    unsigned int    i, crc;
    const char     *ext;
    int             w, h, err;
    Imlib_Image     im, im1, im2, im3, imr;
    Imlib_Load_Error lerr;

    if (prog)
    {
        imlib_context_set_progress_function(progress);
        imlib_context_set_progress_granularity(10);
    }

    snprintf(filei, sizeof(filei), "%s/%s", IMG_SRC, file);
    D("Load '%s'\n", filei);
    im = imlib_load_image(filei);
    ASSERT_TRUE(im);

    imlib_context_set_image(im);
    w = imlib_image_get_width();
    h = imlib_image_get_height();
    im1 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 1, h + 1);
    im2 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 2, h + 2);
    im3 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 3, h + 3);

    ASSERT_TRUE(im1);
    ASSERT_TRUE(im2);
    ASSERT_TRUE(im3);

    for (i = 0; i < N_PFX; i++)
    {
        ext = exts[i].ext;

        if (file_skip(ext))
            continue;

        imlib_context_set_image(im);
        imlib_image_set_format(ext);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, ext);
        pr_info("Save '%s'", fileo);

        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
        {
            D("Error %d saving '%s'\n", err, fileo);
            continue;
        }

        if (check)
        {
            /* Check saved image */
            imr = imlib_load_image(fileo);
            ASSERT_TRUE(imr);
            crc = image_get_crc32(imr);
            EXPECT_EQ(crc, exts[i].crc[check - 1]);
            imlib_context_set_image(imr);
            imlib_free_image_and_decache();
        }

        imlib_context_set_image(im1);
        imlib_image_set_format(ext);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, ext);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
            D("Error %d saving '%s'\n", lerr, fileo);

        imlib_context_set_image(im2);
        imlib_image_set_format(ext);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, ext);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
            D("Error %d saving '%s'\n", err, fileo);

        imlib_context_set_image(im3);
        imlib_image_set_format(ext);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, ext);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
            D("Error %d saving '%s'\n", lerr, fileo);
    }

    imlib_context_set_image(im);
    imlib_free_image_and_decache();
    imlib_context_set_image(im1);
    imlib_free_image_and_decache();
    imlib_context_set_image(im2);
    imlib_free_image_and_decache();
    imlib_context_set_image(im3);
    imlib_free_image_and_decache();

    imlib_context_set_progress_function(NULL);
}

TEST(SAVE, save_1n_rgb)
{
    test_save_1("image-noalp-64.png", 0, 1);
}

TEST(SAVE, save_1p_rgb)
{
    test_save_1("image-noalp-64.png", 1);
}

TEST(SAVE, save_1n_argb)
{
    test_save_1("image-alpha-64.png", 0, 2);
}

TEST(SAVE, save_1p_argb)
{
    test_save_1("image-alpha-64.png", 1);
}

static void
test_save_2(const char *file, const char *fmt, bool load_imm, bool sok,
            unsigned int crc_exp = 0)
{
    char            filei[256];
    char            fileo[256];
    int             err, fd;
    Imlib_Image     im;
    unsigned int    crc;

    flush_loaders();

    snprintf(filei, sizeof(filei), "%s/%s", IMG_SRC, file);

    D("Load '%s'\n", filei);
    err = 0;
    if (load_imm)
        im = imlib_load_image_with_errno_return(filei, &err);
    else
        im = imlib_load_image(filei);
    ASSERT_TRUE(im);
    ASSERT_EQ(err, 0);

    imlib_context_set_image(im);

    snprintf(fileo, sizeof(fileo), "%s/save-%s.%s", IMG_GEN, file, fmt);

    D("Save '%s'\n", fileo);
    imlib_image_set_format(fmt);
    imlib_save_image_with_errno_return(fileo, &err);
    if (sok)
    {
        EXPECT_EQ(err, 0);
        if (err)
            D("Error %d saving '%s'\n", err, fileo);
    }
    else
    {
        EXPECT_EQ(err, IMLIB_ERR_NO_SAVER);
        if (err != IMLIB_ERR_NO_SAVER)
            D("Error %d saving '%s'\n", err, fileo);
    }

    imlib_free_image_and_decache();

    if (!sok)
        return;

    D("Check '%s' ... ", fileo);
    im = imlib_load_image(fileo);
    ASSERT_TRUE(im);
    crc = image_get_crc32(im);
    EXPECT_EQ(crc, crc_exp);
    D("ok\n");
    unlink(fileo);

    D("Save to fd '%s'\n", fileo);
    imlib_image_set_format(fmt);
    fd = open(fileo, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    imlib_save_image_fd(fd, NULL);
    err = imlib_get_error();
    if (sok)
    {
        EXPECT_EQ(err, 0);
        if (err)
            D("Error %d saving '%s'\n", err, fileo);
    }
    else
    {
        EXPECT_EQ(err, IMLIB_ERR_NO_SAVER);
        if (err != IMLIB_ERR_NO_SAVER)
            D("Error %d saving '%s'\n", err, fileo);
    }
    err = close(fd);
    EXPECT_NE(err, 0);

    imlib_free_image_and_decache();

    if (!sok)
        return;

    D("Check '%s' ... ", fileo);
    im = imlib_load_image(fileo);
    ASSERT_TRUE(im);
    crc = image_get_crc32(im);
    EXPECT_EQ(crc, crc_exp);
    D("ok\n");
}

// png and ppm(pnm) have savers
TEST(SAVE, save_2a_immed)
{
    bool            immed = true;

    test_save_2("image-noalp-64.png", "png", immed, true, 1636116234);
    test_save_2("image-noalp-64.png", "ppm", immed, true, 1636116234);
    test_save_2("image-noalp-64.ppm", "ppm", immed, true, 1636116234);
    test_save_2("image-noalp-64.ppm", "png", immed, true, 1636116234);
}

TEST(SAVE, save_2a_defer)
{
    bool            immed = false;

    test_save_2("image-noalp-64.png", "png", immed, true, 1636116234);
    test_save_2("image-noalp-64.png", "ppm", immed, true, 1636116234);
    test_save_2("image-noalp-64.ppm", "ppm", immed, true, 1636116234);
    test_save_2("image-noalp-64.ppm", "png", immed, true, 1636116234);
}

TEST(SAVE, save_2b_immed)
{
    bool            immed = true;

    test_save_2("image-noalp-64.gif", "svg", immed, false);
    test_save_2("image-noalp-64.gif", "png", immed, true, 3493023514);
}

TEST(SAVE, save_2b_defer)
{
    bool            immed = false;

    test_save_2("image-noalp-64.gif", "svg", immed, false);
    test_save_2("image-noalp-64.gif", "png", immed, true, 3493023514);
}

static void
test_save_3(const char *file, const char *dest, int err_new, int err_old)
{
    char            filei[256];
    char            fileo[256];
    unsigned int    i;
    const char     *ext;
    int             err;
    Imlib_Image     im;
    Imlib_Load_Error lerr;

    snprintf(filei, sizeof(filei), "%s/%s", IMG_SRC, file);
    D("Load '%s'\n", filei);
    im = imlib_load_image(filei);
    ASSERT_TRUE(im);

    imlib_context_set_image(im);

    for (i = 0; i < N_PFX; i++)
    {
        ext = exts[i].ext;

        if (file_skip(ext))
            continue;

        imlib_context_set_image(im);
        imlib_image_set_format(ext);
        snprintf(fileo, sizeof(fileo), "%s", dest);
        pr_info("Save %s to '%s'", ext, fileo);

        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, err_new);

        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, err_old);

        imlib_save_image(fileo);
        err = imlib_get_error();
        ASSERT_EQ(err, err_new);
    }

    imlib_context_set_image(im);
    imlib_free_image_and_decache();
}

TEST(SAVE, save_3_full)
{
    test_save_3("image-noalp-64.png", "/dev/full",
                ENOSPC, IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE);
}

#if 0
TEST(SAVE, save_3_dir)
{
    test_save_3("image-noalp-64.png", "/tmp",
                EISDIR, IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY);
}

TEST(SAVE, save_3_rdonly)
{
    test_save_3("image-noalp-64.png", IMG_GEN "/rdonly",
                EACCES, IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE);
}
#endif
