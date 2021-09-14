#include "loader_common.h"
#include <sys/stat.h>
#include <webp/decode.h>
#include <webp/encode.h>

static const char  *
webp_strerror(VP8StatusCode code)
{
   switch (code)
     {
     case VP8_STATUS_OK:
        return "No Error";
     case VP8_STATUS_OUT_OF_MEMORY:
        return "Out of memory";
     case VP8_STATUS_INVALID_PARAM:
        return "Invalid API parameter";
     case VP8_STATUS_BITSTREAM_ERROR:
        return "Bitstream Error";
     case VP8_STATUS_UNSUPPORTED_FEATURE:
        return "Unsupported Feature";
     case VP8_STATUS_SUSPENDED:
        return "Suspended";
     case VP8_STATUS_USER_ABORT:
        return "User abort";
     case VP8_STATUS_NOT_ENOUGH_DATA:
        return "Not enough data/truncated file";
     default:
        return "Unknown error";
     }
}

int
load2(ImlibImage * im, int load_data)
{
   int                 rc, fd;
   struct stat         st;
   uint8_t            *fdata;
   WebPBitstreamFeatures features;
   VP8StatusCode       vp8return;
   unsigned int        size;

   rc = LOAD_FAIL;
   fd = fileno(im->fp);

   if (fstat(fd, &st) < 0)
      return rc;

   fdata = malloc(st.st_size);
   if (!fdata)
      goto quit;

   /* Check signature */
   size = 12;
   if (read(fd, fdata, size) != (long)size)
      goto quit;
   if (memcmp(fdata + 0, "RIFF", 4) != 0 || memcmp(fdata + 8, "WEBP", 4) != 0)
      goto quit;

   size = st.st_size;
   if ((long)size != st.st_size)
      goto quit;

   size -= 12;
   if (read(fd, fdata + 12, size) != (long)size)
      goto quit;

   if (WebPGetInfo(fdata, st.st_size, &im->w, &im->h) == 0)
      goto quit;

   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   vp8return = WebPGetFeatures(fdata, st.st_size, &features);
   if (vp8return != VP8_STATUS_OK)
     {
        fprintf(stderr, "%s: Error reading file header: %s\n",
                im->real_file, webp_strerror(vp8return));
        goto quit;
     }

   if (features.has_alpha == 0)
      UNSET_FLAG(im->flags, F_HAS_ALPHA);
   else
      SET_FLAG(im->flags, F_HAS_ALPHA);

   if (!load_data)
     {
        rc = LOAD_SUCCESS;
        goto quit;
     }

   /* Load data */

   if (!__imlib_AllocateData(im))
      goto quit;

   if (WebPDecodeBGRAInto(fdata, st.st_size, (uint8_t *) im->data,
                          sizeof(DATA32) * im->w * im->h, im->w * 4) == NULL)
      goto quit;

   if (im->lc)
      __imlib_LoadProgressRows(im, 0, im->h);

   rc = LOAD_SUCCESS;

 quit:
   if (rc <= 0)
      __imlib_FreeData(im);
   free(fdata);

   return rc;
}

char
save(ImlibImage * im, ImlibProgressFunction progress, char progress_granularity)
{
   FILE               *f;
   int                 rc;
   ImlibImageTag      *quality_tag;
   float               quality;
   uint8_t            *fdata;
   size_t              encoded_size;

   f = fopen(im->real_file, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_FAIL;
   fdata = NULL;

   quality = 75;
   quality_tag = __imlib_GetTag(im, "quality");
   if (quality_tag)
     {
        quality = quality_tag->val;
        if (quality < 0)
          {
             fprintf(stderr,
                     "Warning: 'quality' setting %f too low for WebP, using 0\n",
                     quality);
             quality = 0;
          }

        if (quality > 100)
          {
             fprintf(stderr,
                     "Warning, 'quality' setting %f too high for WebP, using 100\n",
                     quality);
             quality = 100;
          }
     }

   encoded_size = WebPEncodeBGRA((uint8_t *) im->data, im->w, im->h,
                                 im->w * 4, quality, &fdata);

   if (fwrite(fdata, encoded_size, 1, f) != encoded_size)
      goto quit;

   rc = LOAD_SUCCESS;

 quit:
   if (fdata)
      WebPFree(fdata);
   fclose(f);

   return rc;
}

void
formats(ImlibLoader * l)
{
   static const char  *const list_formats[] = { "webp" };
   __imlib_LoaderSetFormats(l, list_formats,
                            sizeof(list_formats) / sizeof(char *));
}
