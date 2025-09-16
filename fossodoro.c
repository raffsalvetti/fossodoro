#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "osd.h"
#include "sound.h"

#ifndef DATADIR
#define DATADIR                 "../share/fossodoro/"
#endif

#define GETTEXT_PACKAGE         "fossodoro"
#include <locale.h>
#include <glib/gi18n.h>

#define DEFAULT_ICON            DATADIR "icons/100.svg"
#define ICON_100                DATADIR "icons/100.svg"
#define ICON_090                DATADIR "icons/90.svg"
#define ICON_080                DATADIR "icons/80.svg"
#define ICON_070                DATADIR "icons/70.svg"
#define ICON_060                DATADIR "icons/60.svg"
#define ICON_050                DATADIR "icons/50.svg"
#define ICON_040                DATADIR "icons/40.svg"
#define ICON_030                DATADIR "icons/30.svg"
#define ICON_020                DATADIR "icons/20.svg"
#define ICON_010                DATADIR "icons/10.svg"
#define ICON_000                DATADIR "icons/0.svg"
#define ICON_OFF                DATADIR "icons/off.svg"
#define ICON_BREAK              DATADIR "icons/break.svg"
#define ICON_PLAY               DATADIR "icons/play.svg"
#define ICON_PAUSE              DATADIR "icons/pause.svg"

#define DEFAULT_DING_FILE       DATADIR "sounds/ding2.mp3"

#define CONFIG_FILE             "fossodoro.cfg"


typedef enum {
    MODE_POMODORO,
    MODE_SHORT_BREAK,
    MODE_LONG_BREAK
} TimerMode;

typedef enum {
    POMO_ICON_0,
    POMO_ICON_10,
    POMO_ICON_20,
    POMO_ICON_30,
    POMO_ICON_40,
    POMO_ICON_50,
    POMO_ICON_60,
    POMO_ICON_70,
    POMO_ICON_80,
    POMO_ICON_100,
} T_Pomo_Icon;

typedef struct {
    guint            timer_id;
    int              remaining_seconds;
    int              pomodoro_duration;
    int              break_duration;
    int              long_break_duration;
    int              pomodoros_before_long;
    int              current_pomodoro_count;
    TimerMode        current_mode;
    gboolean         timer_active;
    gboolean         timer_paused;
    gboolean         always_on_top_enabled;
    int              volume_level;
    int              notification_delay;
    const char       *current_icon;
} AppData;

static AppData app = {0};

static char *config_path;


GtkStatusIcon   *tray_icon;
GtkWidget       *config_window;
GtkWidget       *always_on_top_window;
GtkWidget       *always_on_top_icon;
GtkWidget       *always_on_top_label;
GtkWidget       *always_on_top_chronometer;
GtkWidget       *play_pause_button;
GtkWidget       *stop_button;


static gboolean timer_callback();
static void update_always_on_top_label();
static void create_chronometer_floating_window();
static void create_config_window();
static void update_play_pause_icon();
static gboolean on_always_on_top_button_press(GtkWidget *widget, GdkEventButton *event);
static void load_config();
static void save_config();
static const char* select_icon();
static void on_play_pause_button_clicked();
static const char *get_current_mode_string();
static char *get_config_path();

static const char* select_icon() {
    int val = 0, duration = 0; 
    if(app.timer_active) {
        if(app.timer_paused) return ICON_PAUSE;

        duration = (app.current_mode == MODE_POMODORO ? app.pomodoro_duration :
        app.current_mode == MODE_SHORT_BREAK ? app.break_duration : app.long_break_duration);

        val = app.remaining_seconds * 100 / duration;
        if(val > 90) return ICON_100;
        if(val > 80) return ICON_090;
        if(val > 70) return ICON_080;
        if(val > 60) return ICON_070;
        if(val > 50) return ICON_060;
        if(val > 40) return ICON_050;
        if(val > 30) return ICON_040;
        if(val > 20) return ICON_030;
        if(val > 10) return ICON_020;
        if(val > 5) return ICON_010;
        return ICON_000;
    } else {
        return DEFAULT_ICON;
    }
}

