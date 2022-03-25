#include "loader_common.h"

#define MAX_RUNNERS 4           /* Maybe set to Ncpu/2? */

#include <jxl/decode.h>
#if MAX_RUNNERS > 0
#include <jxl/thread_parallel_runner.h>
#endif

#define DBG_PFX "LDR-jxl"

static void
_scanline_cb(void *opaque, size_t x, size_t y,
             size_t num_pixels, const void *pixels)
{
   ImlibImage         *im = opaque;
   const uint8_t      *pix = pixels;
   uint32_t           *ptr;
   size_t              i;

   DL("%s: x,y=%ld,%ld len=%lu\n", __func__, x, y, num_pixels);

   ptr = im->data + (im->w * y) + x;

   /* libjxl outputs ABGR pixels (stream order RGBA) - convert to ARGB */
   for (i = 0; i < num_pixels; i++, pix += 4)
      *ptr++ = PIXEL_ARGB(pix[3], pix[0], pix[1], pix[2]);

   /* Due to possible multithreading it's probably best not do do
    * progress calbacks here. */
}

int
load2(ImlibImage * im, int load_data)
{
   static const JxlPixelFormat pbuf_fmt = {
      .num_channels = 4,
      .data_type = JXL_TYPE_UINT8,
      .endianness = JXL_NATIVE_ENDIAN,
   };

   int                 rc;
   void               *fdata;
   JxlDecoderStatus    jst;
   JxlDecoder         *dec;
   JxlBasicInfo        info;

#if MAX_RUNNERS > 0
   size_t              n_runners;
   JxlParallelRunner  *runner = NULL;
#endif

   fdata = mmap(NULL, im->fsize, PROT_READ, MAP_SHARED, fileno(im->fp), 0);
   if (fdata == MAP_FAILED)
      return LOAD_BADFILE;

   rc = LOAD_FAIL;
   dec = NULL;

   switch (JxlSignatureCheck(fdata, 128))
     {
     default:
//   case JXL_SIG_NOT_ENOUGH_BYTES:
//   case JXL_SIG_INVALID:
        goto quit;
     case JXL_SIG_CODESTREAM:
     case JXL_SIG_CONTAINER:
        break;
     }

   rc = LOAD_BADIMAGE;          /* Format accepted */

   dec = JxlDecoderCreate(NULL);
   if (!dec)
      goto quit;

#if MAX_RUNNERS > 0
   n_runners = JxlThreadParallelRunnerDefaultNumWorkerThreads();
   if (n_runners > MAX_RUNNERS)
      n_runners = MAX_RUNNERS;
   D("n_runners = %ld\n", n_runners);
   runner = JxlThreadParallelRunnerCreate(NULL, n_runners);
   if (!runner)
      goto quit;

   jst = JxlDecoderSetParallelRunner(dec, JxlThreadParallelRunner, runner);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;
#endif /* MAX_RUNNERS */

   jst =
      JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;

   jst = JxlDecoderSetInput(dec, fdata, im->fsize);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;

   for (;;)
     {
        jst = JxlDecoderProcessInput(dec);
        switch (jst)
          {
          default:
             D("Unexpected status: %d\n", jst);
             goto quit;

          case JXL_DEC_BASIC_INFO:
             jst = JxlDecoderGetBasicInfo(dec, &info);
             if (jst != JXL_DEC_SUCCESS)
                goto quit;

             D("WxH=%dx%d alpha=%d bps=%d nc=%d+%d\n", info.xsize, info.ysize,
               info.alpha_bits, info.bits_per_sample,
               info.num_color_channels, info.num_extra_channels);
             if (info.have_animation)
               {
                  D("anim=%d loops=%u tps_num/den=%u/%u\n", info.have_animation,
                    info.animation.num_loops, info.animation.tps_numerator,
                    info.animation.tps_denominator);
               }

             if (!IMAGE_DIMENSIONS_OK(info.xsize, info.ysize))
                goto quit;

             im->w = info.xsize;
             im->h = info.ysize;
             IM_FLAG_UPDATE(im, F_HAS_ALPHA, info.alpha_bits > 0);

             if (!load_data)
                QUIT_WITH_RC(LOAD_SUCCESS);
             break;

          case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
             if (!__imlib_AllocateData(im))
                QUIT_WITH_RC(LOAD_OOM);

             jst = JxlDecoderSetImageOutCallback(dec, &pbuf_fmt,
                                                 _scanline_cb, im);
             if (jst != JXL_DEC_SUCCESS)
                goto quit;
             break;

          case JXL_DEC_FULL_IMAGE:
             goto done;
          }
     }
 done:

   if (im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   rc = LOAD_SUCCESS;

 quit:
#if MAX_RUNNERS > 0
   if (runner)
      JxlThreadParallelRunnerDestroy(runner);
#endif
   if (dec)
      JxlDecoderDestroy(dec);
   munmap(fdata, im->fsize);

   return rc;
}

void
formats(ImlibLoader * l)
{
   static const char  *const list_formats[] = { "jxl" };
   __imlib_LoaderSetFormats(l, list_formats, ARRAY_SIZE(list_formats));
}
