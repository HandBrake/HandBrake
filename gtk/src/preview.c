/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preview.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * preview.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * preview.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include "ghbcompat.h"

#if defined(_ENABLE_GST)
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/pbutils/missing-plugins.h>
#endif

#include "settings.h"
#include "jobdict.h"
#include "presets.h"
#include "callbacks.h"
#include "hb-backend.h"
#include "preview.h"
#include "values.h"
#include "queuehandler.h"
#include "handbrake/handbrake.h"

#define PREVIEW_STATE_IMAGE 0
#define PREVIEW_STATE_LIVE 1

struct preview_s
{
#if defined(_ENABLE_GST)
    GstElement *play;
    GstElement *vsink;
#endif
    gint64      len;
    gint64      pos;
    gboolean    seek_lock;
    gboolean    progress_lock;
    gint        width;
    gint        height;
    gint        render_width;
    gint        render_height;
    GtkWidget * view;
    GdkPixbuf * pix;
    GdkPixbuf * scaled_pix;
    gint        button_width;
    gint        button_height;
    gint        frame;
    gint        state;
    gboolean    pause;
    gboolean    encoded[GHB_PREVIEW_MAX];
    gint        encode_frame;
    gint        live_id;
    gchar     * current;
    gint        live_enabled;
    gboolean    is_fullscreen;
};

#if defined(_ENABLE_GST)
G_MODULE_EXPORT gboolean live_preview_cb(GstBus *bus, GstMessage *msg, gpointer data);
#endif

void
ghb_screen_par(signal_user_data_t *ud, gint *par_n, gint *par_d)
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

    ghb_screen_par(ud, &disp_par_n, &disp_par_d);
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

void
preview_set_render_size(signal_user_data_t *ud, int width, int height)
{
    GtkWidget     * widget, *frame, *reset;
    GtkWindow     * window;
    gint            s_w, s_h;
    gint            factor;
    gfloat          ratio = 1.0;

    window = GTK_WINDOW(GHB_WIDGET(ud->builder, "preview_window"));
    widget = GHB_WIDGET (ud->builder, "preview_image");
    frame = GHB_WIDGET (ud->builder, "preview_image_frame");

    if (ghb_dict_get_bool(ud->prefs, "reduce_hd_preview"))
        factor = 90;
    else
        factor = 100;

    ghb_monitor_get_size(GHB_WIDGET(ud->builder, "hb_window"), &s_w, &s_h);

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
    gtk_aspect_frame_set(GTK_ASPECT_FRAME(frame), 0.5, 0.5, ratio, FALSE);

    if (ud->preview->is_fullscreen)
    {
        reset = GHB_WIDGET(ud->builder, "preview_reset");
        gtk_widget_hide(reset);
    }
    else
    {
        gtk_window_unmaximize(window);
        gtk_window_resize(window, width, height);
    }
    gtk_widget_set_size_request(widget, -1, -1);

    ud->preview->render_width = width;
    ud->preview->render_height = height;
}

void
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

