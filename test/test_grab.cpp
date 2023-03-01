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
   int                 func;
   int                 scale;
   int                 do_mask;

   unsigned int        col_bg;
   unsigned int        col_fg;
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
_pmap_mk_fill_solid(int w, int h, unsigned int col_bg, unsigned int col_fg)
{
   XGCValues           gcv;
   GC                  gc;
   Pixmap              pmap;
   XColor              xc;
   unsigned int        pix_bg, pix_fg;

   xc.red = ((col_bg >> 16) & 0xff) * 0x101;
   xc.green = ((col_bg >> 8) & 0xff) * 0x101;
   xc.blue = ((col_bg >> 0) & 0xff) * 0x101;
   XAllocColor(xd.dpy, xd.cmap, &xc);
   pix_bg = xc.pixel;
   D("BG color RGB = %#06x %#06x %#06x  %#08x\n",
     xc.red, xc.green, xc.blue, pix_bg);

   xc.red = ((col_fg >> 16) & 0xff) * 0x101;
   xc.green = ((col_fg >> 8) & 0xff) * 0x101;
   xc.blue = ((col_fg >> 0) & 0xff) * 0x101;
   XAllocColor(xd.dpy, xd.cmap, &xc);
   pix_fg = xc.pixel;
   D("FG color RGB = %#06x %#06x %#06x  %#08x\n",
     xc.red, xc.green, xc.blue, pix_fg);

   pmap = XCreatePixmap(xd.dpy, xd.root, w, h, xd.depth);

   gcv.foreground = pix_bg;
   gcv.graphics_exposures = False;
   gc = XCreateGC(xd.dpy, pmap, GCForeground | GCGraphicsExposures, &gcv);

   XFillRectangle(xd.dpy, pmap, gc, 0, 0, w, h);

   XSetForeground(xd.dpy, gc, pix_fg);
   XDrawRectangle(xd.dpy, pmap, gc, 0, 0, w - 1, h - 1);
   XDrawRectangle(xd.dpy, pmap, gc, 1, 1, w - 3, h - 3);

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
   D("%s: '%s'\n", __func__, buf);
   imlib_context_set_image(im);
   imlib_save_image(buf);
}

static void
_test_grab_1(const int wsrc, const int hsrc, const int xsrc, const int ysrc,
             const int xdst, const int ydst)
{
   Imlib_Image         im;
   uint32_t           *dptr, pix, col;
   int                 x, y, err;
   int                 wimg, himg;
   int                 xi, yi, xo, yo, wo, ho, fac, bw;
   char                buf[128];
   Pixmap              mask;

   bw = 2;                      // Output border width
   wimg = wsrc;                 // Produced Imlib_Image size
   himg = hsrc;
   xi = xsrc;                   // Source coord equivalent after scaling
   yi = ysrc;
   xo = xdst;                   // Dest coord of input pixel 0
   yo = ydst;
   wo = wsrc;                   // Grabbed drawable size after scaling
   ho = hsrc;

   switch (xd.do_mask)
     {
     default:
     case 0:                   // Without mask
        mask = None;
        break;
     case 1:                   // With mask
        mask = _pmap_mk_mask(wsrc, hsrc, 0, 0);
        break;
     }

   switch (xd.func)
     {
     default:
     case 0:
        im = imlib_create_image_from_drawable(mask, xsrc, ysrc, wsrc, hsrc, 0);
        xo = -xsrc;
        yo = -ysrc;
        break;
     case 1:
     case -2:
     case 2:
        if (xd.scale > 0)
          {
             wimg = wsrc * xd.scale;
             xi = xsrc * xd.scale;
             himg = hsrc * xd.scale;
             yi = ysrc * xd.scale;
             bw = bw * xd.scale;
          }
        if (xd.scale < 0)
          {
             fac = -xd.scale;
             wimg = wsrc / fac;
             if (xsrc >= 0)
                xi = (xsrc * wimg + (wsrc - 1)) / wsrc;
             else
                xi = (xsrc * wimg - (wsrc - 1)) / wsrc;
             himg = hsrc / fac;
             if (ysrc >= 0)
                yi = (ysrc * himg + (hsrc - 1)) / hsrc;
             else
                yi = (ysrc * himg - (hsrc - 1)) / hsrc;
             bw = (bw + fac - 1) / fac;
          }
        im = imlib_create_scaled_image_from_drawable(mask,
                                                     xsrc, ysrc, wsrc, hsrc,
                                                     wimg, himg, 0, 0);
        xo = -xi;
        yo = -yi;
        wo = wimg;
        ho = himg;
        break;
     }

   if (mask != None)
      XFreePixmap(xd.dpy, mask);

   D("%s: %3dx%3d(%d,%d) -> %3dx%3d(%d,%d -> %d,%d)\n", __func__,
     wsrc, hsrc, xsrc, ysrc, wimg, himg, xi, yi, xdst, ydst);

   imlib_context_set_image(im);
   imlib_image_set_has_alpha(1);
   snprintf(buf, sizeof(buf), "%s/%s-%%d.png", IMG_GEN, xd.test);
   _img_dump(im, buf);

   D("%s: %3dx%3d(%d,%d -> %d,%d) in %dx%d bw=%d\n", __func__,
     wo, ho, xi, yi, xo, yo, wimg, himg, bw);

   dptr = imlib_image_get_data_for_reading_only();
   err = 0;
   for (y = 0; y < himg; y++)
     {
        for (x = 0; x < wimg; x++)
          {
             pix = *dptr++;
             if (x < xo || y < yo ||
                 x >= xo + wo || y >= yo + ho ||
                 x - xo >= xi + wo || y - yo >= yi + ho ||
                 x - xo < xi || y - yo < yi ||
                 x - xo >= xi + wimg || y - yo >= yi + himg)
               {
#if 1
                  // FIXME - Pixels outside source drawable are not properly initialized
                  if (xo != 0 || yo != 0)
                     continue;
#endif
                  col = 0x00000000;
               }
             else if (x < xo + bw || y < yo + bw ||
                      x >= xo + wo - bw || y >= yo + ho - bw)
               {
                  col = xd.col_fg;
               }
             else
               {
                  col = xd.col_bg;
               }
             if (pix != col && err < 20)
               {
                  D2("%3d,%3d (%3d,%3d): %08x != %08x\n",
                     x, y, xi, yi, pix, col);
                  err += 1;
               }
          }
     }

   EXPECT_FALSE(err);

   imlib_free_image_and_decache();
}