static const char *get_current_mode_string() {
    switch(app.current_mode) {
        case MODE_POMODORO:
            return _("Pomodoro");
        case MODE_SHORT_BREAK:
            return _("Short Break");
        default:
            return _("Long Break");
    }
}

static char *get_config_path() {
    if(config_path != NULL) return config_path;
    const char *home = (char*)g_get_home_dir(); const char *cfg = "/.config/";
    int cp_len = (strlen(home) + strlen(cfg) + strlen(CONFIG_FILE)) + 1;
    config_path = (char*) malloc(cp_len + sizeof(char));
    sprintf(config_path, "%s%s%s", home, cfg, CONFIG_FILE);
    return config_path;
}

static void load_config() {
    FILE *f = fopen(get_config_path(), "r");
    if (!f) {
        app.pomodoro_duration       = 25 * 60;
        app.break_duration          = 5 * 60;
        app.long_break_duration     = 15 * 60;
        app.pomodoros_before_long   = 4;
        app.volume_level            = 100;
        app.notification_delay      = 10;
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        char key[128];
        int value;
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "pomodoro_duration") == 0)
                app.pomodoro_duration = value * 60;
            else if (strcmp(key, "break_duration") == 0)
                app.break_duration = value * 60;
            else if (strcmp(key, "long_break_duration") == 0)
                app.long_break_duration = value * 60;
            else if (strcmp(key, "pomodoros_before_long") == 0)
                app.pomodoros_before_long = value;
            else if (strcmp(key, "volume_level") == 0)
                app.volume_level = value;
            else if (strcmp(key, "notification_delay") == 0)
                app.notification_delay = value;
        }
    }
    fclose(f);
}

static void save_config() {
    FILE *f = fopen(get_config_path(), "w");
    if (!f)
        return;
    fprintf(f, "pomodoro_duration=%d\n", app.pomodoro_duration / 60);
    fprintf(f, "break_duration=%d\n", app.break_duration / 60);
    fprintf(f, "long_break_duration=%d\n", app.long_break_duration / 60);
    fprintf(f, "pomodoros_before_long=%d\n", app.pomodoros_before_long);
    fprintf(f, "volume_level=%d\n", app.volume_level);
    fprintf(f, "notification_delay=%d\n", app.notification_delay);
    fclose(f);
}

static void show_notification(const char *title, const char *message) {
    NotifyNotification *notification = notify_notification_new(title, message, NULL);
    notify_notification_set_timeout(notification, app.notification_delay * 1000);
    notify_notification_show(notification, NULL);
    g_object_unref(G_OBJECT(notification));

    osd_run(message, 2);
}

static void update_application_icon() {
    const char *icon = select_icon();
    if(strcmp(icon, app.current_icon) != 0) {
        GIcon *old_icon;
        app.current_icon = icon;
        old_icon = gtk_status_icon_get_gicon(tray_icon);
        gtk_status_icon_set_from_file(tray_icon, app.current_icon);
        g_free(old_icon);
    }
}

static gboolean timer_callback() {
    if (app.remaining_seconds > 0)
        app.remaining_seconds--;

    int minutes = app.remaining_seconds / 60;
    int seconds = app.remaining_seconds % 60;
    char tooltip[128];
    const char *mode_str = get_current_mode_string();
    snprintf(tooltip, sizeof(tooltip), "%s - %02d:%02d remaining", mode_str, minutes, seconds);
    gtk_status_icon_set_tooltip_text(tray_icon, tooltip);

    if (app.always_on_top_enabled && always_on_top_chronometer)
        update_always_on_top_label();

    if (app.remaining_seconds < 1) {

        if (app.volume_level > 0) {
            sound_play_data_t sound_play_data;
            sound_play_data.audio_file = DEFAULT_DING_FILE;
            sound_play_data.volume = app.volume_level / 100.0;
            
            pthread_t play_audio_thread;
            pthread_create(&play_audio_thread, NULL, sound_play, &sound_play_data);
        }

        if (app.current_mode == MODE_POMODORO) {
            show_notification(_("Pomodoro Timer"), _("Pomodoro session ended!"));

            app.current_pomodoro_count++;
            if (app.current_pomodoro_count >= app.pomodoros_before_long) {
                app.current_mode = MODE_LONG_BREAK;
                app.remaining_seconds = app.long_break_duration;
                app.current_pomodoro_count = 0;
            } else {
                app.current_mode = MODE_SHORT_BREAK;
                app.remaining_seconds = app.break_duration;
            }
        } else {
            show_notification(_("Pomodoro Timer"), _("Break ended! Unpause to continue."));

            app.current_mode = MODE_POMODORO;
            app.remaining_seconds = app.pomodoro_duration;
            if(!app.timer_paused)
                on_play_pause_button_clicked();
        }
    }

    update_application_icon();

    return TRUE;
}

