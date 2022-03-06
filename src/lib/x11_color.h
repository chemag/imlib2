#ifndef X11_COLOR_H
#define X11_COLOR_H 1

#include "common.h"

extern DATA16       _max_colors;

int                 __imlib_XActualDepth(Display * d, Visual * v);
Visual             *__imlib_BestVisual(Display * d, int screen,
                                       int *depth_return);

DATA8              *__imlib_AllocColorTable(Display * d, Colormap cmap,
                                            DATA8 * type_return, Visual * v);

#endif /* X11_COLOR_H */
