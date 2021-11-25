#ifndef X11_GRAB_H
#define X11_GRAB_H 1

#include "common.h"

int                 __imlib_GrabDrawableToRGBA(DATA32 * data, int ox, int oy,
                                               int ow, int oh, Display * d,
                                               Drawable p, Pixmap m, Visual * v,
                                               Colormap cm, int depth, int x,
                                               int y, int w, int h,
                                               char *domask, int grab);

void                __imlib_GrabXImageToRGBA(DATA32 * data, int ox, int oy,
                                             int ow, int oh, Display * d,
                                             XImage * xim, XImage * mxim,
                                             Visual * v, int depth, int x,
                                             int y, int w, int h, int grab);

#endif /* X11_GRAB_H */
