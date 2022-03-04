#ifndef __LOADERS
#define __LOADERS 1

#include "image.h"

struct _ImlibLoader {
   char               *file;
   int                 num_formats;
   char              **formats;
   void               *handle;
   char                (*load)(ImlibImage * im,
                               ImlibProgressFunction progress,
                               char progress_granularity, char load_data);
   char                (*save)(ImlibImage * im,
                               ImlibProgressFunction progress,
                               char progress_granularity);
   ImlibLoader        *next;
   int                 (*load2)(ImlibImage * im, int load_data);
};

void                __imlib_RemoveAllLoaders(void);
ImlibLoader       **__imlib_GetLoaderList(void);
ImlibLoader        *__imlib_FindBestLoaderForFile(const char *file,
                                                  int for_save);
ImlibLoader        *__imlib_FindBestLoaderForFormat(const char *format,
                                                    int for_save);
ImlibLoader        *__imlib_FindBestLoaderForFileFormat(const char *file,
                                                        const char *format,
                                                        int for_save);
void                __imlib_LoaderSetFormats(ImlibLoader * l,
                                             const char *const *fmt,
                                             unsigned int num);

#endif /* __LOADERS */