void
ghb_preview_init(signal_user_data_t *ud)
{
    GtkWidget *widget;

    ud->preview               = g_malloc0(sizeof(preview_t));
    ud->preview->pause        = TRUE;
    ud->preview->encode_frame = -1;
    ud->preview->live_id      = -1;

    widget = GHB_WIDGET(ud->builder, "preview_button_image");
    gtk_widget_get_size_request(widget, &ud->preview->button_width,
                                        &ud->preview->button_height);

#if defined(_ENABLE_GST)
    GstBus *bus;

    ud->preview->play  = gst_element_factory_make("playbin", "play");
    ud->preview->vsink = gst_element_factory_make("gdkpixbufsink", "pixsink");
    if (ud->preview->play == NULL || ud->preview->vsink == NULL)
    {
        g_message("Couldn't initialize gstreamer. Disabling live preview.");
        GtkWidget *widget = GHB_WIDGET(ud->builder, "live_preview_box");
        gtk_widget_hide (widget);
        widget = GHB_WIDGET(ud->builder, "live_preview_duration_box");
        gtk_widget_hide (widget);
        return;
    }
    else
    {
        g_object_set(ud->preview->vsink, "qos", FALSE,
                                         "max-lateness", (gint64) - 1, NULL);
        g_object_set(ud->preview->play, "video-sink", ud->preview->vsink, NULL);
        g_object_set(ud->preview->play, "subtitle-font-desc",
                                        "sans bold 20", NULL);

        bus = gst_pipeline_get_bus(GST_PIPELINE(ud->preview->play));
        gst_bus_add_watch(bus, live_preview_cb, ud);
        gst_object_unref(bus);
        ud->preview->live_enabled = 1;
    }
#else
    widget = GHB_WIDGET(ud->builder, "live_preview_box");
    gtk_widget_hide (widget);
    widget = GHB_WIDGET(ud->builder, "live_preview_duration_box");
    gtk_widget_hide (widget);
#endif
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

#if defined(_ENABLE_GST)
void
live_preview_start(signal_user_data_t *ud)
{
    GtkImage *img;
    gchar *uri;

    if (!ud->preview->live_enabled)
        return;

    img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
    if (!ud->preview->encoded[ud->preview->frame])
    {
        ghb_image_set_from_icon_name(img, GHB_PLAY_ICON, GHB_ICON_SIZE_BUTTON);
        gst_element_set_state(ud->preview->play, GST_STATE_NULL);
        ud->preview->pause = TRUE;
        return;
    }

    if (ud->preview->state != PREVIEW_STATE_LIVE)
    {
#if defined(_WIN32)
        uri = g_strdup_printf("file:///%s", ud->preview->current);
#else
        uri = g_strdup_printf("file://%s", ud->preview->current);
#endif
        ghb_image_set_from_icon_name(img, GHB_PAUSE_ICON, GHB_ICON_SIZE_BUTTON);
        ud->preview->state = PREVIEW_STATE_LIVE;
        g_object_set(G_OBJECT(ud->preview->play), "uri", uri, NULL);
        g_free(uri);
    }
    gst_element_set_state(ud->preview->play, GST_STATE_PLAYING);
    ud->preview->pause = FALSE;
}

void
live_preview_pause(signal_user_data_t *ud)
{
    GtkImage *img;

    if (!ud->preview->live_enabled)
        return;

    img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
    ghb_image_set_from_icon_name(img, GHB_PLAY_ICON, GHB_ICON_SIZE_BUTTON);
    gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
    ud->preview->pause = TRUE;
}
#endif

void
live_preview_stop(signal_user_data_t *ud)
{
    GtkImage *img;
    GtkRange *progress;

    if (!ud->preview->live_enabled)
        return;

    img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
    ghb_image_set_from_icon_name(img, GHB_PLAY_ICON, GHB_ICON_SIZE_BUTTON);
#if defined(_ENABLE_GST)
    gst_element_set_state(ud->preview->play, GST_STATE_NULL);
#endif
    ud->preview->pause = TRUE;
    ud->preview->state = PREVIEW_STATE_IMAGE;

    progress = GTK_RANGE(GHB_WIDGET(ud->builder, "live_preview_progress"));
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
    if (!ud->preview->pause)
        live_preview_stop(ud);
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

#if defined(_ENABLE_GST)
static void
caps_set(GstCaps *caps, signal_user_data_t *ud)
{
    GstStructure *ss;

    ss = gst_caps_get_structure(caps, 0);
    if (ss)
    {
        gint fps_n, fps_d, width, height;
        guint num, den, par_n, par_d;
        gint disp_par_n, disp_par_d;
        const GValue *par;

        gst_structure_get_fraction(ss, "framerate", &fps_n, &fps_d);
        gst_structure_get_int(ss, "width", &width);
        gst_structure_get_int(ss, "height", &height);
        par = gst_structure_get_value(ss, "pixel-aspect-ratio");
        par_n = gst_value_get_fraction_numerator(par);
        par_d = gst_value_get_fraction_denominator(par);

        ghb_screen_par(ud, &disp_par_n, &disp_par_d);
        gst_video_calculate_display_ratio(
            &num, &den, width, height, par_n, par_d, disp_par_n, disp_par_d);

        if (par_n > par_d)
            width = gst_util_uint64_scale_int(height, num, den);
        else
            height = gst_util_uint64_scale_int(width, den, num);

        preview_set_size(ud, width, height);
        if (ghb_dict_get_bool(ud->prefs, "reduce_hd_preview"))
        {
            gint s_w, s_h;

            ghb_monitor_get_size(GHB_WIDGET(ud->builder, "preview_window"),
                                 &s_w, &s_h);
            if (s_w > 0 && s_h > 0)
            {
                if (width > s_w * 80 / 100)
                {
                    width = s_w * 80 / 100;
                    height = gst_util_uint64_scale_int(width, den, num);
                }
                if (height > s_h * 80 / 100)
                {
                    height = s_h * 80 / 100;
                    width = gst_util_uint64_scale_int(height, num, den);
                }
            }
        }
    }
}

static void
update_stream_info(signal_user_data_t *ud)
{
    GstPad *vpad = NULL;
    gint n_video;

    g_object_get(G_OBJECT(ud->preview->play), "n-video", &n_video, NULL);
    if (n_video > 0)
    {
        gint ii;
        for (ii = 0; ii < n_video && vpad == NULL; ii++)
        {
            g_signal_emit_by_name(ud->preview->play, "get-video-pad", ii, &vpad);
        }
    }

    if (vpad)
    {
        GstCaps *caps;

        caps = gst_pad_get_current_caps(vpad);
        if (caps)
        {
            caps_set(caps, ud);
            gst_caps_unref(caps);
        }
        gst_object_unref(vpad);
    }
}

G_MODULE_EXPORT gboolean
live_preview_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
    signal_user_data_t *ud = (signal_user_data_t*)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_UNKNOWN:
        {
            //printf("unknown");
        } break;

        case GST_MESSAGE_EOS:
        {
            // Done
            //printf("eos\n");
            live_preview_stop(ud);
            gst_element_seek(ud->preview->play, 1.0,
                GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                GST_SEEK_TYPE_SET, 0,
                GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        } break;

        case GST_MESSAGE_ERROR:
        {
            //printf("error\n");
            GError *err;
            gchar *debug;

            gst_message_parse_error(msg, &err, &debug);
            g_warning("Gstreamer Error: %s", err->message);
            g_error_free(err);
            g_free(debug);
        } break;

        case GST_MESSAGE_WARNING:
        case GST_MESSAGE_INFO:
        case GST_MESSAGE_TAG:
        case GST_MESSAGE_BUFFERING:
        case GST_MESSAGE_STATE_CHANGED:
        {
            //printf("state change %x\n", state);
            GstState state, pending;
            gst_element_get_state(ud->preview->play, &state, &pending, 0);
            if (state == GST_STATE_PAUSED || state == GST_STATE_PLAYING)
            {
                update_stream_info(ud);
            }
        } break;

        case GST_MESSAGE_STATE_DIRTY:
        {
            //printf("state dirty\n");
        } break;

        case GST_MESSAGE_STEP_DONE:
        {
            //printf("step done\n");
        } break;

        case GST_MESSAGE_CLOCK_PROVIDE:
        {
            //printf("clock provide\n");
        } break;

        case GST_MESSAGE_CLOCK_LOST:
        {
            //printf("clock lost\n");
        } break;

        case GST_MESSAGE_NEW_CLOCK:
        {
            //printf("new clock\n");
        } break;

        case GST_MESSAGE_STRUCTURE_CHANGE:
        {
            //printf("structure change\n");
        } break;

        case GST_MESSAGE_STREAM_STATUS:
        {
            //printf("stream status\n");
        } break;

        case GST_MESSAGE_APPLICATION:
        {
            //printf("application\n");
        } break;

        case GST_MESSAGE_ELEMENT:
        {
            //printf("element\n");
            if (gst_is_missing_plugin_message(msg))
            {
                GtkWindow *hb_window;
                hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
                gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
                gchar *message, *desc;
                desc = gst_missing_plugin_message_get_description(msg);
                message = g_strdup_printf(
                            _("Missing GStreamer plugin\n"
                            "Audio or Video may not play as expected\n\n%s"),
                            desc);
                ghb_message_dialog(hb_window, GTK_MESSAGE_WARNING,
                                   message, _("OK"), NULL);
                g_free(message);
                gst_element_set_state(ud->preview->play, GST_STATE_PLAYING);
            }
            else if (msg->src == GST_OBJECT_CAST(ud->preview->vsink))
            {
                const GstStructure *gstStruct;
                const GValue       *val;

                gstStruct = gst_message_get_structure(msg);
                if (gstStruct != NULL &&
                    (gst_structure_has_name(gstStruct, "preroll-pixbuf") ||
                     gst_structure_has_name(gstStruct, "pixbuf")))
                {
                    val = gst_structure_get_value(gstStruct, "pixbuf");
                    if (val != NULL)
                    {
                        GdkPixbuf * pix;
                        GtkWidget *widget;
                        int        width, height;

                        if (ud->preview->pix != NULL)
                            g_object_unref(ud->preview->pix);
                        if (ud->preview->scaled_pix != NULL)
                            g_object_unref(ud->preview->scaled_pix);
                        pix = GDK_PIXBUF(g_value_dup_object(val));
                        width = gdk_pixbuf_get_width(pix);
                        height = gdk_pixbuf_get_height(pix);
                        if (width  != ud->preview->width ||
                            height != ud->preview->height ||
                            width  != ud->preview->render_width ||
                            height != ud->preview->render_height)
                        {
                            double xscale, yscale;

                            xscale = (double)ud->preview->render_width /
                                             ud->preview->width;
                            yscale = (double)ud->preview->render_height /
                                             ud->preview->height;
                            if (xscale <= yscale)
                            {
                                width  = ud->preview->render_width;
                                height = ud->preview->height * xscale;
                            }
                            else
                            {
                                width  = ud->preview->width * yscale;
                                height = ud->preview->render_height;
                            }

                            ud->preview->scaled_pix =
                                gdk_pixbuf_scale_simple(pix,
                                                        width, height,
                                                        GDK_INTERP_BILINEAR);
                            g_object_ref(pix);
                        }
                        else
                        {
                            ud->preview->scaled_pix = pix;
                        }
                        ud->preview->pix = ud->preview->scaled_pix;
                        g_object_ref(ud->preview->pix);
                        widget = GHB_WIDGET (ud->builder, "preview_image");
                        gtk_widget_queue_draw(widget);
                    }
                }
            }
        } break;

        case GST_MESSAGE_SEGMENT_START:
        {
            //printf("segment start\n");
        } break;

        case GST_MESSAGE_SEGMENT_DONE:
        {
            //printf("segment done\n");
        } break;

        case GST_MESSAGE_DURATION_CHANGED:
        {
            //printf("duration change\n");
        };

        case GST_MESSAGE_LATENCY:
        {
            //printf("latency\n");
        };

        case GST_MESSAGE_ASYNC_START:
        {
            //printf("async start\n");
        } break;

        case GST_MESSAGE_ASYNC_DONE:
        {
            //printf("async done\n");
        } break;

        case GST_MESSAGE_REQUEST_STATE:
        {
            //printf("request state\n");
        } break;

        case GST_MESSAGE_STEP_START:
        {
            //printf("step start\n");
        } break;

        case GST_MESSAGE_QOS:
        {
            //printf("qos\n");
        } break;

        case GST_MESSAGE_PROGRESS:
        {
            //printf("progress\n");
        } break;

        case GST_MESSAGE_TOC:
        {
            //printf("toc\n");
        } break;

        case GST_MESSAGE_RESET_TIME:
        {
            //printf("reset time\n");
        } break;

        case GST_MESSAGE_STREAM_START:
        {
            //printf("stream start\n");
        };

        case GST_MESSAGE_ANY:
        {
            //printf("any\n");
        } break;


        default:
        {
            // Ignore
            //printf("?msg? %x\n", GST_MESSAGE_TYPE(msg));
        }
    }
    return TRUE;
}
#endif

G_MODULE_EXPORT void
live_preview_start_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    gchar *tmp_dir;
    gchar *name;
    gint frame = ud->preview->frame;

    tmp_dir = ghb_get_tmp_dir();
    name = g_strdup_printf("%s/live%02d", tmp_dir, ud->preview->frame);
    free(tmp_dir);
    if (ud->preview->current)
        g_free(ud->preview->current);
    ud->preview->current = name;

    if (ud->preview->encoded[frame] &&
        g_file_test(name, G_FILE_TEST_IS_REGULAR))
    {
#if defined(_ENABLE_GST)
        if (ud->preview->pause)
            live_preview_start(ud);
        else
            live_preview_pause(ud);
#endif
    }
    else
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
    }
}

