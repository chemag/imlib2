#include "config.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "Imlib2.h"
#include "props.h"

Display            *disp;

static Window       win;
static Pixmap       pm = 0;
static int          depth;
static int          image_width = 0, image_height = 0;
static int          window_width = 0, window_height = 0;
static char         scale = 0;
static double       scale_x = 1.;
static double       scale_y = 1.;
static Imlib_Image  bg_im = NULL;
static char         progress_granularity = 10;
static char         progress_print = 0;
static int          progress_delay = 0;

#define MAX_DIM	32767

#define SCALE_X(x) (int)(scale_x * (x) + .5)
#define SCALE_Y(x) (int)(scale_y * (x) + .5)

#define HELP \
   "Usage:\n" \
   "  imlib2_view [OPTIONS] FILE...\n" \
   "OPTIONS:\n" \
   "  -g N: Set progress granularity to N%% (default 10(%%))\n" \
   "  -l N: Introduce N ms delay in progress callback (default 0)\n" \
   "  -p  : Print info in progress callback (default no)\n" \
   "  -s S: Set scaling factor to S (default 1.0)\n" \
   "  -v  : Increase verbosity\n"

static void
usage(void)
{
   printf(HELP);
}

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
   int                 up_wx, up_wy, up_ww, up_wh;

   if (progress_print)
      printf("%s: %3d%% %4d,%4d %4dx%4d\n",
             __func__, percent, update_x, update_y, update_w, update_h);

   /* first time it's called */
   imlib_context_set_drawable(pm);
   imlib_context_set_anti_alias(0);
   imlib_context_set_dither(0);
   imlib_context_set_blend(0);
   if (image_width == 0)
     {
        int                 x, y, onoff;

        window_width = DisplayWidth(disp, DefaultScreen(disp));
        window_height = DisplayHeight(disp, DefaultScreen(disp));
        window_width -= 32;     /* Allow for decorations */
        window_height -= 32;

        imlib_context_set_image(im);
        image_width = imlib_image_get_width();
        image_height = imlib_image_get_height();

        if (!scale &&
            (image_width > window_width || image_height > window_height))
          {
             scale_x = scale_y = 1.;
             while (window_width < SCALE_X(image_width) ||
                    window_height < SCALE_Y(image_height))
               {
                  scale_x *= .5;
                  scale_y = scale_x;
               }
          }

        window_width = SCALE_X(image_width);
        window_height = SCALE_Y(image_height);
        if (window_width > MAX_DIM)
          {
             window_width = MAX_DIM;
             scale_x = (double)MAX_DIM / image_width;
          }
        if (window_height > MAX_DIM)
          {
             window_height = MAX_DIM;
             scale_y = (double)MAX_DIM / image_height;
          }

        if (pm)
           XFreePixmap(disp, pm);
        pm = XCreatePixmap(disp, win, window_width, window_height, depth);
        imlib_context_set_drawable(pm);
        if (bg_im)
          {
             imlib_context_set_image(bg_im);
             imlib_free_image_and_decache();
          }
        bg_im = imlib_create_image(image_width, image_height);
        imlib_context_set_image(bg_im);
        for (y = 0; y < image_height; y += 8)
          {
             onoff = (y / 8) & 0x1;
             for (x = 0; x < image_width; x += 8)
               {
                  if (onoff)
                     imlib_context_set_color(144, 144, 144, 255);
                  else
                     imlib_context_set_color(100, 100, 100, 255);
                  imlib_image_fill_rectangle(x, y, 8, 8);
                  onoff++;
                  if (onoff == 2)
                     onoff = 0;
               }
          }
        imlib_render_image_part_on_drawable_at_size(0, 0, image_width,
                                                    image_height, 0, 0,
                                                    window_width,
                                                    window_height);
        XSetWindowBackgroundPixmap(disp, win, pm);
        XResizeWindow(disp, win, window_width, window_height);
        XClearWindow(disp, win);
        XMapWindow(disp, win);
        XSync(disp, False);
     }

   imlib_context_set_anti_alias(0);
   imlib_context_set_dither(0);
   imlib_context_set_blend(1);
   imlib_blend_image_onto_image(im, 0,
                                update_x, update_y, update_w, update_h,
                                update_x, update_y, update_w, update_h);

   up_wx = SCALE_X(update_x);
   up_wy = SCALE_Y(update_y);
   up_ww = SCALE_X(update_w);
   up_wh = SCALE_Y(update_h);
   imlib_context_set_blend(0);
   imlib_render_image_part_on_drawable_at_size(update_x, update_y,
                                               update_w, update_h,
                                               up_wx, up_wy, up_ww, up_wh);
   XClearArea(disp, win, up_wx, up_wy, up_ww, up_wh, False);
   XFlush(disp);

   if (progress_delay > 0)
      usleep(progress_delay);

   return 1;
}

