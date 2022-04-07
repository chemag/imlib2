#include <gtest/gtest.h>

#include "config.h"
#define IMLIB2_DEPRECATED       // Suppress deprecation warnings
#include <Imlib2.h>

#include "test.h"

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

#define FILE_REF1	"icon-64"       // RGB
#define FILE_REF2	"xeyes" // ARGB (shaped)

static const char  *const pfxs[] = {
   "argb",
   "bmp",
   "ff",
// "gif",
// "ico",
   "jpeg",
// "lbm",
   "png",
   "pnm",
   "tga",
   "tiff",
   "webp",
   "xbm",
// "xpm",
};
#define N_PFX (sizeof(pfxs) / sizeof(char*))

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
   D("%s: %3d%% %4d,%4d %4dx%4d\n",
     __func__, percent, update_x, update_y, update_w, update_h);

   return 1;                    /* Continue */
}

static void
test_save_1(const char *file, const char *ext, int prog)
{
   char                filei[256];
   char                fileo[256];
   unsigned int        i;
   int                 w, h, err;
   Imlib_Image         im, im1, im2, im3;
   Imlib_Load_Error    lerr;

   if (prog)
     {
        imlib_context_set_progress_function(progress);
        imlib_context_set_progress_granularity(10);
     }

   snprintf(filei, sizeof(filei), "%s/%s.%s", IMG_SRC, file, ext);
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
        imlib_context_set_image(im);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
           D("Error %d saving '%s'\n", err, fileo);

        imlib_context_set_image(im1);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
           D("Error %d saving '%s'\n", lerr, fileo);

        imlib_context_set_image(im2);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_errno_return(fileo, &err);
        EXPECT_EQ(err, 0);
        if (err)
           D("Error %d saving '%s'\n", err, fileo);

        imlib_context_set_image(im3);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/save-%s-%dx%d.%s",
                 IMG_GEN, file, w, h, pfxs[i]);
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
   test_save_1(FILE_REF1, "png", 0);
}

TEST(SAVE, save_1p_rgb)
{
   test_save_1(FILE_REF1, "png", 1);
}

TEST(SAVE, save_1n_argb)
{
   test_save_1(FILE_REF2, "png", 0);
}

TEST(SAVE, save_1p_argb)
{
   test_save_1(FILE_REF2, "png", 1);
}

static void
test_save_2(const char *file, const char *ext, const char *fmt,
            bool load_imm, bool sok)
{
   char                filei[256];
   char                fileo[256];
   int                 err;
   Imlib_Image         im;

   imlib_flush_loaders();

   snprintf(filei, sizeof(filei), "%s/%s.%s", IMG_SRC, file, ext);

   D("Load '%s'\n", filei);
   err = 0;
   if (load_imm)
      im = imlib_load_image_with_errno_return(filei, &err);
   else
      im = imlib_load_image(filei);
   ASSERT_TRUE(im);
   ASSERT_EQ(err, 0);

   imlib_context_set_image(im);

   snprintf(fileo, sizeof(fileo), "%s/save-%s-%s.%s", IMG_GEN, file, ext, fmt);

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
}

// png and ppm(pnm) have savers
TEST(SAVE, save_2a_immed)
{
   bool                immed = true;

   test_save_2(FILE_REF1, "png", "png", immed, true);
   test_save_2(FILE_REF1, "png", "ppm", immed, true);
   test_save_2(FILE_REF1, "ppm", "ppm", immed, true);
   test_save_2(FILE_REF1, "ppm", "png", immed, true);
}

TEST(SAVE, save_2a_defer)
{
   bool                immed = false;

   test_save_2(FILE_REF1, "png", "png", immed, true);
   test_save_2(FILE_REF1, "png", "ppm", immed, true);
   test_save_2(FILE_REF1, "ppm", "ppm", immed, true);
   test_save_2(FILE_REF1, "ppm", "png", immed, true);
}

// No gif saver
TEST(SAVE, save_2b_immed)
{
   bool                immed = true;

   test_save_2(FILE_REF1, "gif", "svg", immed, false);
   test_save_2(FILE_REF1, "gif", "png", immed, true);
}

TEST(SAVE, save_2b_defer)
{
   bool                immed = false;

   test_save_2(FILE_REF1, "gif", "svg", immed, false);
   test_save_2(FILE_REF1, "gif", "png", immed, true);
}