void
ghb_live_encode_done(signal_user_data_t *ud, gboolean success)
{
    GtkWidget *widget;
    GtkWidget *prog;

    ud->preview->live_id = -1;
    prog = GHB_WIDGET(ud->builder, "live_encode_progress");
    if (success &&
        ud->preview->encode_frame == ud->preview->frame)
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), "Done");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog), 1);
        ud->preview->encoded[ud->preview->encode_frame] = TRUE;
#if defined(_ENABLE_GST)
        live_preview_start(ud);
#endif
        widget = GHB_WIDGET(ud->builder, "live_progress_box");
        gtk_widget_hide (widget);
        widget = GHB_WIDGET(ud->builder, "live_preview_progress");
        gtk_widget_show (widget);
    }
    else
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog), 0);
        ud->preview->encoded[ud->preview->encode_frame] = FALSE;
    }
}

#if defined(_ENABLE_GST)
G_MODULE_EXPORT gboolean
unlock_progress_cb(signal_user_data_t *ud)
{
    ud->preview->progress_lock = FALSE;
    // This function is initiated by g_idle_add.  Must return false
    // so that it is not called again
    return FALSE;
}
#endif

void
ghb_live_preview_progress(signal_user_data_t *ud)
{
#if defined(_ENABLE_GST)
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 len = -1, pos = -1;

    if (!ud->preview->live_enabled)
        return;

    if (ud->preview->state != PREVIEW_STATE_LIVE || ud->preview->seek_lock)
        return;

    ud->preview->progress_lock = TRUE;
    if (gst_element_query_duration(ud->preview->play, fmt, &len))
    {
        if (len != -1 && fmt == GST_FORMAT_TIME)
        {
            ud->preview->len = len / GST_MSECOND;
        }
    }
    if (gst_element_query_position(ud->preview->play, fmt, &pos))
    {
        if (pos != -1 && fmt == GST_FORMAT_TIME)
        {
            ud->preview->pos = pos / GST_MSECOND;
        }
    }
    if (ud->preview->len > 0)
    {
        GtkRange *progress;
        gdouble percent;

        percent = (gdouble)ud->preview->pos * 100 / ud->preview->len;
        progress = GTK_RANGE(GHB_WIDGET(ud->builder, "live_preview_progress"));
        gtk_range_set_value(progress, percent);
    }
    g_idle_add((GSourceFunc)unlock_progress_cb, ud);
#endif
}

