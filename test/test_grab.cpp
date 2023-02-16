#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include <X11/Xutil.h>

#include "test.h"

typedef struct {
   Display            *dpy;
   Window              root;
   Visual             *vis;
   Colormap            cmap;
   Visual             *argb_vis;
   Colormap            argb_cmap;
   int                 depth;

   const char         *test;
   int                 scale;
   int                 do_mask;

   unsigned int        color;
} xd_t;

static xd_t         xd;

static Visual      *
_x11_vis_argb(void)
{
   XVisualInfo        *xvi, xvit;
   int                 i, num;
   Visual             *vis;

   xvit.screen = DefaultScreen(xd.dpy);
   xvit.depth = 32;
   xvit.c_class = TrueColor;

   xvi = XGetVisualInfo(xd.dpy,
                        VisualScreenMask | VisualDepthMask | VisualClassMask,
                        &xvit, &num);
   if (!xvi)
      return NULL;

   for (i = 0; i < num; i++)
     {
        if (xvi[i].depth == 32)
           break;
     }

   vis = (i < num) ? xvi[i].visual : NULL;

   XFree(xvi);

   xd.argb_vis = vis;
   xd.argb_cmap =
      (vis) ? XCreateColormap(xd.dpy, xd.root, vis, AllocNone) : None;

   return vis;
}

static int
_x11_init(int depth)
{
   xd.dpy = XOpenDisplay(NULL);
   if (!xd.dpy)
     {
        fprintf(stderr, "Can't open display\n");
        exit(1);
     }

   if (depth != 32)
      depth = DefaultDepth(xd.dpy, DefaultScreen(xd.dpy));
   xd.depth = depth;

   xd.root = DefaultRootWindow(xd.dpy);
   if (depth == 32)
     {
        xd.vis = _x11_vis_argb();
        xd.cmap = xd.argb_cmap;
        if (!xd.vis)
          {
             fprintf(stderr, "No 32 bit depthh visual\n");
             return 1;
          }

     }
   else
     {
        xd.vis = DefaultVisual(xd.dpy, DefaultScreen(xd.dpy));
        xd.cmap = DefaultColormap(xd.dpy, DefaultScreen(xd.dpy));
     }

   imlib_context_set_display(xd.dpy);
   imlib_context_set_visual(xd.vis);
   imlib_context_set_colormap(xd.cmap);

   return 0;
}

static void
_x11_fini(void)
{
   XCloseDisplay(xd.dpy);
}

static              Pixmap
_pmap_mk_fill_solid(int w, int h, unsigned int color)
{
   XGCValues           gcv;
   GC                  gc;
   Pixmap              pmap;
   XColor              xc;

   xc.red = ((color >> 16) & 0xff) * 0x101;
   xc.green = ((color >> 8) & 0xff) * 0x101;
   xc.blue = ((color >> 0) & 0xff) * 0x101;
   XAllocColor(xd.dpy, xd.cmap, &xc);
   D("color RGB = %#06x %#06x %#06x  %#08lx\n",
     xc.red, xc.green, xc.blue, xc.pixel);
#if 0
   xc.red = xc.green = xc.blue = 0;
   XQueryColor(xd.dpy, xd.cmap, &xc);
   D("color RGB = %#06x %#06x %#06x  %#08lx\n",
     xc.red, xc.green, xc.blue, xc.pixel);
#endif

   pmap = XCreatePixmap(xd.dpy, xd.root, w, h, xd.depth);

   gcv.foreground = xc.pixel;
   gcv.graphics_exposures = False;
   gc = XCreateGC(xd.dpy, pmap, GCForeground | GCGraphicsExposures, &gcv);

   XFillRectangle(xd.dpy, pmap, gc, 0, 0, w, h);

   XFreeGC(xd.dpy, gc);

   return pmap;
}

static              Pixmap
_pmap_mk_mask(int w, int h, int x, int y)
{
   XGCValues           gcv;
   GC                  gc;
   Pixmap              pmap;

   pmap = XCreatePixmap(xd.dpy, xd.root, w, h, 1);

   gcv.foreground = 1;
   gcv.graphics_exposures = False;
   gc = XCreateGC(xd.dpy, pmap, GCForeground | GCGraphicsExposures, &gcv);

   XFillRectangle(xd.dpy, pmap, gc, 0, 0, w, h);

   XFreeGC(xd.dpy, gc);

   return pmap;
}

static void
_img_dump(Imlib_Image im, const char *file)
{
   static int          seqn = 0;
   char                buf[1024];

   snprintf(buf, sizeof(buf), file, seqn++);
   imlib_context_set_image(im);
   imlib_save_image(buf);
}

