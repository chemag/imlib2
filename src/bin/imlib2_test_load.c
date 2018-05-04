#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#include <Imlib2.h>

#define PROG_NAME "imlib2_test_load"

static void
usage(int exit_status)
{
   fprintf(exit_status ? stderr : stdout,
           PROG_NAME ": Load images to test loaders.\n"
           "Usage: \n  " PROG_NAME " image ...\n");

   exit(exit_status);
}

int
main(int argc, char **argv)
{
   Imlib_Image         im;
   Imlib_Load_Error    lerr;

   if (argc <= 1)
      usage(0);

   for (;;)
     {
        argc--;
        argv++;
        if (argc <= 0)
           break;

        printf("Loading image: '%s'\n", argv[0]);
        im = imlib_load_image_with_error_return(argv[0], &lerr);
        if (!im)
          {
             fprintf(stderr, PROG_NAME ": Error %d loading image: %s\n", lerr,
                     argv[0]);
             continue;
          }
        imlib_context_set_image(im);
        imlib_free_image_and_decache();
     }

   return 0;
}