static void update_always_on_top_label() {
    char text[64], mode_str_label[64];
    int minutes = app.remaining_seconds / 60;
    int seconds = app.remaining_seconds % 60;
    snprintf(text, sizeof(text), "%02d:%02d", minutes, seconds);
    gtk_label_set_text(GTK_LABEL(always_on_top_chronometer), text);

    const char *mode_str = get_current_mode_string();
    snprintf(mode_str_label, sizeof(mode_str_label), "%s", mode_str);
    gtk_label_set_text(GTK_LABEL(always_on_top_label), mode_str_label);

    const char *img_path = app.timer_active ? (app.timer_paused ? ICON_PAUSE : (app.current_mode == MODE_POMODORO ? DEFAULT_ICON : ICON_BREAK)) : DEFAULT_ICON;
    GdkPixbuf *p_buf = gdk_pixbuf_new_from_file_at_size(img_path, 32, 32, NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(always_on_top_icon), p_buf);
}

static void update_play_pause_icon() {
    if (play_pause_button) {
        GtkWidget *image;
        
        if (app.timer_active && !app.timer_paused) {
            image = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
        } else {
            image = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
        }
        update_application_icon();
        update_always_on_top_label();
        gtk_button_set_image(GTK_BUTTON(play_pause_button), image);
    }
}

static void on_start_activate() {
    if (!app.timer_active) {
        app.current_mode = MODE_POMODORO;
        app.remaining_seconds = app.pomodoro_duration;
        app.timer_active = TRUE;
        app.timer_paused = FALSE;
        app.timer_id = g_timeout_add_seconds(1, timer_callback, NULL);
        update_play_pause_icon(app);
    } else {
        app.timer_paused = !app.timer_paused;
        if (app.timer_paused && app.timer_id) {
            g_source_remove(app.timer_id);
            app.timer_id = 0;
        } else {
            app.timer_id = g_timeout_add_seconds(1, timer_callback, NULL);
        }
        update_play_pause_icon(app);
    }
}

static void on_stop_activate(GtkMenuItem *menuitem) {
    if (app.timer_active) {
        if (!app.timer_paused && app.timer_id) {
            g_source_remove(app.timer_id);
            app.timer_id = 0;
        }
        app.timer_active = FALSE;
        app.timer_paused = FALSE;

        if (app.current_mode == MODE_POMODORO)
            app.remaining_seconds = app.pomodoro_duration;
        else if (app.current_mode == MODE_SHORT_BREAK)
            app.remaining_seconds = app.break_duration;
        else if (app.current_mode == MODE_LONG_BREAK)
            app.remaining_seconds = app.long_break_duration;
        update_always_on_top_label(app);
        update_play_pause_icon(app);
    }
}

static void on_quit_activate() {
    if (app.timer_active && app.timer_id)
        g_source_remove(app.timer_id);
    gtk_main_quit();
}

static void on_config_activate() {
    create_config_window();
}

static void on_toggle_always_on_top_activate() {
    app.always_on_top_enabled = !app.always_on_top_enabled;
    if (app.always_on_top_enabled)
        gtk_widget_show_all(always_on_top_window);
    else
        gtk_widget_hide(always_on_top_window);
}

