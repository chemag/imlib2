#ifndef TEST_H
#define TEST_H 1

#define IMG_SRC		SRC_DIR "/images"
#define IMG_GEN		BLD_DIR "/generated"

#define D(...)  if (debug) printf(__VA_ARGS__)
#define D2(...) if (debug > 1) printf(__VA_ARGS__)

extern int          debug;

#endif /* TEST_H */
