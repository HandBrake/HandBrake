/* preview.c
 *
 * Copyright (C) 2008-2025 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "preview.h"

#include "application.h"
#include "callbacks.h"
#include "handbrake/handbrake.h"
#include "hb-backend.h"
#include "jobdict.h"
#include "presets.h"
#include "title-add.h"
#include "values.h"

#include <unistd.h>

enum {
    PREVIEW_STATE_STOPPED = 0,
    PREVIEW_STATE_ENCODING,
    PREVIEW_STATE_PLAYING,
    PREVIEW_STATE_PAUSED,
};

struct preview_s
{
    gboolean    seek_lock;
    gboolean    progress_lock;
    gint        width;
    gint        height;
    gint        render_width;
    gint        render_height;
    GtkWidget  *view;
    GdkPixbuf  *pix;
    gint        button_width;
    gint        button_height;
    gint        frame;
    gint        state;
    gboolean    pause;
    gboolean    encoded[GHB_PREVIEW_MAX];
    gint        encode_frame;
    gint        live_id;
    gchar      *current;
    gint        live_enabled;
    GtkMediaStream *video;
};

static void live_preview_progress_cb(GtkMediaStream *video, GParamSpec *spec, signal_user_data_t *ud);

static void
get_display_size (GtkWidget *widget, int *w, int *h)
{
    *w = *h = 0;

    GdkDisplay *display = gtk_widget_get_display(widget);
    GtkRoot    *root    = gtk_widget_get_root(widget);
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(root));

    if (surface != NULL && display != NULL)
    {
        GdkMonitor * monitor;

        monitor = gdk_display_get_monitor_at_surface(display, surface);
        if (monitor != NULL)
        {
            GdkRectangle rect;

            gdk_monitor_get_geometry(monitor, &rect);
            *w = rect.width;
            *h = rect.height;
        }
    }
}

static void
screen_par (signal_user_data_t *ud, gint *par_n, gint *par_d)
{
    // Assume 1:1
    // I could get it from GtkWindow->GdkScreen monitor methods.
    // But it's going to be 1:1 anyway, so why bother.
    *par_n = 1;
    *par_d = 1;
}

void
ghb_par_scale(signal_user_data_t *ud, gint *width, gint *height, gint par_n, gint par_d)
{
    gint disp_par_n, disp_par_d;
    gint64 num, den;

    screen_par(ud, &disp_par_n, &disp_par_d);
    if (disp_par_n < 1 || disp_par_d < 1)
    {
        disp_par_n = 1;
        disp_par_d = 1;
    }
    num = par_n * disp_par_d;
    den = par_d * disp_par_n;

    if (par_n > par_d)
        *width = *width * num / den;
    else
        *height = *height * den / num;
}

static int
live_preview_get_state (void)
{
    signal_user_data_t *ud = ghb_ud();

    if (!ud || !ud->preview)
        return PREVIEW_STATE_STOPPED;

    return ud->preview->state;
}

/* Returns TRUE if the preview video is loaded. */
static gboolean
live_preview_is_ready (void)
{
    int state = live_preview_get_state();
    return (state == PREVIEW_STATE_PLAYING || state == PREVIEW_STATE_PAUSED);
}

static void
live_preview_set_state (int state)
{
    signal_user_data_t *ud = ghb_ud();

    if (!ud || !ud->preview)
        return;

    const char *icon_name, *tooltip;

    switch (state)
    {
        case PREVIEW_STATE_PLAYING:
            icon_name = "media-playback-pause-symbolic";
            tooltip = _("Pause Preview");
            break;
        case PREVIEW_STATE_ENCODING:
            icon_name = "media-playback-stop-symbolic";
            tooltip = _("Stop Encoding Preview");
            break;
        case PREVIEW_STATE_PAUSED:
            icon_name = "media-playback-start-symbolic";
            tooltip = _("Play Preview");
            break;
        case PREVIEW_STATE_STOPPED:
        default:
            icon_name = "media-playback-start-symbolic";
            tooltip = _("Encode and play a short sequence of video starting from the current preview position.");
            break;
    }

    GtkWidget *play_button = ghb_builder_widget("live_preview_play");
    gtk_widget_set_tooltip_text(play_button, tooltip);
    gtk_button_set_icon_name(GTK_BUTTON(play_button), icon_name);

    ud->preview->state = state;
}

