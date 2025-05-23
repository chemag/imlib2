#include <gtest/gtest.h>

#include "config.h"
#define IMLIB2_DEPRECATED       // Suppress deprecation warnings
#include <Imlib2.h>

#include <fcntl.h>
#include <sys/mman.h>

#include "test.h"

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

#define FILE_REF    FILE_PFX1 ".png"

static const char *const pfxs[] = {
    "argb",
#ifdef BUILD_AVIF_LOADER
    "avif",
#endif
    "bmp",
    "ff",
#ifdef BUILD_GIF_LOADER
    "gif",
#endif
#ifdef BUILD_HEIF_LOADER
    "heif",
#endif
    "ico",
#ifdef BUILD_JPEG_LOADER
    "jpg",
#endif
#ifdef BUILD_J2K_LOADER
    "jp2",
    "j2k",
#endif
#ifdef BUILD_JXL_LOADER
    "jxl",
#endif
    "ilbm",                     // lbm
#ifdef BUILD_PNG_LOADER
    "png",
#endif
    "ppm",                      // pnm
    "pgm",                      // pnm
    "pbm",                      // pnm
    "pam",                      // pnm
    "qoi",
    "tga",
#ifdef BUILD_SVG_LOADER
    "svg",
#endif
#ifdef BUILD_TIFF_LOADER
    "tiff",
#endif
#ifdef BUILD_WEBP_LOADER
    "webp",
#endif
    "xbm",
    "xpm",
#ifdef BUILD_Y4M_LOADER
    "y4m",
#endif

#ifdef BUILD_BZ2_LOADER
    "ff.bz2",                   // bz2
#endif
#ifdef BUILD_ZLIB_LOADER
    "ff.gz",                    // zlib
#endif
#ifdef BUILD_LZMA_LOADER
    "ff.xz",                    // lzma
#endif

#ifdef BUILD_ID3_LOADER
#ifdef BUILD_PNG_LOADER
    "png.mp3",                  // id3
#endif
#endif
};
#define N_PFX (sizeof(pfxs) / sizeof(char*))

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
    D2("%s: %3d%% %4d,%4d %4dx%4d\n",
       __func__, percent, update_x, update_y, update_w, update_h);

    return 1;                   /* Continue */
}

static void
image_free(Imlib_Image im)
{
    imlib_context_set_image(im);
    imlib_free_image_and_decache();
}

