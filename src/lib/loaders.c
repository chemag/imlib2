#include "common.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "file.h"
#include "image.h"

static void         __imlib_LoadAllLoaders(void);

static ImlibLoader *loaders = NULL;

ImlibLoader       **
__imlib_GetLoaderList(void)
{
   return &loaders;
}

/* try dlopen()ing the file if we succeed finish filling out the malloced */
/* loader struct and return it */
static ImlibLoader *
__imlib_ProduceLoader(char *file)
{
   ImlibLoader        *l;
   void                (*l_formats)(ImlibLoader * l);

   l = malloc(sizeof(ImlibLoader));
   l->num_formats = 0;
   l->formats = NULL;
   l->handle = dlopen(file, RTLD_NOW | RTLD_LOCAL);
   if (!l->handle)
     {
        free(l);
        return NULL;
     }
   l->load = dlsym(l->handle, "load");
   l->save = dlsym(l->handle, "save");
   l_formats = dlsym(l->handle, "formats");

   /* each loader must provide formats() and at least load() or save() */
   if (!l_formats || (!l->load && !l->save))
     {
        dlclose(l->handle);
        free(l);
        return NULL;
     }
   l_formats(l);
   l->file = strdup(file);
   l->next = NULL;
   return l;
}

/* fre the struct for a loader and close its dlopen'd handle */
static void
__imlib_ConsumeLoader(ImlibLoader * l)
{
   if (l->file)
      free(l->file);
   if (l->handle)
      dlclose(l->handle);
   if (l->formats)
     {
        int                 i;

        for (i = 0; i < l->num_formats; i++)
           free(l->formats[i]);
        free(l->formats);
     }
   free(l);
}

void
__imlib_RescanLoaders(void)
{
   static time_t       last_scan_time = 0;
   static time_t       last_modified_system_time = 0;
   static int          scanned = 0;
   time_t              current_time;
   char                do_reload = 0;

   /* dont stat the dir and rescan if we checked in the last 5 seconds */
   current_time = time(NULL);
   if ((current_time - last_scan_time) < 5)
      return;

   /* ok - was the system loaders dir contents modified ? */
   last_scan_time = current_time;

   current_time = __imlib_FileModDate(__imlib_PathToLoaders());
   if (current_time == 0)
      return;                   /* Loader directory not found */
   if ((current_time > last_modified_system_time) || (!scanned))
     {
        /* yup - set the "do_reload" flag */
        do_reload = 1;
        last_modified_system_time = current_time;
     }

   /* if we dont ned to reload the loaders - get out now */
   if (!do_reload)
      return;

   __imlib_RemoveAllLoaders();
   __imlib_LoadAllLoaders();

   scanned = 1;
}

/* remove all loaders int eh list we have cached so we can re-load them */
void
__imlib_RemoveAllLoaders(void)
{
   ImlibLoader        *l, *il;

   l = loaders;
   while (l)
     {
        il = l;
        l = l->next;
        __imlib_ConsumeLoader(il);
     }
   loaders = NULL;
}

/* find all the loaders we can find and load them up to see what they can */
/* load / save */
static void
__imlib_LoadAllLoaders(void)
{
   int                 i, num;
   char              **list;

   /* list all the loaders imlib can find */
   list = __imlib_ListModules(__imlib_PathToLoaders(), &num);
   /* no loaders? well don't load anything */
   if (!list)
      return;

   /* go through the list of filenames for loader .so's and load them */
   /* (or try) and if it succeeds, append to our loader list */
   for (i = num - 1; i >= 0; i--)
     {
        ImlibLoader        *l;

        l = __imlib_ProduceLoader(list[i]);
        if (l)
          {
             l->next = loaders;
             loaders = l;
          }
        if (list[i])
           free(list[i]);
     }
   free(list);
}

__EXPORT__ ImlibLoader *
__imlib_FindBestLoaderForFormat(const char *format, int for_save)
{
   ImlibLoader        *l;

   if (!format || format[0] == '\0')
      return NULL;

   /* go through the loaders - first loader that claims to handle that */
   /* image type (extension wise) wins as a first guess to use - NOTE */
   /* this is an OPTIMISATION - it is possible the file has no extension */
   /* or has an unrecognised one but still is loadable by a loader. */
   /* if thkis initial loader failes to load the load mechanism will */
   /* systematically go from most recently used to least recently used */
   /* loader until one succeeds - or none are left and all have failed */
   /* and only if all fail does the laod fail. the lao9der that does */
   /* succeed gets it way to the head of the list so it's going */
   /* to be used first next time in this search mechanims - this */
   /* assumes you tend to laod a few image types and ones generally */
   /* of the same format */
   for (l = loaders; l; l = l->next)
     {
        int                 i;

        /* go through all the formats that loader supports */
        for (i = 0; i < l->num_formats; i++)
          {
             /* does it match ? */
             if (strcasecmp(l->formats[i], format) == 0)
               {
                  /* does it provide the function we need? */
                  if ((for_save && l->save) || (!for_save && l->load))
                     goto done;
               }
          }
     }

 done:
   return l;
}

__EXPORT__ ImlibLoader *
__imlib_FindBestLoaderForFile(const char *file, int for_save)
{
   ImlibLoader        *l;

   l = __imlib_FindBestLoaderForFormat(__imlib_FileExtension(file), for_save);

   return l;
}

ImlibLoader        *
__imlib_FindBestLoaderForFileFormat(const char *file, char *format,
                                    int for_save)
{
   /* if the format is provided ("png" "jpg" etc.) use that */
   /* otherwise us the file extension */
   if (format)
      return __imlib_FindBestLoaderForFormat(format, for_save);
   else
      return __imlib_FindBestLoaderForFile(file, for_save);
}