#if defined(_ENABLE_GST)
G_MODULE_EXPORT gboolean
unlock_seek_cb(signal_user_data_t *ud)
{
    ud->preview->seek_lock = FALSE;
    // This function is initiated by g_idle_add.  Must return false
    // so that it is not called again
    return FALSE;
}
#endif

G_MODULE_EXPORT void
live_preview_seek_cb(GtkWidget *widget, signal_user_data_t *ud)
{
#if defined(_ENABLE_GST)
    gdouble dval;
    gint64 pos;

    if (!ud->preview->live_enabled)
        return;

    if (ud->preview->progress_lock)
        return;

    ud->preview->seek_lock = TRUE;
    dval = gtk_range_get_value(GTK_RANGE(widget));
    pos = ((ud->preview->len * dval) / 100) * GST_MSECOND;
    gst_element_seek(ud->preview->play, 1.0,
        GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
        GST_SEEK_TYPE_SET, pos,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
    g_idle_add((GSourceFunc)unlock_seek_cb, ud);
#endif
}

G_MODULE_EXPORT void
preview_fullscreen_action_cb(GSimpleAction *action, GVariant *param,
                             signal_user_data_t *ud)
{
    gboolean state = g_variant_get_boolean(param);
    GtkWindow *window = GTK_WINDOW(GHB_WIDGET(ud->builder, "preview_window"));

    if (state != ud->preview->is_fullscreen)
        g_simple_action_set_state(action, param);

    if (!ud->preview->is_fullscreen)
    {
        gtk_window_fullscreen(window);
    }
    else
    {
        gtk_window_unfullscreen(window);
    }
}

static void _draw_pixbuf(signal_user_data_t * ud, cairo_t *cr, GdkPixbuf *pix)
{
    int pix_width, pix_height, hoff, voff;

    cairo_save(cr);
    cairo_rectangle(cr, 0, 0, ud->preview->render_width,
                              ud->preview->render_height);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_fill(cr);
    cairo_restore(cr);

    pix_width  = gdk_pixbuf_get_width(pix);
    pix_height = gdk_pixbuf_get_height(pix);
    hoff = MAX((ud->preview->render_width  - pix_width)  / 2, 0);
    voff = MAX((ud->preview->render_height - pix_height) / 2, 0);
    if (voff > 0 || hoff > 0)
    {
        cairo_translate(cr, hoff, voff);
    }

    gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
    cairo_paint(cr);
}

static void set_mini_preview_image(signal_user_data_t *ud, GdkPixbuf * pix)
{
    int         preview_width, preview_height, width, height;
    GdkPixbuf * scaled_preview;

    if (pix == NULL)
    {
        return;
    }

    preview_width  = gdk_pixbuf_get_width(pix);
    preview_height = gdk_pixbuf_get_height(pix);

    // Scale and display the mini-preview
    height = MIN(ud->preview->button_height - 32, preview_height);
    width = preview_width * height / preview_height;
    if (width > ud->preview->button_width - 32)
    {
        width = MIN(ud->preview->button_width - 32, preview_width);
        height = preview_height * width / preview_width;
    }
    if ((height >= 16) && (width >= 16))
    {
        scaled_preview = gdk_pixbuf_scale_simple(pix, width, height,
                                                 GDK_INTERP_NEAREST);
        if (scaled_preview != NULL)
        {
            GtkWidget * widget;

            widget = GHB_WIDGET (ud->builder, "preview_button_image");
#if GTK_CHECK_VERSION(3, 90, 0)
            gtk_picture_set_pixbuf(GTK_PICTURE(widget), scaled_preview);
#else
            gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled_preview);
#endif
            g_object_unref(scaled_preview);
        }
    }
}