static void
_test_grab_2(const char *test, int depth, int func, int opt, int mask)
{
   char                buf[64];
   Pixmap              pmap;
   int                 w, h, d;

   D("%s: %s: depth=%d func=%d opt=%d mask=%d", __func__,
     test, depth, func, opt, mask);

   snprintf(buf, sizeof(buf), "%s_d%02d_f%d_o%d_m%d",
            test, depth, func, opt, mask);

   xd.col_bg = 0xff0000ff;      // B ok
   xd.col_bg = 0xff00ff00;      // G ok
   xd.col_bg = 0xffff0000;      // R ok

   xd.col_fg = 0xff0000ff;      // B

   if (_x11_init(depth))
      return;

   xd.func = func;
   xd.scale = 0;
   switch (func)
     {
     default:
        break;
     case 2:
     case -2:
        xd.scale = func;
        break;
     }
   xd.test = buf;
   xd.do_mask = mask;

   w = 16;
   h = 22;

   pmap = _pmap_mk_fill_solid(w, h, xd.col_bg, xd.col_fg);
   imlib_context_set_drawable(pmap);

   switch (opt)
     {
     case 0:
        _test_grab_1(w, h, 0, 0, 0, 0);
        break;
     case 1:
        if (xd.scale >= 0)
          {
             d = 1;
             _test_grab_1(w, h, -d, -d, 0, 0);
             _test_grab_1(w, h, -d, d, 0, 0);
             _test_grab_1(w, h, d, d, 0, 0);
             _test_grab_1(w, h, d, -d, 0, 0);
          }
        if (xd.scale < 0)
          {
             d = 2;
             _test_grab_1(w, h, -d, -d, 0, 0);
             _test_grab_1(w, h, -d, d, 0, 0);
             _test_grab_1(w, h, d, d, 0, 0);
             _test_grab_1(w, h, d, -d, 0, 0);
          }
        break;
     }

   XFreePixmap(xd.dpy, pmap);

   _x11_fini();
}

static void
_test_grab(const char *test, int func, int opt)
{
   _test_grab_2(test, 24, func, opt, 0);
   _test_grab_2(test, 32, func, opt, 0);

   _test_grab_2(test, 24, func, opt, 1);
   _test_grab_2(test, 32, func, opt, 1);
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
