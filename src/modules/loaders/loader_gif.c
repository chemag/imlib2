#include "loader_common.h"

#include <gif_lib.h>

#define DBG_PFX "LDR-gif"

static void
make_colormap(DATA32 * cmi, ColorMapObject * cmg, int bg, int tr)
{
   int                 i, r, g, b;

   memset(cmi, 0, 256 * sizeof(DATA32));
   if (!cmg)
      return;

   for (i = cmg->ColorCount > 256 ? 256 : cmg->ColorCount; i-- > 0;)
     {
        r = cmg->Colors[i].Red;
        g = cmg->Colors[i].Green;
        b = cmg->Colors[i].Blue;
        cmi[i] = PIXEL_ARGB(0xff, r, g, b);
     }

   /* if bg > cmg->ColorCount, it is transparent black already */
   if (tr >= 0 && tr < 256)
      cmi[tr] = bg >= 0 && bg < 256 ? cmi[bg] & 0x00ffffff : 0x00000000;
}

int
load2(ImlibImage * im, int load_data)
{
   int                 rc;
   DATA32             *ptr;
   GifFileType        *gif;
   GifRowType         *rows;
   GifRecordType       rec;
   ColorMapObject     *cmap;
   int                 i, j, bg, bits, done;
   int                 transp;
   int                 fd;
   DATA32              colormap[256];

   fd = dup(fileno(im->fp));

#if GIFLIB_MAJOR >= 5
   gif = DGifOpenFileHandle(fd, NULL);
#else
   gif = DGifOpenFileHandle(fd);
#endif
   if (!gif)
      return LOAD_FAIL;

   rc = LOAD_FAIL;
   done = 0;
   rows = NULL;
   transp = -1;

   bg = gif->SBackGroundColor;
   cmap = gif->SColorMap;

   for (;;)
     {
        if (DGifGetRecordType(gif, &rec) == GIF_ERROR)
          {
             /* PrintGifError(); */
             DL("err1\n");
             break;
          }

        if (rec == TERMINATE_RECORD_TYPE)
          {
             DL(" TERMINATE_RECORD_TYPE(%d)\n", rec);
             break;
          }

        if (rec == IMAGE_DESC_RECORD_TYPE)
          {
             if (DGifGetImageDesc(gif) == GIF_ERROR)
               {
                  /* PrintGifError(); */
                  DL("err2\n");
                  break;
               }

             DL(" IMAGE_DESC_RECORD_TYPE(%d): ic=%d x,y=%d,%d wxh=%dx%d\n",
                rec, gif->ImageCount, gif->Image.Left, gif->Image.Top,
                gif->Image.Width, gif->Image.Height);

             if (done)
                continue;

             im->w = gif->Image.Width;
             im->h = gif->Image.Height;
             if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
                goto quit;

             D(" Frame %d: x,y=%d,%d wxh=%dx%d\n", gif->ImageCount,
               gif->Image.Left, gif->Image.Top, im->w, im->h);

             DL(" CM S=%p I=%p\n", cmap, gif->Image.ColorMap);
             if (gif->Image.ColorMap)
                cmap = gif->Image.ColorMap;
             if (load_data)
                make_colormap(colormap, cmap, bg, transp);

             rows = calloc(im->h, sizeof(GifRowType));
             if (!rows)
                goto quit;

             for (i = 0; i < im->h; i++)
               {
                  rows[i] = calloc(im->w, sizeof(GifPixelType));
                  if (!rows[i])
                     goto quit;
               }

             if (gif->Image.Interlace)
               {
                  static const int    intoffset[] = { 0, 4, 2, 1 };
                  static const int    intjump[] = { 8, 8, 4, 2 };
                  for (i = 0; i < 4; i++)
                    {
                       for (j = intoffset[i]; j < im->h; j += intjump[i])
                         {
                            DGifGetLine(gif, rows[j], im->w);
                         }
                    }
               }
             else
               {
                  for (i = 0; i < im->h; i++)
                    {
                       DGifGetLine(gif, rows[i], im->w);
                    }
               }

             break;
          }
        else if (rec == EXTENSION_RECORD_TYPE)
          {
             int                 ext_code;
             GifByteType        *ext;

             ext = NULL;
             DGifGetExtension(gif, &ext_code, &ext);
             while (ext)
               {
                  DL(" EXTENSION_RECORD_TYPE(%d): ic=%d: ext_code=%02x: %02x %02x %02x %02x %02x\n",    //
                     rec, gif->ImageCount, ext_code,
                     ext[0], ext[1], ext[2], ext[3], ext[4]);
                  if (ext_code == GRAPHICS_EXT_FUNC_CODE
                      && gif->ImageCount == 0)
                    {
                       bits = ext[1];
                       if (bits & 1)
                          transp = ext[4];
                       D(" Frame %d: disp=%d ui=%d tr=%d, transp = #%02x\n",
                         gif->ImageCount + 1,
                         (bits >> 2) & 0x7, (bits >> 1) & 1, bits & 1, transp);
                    }
                  ext = NULL;
                  DGifGetExtensionNext(gif, &ext);
               }
          }
        else
          {
             DL(" Unknown record type(%d)\n", rec);
          }
     }

   UPDATE_FLAG(im->flags, F_HAS_ALPHA, transp >= 0);

   if (!rows)
      goto quit;

   if (!load_data)
     {
        rc = LOAD_SUCCESS;
        goto quit;
     }

   /* Load data */

   ptr = __imlib_AllocateData(im);
   if (!ptr)
      goto quit;

   for (i = 0; i < im->h; i++)
     {
        for (j = 0; j < im->w; j++)
          {
             *ptr++ = colormap[rows[i][j]];
          }

        if (im->lc && __imlib_LoadProgressRows(im, i, 1))
          {
             rc = LOAD_BREAK;
             goto quit;
          }
     }

   rc = LOAD_SUCCESS;

 quit:
   if (rows)
     {
        for (i = 0; i < im->h; i++)
           free(rows[i]);
        free(rows);
     }

#if GIFLIB_MAJOR > 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1)
   DGifCloseFile(gif, NULL);
#else
   DGifCloseFile(gif);
#endif

   if (rc <= 0)
      __imlib_FreeData(im);

   return rc;
}

void
formats(ImlibLoader * l)
{
   static const char  *const list_formats[] = { "gif" };
   __imlib_LoaderSetFormats(l, list_formats, ARRAY_SIZE(list_formats));
}