static void
preview_set_render_size(signal_user_data_t *ud, int width, int height)
{
    GtkWidget     * widget, *frame, *reset;
    GtkWindow     * window;
    gint            s_w, s_h;
    gint            factor;
    gfloat          ratio = 1.0;

    window = GTK_WINDOW(ghb_builder_widget("preview_window"));
    widget = ghb_builder_widget("preview_image");
    frame = ghb_builder_widget("preview_image_frame");

    if (ghb_dict_get_bool(ud->prefs, "reduce_hd_preview"))
        factor = 90;
    else
        factor = 100;

    get_display_size(ghb_builder_widget("hb_window"), &s_w, &s_h);

    if (s_w > 0 && s_h > 0)
    {
        int orig_w = width;
        int orig_h = height;

        if (width > s_w * factor / 100)
        {
            width = s_w * factor / 100;
            height = height * width / orig_w;
        }
        if (height > s_h * factor / 100)
        {
            height = s_h * factor / 100;
            width = orig_w * height / orig_h;
        }
    }
    if (height && width)
        ratio = (gfloat) width / height;

    gtk_widget_set_size_request(widget, width, height);
    gtk_aspect_frame_set_xalign(GTK_ASPECT_FRAME(frame), 0.5);
    gtk_aspect_frame_set_yalign(GTK_ASPECT_FRAME(frame), 0.5);
    gtk_aspect_frame_set_ratio(GTK_ASPECT_FRAME(frame), ratio);
    gtk_aspect_frame_set_obey_child(GTK_ASPECT_FRAME(frame), FALSE);

    if (gtk_window_is_fullscreen(window))
    {
        reset = ghb_builder_widget("preview_reset");
        gtk_widget_hide(reset);
    }
    else
    {
        gtk_window_unmaximize(window);
        gtk_window_set_default_size(window, width, -1);
    }
    gtk_widget_set_size_request(widget, -1, -1);

    ud->preview->render_width = width;
    ud->preview->render_height = height;
}

static void
preview_set_size(signal_user_data_t *ud, int width, int height)
{
    if (height == ud->preview->width &&
        width == ud->preview->height)
    {
        // Rotation happened, fix up render size
        preview_set_render_size(ud, ud->preview->render_height,
                                ud->preview->render_width);
    }
    else if (width != ud->preview->width ||
             height != ud->preview->height)
    {
        preview_set_render_size(ud, width, height);
    }
    ud->preview->width = width;
    ud->preview->height = height;
}

static void
live_preview_play (signal_user_data_t *ud)
{
    live_preview_set_state(PREVIEW_STATE_PLAYING);
    gtk_media_stream_play(ud->preview->video);
}

static void
live_preview_pause (signal_user_data_t *ud)
{
    live_preview_set_state(PREVIEW_STATE_PAUSED);
    gtk_media_stream_pause(ud->preview->video);
}

static void
live_preview_ended_cb (GtkMediaStream *video, GParamSpec *spec, signal_user_data_t *ud)
{
    if (gtk_media_stream_get_ended(video))
        live_preview_pause(ud);
}

static void
live_preview_error_cb (GtkMediaStream *video, GParamSpec *spec, signal_user_data_t *ud)
{
    const GError *error = gtk_media_stream_get_error(video);
    if (error)
    {
        // Display an error dialog
        GtkWindow *window = GTK_WINDOW(ghb_builder_widget("preview_window"));
        GtkWidget *dialog = gtk_message_dialog_new(window, GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, _("Playback Error"));
        // TODO: Make this message more useful
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
            _("The video could not be played. For more details, see the Activity Log."));

        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_widget_set_visible(dialog, TRUE);
        // Also log the error to the activity log
        g_warning("%s", error->message);
    }
}

