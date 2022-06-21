#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

static const char  *
__imlib_PathToModules(void)
{
#if 0
   static const char  *path = NULL;

   if (path)
      return path;

   path = getenv("IMLIB2_MODULE_PATH");
   if (path && __imlib_FileIsDir(path))
      return path;
#endif

   return PACKAGE_LIB_DIR "/imlib2";
}

const char         *
__imlib_PathToFilters(void)
{
   static char        *path = NULL;
   char                buf[1024];

   if (path)
      return path;

   path = getenv("IMLIB2_FILTER_PATH");
   if (path && __imlib_FileIsDir(path))
      return path;

   snprintf(buf, sizeof(buf), "%s/%s", __imlib_PathToModules(), "filters");
   path = strdup(buf);

   return path;
}

const char         *
__imlib_PathToLoaders(void)
{
   static char        *path = NULL;
   char                buf[1024];

   if (path)
      return path;

   path = getenv("IMLIB2_LOADER_PATH");
   if (path && __imlib_FileIsDir(path))
      return path;

   snprintf(buf, sizeof(buf), "%s/%s", __imlib_PathToModules(), "loaders");
   path = strdup(buf);

   return path;
}

static bool
_file_is_module(const char *name)
{
   const char         *ext;

   ext = strrchr(name, '.');
   if (!ext)
      return false;

   if (
#ifdef __CYGWIN__
         strcasecmp(ext, ".dll") != 0 &&
#endif
         strcasecmp(ext, ".so") != 0)
      return false;

   return true;
}

char              **
__imlib_ListModules(const char *path, int *num_ret)
{
   char              **list = NULL, **l;
   char                file[1024], *p;
   int                 num, i, ntot;

   *num_ret = 0;
   ntot = 0;

   l = __imlib_FileDir(path, &num);
   if (num <= 0)
      return NULL;

   list = malloc(num * sizeof(char *));
   if (list)
     {
        for (i = 0; i < num; i++)
          {
             if (!_file_is_module(l[i]))
                continue;
             snprintf(file, sizeof(file), "%s/%s", path, l[i]);
             p = strdup(file);
             if (p)
                list[ntot++] = p;
          }
     }
   __imlib_FileFreeDirList(l, num);

   *num_ret = ntot;

   return list;
}