static void on_stop_button_clicked() {
    if (app.timer_active) {
        if (app.timer_id) {
            g_source_remove(app.timer_id);
            app.timer_id = 0;
        }
        app.timer_active = FALSE;
        app.timer_paused = FALSE;
        if (app.current_mode == MODE_POMODORO)
            app.remaining_seconds = app.pomodoro_duration;
        else if (app.current_mode == MODE_SHORT_BREAK)
            app.remaining_seconds = app.break_duration;
        else if (app.current_mode == MODE_LONG_BREAK)
            app.remaining_seconds = app.long_break_duration;
        update_always_on_top_label(app);
        update_play_pause_icon(app);
    }
}

static void on_play_pause_button_clicked() {
    if (!app.timer_active) {
        app.current_mode = MODE_POMODORO;
        app.remaining_seconds = app.pomodoro_duration;
        app.timer_active = TRUE;
        app.timer_paused = FALSE;
        app.timer_id = g_timeout_add_seconds(1, timer_callback, NULL);
    } else if (app.timer_active && !app.timer_paused) {
        if (app.timer_id) {
            g_source_remove(app.timer_id);
            app.timer_id = 0;
        }
        app.timer_paused = TRUE;
    } else if (app.timer_active && app.timer_paused) {
        app.timer_paused = FALSE;
        app.timer_id = g_timeout_add_seconds(1, timer_callback, NULL);
    }
    update_play_pause_icon(app);
}

static gboolean on_always_on_top_button_press(GtkWidget *widget, GdkEventButton *event) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        gtk_window_begin_move_drag(GTK_WINDOW(widget),
                                     event->button,
                                     event->x_root,
                                     event->y_root,
                                     event->time);
        return TRUE;
    }
    return FALSE;
}

static void on_stop_button_clicked_confirm() {
    if (!app.timer_active) return;
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(NULL),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", _("Are you sure you want to stop the timer?")
    );
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response != GTK_RESPONSE_YES)
        return;

    on_stop_button_clicked();
}

static void create_chronometer_floating_window() {
    if (!always_on_top_window) {
        always_on_top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_icon_from_file(GTK_WINDOW(always_on_top_window), DEFAULT_ICON, NULL);
        gtk_window_set_title(GTK_WINDOW(always_on_top_window), _("Pomodoro Timer"));
        gtk_window_set_default_size(GTK_WINDOW(always_on_top_window), 280, 16);
        gtk_window_set_keep_above(GTK_WINDOW(always_on_top_window), TRUE);
        gtk_window_set_decorated(GTK_WINDOW(always_on_top_window), FALSE);
        gtk_window_set_resizable(GTK_WINDOW(always_on_top_window), FALSE);
        gtk_window_stick(GTK_WINDOW(always_on_top_window));

        gtk_widget_add_events(always_on_top_window, GDK_BUTTON_PRESS_MASK);
        g_signal_connect(always_on_top_window, "button-press-event", G_CALLBACK(on_always_on_top_button_press), NULL);
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 1);
        
        GdkPixbuf *p_buf = gdk_pixbuf_new_from_file_at_size(DEFAULT_ICON, 8, 8, NULL);
        always_on_top_icon = gtk_image_new_from_pixbuf(p_buf);

        gtk_box_pack_start(GTK_BOX(hbox), always_on_top_icon, FALSE, FALSE, 0);

        // GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        // gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

        // always_on_top_label = gtk_label_new("Mode");
        // gtk_box_pack_start(GTK_BOX(vbox), always_on_top_label, TRUE, TRUE, 0);

        // always_on_top_chronometer = gtk_label_new("00:00");
        // gtk_box_pack_start(GTK_BOX(vbox), always_on_top_chronometer, TRUE, TRUE, 0);

        always_on_top_label = gtk_label_new("Mode");
        gtk_box_pack_start(GTK_BOX(hbox), always_on_top_label, TRUE, TRUE, 0);

        always_on_top_chronometer = gtk_label_new("00:00");
        gtk_box_pack_start(GTK_BOX(hbox), always_on_top_chronometer, TRUE, TRUE, 0);

        stop_button = gtk_button_new();
        GtkWidget *stop_image = gtk_image_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_button_set_image(GTK_BUTTON(stop_button), stop_image);
        gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0);
        g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_button_clicked_confirm), NULL);

        play_pause_button = gtk_button_new();
        GtkWidget *play_image = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_button_set_image(GTK_BUTTON(play_pause_button), play_image);
        gtk_box_pack_start(GTK_BOX(hbox), play_pause_button, FALSE, FALSE, 0);
        g_signal_connect(play_pause_button, "clicked", G_CALLBACK(on_play_pause_button_clicked), NULL);

        gtk_container_add(GTK_CONTAINER(always_on_top_window), hbox);
    }
    update_always_on_top_label();
}