static void
_test_grab_1(int w, int h, int x0, int y0)
{
   Imlib_Image         im;
   uint32_t           *dptr, pix, col;
   int                 x, y, err;
   int                 xs, ys, ws, hs;
   char                buf[128];
   Pixmap              mask;

   xs = x0;
   ws = w;
   ys = y0;
   hs = h;
   if (xd.scale > 0)
     {
        xs *= xd.scale;
        ws *= xd.scale;
        ys *= xd.scale;
        hs *= xd.scale;
     }
   if (xd.scale < 0)
     {
        xs = (xs & ~1) / -xd.scale;
        ws /= -xd.scale;
        ys = (ys & ~1) / -xd.scale;
        hs /= -xd.scale;
     }

   D("%s: %3dx%3d(%3d,%3d) -> %3dx%3d(%d,%d)\n", __func__,
     w, h, x0, y0, ws, hs, xs, ys);

   switch (xd.do_mask)
     {
     default:
     case 0:                   // Without mask
        mask = None;
        break;
     case 1:                   // With mask
        mask = _pmap_mk_mask(w, h, 0, 0);
        break;
     }

   if (xd.scale == 0)
      im = imlib_create_image_from_drawable(mask, x0, y0, w, h, 0);
   else
      im = imlib_create_scaled_image_from_drawable(mask, x0, y0, w, h,
                                                   ws, hs, 0, 0);

   if (mask != None)
      XFreePixmap(xd.dpy, mask);

   imlib_context_set_image(im);
   snprintf(buf, sizeof(buf), "%s/%s-%%d.png", IMG_GEN, xd.test);
   _img_dump(im, buf);

   dptr = imlib_image_get_data_for_reading_only();
   err = 0;
   for (y = 0; y < hs; y++)
     {
        for (x = 0; x < ws; x++)
          {
             pix = *dptr++;
             if (xs + x < 0 || ys + y < 0 || xs + x >= ws || ys + y >= hs)
               {
#if 1
                  // FIXME - Pixels outside source drawable are not properly initialized
                  if (x0 != 0 || y0 != 0)
                     continue;
#endif
                  col = 0x00000000;     // Use 0xff000000 if non-alpa
               }
             else
               {
                  col = xd.color;
               }
             if (pix != col)
               {
                  D2("%3d,%3d (%3d,%3d): %08x != %08x\n",
                     x, y, x0, y0, pix, col);
                  err = 1;
               }
          }
     }

   EXPECT_FALSE(err);

   imlib_free_image_and_decache();
}

static void
_test_grab_2(const char *test, int depth, int scale, int opt, int mask)
{
   char                buf[64];
   Pixmap              pmap;
   int                 w, h, d;

   D("%s: %s: depth=%d scale=%d opt=%d mask=%d\n", __func__, test,
     depth, scale, opt, mask);

   snprintf(buf, sizeof(buf), "%s_d%02d_s%d_o%d_m%d",
            test, depth, scale, opt, mask);

   xd.color = 0xff0000ff;       // B ok
   xd.color = 0xff00ff00;       // G ok
   xd.color = 0xffff0000;       // R ok

   if (_x11_init(depth))
      return;

   xd.scale = scale;
   xd.test = buf;
   xd.do_mask = mask;

   w = 32;
   h = 45;

   pmap = _pmap_mk_fill_solid(w, h, xd.color);
   imlib_context_set_drawable(pmap);

   switch (opt)
     {
     case 0:
        _test_grab_1(w, h, 0, 0);
        break;
     case 1:
        d = 1;
        _test_grab_1(w, h, -d, -d);
        _test_grab_1(w, h, -d, d);
        _test_grab_1(w, h, d, d);
        _test_grab_1(w, h, d, -d);
        if (scale < 0)
          {
             d = 2;
             _test_grab_1(w, h, -d, -d);
             _test_grab_1(w, h, -d, d);
             _test_grab_1(w, h, d, d);
             _test_grab_1(w, h, d, -d);
          }
        break;
     }

   XFreePixmap(xd.dpy, pmap);

   _x11_fini();
}

static void
_test_grab(const char *test, int scale, int opt)
{
   _test_grab_2(test, 24, scale, opt, 0);
   _test_grab_2(test, 32, scale, opt, 0);

   _test_grab_2(test, 24, scale, opt, 1);
   _test_grab_2(test, 32, scale, opt, 1);
}

// No scaling - imlib_create_image_from_drawable

TEST(GRAB, grab_noof_s0)
{
   _test_grab("grab_noof", 0, 0);
}

TEST(GRAB, grab_offs_s0)
{
   _test_grab("grab_offs", 0, 1);
}

// No scaling - imlib_create_scaled_image_from_drawable

TEST(GRAB, grab_noof_s1)
{
   _test_grab("grab_noof", 1, 0);
}

TEST(GRAB, grab_offs_s1)
{
   _test_grab("grab_offs", 1, 1);
}

// Scaling - imlib_create_scaled_image_from_drawable

TEST(GRAB, grab_noof_su2)
{
   _test_grab("grab_noof", 2, 0);
}

TEST(GRAB, grab_noof_sd2)
{
   _test_grab("grab_noof", -2, 0);
}

TEST(GRAB, grab_offs_su2)
{
   _test_grab("grab_offs", 2, 1);
}

TEST(GRAB, grab_offs_sd2)
{
   _test_grab("grab_offs", -2, 1);
}