static void
test_load(void)
{
    char            filei[256];
    char            fileo[256];
    unsigned int    i, j;
    Imlib_Image     im;
    Imlib_Load_Error lerr;
    FILE           *fp;
    int             fd;
    int             err;
    uint32_t       *data;

    snprintf(filei, sizeof(filei), "%s/%s", IMG_SRC, FILE_REF);
    D("Load '%s'\n", filei);
    im = imlib_load_image(filei);

    ASSERT_TRUE(im);

    image_free(im);

    for (i = 0; i < N_PFX; i++)
    {
#if 0
        if (file_skip(pfxs[i]))
            continue;
#endif

        // Load files of all types
        snprintf(fileo, sizeof(fileo), "%s/%s.%s", IMG_SRC, FILE_PFX1, pfxs[i]);
        pr_info("Load '%s'", fileo);

        D("Load '%s' (deferred)\n", fileo);
        im = imlib_load_image(fileo);
        ASSERT_TRUE(im);
        if (im)
        {
            imlib_context_set_image(im);
            data = imlib_image_get_data();
            EXPECT_TRUE(data);
            image_free(im);
        }
        flush_loaders();

        D("Load '%s' (immediate)\n", fileo);
        im = imlib_load_image_with_errno_return(fileo, &err);
        EXPECT_TRUE(im);
        EXPECT_EQ(err, 0);
        if (!im || err)
            D("Error %d im=%p loading '%s'\n", err, im, fileo);
        if (im)
            image_free(im);
        flush_loaders();

        if (strchr(pfxs[i], '.') == 0)
        {
            snprintf(filei, sizeof(filei),
                     "../%s/%s.%s", IMG_SRC, FILE_PFX1, pfxs[i]);
            for (j = 0; j < N_PFX; j++)
            {
                // Load certain types pretending they are something else
                snprintf(fileo, sizeof(fileo), "%s/%s.%s.%s", IMG_GEN,
                         FILE_PFX1, pfxs[i], pfxs[j]);
                unlink(fileo);
                err = symlink(filei, fileo);
                ASSERT_EQ(err, 0);
                D("Load incorrect suffix '%s'\n", fileo);
                im = imlib_load_image_with_errno_return(fileo, &err);
                EXPECT_TRUE(im);
                EXPECT_EQ(err, 0);
                if (!im || err)
                    D("Error %d im=%p loading '%s'\n", err, im, fileo);
                if (im)
                    image_free(im);
            }
        }

        // Empty files of all types
        snprintf(fileo, sizeof(fileo), "%s/%s.%s", IMG_GEN, "empty", pfxs[i]);
        unlink(fileo);
        fp = fopen(fileo, "wb");
        fclose(fp);
        D("Load empty '%s'\n", fileo);

        im = imlib_load_image(fileo);
        err = imlib_get_error();
        EXPECT_FALSE(im);
        D("  err = %d\n", err);
        EXPECT_EQ(err, IMLIB_ERR_BAD_IMAGE);

        im = imlib_load_image_with_errno_return(fileo, &err);
        D("  err = %d\n", err);
        EXPECT_FALSE(im);
        EXPECT_EQ(err, IMLIB_ERR_BAD_IMAGE);

        im = imlib_load_image_with_error_return(fileo, &lerr);
        D("  err = %d\n", lerr);
        EXPECT_FALSE(im);
        EXPECT_EQ(lerr, IMLIB_LOAD_ERROR_IMAGE_READ);

        // Non-existing files of all types
        snprintf(fileo, sizeof(fileo), "%s/%s.%s", IMG_GEN, "nonex", pfxs[i]);
        unlink(fileo);
        err = symlink("non-existing", fileo);
        ASSERT_EQ(err, 0);
        D("Load non-existing '%s'\n", fileo);

        im = imlib_load_image(fileo);
        EXPECT_FALSE(im);
        err = imlib_get_error();
        D("  err = %d\n", err);
        EXPECT_EQ(err, ENOENT);

        im = imlib_load_image_with_errno_return(fileo, &err);
        D("  err = %d\n", err);
        EXPECT_FALSE(im);
        EXPECT_EQ(err, ENOENT);

        im = imlib_load_image_with_error_return(fileo, &lerr);
        D("  err = %d\n", lerr);
        EXPECT_FALSE(im);
        EXPECT_EQ(lerr, IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST);

        // Load via fd
        snprintf(fileo, sizeof(fileo), "%s/%s.%s", IMG_SRC, FILE_PFX1, pfxs[i]);
        fd = open(fileo, O_RDONLY);
        D("Load fd %d '%s'\n", fd, fileo);
        snprintf(fileo, sizeof(fileo), ".%s", pfxs[i]);
        im = imlib_load_image_fd(fd, pfxs[i]);
        err = imlib_get_error();
        EXPECT_TRUE(im);
        EXPECT_EQ(err, 0);
        if (im)
            image_free(im);
        err = close(fd);
        EXPECT_NE(err, 0);

        if (strstr(pfxs[i], ".mp3"))    // id3 cannot do mem
            continue;

        // Load via mem
        snprintf(fileo, sizeof(fileo), "%s/%s.%s", IMG_SRC, FILE_PFX1, pfxs[i]);
        fd = open(fileo, O_RDONLY);
        void           *fdata;
        struct stat     st;

        err = stat(fileo, &st);
        ASSERT_EQ(err, 0);
        fdata = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        ASSERT_TRUE(fdata != NULL);
        ASSERT_TRUE(fdata != MAP_FAILED);
        for (int n = 0; n < 3; n++)
        {
            D("Load mem[%d] %d '%s'\n", n, fd, fileo);
            snprintf(fileo, sizeof(fileo), ".%s", pfxs[i]);
            im = imlib_load_image_mem(pfxs[i], fdata, st.st_size);
            err = imlib_get_error();
            EXPECT_TRUE(im);
            EXPECT_EQ(err, 0);
            if (im)
                image_free(im);
        }
        munmap(fdata, st.st_size);
        err = close(fd);
        EXPECT_EQ(err, 0);
    }
}

TEST(LOAD, load_1)
{
    test_load();
}

TEST(LOAD, load_2)
{
    imlib_context_set_progress_function(progress);
    imlib_context_set_progress_granularity(10);
    test_load();
}