GdkPixbuf * do_preview_scaling(signal_user_data_t *ud, GdkPixbuf *pix)
{
    int         preview_width, preview_height, width, height;
    GdkPixbuf * scaled_preview;

    if (pix == NULL)
    {
        return NULL;
    }

    preview_width  = gdk_pixbuf_get_width(pix);
    preview_height = gdk_pixbuf_get_height(pix);

    if (ud->preview->render_width <= 0 || ud->preview->render_height <= 0)
    {
        // resize preview window to fit preview
        preview_set_render_size(ud, preview_width, preview_height);
        g_object_ref(pix);
        return pix;
    }

    // Scale if necessary
    if (preview_width  != ud->preview->render_width ||
        preview_height != ud->preview->render_height)
    {
        double xscale, yscale;

        xscale = (double)ud->preview->render_width  / preview_width;
        yscale = (double)ud->preview->render_height / preview_height;
        if (xscale <= yscale)
        {
            width  = ud->preview->render_width;
            height = preview_height * xscale;
        }
        else
        {
            width  = preview_width * yscale;
            height = ud->preview->render_height;
        }
        // Allow some slop in aspect ratio so that we fill the window
        int delta = ud->preview->render_width - width;
        if (delta > 0 && delta <= 16)
            width = ud->preview->render_width;

        delta = ud->preview->render_height - height;
        if (delta > 0 && delta <= 16)
            height = ud->preview->render_height;

        scaled_preview = gdk_pixbuf_scale_simple(pix, width, height,
                                                 GDK_INTERP_BILINEAR);
        return scaled_preview;
    }
    else
    {
        g_object_ref(pix);
    }
    return pix;
}

