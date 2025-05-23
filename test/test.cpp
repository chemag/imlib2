#include <gtest/gtest.h>

#include "config.h"
#include "test.h"

int             debug = 0;
static bool     ttyout = false;

int
main(int argc, char **argv)
{
    const char     *s;

    ttyout = isatty(STDOUT_FILENO);

    ::testing::InitGoogleTest(&argc, argv);

    for (argc--, argv++; argc > 0; argc--, argv++)
    {
        s = argv[0];
        if (*s++ != '-')
            break;
      again:
        switch (*s++)
        {
        case 'd':
            debug++;
            goto again;
        }
    }

    // Required by some tests
    mkdir(IMG_GEN, 0755);

    return RUN_ALL_TESTS();
}

#define COL_RST  "\x1B[0m"
#define COL_RED  "\x1B[31m"
#define COL_GRN  "\x1B[32m"
#define COL_YEL  "\x1B[33m"
#define COL_BLU  "\x1B[34m"
#define COL_MAG  "\x1B[35m"
#define COL_CYN  "\x1B[36m"
#define COL_WHT  "\x1B[37m"

#include <stdarg.h>
static void
_pr_text(const char *col, const char *fmt, va_list args)
{
    char            fmtx[1024];

    if (ttyout)
        snprintf(fmtx, sizeof(fmtx), "%s[          ] -  %s%s\n",
                 col, fmt, COL_RST);
    else
        snprintf(fmtx, sizeof(fmtx), "[          ] -  %s\n", fmt);
    fmt = fmtx;

    vprintf(fmt, args);
}

static void
pr_text(const char *col, const char *fmt, ...)
{
    va_list         args;

    va_start(args, fmt);
    _pr_text(col, fmt, args);
    va_end(args);
}

void
pr_info(const char *fmt, ...)
{
    va_list         args;

    va_start(args, fmt);
    _pr_text(COL_YEL, fmt, args);
    va_end(args);
}

#include <Imlib2.h>
#include <zlib.h>

unsigned int
image_get_crc32(Imlib_Image im)
{
    const unsigned char *data;
    unsigned int    crc, w, h;

    imlib_context_set_image(im);
    w = imlib_image_get_width();
    h = imlib_image_get_height();
    data = (const unsigned char *)imlib_image_get_data_for_reading_only();
    crc = crc32(0, data, w * h * sizeof(uint32_t));

    return crc;
}

void
flush_loaders(void)
{
    static signed char do_flush = -1;   // Normally do flush

    if (do_flush < 0)
        do_flush = getenv("IMLIB2_TEST_NO_LOADER_FLUSH") == NULL;

    if (do_flush)
        imlib_flush_loaders();
}

/**INDENT-OFF**/
extern "C" {
#include "strutils.h"
}
/**INDENT-ON**/

/* Skip crc checks on jxl images by default.
 * They seem to be quite sensitive to libjxl version and cpu type(?) */
#define IMLIB2_TEST_FILES_SKIP_DEFAULT "jxl"

bool
file_skip(const char *file)
{
    static char   **skiplist = (char **)(-1);
    const char     *s;

    if (skiplist == (char **)(-1))
    {
        s = getenv("IMLIB2_TEST_FILES_SKIP");
        if (!s)
            s = IMLIB2_TEST_FILES_SKIP_DEFAULT;
        skiplist = __imlib_StrSplit(s, ':');
    }

    if (!skiplist)
        return false;

    for (int i = 0; skiplist[i]; i++)
    {
        if (strstr(file, skiplist[i]))
        {
            pr_text(COL_MAG, "Skip '%s'", file);
            return true;
        }
    }

    return false;
}
