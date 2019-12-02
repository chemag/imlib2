#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#include <Imlib2.h>

#define PROG_NAME "imlib2_load"

static char         progress_called;
static FILE        *fout;

#define HELP \
   "Usage:\n" \
   "  imlib2_load [OPTIONS] FILE...\n" \
   "OPTIONS:\n" \
   "  -e  : Break on error\n" \
   "  -p  : Check that progress is called\n" \
   "  -x  : Print to stderr\n"

static void
usage(void)
{
   printf(HELP);
}

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
   progress_called = 1;
   return 1;                    /* Continue */
}

int
main(int argc, char **argv)
{
   int                 opt;
   Imlib_Image         im;
   Imlib_Load_Error    lerr;
   int                 check_progress;
   int                 break_on_error;

   fout = stdout;
   check_progress = 0;
   break_on_error = 0;

   while ((opt = getopt(argc, argv, "epx")) != -1)
     {
        switch (opt)
          {
          case 'e':
             break_on_error += 1;
             break;
          case 'p':
             check_progress = 1;
             break;
          case 'x':
             fout = stderr;
             break;
          }
     }

   argc -= optind;
   argv += optind;

   if (argc <= 0)
     {
        usage();
        return 1;
     }

   if (check_progress)
     {
        imlib_context_set_progress_function(progress);
        imlib_context_set_progress_granularity(10);
     }

   for (; argc > 0; argc--, argv++)
     {
        progress_called = 0;

        fprintf(fout, "Loading image: '%s'\n", argv[0]);

        lerr = 0;

        if (check_progress)
           im = imlib_load_image_with_error_return(argv[0], &lerr);
        else
           im = imlib_load_image(argv[0]);

        if (!im)
          {
             fprintf(fout, "*** Error %d loading image: %s\n", lerr, argv[0]);
             if (break_on_error & 2)
                break;
             continue;
          }

        imlib_context_set_image(im);

        if (!check_progress)
           imlib_image_get_data();

        imlib_free_image_and_decache();

        if (check_progress && !progress_called)
          {
             fprintf(fout, "*** No progress during image load\n");
             if (break_on_error & 1)
                break;
          }
     }

   return 0;
}
