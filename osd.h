#ifndef OSD_H
#define OSD_H

#include <stdio.h>
#include <libaosd/aosd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

typedef struct {
    int width;
    int height;
    int thickness;
    const char *text;
} osd_data_t;

void osd_render(cairo_t *cr, void *user_data);
int osd_calculate_thickness(osd_data_t *rects_data);
int osd_get_mouse_monitor_dimension(osd_data_t *rects_data);
int osd_run(const char *text, int duration);

#endif // OSD_H