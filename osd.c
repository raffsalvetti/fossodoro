#include "osd.h"

void osd_render(cairo_t *cr, void *user_data)
{
    osd_data_t *data = (osd_data_t*)user_data;

    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1);
    cairo_set_line_width(cr, data->thickness);

    // top line
    cairo_move_to(cr, 0, data->thickness/2.0);
    cairo_line_to(cr, data->width, data->thickness/2.0);

    // bottom line
    cairo_move_to(cr, 0, data->height - data->thickness/2.0);
    cairo_line_to(cr, data->width, data->height - data->thickness/2.0);

    // left line
    cairo_move_to(cr, data->thickness/2.0, 0);
    cairo_line_to(cr, data->thickness/2.0, data->height);

    // right line
    cairo_move_to(cr, data->width - data->thickness/2.0, 0);
    cairo_line_to(cr, data->width - data->thickness/2.0, data->height);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10 * data->thickness);
    
    //const char *text = "POMODORO!";
    cairo_text_extents_t extents;
    cairo_text_extents(cr, data->text, &extents);

    double x = (data->width - extents.width) / 2 - extents.x_bearing;
    double y = (data->height - extents.height) / 2 - extents.y_bearing;

    cairo_move_to(cr, x, y);

    cairo_show_text(cr, data->text);

    cairo_stroke(cr);
}

int osd_calculate_thickness(osd_data_t *rects_data) {
    // use smaller dimension for consistency
    int min_dim = (rects_data->width < rects_data->height) 
        ? rects_data->width 
        : rects_data->height;

    // uake thickness ~0.75% of smaller dimension
    rects_data->thickness = min_dim / 175;

    // Ensure at least 1 pixel
    if (rects_data->thickness < 1) rects_data->thickness = 1;

    return rects_data->thickness;
}

int osd_get_mouse_monitor_dimension(osd_data_t *rects_data) {

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Unable to open X display\n");
        return 1;
    }

    Window root = DefaultRootWindow(dpy);

    int monitor_cnt;
    XRRMonitorInfo *monitors = XRRGetMonitors(dpy, root, True, &monitor_cnt);
    if (!monitors) return -1;

    Window ret_root, ret_child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(dpy, root, &ret_root, &ret_child, &root_x, &root_y, &win_x, &win_y, &mask);

    int resut = -1;

    for (int i = 0; i < monitor_cnt; ++i) {
        int mx = monitors[i].x;
        int my = monitors[i].y;
        int mw = monitors[i].width;
        int mh = monitors[i].height;
        if (root_x >= mx && root_x < mx + mw &&
            root_y >= my && root_y < my + mh) {
            rects_data->width = mw;
            rects_data->height = mh;
            resut = i;
            break;
        }
    }

    XRRFreeMonitors(monitors);
    XCloseDisplay(dpy);
    return resut;
}

int osd_run(const char *text, int duration) {
    Aosd* aosd = aosd_new();
    if (!aosd) {
        fprintf(stderr, "Failed to create aosd object\n");
        return 1;
    }

    osd_data_t rects_data;

    int i = osd_get_mouse_monitor_dimension(&rects_data);
    if (i < 0) {
        fprintf(stderr, "Failed to get monitor\n");
        return 1;
    }

    rects_data.text = text;

    osd_calculate_thickness(&rects_data);

    aosd_set_transparency(aosd, TRANSPARENCY_COMPOSITE);
    aosd_set_hide_upon_mouse_event(aosd, 0);
    aosd_set_position(aosd, i > 0 ? 2 : 0, rects_data.width, rects_data.height);
    aosd_set_renderer(aosd, osd_render, &rects_data);
    
    aosd_show(aosd);
    aosd_loop_for(aosd, duration * 1000);
    
    aosd_destroy(aosd);
}