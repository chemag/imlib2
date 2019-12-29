#include "loader_common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gif_lib.h>

char
load(ImlibImage * im, ImlibProgressFunction progress,
     char progress_granularity, char load_data)
{
   static const int    intoffset[] = { 0, 4, 2, 1 };
   static const int    intjump[] = { 8, 8, 4, 2 };
   int                 rc;
   DATA32             *ptr;
   GifFileType        *gif;
   GifRowType         *rows;
   GifRecordType       rec;
   ColorMapObject     *cmap;
   int                 i, j, done, bg, r, g, b, w = 0, h = 0;
   float               per = 0.0, per_inc;
   int                 last_per = 0, last_y = 0;
   int                 transp;
   int                 fd;
   DATA32              colormap[256];

   fd = open(im->real_file, O_RDONLY);
   if (fd < 0)
      return LOAD_FAIL;

   rc = LOAD_FAIL;
   done = 0;
   rows = NULL;
   transp = -1;

#if GIFLIB_MAJOR >= 5
   gif = DGifOpenFileHandle(fd, NULL);
#else
   gif = DGifOpenFileHandle(fd);
#endif
   if (!gif)
      goto quit;

   do
     {
        if (DGifGetRecordType(gif, &rec) == GIF_ERROR)
          {
             /* PrintGifError(); */
             rec = TERMINATE_RECORD_TYPE;
          }

        if ((rec == IMAGE_DESC_RECORD_TYPE) && (!done))
          {
             if (DGifGetImageDesc(gif) == GIF_ERROR)
               {
                  /* PrintGifError(); */
                  rec = TERMINATE_RECORD_TYPE;
                  break;
               }

             im->w = w = gif->Image.Width;
             im->h = h = gif->Image.Height;
             if (!IMAGE_DIMENSIONS_OK(w, h))
                goto quit;

             rows = calloc(h, sizeof(GifRowType *));
             if (!rows)
                goto quit;

             for (i = 0; i < h; i++)
               {
                  rows[i] = calloc(w, sizeof(GifPixelType));
                  if (!rows[i])
                     goto quit;
               }

             if (gif->Image.Interlace)
               {
                  for (i = 0; i < 4; i++)
                    {
                       for (j = intoffset[i]; j < h; j += intjump[i])
                         {
                            DGifGetLine(gif, rows[j], w);
                         }
                    }
               }
             else
               {
                  for (i = 0; i < h; i++)
                    {
                       DGifGetLine(gif, rows[i], w);
                    }
               }
             done = 1;
          }
        else if (rec == EXTENSION_RECORD_TYPE)
          {
             int                 ext_code;
             GifByteType        *ext;

             ext = NULL;
             DGifGetExtension(gif, &ext_code, &ext);
             while (ext)
               {
                  if ((ext_code == 0xf9) && (ext[1] & 1) && (transp < 0))
                    {
                       transp = (int)ext[4];
                    }
                  ext = NULL;
                  DGifGetExtensionNext(gif, &ext);
               }
          }
     }
   while (rec != TERMINATE_RECORD_TYPE);

   if (transp >= 0)
      SET_FLAG(im->flags, F_HAS_ALPHA);
   else
      UNSET_FLAG(im->flags, F_HAS_ALPHA);

   if (!rows)
      goto quit;

   if (!load_data)
     {
        rc = LOAD_SUCCESS;
        goto quit;
     }

   /* Load data */

   bg = gif->SBackGroundColor;
   cmap = (gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap);
   memset(colormap, 0, sizeof(colormap));
   if (cmap != NULL)
     {
        for (i = cmap->ColorCount > 256 ? 256 : cmap->ColorCount; i-- > 0;)
          {
             r = cmap->Colors[i].Red;
             g = cmap->Colors[i].Green;
             b = cmap->Colors[i].Blue;
             colormap[i] = PIXEL_ARGB(0xff, r, g, b);
          }
        /* if bg > cmap->ColorCount, it is transparent black already */
        if (transp >= 0 && transp < 256)
           colormap[transp] = bg >= 0 && bg < 256 ?
              colormap[bg] & 0x00ffffff : 0x00000000;
     }

   ptr = __imlib_AllocateData(im);
   if (!ptr)
      goto quit;

   per_inc = 100.0 / (float)h;
   for (i = 0; i < h; i++)
     {
        for (j = 0; j < w; j++)
          {
             *ptr++ = colormap[rows[i][j]];
          }
        per += per_inc;
        if (progress && (((int)per) != last_per)
            && (((int)per) % progress_granularity == 0))
          {
             last_per = (int)per;
             if (!(progress(im, per, 0, last_y, w, i)))
               {
                  rc = LOAD_BREAK;
                  goto quit;
               }
             last_y = i;
          }
     }

   if (progress)
      progress(im, 100, 0, last_y, im->w, im->h);

   rc = LOAD_SUCCESS;

 quit:
   if (rows)
     {
        for (i = 0; i < h; i++)
           free(rows[i]);
        free(rows);
     }

   if (gif)
     {
#if GIFLIB_MAJOR > 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1)
        DGifCloseFile(gif, NULL);
#else
        DGifCloseFile(gif);
#endif
     }

   close(fd);

   if (rc <= 0)
      __imlib_FreeData(im);

   return rc;
}

void
formats(ImlibLoader * l)
{
   static const char  *const list_formats[] = { "gif" };
   int                 i;

   l->num_formats = sizeof(list_formats) / sizeof(char *);
   l->formats = malloc(sizeof(char *) * l->num_formats);

   for (i = 0; i < l->num_formats; i++)
      l->formats[i] = strdup(list_formats[i]);
}
