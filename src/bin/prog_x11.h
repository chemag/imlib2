#ifndef PROG_X11_H
#define PROG_X11_H

#include <X11/Xlib.h>

extern Display *disp;

int             prog_x11_init(void);
int             prog_x11_event(XEvent * ev);

Window          prog_x11_create_window(const char *name, int w, int h);
void            prog_x11_set_title(Window win, const char *name);

#endif                          /* PROG_X11_H */
