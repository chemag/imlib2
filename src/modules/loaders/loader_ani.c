/*
 * ANI loader
 *
 * File format:
 * https://en.wikipedia.org/wiki/Resource_Interchange_File_Format
 * https://www.daubnet.com/en/file-format-ani
 */
#include "config.h"
#include "Imlib2_Loader.h"

#if IMLIB2_DEBUG
#define Dx(fmt...) if (__imlib_debug & DBG_LDR) __imlib_printf(NULL, fmt)
#else
#define Dx(fmt...)
#endif

#define DBG_PFX "LDR-ani"

#define T(a,b,c,d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))

#define RIFF_TYPE_RIFF	T('R', 'I', 'F', 'F')
#define RIFF_TYPE_LIST	T('L', 'I', 'S', 'T')
#define RIFF_TYPE_INAM	T('I', 'N', 'A', 'M')
#define RIFF_TYPE_IART	T('I', 'A', 'R', 'T')
#define RIFF_TYPE_icon	T('i', 'c', 'o', 'n')

#define RIFF_NAME_ACON	T('A', 'C', 'O', 'N')

static const char  *const _formats[] = { "ani" };

typedef struct {
   unsigned char       nest;
} riff_ctx_t;

static int
_load_embedded(ImlibImage * im, int load_data, const char *data,
               unsigned int size)
{
   int                 rc;
   ImlibLoader        *loader;
   int                 frame;
   void               *lc;

   loader = __imlib_FindBestLoader(NULL, "ico", 0);
   if (!loader)
      return LOAD_FAIL;

   /* Disable frame and progress handling in sub-loader */
   frame = im->frame;
   lc = im->lc;
   im->frame = 0;
   im->lc = NULL;

   rc = __imlib_LoadEmbeddedMem(loader, im, load_data, data, size);

   im->frame = frame;
   im->lc = lc;

   if (rc == LOAD_SUCCESS && im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   return rc;
}

#define LE32(p) (SWAP_LE_32(*((const uint32_t*)(p))))
#define OFFS(p) ((const char*)(p) - (const char*)im->fi->fdata)

static int
_riff_parse(ImlibImage * im, riff_ctx_t * ctx, const char *fdata,
            unsigned int fsize, const char *fptr)
{
   int                 rc;
   unsigned int        type;
   int                 size, avail;

   rc = LOAD_FAIL;
   ctx->nest += 1;

   for (; rc == 0; fptr += 8 + size)
     {
        avail = fdata + fsize - fptr;   /* Bytes left in chunk */

        if (avail <= 0)
           break;               /* Normal end (= 0) */
        if (avail < 8)
          {
             D("%5lu: %*s Chunk: %.4s premature end\n",
               OFFS(fptr), ctx->nest, "", fptr);
             break;
          }

        type = LE32(fptr);
        size = LE32(fptr + 4);

        D("%5lu: %*s Chunk: %.4s size %u: ",
          OFFS(fptr), ctx->nest, "", fptr, size);

        if (ctx->nest == 1 && fptr == fdata)
          {
             Dx("\n");
             /* First chunk of file */
             if (type != RIFF_TYPE_RIFF || (LE32(fptr + 8)) != RIFF_NAME_ACON)
                return LOAD_FAIL;
             size = 4;
             continue;
          }

        if (avail < 8 + size)
          {
             Dx("incorrect size\n");
             rc = LOAD_BADFILE;
             break;
          }

        switch (type)
          {
          default:
          case RIFF_TYPE_RIFF:
             Dx("\n");
             break;
          case RIFF_TYPE_INAM:
          case RIFF_TYPE_IART:
             Dx("'%.*s'\n", size, fptr + 8);
             break;
          case RIFF_TYPE_LIST:
             Dx("'%.*s'\n", 4, fptr + 8);
             rc = _riff_parse(im, ctx, fptr + 12, size - 4, fptr + 12);
             break;
          case RIFF_TYPE_icon:
             Dx("\n");
             rc = _load_embedded(im, 1, fptr + 8, size);
             break;
          }
        size = (size + 1) & ~1;
     }

   ctx->nest -= 1;

   return rc;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   riff_ctx_t          ctx = { };

   rc = _riff_parse(im, &ctx, im->fi->fdata, im->fi->fsize, im->fi->fdata);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