void
ghb_preview_init(signal_user_data_t *ud)
{
    ud->preview               = g_malloc0(sizeof(preview_t));
    ud->preview->encode_frame = -1;
    ud->preview->live_id      = -1;

    ud->preview->video = gtk_media_file_new();
    g_signal_connect(ud->preview->video, "notify::timestamp", G_CALLBACK(live_preview_progress_cb), ud);
    g_signal_connect(ud->preview->video, "notify::ended", G_CALLBACK(live_preview_ended_cb), ud);
    g_signal_connect(ud->preview->video, "notify::error", G_CALLBACK(live_preview_error_cb), ud);
    ud->preview->live_enabled = 1;
}

void
ghb_preview_cleanup(signal_user_data_t *ud)
{
    if (ud->preview->current)
    {
        g_free(ud->preview->current);
        ud->preview->current = NULL;
    }
}

static void
live_preview_start_new (signal_user_data_t *ud)
{
    if (!ud->preview->live_enabled)
        return;

    if (!ud->preview->encoded[ud->preview->frame])
    {
        gtk_media_stream_pause(ud->preview->video);
    }
    else
    {
        GtkPicture *preview_image = GTK_PICTURE(ghb_builder_widget("preview_image"));
        gtk_media_file_set_filename(GTK_MEDIA_FILE(ud->preview->video), ud->preview->current);
        gtk_picture_set_paintable(preview_image, GDK_PAINTABLE(ud->preview->video));
        live_preview_play(ud);
    }
}

static void
live_preview_toggle_playback (signal_user_data_t *ud)
{
    if (!ud->preview->live_enabled)
        return;

    if (!gtk_media_file_get_file(GTK_MEDIA_FILE(ud->preview->video)))
    {
        live_preview_start_new(ud);
    }
    else if (!gtk_media_stream_get_playing(ud->preview->video))
    {
        live_preview_play(ud);
    }
    else
    {
        live_preview_pause(ud);
    }
}

static void
live_preview_stop (void)
{
    GtkRange *progress;
    signal_user_data_t *ud = ghb_ud();

    if (!ud->preview->live_enabled)
        return;

    live_preview_pause(ud);
    gtk_media_file_clear(GTK_MEDIA_FILE(ud->preview->video));
    live_preview_set_state(PREVIEW_STATE_STOPPED);

    progress = GTK_RANGE(ghb_builder_widget("live_preview_progress"));
    gtk_range_set_value(progress, 0);
}

void
ghb_live_reset(signal_user_data_t *ud)
{
    gboolean encoded;

    if (ud->preview->live_id >= 0)
    {
        ghb_stop_live_encode();
    }
    ud->preview->live_id = -1;
    ud->preview->encode_frame = -1;
    live_preview_stop();
    if (ud->preview->current)
    {
        g_free(ud->preview->current);
        ud->preview->current = NULL;
    }
    encoded = ud->preview->encoded[ud->preview->frame];
    memset(ud->preview->encoded, 0, sizeof(gboolean) * GHB_PREVIEW_MAX);
    if (encoded)
        ghb_set_preview_image(ud);
}

G_MODULE_EXPORT void
live_preview_play_clicked_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    gint frame = ud->preview->frame;
    const char *tmp_dir = ghb_get_tmp_dir();
    char *name = g_strdup_printf("%s/live%02d", tmp_dir, ud->preview->frame);
    if (ud->preview->current)
        g_free(ud->preview->current);
    ud->preview->current = name;

    if (ud->preview->encoded[frame] &&
        g_file_test(name, G_FILE_TEST_IS_REGULAR))
    {
        live_preview_toggle_playback(ud);
    }
    else if (ud->preview->live_id < 0)
    {
        GhbValue *js;
        GhbValue *range, *dest;

        ud->preview->encode_frame = frame;
        js = ghb_value_dup(ud->settings);

        ghb_finalize_job(js);
        range = ghb_get_job_range_settings(js);
        dest = ghb_get_job_dest_settings(js);

        ghb_dict_set_string(dest, "File", name);
        ghb_dict_set_string(range, "Type", "preview");
        ghb_dict_set_int(range, "Start", ud->preview->frame + 1);
        ghb_dict_set_int(range, "End",
            ghb_dict_get_int(ud->prefs, "live_duration") * 90000);
        ghb_dict_set_int(range, "SeekPoints",
            ghb_dict_get_int(ud->prefs, "preview_count"));

        GhbValue *job_dict = ghb_dict_get(js, "Job");
        ud->preview->live_id = ghb_add_job(ghb_live_handle(), job_dict);
        ghb_start_live_encode();
        ghb_value_free(&js);

        live_preview_set_state(PREVIEW_STATE_ENCODING);
    }
    else
    {
        // An encode is running, stop it
        ghb_stop_live_encode();
        ud->preview->live_id = -1;
        ud->preview->encode_frame = -1;
        live_preview_set_state(PREVIEW_STATE_STOPPED);
    }
}