static void on_config_save_clicked(GtkButton *button) {
    GtkWidget *pomodoro_entry = g_object_get_data(G_OBJECT(button), "pomodoro_entry");
    GtkWidget *break_entry = g_object_get_data(G_OBJECT(button), "break_entry");
    GtkWidget *long_break_entry = g_object_get_data(G_OBJECT(button), "long_break_entry");
    GtkWidget *pomodoros_count_entry = g_object_get_data(G_OBJECT(button), "pomodoros_count_entry");
    GtkWidget *volume_scale = g_object_get_data(G_OBJECT(button), "volume_scale");

    const char *pomo_text = gtk_entry_get_text(GTK_ENTRY(pomodoro_entry));
    const char *break_text = gtk_entry_get_text(GTK_ENTRY(break_entry));
    const char *long_break_text = gtk_entry_get_text(GTK_ENTRY(long_break_entry));
    const char *count_text = gtk_entry_get_text(GTK_ENTRY(pomodoros_count_entry));

    int pomo_minutes = atoi(pomo_text);
    int break_minutes = atoi(break_text);
    int long_break_minutes = atoi(long_break_text);
    int count = atoi(count_text);
    int volume = (int) gtk_range_get_value(GTK_RANGE(volume_scale));

    if (pomo_minutes < 1 || break_minutes < 1 || long_break_minutes < 1 || count < 1) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(config_window),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     _("Please enter valid positive numbers."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    app.pomodoro_duration    = pomo_minutes * 60;
    app.break_duration       = break_minutes * 60;
    app.long_break_duration  = long_break_minutes * 60;
    app.pomodoros_before_long = count;
    app.volume_level         = volume;

    save_config(app);

    gtk_widget_destroy(config_window);
    config_window = NULL;
}

static void on_config_window_destroy(GtkWidget *widget, gpointer user_data) {
    AppData *app = (AppData *) user_data;
    config_window = NULL;
}

static void create_config_window() {
    if (config_window) {
        gtk_window_present(GTK_WINDOW(config_window));
        return;
    }

    config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_from_file(GTK_WINDOW(config_window), DEFAULT_ICON, NULL);
    gtk_window_set_title(GTK_WINDOW(config_window), _("Configuration"));
    gtk_window_set_default_size(GTK_WINDOW(config_window), 300, 250);
    gtk_container_set_border_width(GTK_CONTAINER(config_window), 10);
    gtk_window_set_resizable(GTK_WINDOW(config_window), FALSE);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(config_window), grid);

    GtkWidget *pomodoro_label = gtk_label_new(_("Pomodoro Duration (minutes):"));
    GtkWidget *break_label = gtk_label_new(_("Break Duration (minutes):"));
    GtkWidget *long_break_label = gtk_label_new(_("Long Break Duration (minutes):"));
    GtkWidget *pomodoros_count_label = gtk_label_new(_("Pomodoros before Long Break:"));
    GtkWidget *volume_label = gtk_label_new(_("Volume Level:"));
    GtkWidget *notification_delay_label = gtk_label_new(_("Notification Delay (seconds):"));

    GtkWidget *pomodoro_entry = gtk_entry_new();
    GtkWidget *break_entry = gtk_entry_new();
    GtkWidget *long_break_entry = gtk_entry_new();
    GtkWidget *pomodoros_count_entry = gtk_entry_new();
    GtkWidget *volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(volume_scale), app.volume_level);
    
    GtkWidget *notification_delay_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 2, 60, 1);
    gtk_range_set_value(GTK_RANGE(notification_delay_scale), app.notification_delay);

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", app.pomodoro_duration / 60);
    gtk_entry_set_text(GTK_ENTRY(pomodoro_entry), buffer);
    snprintf(buffer, sizeof(buffer), "%d", app.break_duration / 60);
    gtk_entry_set_text(GTK_ENTRY(break_entry), buffer);
    snprintf(buffer, sizeof(buffer), "%d", app.long_break_duration / 60);
    gtk_entry_set_text(GTK_ENTRY(long_break_entry), buffer);
    snprintf(buffer, sizeof(buffer), "%d", app.pomodoros_before_long);
    gtk_entry_set_text(GTK_ENTRY(pomodoros_count_entry), buffer);

    gtk_grid_attach(GTK_GRID(grid), pomodoro_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pomodoro_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), break_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), break_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), long_break_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), long_break_entry, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pomodoros_count_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pomodoros_count_entry, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), volume_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), volume_scale, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), notification_delay_label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), notification_delay_scale, 1, 5, 1, 1);

    GtkWidget *save_button = gtk_button_new_with_label(_("Save"));
    gtk_grid_attach(GTK_GRID(grid), save_button, 0, 6, 2, 1);

    g_object_set_data(G_OBJECT(save_button), "pomodoro_entry", pomodoro_entry);
    g_object_set_data(G_OBJECT(save_button), "break_entry", break_entry);
    g_object_set_data(G_OBJECT(save_button), "long_break_entry", long_break_entry);
    g_object_set_data(G_OBJECT(save_button), "pomodoros_count_entry", pomodoros_count_entry);
    g_object_set_data(G_OBJECT(save_button), "volume_scale", volume_scale);

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_config_save_clicked), NULL);
    g_signal_connect(config_window, "destroy", G_CALLBACK(on_config_window_destroy), NULL);

    gtk_widget_show_all(config_window);
}