int
main(int argc, char **argv)
{
   int                 opt;
   Imlib_Image        *im;
   char               *file;
   int                 no, inc;
   int                 verbose;

   verbose = 0;

   while ((opt = getopt(argc, argv, "g:l:ps:v")) != -1)
     {
        switch (opt)
          {
          case 'g':
             progress_granularity = atoi(optarg);
             break;
          case 'l':
             progress_delay = 1000 * atoi(optarg);
             break;
          case 'p':
             progress_print = 1;
             break;
          case 's':            /* Scale (window size wrt. image size) */
             scale = 1;
             scale_x = scale_y = atof(optarg);
             break;
          case 'v':
             verbose += 1;
             break;
          default:
             usage();
             return 1;
          }
     }

   argc -= optind;
   argv += optind;

   if (argc <= 0)
     {
        usage();
        return 1;
     }

   disp = XOpenDisplay(NULL);
   if (!disp)
     {
        fprintf(stderr, "Cannot open display\n");
        return 1;
     }

   depth = DefaultDepth(disp, DefaultScreen(disp));

   win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 0, 0, 10, 10,
                             0, 0, 0);
   XSelectInput(disp, win, KeyPressMask | ButtonPressMask | ButtonReleaseMask |
                ButtonMotionMask | PointerMotionMask);

   props_win_set_proto_quit(win);

   imlib_context_set_display(disp);
   imlib_context_set_visual(DefaultVisual(disp, DefaultScreen(disp)));
   imlib_context_set_colormap(DefaultColormap(disp, DefaultScreen(disp)));
   imlib_context_set_progress_function(progress);
   imlib_context_set_progress_granularity(progress_granularity);
   imlib_context_set_drawable(win);

   no = -1;
   for (im = NULL; !im;)
     {
        no++;
        if (no == argc)
          {
             fprintf(stderr, "No loadable image\n");
             exit(0);
          }
        file = argv[no];
        if (verbose)
           printf("Show  %d: '%s'\n", no, file);
        image_width = 0;
        im = imlib_load_image(file);
     }

   imlib_context_set_image(im);

   for (;;)
     {
        int                 x, y, b, count, fdsize, xfd, timeout = 0;
        XEvent              ev;
        static int          zoom_mode = 0, zx, zy;
        static double       zoom = 1.0;
        struct timeval      tval;
        fd_set              fdset;
        double              t1;
        KeySym              key;
        Imlib_Image        *im2;
        int                 no2;

        XFlush(disp);
        XNextEvent(disp, &ev);
        switch (ev.type)
          {
          default:
             break;

          case ClientMessage:
             if (props_clientmsg_check_quit(&ev.xclient))
                goto quit;
             break;
          case KeyPress:
             while (XCheckTypedWindowEvent(disp, win, KeyPress, &ev))
                ;
             key = XLookupKeysym(&ev.xkey, 0);
             if (key == XK_q || key == XK_Escape)
                goto quit;
             if (key == XK_Right)
                goto show_next;
             if (key == XK_Left)
                goto show_prev;
             break;
          case ButtonPress:
             b = ev.xbutton.button;
             x = ev.xbutton.x;
             y = ev.xbutton.y;
             if (b == 3)
               {
                  zoom_mode = 1;
                  zx = x;
                  zy = y;
                  imlib_context_set_drawable(pm);
                  imlib_context_set_image(bg_im);
                  imlib_context_set_anti_alias(0);
                  imlib_context_set_dither(0);
                  imlib_context_set_blend(0);
                  imlib_render_image_part_on_drawable_at_size
                     (0, 0, image_width, image_height,
                      0, 0, window_width, window_height);
                  XClearWindow(disp, win);
               }
             break;
          case ButtonRelease:
             b = ev.xbutton.button;
             if (b == 3)
                zoom_mode = 0;
             if (b == 1)
                goto show_next;
             if (b == 2)
                goto show_prev;
             break;
          case MotionNotify:
             while (XCheckTypedWindowEvent(disp, win, MotionNotify, &ev))
                ;
             x = ev.xmotion.x;
             if (zoom_mode)
               {
                  int                 sx, sy, sw, sh, dx, dy, dw, dh;

                  zoom = ((double)x - (double)zx) / 32.0;
                  if (zoom < 0)
                     zoom = 1.0 + ((zoom * 32.0) / ((double)(zx + 1)));
                  else
                     zoom += 1.0;
                  if (zoom <= 0.0001)
                     zoom = 0.0001;
                  if (zoom > 1.0)
                    {
                       dx = 0;
                       dy = 0;
                       dw = window_width;
                       dh = window_height;

                       sx = zx - (zx / zoom);
                       sy = zy - (zy / zoom);
                       sw = image_width / zoom;
                       sh = image_height / zoom;
                    }
                  else
                    {
                       dx = zx - (zx * zoom);
                       dy = zy - (zy * zoom);
                       dw = window_width * zoom;
                       dh = window_height * zoom;

                       sx = 0;
                       sy = 0;
                       sw = image_width;
                       sh = image_height;
                    }
                  imlib_context_set_anti_alias(0);
                  imlib_context_set_dither(0);
                  imlib_context_set_blend(0);
                  imlib_context_set_image(bg_im);
                  imlib_render_image_part_on_drawable_at_size
                     (sx, sy, sw, sh, dx, dy, dw, dh);
                  XClearWindow(disp, win);
                  XFlush(disp);
                  timeout = 1;
               }
             break;

           show_next:
             inc = 1;
             goto show_next_prev;
           show_prev:
             inc = -1;
             goto show_next_prev;
           show_next_prev:
             for (no2 = no;;)
               {
                  no2 += inc;
                  if (no2 >= argc)
                    {
                       inc = -1;
                       continue;
                    }
                  else if (no2 < 0)
                    {
                       inc = 1;
                       continue;
                    }
                  file = argv[no2];
                  if (verbose)
                     printf("Show  %d: '%s'\n", no2, file);
                  if (no2 == no)
                     break;
                  image_width = 0;
                  im2 = imlib_load_image(file);
                  if (!im2)
                     continue;
                  zoom = 1.0;
                  zoom_mode = 0;
                  imlib_context_set_image(im);
                  imlib_free_image_and_decache();
                  no = no2;
                  im = im2;
                  imlib_context_set_image(im);
                  break;
               }
             break;
          }

        if (XPending(disp))
           continue;

        t1 = 0.2;
        tval.tv_sec = (long)t1;
        tval.tv_usec = (long)((t1 - ((double)tval.tv_sec)) * 1000000);
        xfd = ConnectionNumber(disp);
        fdsize = xfd + 1;
        FD_ZERO(&fdset);
        FD_SET(xfd, &fdset);

        if (timeout)
           count = select(fdsize, &fdset, NULL, NULL, &tval);
        else
           count = select(fdsize, &fdset, NULL, NULL, NULL);

        if (count < 0)
          {
             if ((errno == ENOMEM) || (errno == EINVAL) || (errno == EBADF))
                exit(1);
          }
        else if (count == 0)
          {
             int                 sx, sy, sw, sh, dx, dy, dw, dh;

             if (zoom > 1.0)
               {
                  dx = 0;
                  dy = 0;
                  dw = window_width;
                  dh = window_height;

                  sx = zx - (zx / zoom);
                  sy = zy - (zy / zoom);
                  sw = image_width / zoom;
                  sh = image_height / zoom;
               }
             else
               {
                  dx = zx - (zx * zoom);
                  dy = zy - (zy * zoom);
                  dw = window_width * zoom;
                  dh = window_height * zoom;

                  sx = 0;
                  sy = 0;
                  sw = image_width;
                  sh = image_height;
               }
             imlib_context_set_anti_alias(1);
             imlib_context_set_dither(1);
             imlib_context_set_blend(0);
             imlib_context_set_image(bg_im);
             imlib_render_image_part_on_drawable_at_size
                (sx, sy, sw, sh, dx, dy, dw, dh);
             XClearWindow(disp, win);
             XFlush(disp);
          }
     }

 quit:
   return 0;
}