void
ghb_live_encode_done(signal_user_data_t *ud, gboolean success)
{
    GtkWidget *widget;
    GtkWidget *prog;

    ud->preview->live_id = -1;
    prog = ghb_builder_widget("live_encode_progress");
    if (success &&
        ud->preview->encode_frame == ud->preview->frame)
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), "Done");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog), 1);
        ud->preview->encoded[ud->preview->encode_frame] = TRUE;
        live_preview_start_new(ud);
        widget = ghb_builder_widget("live_encode_progress");
        gtk_widget_hide (widget);
        widget = ghb_builder_widget("live_preview_progress");
        gtk_widget_show (widget);
    }
    else
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog), 0);
        ud->preview->encoded[ud->preview->encode_frame] = FALSE;
        live_preview_set_state(PREVIEW_STATE_STOPPED);
    }
}

G_MODULE_EXPORT gboolean
unlock_progress_cb(signal_user_data_t *ud)
{
    ud->preview->progress_lock = FALSE;
    return G_SOURCE_REMOVE;
}

static void
live_preview_progress_cb (GtkMediaStream *video, GParamSpec *spec, signal_user_data_t *ud)
{
    int64_t len, pos;

    if (!live_preview_is_ready() || ud->preview->seek_lock)
        return;

    ud->preview->progress_lock = TRUE;

    len = gtk_media_stream_get_duration(video);
    pos = gtk_media_stream_get_timestamp(video);

    if (len > 0)
    {
        double percent = (double) pos * 100 / len;
        GtkRange *progress = GTK_RANGE(ghb_builder_widget("live_preview_progress"));
        gtk_range_set_value(progress, percent);
    }
    g_idle_add((GSourceFunc)unlock_progress_cb, ud);
}

G_MODULE_EXPORT gboolean
unlock_seek_cb(signal_user_data_t *ud)
{
    ud->preview->seek_lock = FALSE;
    return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT void
live_preview_seek_cb (GtkWidget *widget, gpointer data)
{
    double dval;
    int64_t len, pos;
    signal_user_data_t *ud = ghb_ud();

    if (!live_preview_is_ready() || ud->preview->progress_lock)
        return;

    ud->preview->seek_lock = TRUE;

    len = gtk_media_stream_get_duration(GTK_MEDIA_STREAM(ud->preview->video));
    dval = gtk_range_get_value(GTK_RANGE(widget));
    pos = (int64_t) (len * dval / 100);
    gtk_media_stream_seek(GTK_MEDIA_STREAM(ud->preview->video), pos);

    g_idle_add((GSourceFunc)unlock_seek_cb, ud);
}

G_MODULE_EXPORT void
preview_fullscreen_action_cb(GSimpleAction *action, GVariant *param,
                             signal_user_data_t *ud)
{
    gboolean state = g_variant_get_boolean(param);
    GtkWindow *window = GTK_WINDOW(ghb_builder_widget("preview_window"));

    if (gtk_window_is_fullscreen(window) != state)
        g_simple_action_set_state(action, param);

    if (!gtk_window_is_fullscreen(window))
    {
        gtk_window_fullscreen(window);
    }
    else
    {
        gtk_window_unfullscreen(window);
    }
}

static void
set_preview_image_static (signal_user_data_t *ud, GdkPixbuf * pix)
{
    if (pix && !live_preview_is_ready())
    {
        GtkWidget *widget = ghb_builder_widget("summary_preview_image");
        gtk_picture_set_pixbuf(GTK_PICTURE(widget), pix);
        widget = ghb_builder_widget("preview_image");
        gtk_picture_set_pixbuf(GTK_PICTURE(widget), pix);
    }
}

static void
init_preview_image(signal_user_data_t *ud)
{
    GtkWidget *widget;

    live_preview_stop();

    widget = ghb_builder_widget("preview_frame");
    ud->preview->frame = ghb_widget_int(widget) - 1;
    if (ud->preview->encoded[ud->preview->frame])
    {
        widget = ghb_builder_widget("live_encode_progress");
        gtk_widget_hide (widget);
        widget = ghb_builder_widget("live_preview_progress");
        gtk_widget_show (widget);
    }
    else
    {
        widget = ghb_builder_widget("live_preview_progress");
        gtk_widget_hide (widget);
        widget = ghb_builder_widget("live_encode_progress");
        gtk_widget_show (widget);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widget), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(widget), 0);
    }
    if (ud->preview->pix != NULL)
        g_object_unref(ud->preview->pix);

    ud->preview->pix = ghb_get_preview_image(ud->preview->frame, ud);

    if (ud->preview->pix != NULL)
    {
        int pix_width, pix_height;
        pix_width  = gdk_pixbuf_get_width(ud->preview->pix);
        pix_height = gdk_pixbuf_get_height(ud->preview->pix);
        preview_set_size(ud, pix_width, pix_height);
    }
}