void
init_preview_image(signal_user_data_t *ud)
{
    GtkWidget *widget;

    live_preview_stop(ud);

    widget = GHB_WIDGET (ud->builder, "preview_frame");
    ud->preview->frame = ghb_widget_int(widget) - 1;
    if (ud->preview->encoded[ud->preview->frame])
    {
        widget = GHB_WIDGET(ud->builder, "live_progress_box");
        gtk_widget_hide (widget);
        widget = GHB_WIDGET(ud->builder, "live_preview_progress");
        gtk_widget_show (widget);
    }
    else
    {
        widget = GHB_WIDGET(ud->builder, "live_preview_progress");
        gtk_widget_hide (widget);
        widget = GHB_WIDGET(ud->builder, "live_progress_box");
        gtk_widget_show (widget);
        widget = GHB_WIDGET(ud->builder, "live_encode_progress");
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widget), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(widget), 0);
    }
    if (ud->preview->pix != NULL)
        g_object_unref(ud->preview->pix);
    if (ud->preview->scaled_pix != NULL)
        g_object_unref(ud->preview->scaled_pix);

    ud->preview->pix = ghb_get_preview_image(ud->preview->frame, ud);
    if (ud->preview->pix == NULL)
        return;

    int pix_width, pix_height;
    pix_width  = gdk_pixbuf_get_width(ud->preview->pix);
    pix_height = gdk_pixbuf_get_height(ud->preview->pix);
    preview_set_size(ud, pix_width, pix_height);
}

void
ghb_set_preview_image(signal_user_data_t *ud)
{
    init_preview_image(ud);

    // Scale and display the mini-preview
    set_mini_preview_image(ud, ud->preview->pix);

    // Scale the full size preview
    ud->preview->scaled_pix = do_preview_scaling(ud, ud->preview->pix);

    // Display full size preview
    GtkWidget *widget = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_queue_draw(widget);
}

void
ghb_rescale_preview_image(signal_user_data_t *ud)
{
    init_preview_image(ud);
    if (ud->preview->width <= 0 || ud->preview->height <= 0)
    {
        return;
    }
    double scale = (double)ud->preview->render_width / ud->preview->width;
    preview_set_render_size(ud, ud->preview->width * scale,
                                ud->preview->height * scale);

    // Scale and display the mini-preview
    set_mini_preview_image(ud, ud->preview->pix);

    // Scale the full size preview
    ud->preview->scaled_pix = do_preview_scaling(ud, ud->preview->pix);

    // Display full size preview
    GtkWidget *widget = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_queue_draw(widget);
}

void
ghb_reset_preview_image(signal_user_data_t *ud)
{
    init_preview_image(ud);
    if (ud->preview->width <= 0 || ud->preview->height <= 0)
    {
        return;
    }
    preview_set_render_size(ud, ud->preview->width, ud->preview->height);

    // Scale and display the mini-preview
    set_mini_preview_image(ud, ud->preview->pix);

    // Scale the full size preview
    ud->preview->scaled_pix = do_preview_scaling(ud, ud->preview->pix);

    // Display full size preview
    GtkWidget *widget = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_queue_draw(widget);
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
preview_draw_cb(
    GtkDrawingArea * da,
    cairo_t        * cr,
    int              width,
    int              height,
    signal_user_data_t *ud)
{
    if (ud->preview->scaled_pix != NULL)
    {
        _draw_pixbuf(ud, cr, ud->preview->scaled_pix);
    }
}

#else

G_MODULE_EXPORT gboolean
preview_draw_cb(
    GtkWidget *widget,
    cairo_t *cr,
    signal_user_data_t *ud)
{
    if (ud->preview->scaled_pix != NULL)
    {
        _draw_pixbuf(ud, cr, ud->preview->scaled_pix);
    }
    return FALSE;
}
#endif

G_MODULE_EXPORT void
preview_button_size_allocate_cb(
    GtkWidget *widget,
#if GTK_CHECK_VERSION(3, 90, 0)
    int width,
    int height,
    int baseline,
#else
    GdkRectangle *rect,
#endif
    signal_user_data_t *ud)
{
#if !GTK_CHECK_VERSION(3, 90, 0)
    int width  = rect->width;
    int height = rect->height;
#endif
    if (ud->preview->button_width  == width &&
        ud->preview->button_height == height)
    {
        // Nothing to do. Bug out.
        g_debug("nothing to do");
        return;
    }
    ud->preview->button_width  = width;
    ud->preview->button_height = height;
    set_mini_preview_image(ud, ud->preview->pix);
}

void
ghb_preview_set_visible(signal_user_data_t *ud, gboolean visible)
{
    GtkWidget *widget;
#if 0
    gint title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    visible &= title != NULL;
#endif
    widget = GHB_WIDGET(ud->builder, "preview_window");
    if (visible)
    {
#if !GTK_CHECK_VERSION(3, 90, 0)
        // TODO: can this be done in GTK4?
        gint x, y;
        x = ghb_dict_get_int(ud->prefs, "preview_x");
        y = ghb_dict_get_int(ud->prefs, "preview_y");

        if (x >= 0 && y >= 0)
            gtk_window_move(GTK_WINDOW(widget), x, y);
#endif
        gtk_window_deiconify(GTK_WINDOW(widget));
    }
    gtk_widget_set_visible(widget, visible);
}

G_MODULE_EXPORT void
show_preview_action_cb(GSimpleAction *action, GVariant *value,
                       signal_user_data_t *ud)
{
    gboolean state = g_variant_get_boolean(value);

    g_simple_action_set_state(action, value);
    ghb_preview_set_visible(ud, state);
}

G_MODULE_EXPORT void
preview_reset_clicked_cb(GtkWidget *toggle, signal_user_data_t *ud)
{
    g_debug("preview_reset_clicked_cb()");
    if (ud->preview->width <= 0 || ud->preview->height <= 0)
    {
        return;
    }
    preview_set_render_size(ud, ud->preview->width, ud->preview->height);

    // On windows, preview_resize_cb does not get called when the size
    // is reset above.  So assume it got reset and disable the
    // "Source Resolution" button.
    GtkWidget * widget = GHB_WIDGET(ud->builder, "preview_reset");
    gtk_widget_hide(widget);

    if (ud->preview->scaled_pix != NULL)
        g_object_unref(ud->preview->scaled_pix);
    ud->preview->scaled_pix = do_preview_scaling(ud, ud->preview->pix);

    widget = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_queue_draw(widget);
}

G_MODULE_EXPORT void
preview_frame_value_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    if (ud->preview->live_id >= 0)
    {
        ghb_stop_live_encode();
        ud->preview->live_id = -1;
        ud->preview->encode_frame = -1;
    }
    ghb_set_preview_image(ud);
}

