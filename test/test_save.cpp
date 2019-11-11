#include <gtest/gtest.h>

#include <Imlib2.h>

#if 0
#define D(...) printf(__VA_ARGS__)
#else
#define D(...) do { } while (0)
#endif

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

#define TOPDIR  	SRC_DIR
#define FILE_REF	"test/images/icon-64.bmp"

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
test_save(void)
{
   char                filei[256];
   char                fileo[256];
   unsigned int        i;
   int                 w, h;
   Imlib_Image         im, im1, im2, im3;
   Imlib_Load_Error    lerr;

   snprintf(filei, sizeof(filei), "%s/%s", TOPDIR, FILE_REF);
   D("Load '%s'\n", filei);
   im = imlib_load_image(filei);
   if (!im)
     {
        printf("Error loading '%s'\n", filei);
        exit(0);
     }

   imlib_context_set_image(im);
   w = imlib_image_get_width();
   h = imlib_image_get_height();
   im1 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 1, h + 1);
   im2 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 2, h + 2);
   im3 = imlib_create_cropped_scaled_image(0, 0, w, h, w + 3, h + 3);

   ASSERT_TRUE(im);
   ASSERT_TRUE(im1);
   ASSERT_TRUE(im2);
   ASSERT_TRUE(im3);

   for (i = 0; i < N_PFX; i++)
     {
        imlib_context_set_image(im);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/%s-%dx%d.%s",
                 ".", "img_save", w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
           D("Error %d saving '%s'\n", lerr, fileo);

        imlib_context_set_image(im1);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/%s-%dx%d.%s",
                 ".", "img_save", w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
           D("Error %d saving '%s'\n", lerr, fileo);

        imlib_context_set_image(im2);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/%s-%dx%d.%s",
                 ".", "img_save", w, h, pfxs[i]);
        D("Save '%s'\n", fileo);
        imlib_save_image_with_error_return(fileo, &lerr);
        EXPECT_EQ(lerr, 0);
        if (lerr)
           D("Error %d saving '%s'\n", lerr, fileo);

        imlib_context_set_image(im3);
        imlib_image_set_format(pfxs[i]);
        w = imlib_image_get_width();
        h = imlib_image_get_height();
        snprintf(fileo, sizeof(fileo), "%s/%s-%dx%d.%s",
                 ".", "img_save", w, h, pfxs[i]);
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
}

TEST(SAVE, save_1)
{
   test_save();
}

TEST(SAVE, save_2)
{
   imlib_context_set_progress_function(progress);
   imlib_context_set_progress_granularity(10);
   test_save();
}

int
main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