void
ghb_set_preview_image(signal_user_data_t *ud)
{
    init_preview_image(ud);

    // Display the preview
    set_preview_image_static(ud, ud->preview->pix);
}

void
ghb_reset_preview_image(signal_user_data_t *ud)
{
    init_preview_image(ud);
    if (ud->preview->width > 0 && ud->preview->height > 0)
    {
        preview_set_render_size(ud, ud->preview->width, ud->preview->height);

        // Display the preview
        set_preview_image_static(ud, ud->preview->pix);
    }
}

G_MODULE_EXPORT void
show_preview_action_cb(GSimpleAction *action, GVariant *value,
                       signal_user_data_t *ud)
{
    GtkWidget *widget;
#if 0
    gint title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    visible &= title != NULL;
#endif
    widget = ghb_builder_widget("preview_window");
    gtk_window_present(GTK_WINDOW(widget));
}

G_MODULE_EXPORT void
preview_reset_clicked_cb (GtkWidget *toggle, gpointer data)
{
    ghb_log_func();
    signal_user_data_t *ud = ghb_ud();

    if (ud->preview->width > 0 && ud->preview->height > 0)
    {
        preview_set_render_size(ud, ud->preview->width, ud->preview->height);

        // On windows, preview_resize_cb does not get called when the size
        // is reset above.  So assume it got reset and disable the
        // "Source Resolution" button.
        GtkWidget * widget = ghb_builder_widget("preview_reset");
        gtk_widget_hide(widget);
    }
}

G_MODULE_EXPORT void
preview_frame_value_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    if (ud->preview->live_id >= 0)
    {
        ghb_stop_live_encode();
        ud->preview->live_id = -1;
        ud->preview->encode_frame = -1;
    }
    ghb_set_preview_image(ud);
}

G_MODULE_EXPORT gboolean
preview_close_request_cb (GtkWidget *widget, gpointer data)
{
    live_preview_stop();
    gtk_widget_set_visible(widget, FALSE);
    return TRUE;
}

G_MODULE_EXPORT void
preview_duration_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_log_func();
    ghb_live_reset(ud);
    ghb_widget_to_setting (ud->prefs, widget);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_save(ud->prefs, name);
}

static guint hud_timeout_id = 0;
static guint hud_fade_id = 0;

static gboolean in_hud = FALSE;

static void
cancel_source_function (guint id)
{
    if (id != 0)
    {
        GSource *source = g_main_context_find_source_by_id(g_main_context_default(), id);
        if (source != NULL)
        {
            g_source_destroy(source);
        }
    }
}