G_MODULE_EXPORT gboolean
preview_window_delete_cb(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(3, 90, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    live_preview_stop(ud);
    g_action_activate(GHB_ACTION(ud->builder, "show-preview"), NULL);
    return TRUE;
}

G_MODULE_EXPORT void
preview_duration_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("preview_duration_changed_cb ()");
    ghb_live_reset(ud);
    ghb_widget_to_setting (ud->prefs, widget);
    ghb_check_dependency(ud, widget, NULL);
    const gchar *name = ghb_get_setting_key(widget);
    ghb_pref_save(ud->prefs, name);
}

static guint hud_timeout_id = 0;

static gboolean in_hud = FALSE;

static gboolean
hud_timeout(signal_user_data_t *ud)
{
    GtkWidget *widget;

    g_debug("hud_timeout()");
    widget = GHB_WIDGET(ud->builder, "preview_hud");
    gtk_widget_hide(widget);
    hud_timeout_id = 0;
    return FALSE;
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
hud_enter_cb(
    GtkEventControllerMotion * econ,
    double                     x,
    double                     y,
    GdkCrossingMode            cross,
    GdkNotifyType              notify,
    signal_user_data_t *ud)
#else
G_MODULE_EXPORT gboolean
hud_enter_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
#endif
{
    GtkWidget * hud;
    if (hud_timeout_id != 0)
    {
        GMainContext *mc;
        GSource *source;

        mc = g_main_context_default();
        source = g_main_context_find_source_by_id(mc, hud_timeout_id);
        if (source != NULL)
            g_source_destroy(source);
    }
    hud = GHB_WIDGET(ud->builder, "preview_hud");
    if (!gtk_widget_get_visible(hud))
    {
        gtk_widget_show(hud);
    }
    hud_timeout_id = 0;
    in_hud = TRUE;
#if !GTK_CHECK_VERSION(3, 90, 0)
    return FALSE;
#endif
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
hud_leave_cb(
    GtkEventControllerMotion * econ,
    GdkCrossingMode            cross,
    GdkNotifyType              notify,
    signal_user_data_t *ud)
#else
G_MODULE_EXPORT gboolean
hud_leave_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
#endif
{
    in_hud = FALSE;
#if !GTK_CHECK_VERSION(3, 90, 0)
    return FALSE;
#endif
}

G_MODULE_EXPORT void
preview_click_cb (GtkGesture *gest,
                  gint n_press,
                  gdouble x,
                  gdouble y,
                  signal_user_data_t *ud)
{
    if (n_press == 2)
        g_action_activate(GHB_ACTION(ud->builder, "preview-fullscreen"), NULL);
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
preview_leave_cb(
    GtkEventControllerMotion * econ,
    GdkCrossingMode            cross,
    GdkNotifyType              notify,
    signal_user_data_t *ud)
#else
G_MODULE_EXPORT gboolean
preview_leave_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
#endif
{
    if (hud_timeout_id != 0)
    {
        GMainContext *mc;
        GSource *source;

        mc = g_main_context_default();
        source = g_main_context_find_source_by_id(mc, hud_timeout_id);
        if (source != NULL)
            g_source_destroy(source);
    }
    hud_timeout_id = g_timeout_add(300, (GSourceFunc)hud_timeout, ud);
#if !GTK_CHECK_VERSION(3, 90, 0)
    return FALSE;
#endif
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
preview_motion_cb(
    GtkEventControllerMotion * econ,
    gdouble                    x,
    gdouble                    y,
    signal_user_data_t *ud)
#else
G_MODULE_EXPORT gboolean
preview_motion_cb(
    GtkWidget *widget,
    GdkEventMotion *event,
    signal_user_data_t *ud)
#endif
{
    GtkWidget * hud;

    if (hud_timeout_id != 0)
    {
        GMainContext *mc;
        GSource *source;

        mc = g_main_context_default();
        source = g_main_context_find_source_by_id(mc, hud_timeout_id);
        if (source != NULL)
            g_source_destroy(source);
    }
    hud = GHB_WIDGET(ud->builder, "preview_hud");
    if (!gtk_widget_get_visible(hud))
    {
        gtk_widget_show(hud);
    }
    if (!in_hud)
    {
        hud_timeout_id = g_timeout_add_seconds(4, (GSourceFunc)hud_timeout, ud);
    }
#if !GTK_CHECK_VERSION(3, 90, 0)
    return FALSE;
#endif
}

// TODO: GTK4 eliminated "configure-event" signal.
// From a read of the gtk4 code, it appears GDK_CONFIGURE does
// not get propagated.  And there is no way to set position of
// a window :*(
//
// Hopefully they will fix this or provide a better alternative
// before gtk4 is released.
#if !GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT gboolean
preview_configure_cb(
    GtkWidget *widget,
    GdkEventConfigure *event,
    signal_user_data_t *ud)
{
    if (gtk_widget_get_visible(widget))
    {
        // TODO: can this be done in GTK4?
        gint x, y;

        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
        ghb_dict_set_int(ud->prefs, "preview_x", x);
        ghb_dict_set_int(ud->prefs, "preview_y", y);
        ghb_pref_set(ud->prefs, "preview_x");
        ghb_pref_set(ud->prefs, "preview_y");
        ghb_prefs_store();
    }
    return FALSE;
}
#endif

#if !GTK_CHECK_VERSION(3, 90, 0)
// GTK4 no longer has GDK_WINDOW_STATE events :*(
G_MODULE_EXPORT gboolean
preview_state_cb(
    GtkWidget *widget,
    GdkEvent  *event,
    signal_user_data_t *ud)
{
    GdkEventType type = ghb_event_get_event_type(event);
    if (type == GDK_WINDOW_STATE)
    {
        // Look for transition to iconified state.
        // Toggle "Show Preview" button when iconified.
        // I only do this because there seems to be no
        // way to reliably disable the iconify button without
        // also disabling the maximize button.
        GdkEventWindowState * wse = (GdkEventWindowState*)event;
        if (wse->changed_mask & wse->new_window_state &
            GDK_WINDOW_STATE_ICONIFIED)
        {
            live_preview_stop(ud);
			g_action_activate(GHB_ACTION(ud->builder, "show-preview"), NULL);
        }
        ud->preview->is_fullscreen = wse->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;
    }
    return FALSE;
}
#endif

G_MODULE_EXPORT void
preview_resize_cb(
    GtkWidget     *widget,
#if GTK_CHECK_VERSION(3, 90, 0)
    int width,
    int height,
    int baseline,
#else
    GdkRectangle  *rect,
#endif
    signal_user_data_t *ud)
{
#if !GTK_CHECK_VERSION(3, 90, 0)
    int width  = rect->width;
    int height = rect->height;
#endif
    if (ud->preview->render_width != width ||
        ud->preview->render_height != height)
    {
        ud->preview->render_width  = width;
        ud->preview->render_height = height;
        if (ud->preview->scaled_pix != NULL)
            g_object_unref(ud->preview->scaled_pix);
        ud->preview->scaled_pix = do_preview_scaling(ud, ud->preview->pix);

        GtkWidget *widget = GHB_WIDGET(ud->builder, "preview_image");
        gtk_widget_queue_draw(widget);

        if (ABS(ud->preview->render_width  - ud->preview->width)  <= 2 ||
            ABS(ud->preview->render_height - ud->preview->height) <= 2)
        {
            GtkWidget * widget = GHB_WIDGET(ud->builder, "preview_reset");
            gtk_widget_hide(widget);
        }
        else if (!ud->preview->is_fullscreen)
        {
            GtkWidget * widget = GHB_WIDGET(ud->builder, "preview_reset");
            gtk_widget_show(widget);
        }
    }
}

G_MODULE_EXPORT void
show_crop_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
#if 0
    // Disabled until we reimplement this or come up with something better
    //
    g_debug("show_crop_changed_cb ()");
    ghb_widget_to_setting(ud->prefs, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_live_reset(ud);
    if (gtk_widget_is_sensitive(widget))
        ghb_set_scale(ud, 0);
    ghb_pref_save(ud->prefs, "preview_show_crop");
    ghb_rescale_preview_image(ud);
#endif
}
