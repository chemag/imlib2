/*
 * Convert images between formats, using Imlib2's API.
 * Defaults to jpg's.
 */
#include "config.h"
#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#include <Imlib2.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define HELP \
   "Usage:\n" \
   "  imlib2_conv [OPTIONS] [ input-file output-file[.fmt] ]\n" \
   "    <fmt> defaults to jpg if not provided.\n" \
   "\n" \
   "OPTIONS:\n" \
   "  -h  : Show this help\n"

static void
usage(void)
{
   printf(HELP);
}

int
main(int argc, char **argv)
{
   int                 opt, err;
   const char         *fin, *fout;
   char               *dot, *colon;
   Imlib_Image         im;

   while ((opt = getopt(argc, argv, "h")) != -1)
     {
        switch (opt)
          {
          default:
          case 'h':
             usage();
             exit(0);
          }
     }

   if (argc - optind < 2)
     {
        usage();
        exit(1);
     }

   fin = argv[optind];
   fout = argv[optind + 1];

   im = imlib_load_image_with_errno_return(fin, &err);
   if (!im)
     {
        fprintf(stderr, "*** Error %d:'%s' loading image: '%s'\n",
                err, imlib_strerror(err), fin);
        return 1;
     }

   /* we only care what format the export format is. */
   imlib_context_set_image(im);

   /* hopefully the last one will be the one we want.. */
   dot = strrchr(fout, '.');

   /* if there's a format, snarf it and set the format. */
   if (dot && *(dot + 1))
     {
        colon = strrchr(++dot, ':');
        /* if a db file with a key, export it to a db. */
        if (colon && *(colon + 1))
          {
             *colon = 0;
             /* beats having to look for strcasecmp() */
             if (!strncmp(dot, "db", 2) || !strncmp(dot, "dB", 2) ||
                 !strncmp(dot, "DB", 2) || !strncmp(dot, "Db", 2))
               {
                  imlib_image_set_format("db");
               }
             *colon = ':';
          }
        else
          {
             char               *p, *q;

             /* max length of 8 for format name. seems reasonable. */
             p = strndup(dot, 8);
             /* Imlib2 only recognizes lowercase formats. convert it. */
             for (q = p; *q; q++)
                *q = tolower(*q);
             imlib_image_set_format(p);
             free(p);
          }
     }
   else
      imlib_image_set_format("jpg");

   imlib_save_image_with_errno_return(fout, &err);
   if (err)
      fprintf(stderr, "*** Error %d:'%s' saving image: '%s'\n",
              err, imlib_strerror(err), fout);

   return err;
}