static gboolean
hud_fade_out (GtkWidget *hud)
{
    double opacity = gtk_widget_get_opacity(hud);

    if (opacity > 0.0)
    {
        gtk_widget_set_opacity(hud, opacity - 0.0625);
        return G_SOURCE_CONTINUE;
    }
    else
    {
        gtk_widget_set_visible(hud, FALSE);
        hud_fade_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static gboolean
hud_fade_in (GtkWidget *hud)
{
    double opacity = gtk_widget_get_opacity(hud);
    gtk_widget_set_visible(hud, TRUE);

    if (opacity < 1.0)
    {
        gtk_widget_set_opacity(hud, opacity + 0.0625);
        return G_SOURCE_CONTINUE;
    }
    else
    {
        hud_fade_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static gboolean
hud_timeout (signal_user_data_t *ud)
{
    ghb_log_func();
    if (live_preview_get_state() != PREVIEW_STATE_ENCODING)
    {
        GtkWidget *widget = ghb_builder_widget("preview_hud");
        cancel_source_function(hud_fade_id);
        hud_fade_id = g_timeout_add(16, (GSourceFunc)hud_fade_out, widget);
        hud_timeout_id = 0;
        return G_SOURCE_REMOVE;
    }
    else
    {
        return G_SOURCE_CONTINUE;
    }
}

G_MODULE_EXPORT void
hud_enter_cb (GtkEventControllerMotion *econ, double x, double y, gpointer data)
{
    GtkWidget *hud = ghb_builder_widget("preview_hud");

    cancel_source_function(hud_timeout_id);
    hud_timeout_id = 0;
    cancel_source_function(hud_fade_id);
    hud_fade_id = g_timeout_add(16, (GSourceFunc)hud_fade_in, hud);
    in_hud = TRUE;
}

G_MODULE_EXPORT void
hud_leave_cb (GtkEventControllerMotion *econ, gpointer data)
{
    in_hud = FALSE;
}

G_MODULE_EXPORT void
preview_click_cb (GtkGesture *gest, int n_press, double x, double y, gpointer data)
{
    if (n_press == 2)
        g_action_activate(GHB_ACTION("preview-fullscreen"), NULL);
}

G_MODULE_EXPORT void
preview_leave_cb (GtkEventControllerMotion *econ, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();

    cancel_source_function(hud_timeout_id);
    hud_timeout_id = g_timeout_add(500, (GSourceFunc)hud_timeout, ud);
}

G_MODULE_EXPORT void
preview_motion_cb (GtkEventControllerMotion *econ, double x, double y,
                   gpointer data)
{
    GtkWidget * hud;
    signal_user_data_t *ud = ghb_ud();

    cancel_source_function(hud_timeout_id);
    hud_timeout_id = 0;
    hud = ghb_builder_widget("preview_hud");
    if (!gtk_widget_is_visible(hud))
    {
        gtk_widget_set_visible(hud, TRUE);
        cancel_source_function(hud_fade_id);
        hud_fade_id = g_timeout_add(16, (GSourceFunc)hud_fade_in, hud);
    }

    if (!in_hud)
    {
        hud_timeout_id = g_timeout_add_seconds(4, (GSourceFunc)hud_timeout, ud);
    }
}

G_MODULE_EXPORT void
preview_notify_fullscreen_cb (GtkWindow *window, GParamSpec *pspec, gpointer data)
{
    gboolean is_fullscreen = gtk_window_is_fullscreen(window);

    GtkWidget *widget = ghb_builder_widget("live_preview_fullscreen");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), is_fullscreen);
    gtk_button_set_icon_name(GTK_BUTTON(widget), is_fullscreen ?
                             "view-restore-symbolic" : "view-fullscreen-symbolic");
}

void
ghb_preview_dispose (signal_user_data_t *ud)
{
    if (!ud || !ud->preview)
        return;
    if (ud->preview->pix)
        g_object_unref(ud->preview->pix);
    if (ud->preview->video)
    {
        g_signal_handlers_disconnect_by_func(ud->preview->video, live_preview_ended_cb, ud);
        g_signal_handlers_disconnect_by_func(ud->preview->video, live_preview_progress_cb, ud);
        g_signal_handlers_disconnect_by_func(ud->preview->video, live_preview_error_cb, ud);
        g_clear_object(&ud->preview->video);
    }
    g_free(ud->preview);
}
