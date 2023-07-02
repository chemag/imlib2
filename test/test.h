#ifndef TEST_H
#define TEST_H 1

#define IMG_SRC     SRC_DIR "/images"
#define IMG_GEN     BLD_DIR "/generated"

#define D(...)  do{ if (debug)     printf(__VA_ARGS__); }while(0)
#define D2(...) do{ if (debug > 1) printf(__VA_ARGS__); }while(0)

#include <Imlib2.h>

extern int      debug;

void            pr_info(const char *fmt, ...);

unsigned int    image_get_crc32(Imlib_Image im);

void            flush_loaders(void);

bool            file_skip(const char *file);

#endif                          /* TEST_H */