static gboolean on_tray_icon_button_press(GtkStatusIcon *status_icon, GdkEventButton *event) {
    if (event->button == GDK_BUTTON_PRIMARY || event->button == GDK_BUTTON_SECONDARY) {
        GtkWidget *menu = gtk_menu_new();

        GtkWidget *start_item = gtk_menu_item_new_with_label(app.timer_active && !app.timer_paused ? _("Pause") : _("Start"));
        g_signal_connect(start_item, "activate", G_CALLBACK(on_start_activate), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), start_item);
        
        if (app.timer_active) {
            GtkWidget *stop_item = gtk_menu_item_new_with_label(_("Stop"));
            g_signal_connect(stop_item, "activate", G_CALLBACK(on_stop_activate), NULL);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), stop_item);
        }

        GtkWidget *toggle_item = gtk_menu_item_new_with_label(_("Toggle Chronometer"));
        g_signal_connect(toggle_item, "activate", G_CALLBACK(on_toggle_always_on_top_activate), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_item);

        GtkWidget *config_item = gtk_menu_item_new_with_label(_("Configure"));
        g_signal_connect(config_item, "activate", G_CALLBACK(on_config_activate), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), config_item);

        GtkWidget *quit_item = gtk_menu_item_new_with_label(_("Quit"));
        g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *) event);
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, DATADIR "locale");
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    notify_init(_("Pomodoro Timer"));

    load_config();

    app.current_pomodoro_count = 0;
    app.timer_active = FALSE;
    app.timer_paused = FALSE;
    app.always_on_top_enabled = FALSE;
    app.current_icon = DEFAULT_ICON;

    tray_icon = gtk_status_icon_new_from_file(DEFAULT_ICON);
    gtk_status_icon_set_visible(tray_icon, TRUE);
    gtk_status_icon_set_tooltip_text(tray_icon, _("Pomodoro Timer"));
    g_signal_connect(tray_icon, "button-press-event", G_CALLBACK(on_tray_icon_button_press), NULL);

    create_chronometer_floating_window();

    gtk_main();

    notify_uninit();
    return 0;
}

