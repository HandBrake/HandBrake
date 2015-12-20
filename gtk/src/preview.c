/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preview.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * preview.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include "ghbcompat.h"

#if !defined(_WIN32)
#include <gdk/gdkx.h>
#endif

#if defined(_ENABLE_GST)
#include <gst/gst.h>
#if GST_CHECK_VERSION(1, 0, 0)
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif
#include <gst/video/video.h>
#include <gst/pbutils/missing-plugins.h>
#endif

#include "settings.h"
#include "presets.h"
#include "callbacks.h"
#include "hb-backend.h"
#include "preview.h"
#include "values.h"
#include "queuehandler.h"
#include "hb.h"

#define PREVIEW_STATE_IMAGE 0
#define PREVIEW_STATE_LIVE 1

struct preview_s
{
#if defined(_ENABLE_GST)
    GstElement *play;
    gulong xid;
#endif
    gint64 len;
    gint64 pos;
    gboolean seek_lock;
    gboolean progress_lock;
    gint width;
    gint height;
    GtkWidget *view;
    GdkPixbuf *pix;
    gint button_width;
    gint button_height;
    gint frame;
    gint state;
    gboolean pause;
    gboolean encoded[GHB_PREVIEW_MAX];
    gint encode_frame;
    gint live_id;
    gchar *current;
    gint live_enabled;
};

#if defined(_ENABLE_GST)
G_MODULE_EXPORT gboolean live_preview_cb(GstBus *bus, GstMessage *msg, gpointer data);
static GstBusSyncReply create_window(GstBus *bus, GstMessage *msg,
                gpointer data);
#endif

void
ghb_screen_par(signal_user_data_t *ud, gint *par_n, gint *par_d)
{
#if defined(_ENABLE_GST)
    GValue disp_par = {0,};
    GstElement *xover;
    GObjectClass *klass;
    GParamSpec *pspec;

    if (!ud->preview->live_enabled)
        goto fail;

    g_value_init(&disp_par, GST_TYPE_FRACTION);
    gst_value_set_fraction(&disp_par, 1, 1);
    g_object_get(ud->preview->play, "video-sink", &xover, NULL);
    if (xover == NULL)
        goto fail;

    klass = G_OBJECT_GET_CLASS(xover);
    if (klass == NULL)
        goto fail;

    pspec = g_object_class_find_property(klass, "pixel-aspect_ratio");
    if (pspec)
    {
        GValue par_prop = {0,};

        g_value_init(&par_prop, pspec->value_type);
        g_object_get_property(G_OBJECT(xover), "pixel-aspect-ratio",
                                &par_prop);
        if (!g_value_transform(&par_prop, &disp_par))
        {
            g_warning("transform failed");
            gst_value_set_fraction(&disp_par, 1, 1);
        }
        g_value_unset(&par_prop);
    }
    *par_n = gst_value_get_fraction_numerator(&disp_par);
    *par_d = gst_value_get_fraction_denominator(&disp_par);
    g_value_unset(&disp_par);
    return;

fail:
    *par_n = 1;
    *par_d = 1;
#else
    *par_n = 1;
    *par_d = 1;
#endif
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

static GdkWindow*
preview_window(GtkWidget *widget)
{
    return gtk_layout_get_bin_window(GTK_LAYOUT(widget));
}

void
preview_set_size(signal_user_data_t *ud, int width, int height)
{
    GtkWidget *widget;

    widget = GHB_WIDGET (ud->builder, "preview_image");
    gtk_widget_set_size_request(widget, width, height);
    gtk_layout_set_size(GTK_LAYOUT(widget), width, height);
    widget = GHB_WIDGET (ud->builder, "preview_hud_box");
    gtk_widget_set_size_request(widget, width, height);

    ud->preview->width = width;
    ud->preview->height = height;
}

void
ghb_preview_init(signal_user_data_t *ud)
{
    GtkWidget *widget;

    ud->preview = g_malloc0(sizeof(preview_t));
    ud->preview->view = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_realize(ud->preview->view);

    ud->preview->pause = TRUE;
    ud->preview->encode_frame = -1;
    ud->preview->live_id = -1;
    widget = GHB_WIDGET(ud->builder, "preview_button_image");
    gtk_widget_get_size_request(widget, &ud->preview->button_width, &ud->preview->button_height);

#if defined(_ENABLE_GST)
    GstBus *bus;
    GstElement *xover;

    if (!gdk_window_ensure_native(preview_window(ud->preview->view)))
    {
        g_message("Couldn't create native window for GstXOverlay. Disabling live preview.");
        GtkWidget *widget = GHB_WIDGET(ud->builder, "live_preview_box");
        gtk_widget_hide (widget);
        widget = GHB_WIDGET(ud->builder, "live_preview_duration_box");
        gtk_widget_hide (widget);
        return;
    }
    widget = GHB_WIDGET(ud->builder, "preview_hud");
    gtk_widget_realize(widget);
    // Use a native window for the HUD.  Client side windows don't get
    // updated properly as video changes benieth them.
    if (!gdk_window_ensure_native(gtk_widget_get_window(widget)))
    {
        g_message("Couldn't create native window for HUD.");
    }

#if !defined(_WIN32)
    ud->preview->xid = GDK_WINDOW_XID(preview_window(ud->preview->view));
#else
    ud->preview->xid = GDK_WINDOW_HWND(preview_window(ud->preview->view));
#endif
    ud->preview->play = gst_element_factory_make("playbin", "play");
    xover = gst_element_factory_make("gconfvideosink", "xover");
    if (xover == NULL)
    {
        xover = gst_element_factory_make("xvimagesink", "xover");
    }
    if (xover == NULL)
    {
        xover = gst_element_factory_make("ximagesink", "xover");
    }
    if (ud->preview->play == NULL || xover == NULL)
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

        g_object_set(G_OBJECT(ud->preview->play), "video-sink", xover, NULL);
        g_object_set(ud->preview->play, "subtitle-font-desc",
                    "sans bold 20", NULL);

        bus = gst_pipeline_get_bus(GST_PIPELINE(ud->preview->play));
        gst_bus_add_watch(bus, live_preview_cb, ud);
#if GST_CHECK_VERSION(1, 0, 0)
        gst_bus_set_sync_handler(bus, create_window, ud->preview, NULL);
#else
        gst_bus_set_sync_handler(bus, create_window, ud->preview);
#endif
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
static GstBusSyncReply
create_window(GstBus *bus, GstMessage *msg, gpointer data)
{
    preview_t *preview = (preview_t*)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ELEMENT:
    {
#if GST_CHECK_VERSION(1, 0, 0)
        if (!gst_is_video_overlay_prepare_window_handle_message(msg))
            return GST_BUS_PASS;
        gst_video_overlay_set_window_handle(
            GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg)), preview->xid);
#else
        if (!gst_structure_has_name(msg->structure, "prepare-xwindow-id"))
            return GST_BUS_PASS;
#if !defined(_WIN32)
        gst_x_overlay_set_xwindow_id(
            GST_X_OVERLAY(GST_MESSAGE_SRC(msg)), preview->xid);
#else
        gst_directdraw_sink_set_window_id(
            GST_X_OVERLAY(GST_MESSAGE_SRC(msg)), preview->xid);
#endif
#endif
        gst_message_unref(msg);
        return GST_BUS_DROP;
    } break;

    default:
    {
    } break;
    }
    return GST_BUS_PASS;
}

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

        if (ghb_dict_get_bool(ud->prefs, "reduce_hd_preview"))
        {
            GdkScreen *ss;
            gint s_w, s_h;

            ss = gdk_screen_get_default();
            s_w = gdk_screen_get_width(ss);
            s_h = gdk_screen_get_height(ss);

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

        if (width != ud->preview->width || height != ud->preview->height)
        {
            preview_set_size(ud, width, height);
        }
    }
}

#if GST_CHECK_VERSION(1, 0, 0)
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
        //g_signal_connect(vpad, "notify::caps", G_CALLBACK(caps_set_cb), preview);
        gst_object_unref(vpad);
    }
}

#else

static GList *
get_stream_info_objects_for_type (GstElement *play, const gchar *typestr)
{
    GList *info_list = NULL, *link;
    GList *ret = NULL;

    if (play == NULL)
        return NULL;

    g_object_get(play, "stream-info", &info_list, NULL);
    if (info_list == NULL)
        return NULL;

    link = info_list;
    while (link)
    {
        GObject *info_obj = (GObject*)link->data;
        if (info_obj)
        {
            GParamSpec *pspec;
            GEnumValue *value;
            gint type = -1;

            g_object_get(info_obj, "type", &type, NULL);
            pspec = g_object_class_find_property(
                        G_OBJECT_GET_CLASS (info_obj), "type");
            value = g_enum_get_value(
                        G_PARAM_SPEC_ENUM (pspec)->enum_class, type);
            if (value)
            {
                if (g_ascii_strcasecmp (value->value_nick, typestr) == 0 ||
                    g_ascii_strcasecmp (value->value_name, typestr) == 0)
                {
                    ret = g_list_prepend (ret, g_object_ref (info_obj));
                }
            }
        }
        if (link) link = link->next;
    }
    return g_list_reverse (ret);
}

static void
update_stream_info(signal_user_data_t *ud)
{
    GList *vstreams, *ll;
    GstPad *vpad = NULL;

    vstreams = get_stream_info_objects_for_type(ud->preview->play, "video");
    if (vstreams)
    {
        for (ll = vstreams; vpad == NULL && ll != NULL; ll = ll->next)
        {
            g_object_get(ll->data, "object", &vpad, NULL);
        }
    }
    if (vpad)
    {
        GstCaps *caps;

        caps = gst_pad_get_negotiated_caps(vpad);
        if (caps)
        {
            caps_set(caps, ud);
            gst_caps_unref(caps);
        }
        //g_signal_connect(vpad, "notify::caps", G_CALLBACK(caps_set_cb), preview);
        gst_object_unref(vpad);
    }
    g_list_foreach(vstreams, (GFunc)g_object_unref, NULL);
    g_list_free(vstreams);
}

#endif

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
        GtkImage *img;

        //printf("eos");
        img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
        gtk_image_set_from_icon_name(img, GHB_PLAY_ICON, GTK_ICON_SIZE_BUTTON);
        gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
        ud->preview->pause = TRUE;
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
        GstState state, pending;
        gst_element_get_state(ud->preview->play, &state, &pending, 0);
        //printf("state change %x\n", state);
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
                               message, "Ok", NULL);
            g_free(message);
            gst_element_set_state(ud->preview->play, GST_STATE_PLAYING);
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

#if GST_CHECK_VERSION(1, 0, 0)
    case GST_MESSAGE_DURATION_CHANGED:
    {
        //printf("duration change\n");
    };
#endif

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

#if GST_CHECK_VERSION(1, 0, 0)
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
#endif

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
        gtk_image_set_from_icon_name(img, GHB_PLAY_ICON, GTK_ICON_SIZE_BUTTON);
        gst_element_set_state(ud->preview->play, GST_STATE_NULL);
        ud->preview->pause = TRUE;
        return;
    }

    uri = g_strdup_printf("file://%s", ud->preview->current);
    gtk_image_set_from_icon_name(img, GHB_PAUSE_ICON, GTK_ICON_SIZE_BUTTON);
    ud->preview->state = PREVIEW_STATE_LIVE;
    g_object_set(G_OBJECT(ud->preview->play), "uri", uri, NULL);
    gst_element_set_state(ud->preview->play, GST_STATE_PLAYING);
    ud->preview->pause = FALSE;
    g_free(uri);
}

void
live_preview_pause(signal_user_data_t *ud)
{
    GtkImage *img;

    if (!ud->preview->live_enabled)
        return;

    img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
    gtk_image_set_from_icon_name(img, GHB_PLAY_ICON, GTK_ICON_SIZE_BUTTON);
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
    gtk_image_set_from_icon_name(img, GHB_PLAY_ICON, GTK_ICON_SIZE_BUTTON);
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

G_MODULE_EXPORT void
live_preview_start_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    gchar *tmp_dir;
    gchar *name;
    gint frame = ud->preview->frame;

    tmp_dir = ghb_get_tmp_dir();
    name = g_strdup_printf("%s/live%02d", tmp_dir, ud->preview->frame);
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

        ud->preview->live_id = ghb_add_job(ghb_live_handle(), js);
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
#if GST_CHECK_VERSION(1, 0, 0)
    if (gst_element_query_duration(ud->preview->play, fmt, &len))
#else
    if (gst_element_query_duration(ud->preview->play, &fmt, &len))
#endif
    {
        if (len != -1 && fmt == GST_FORMAT_TIME)
        {
            ud->preview->len = len / GST_MSECOND;
        }
    }
#if GST_CHECK_VERSION(1, 0, 0)
    if (gst_element_query_position(ud->preview->play, fmt, &pos))
#else
    if (gst_element_query_position(ud->preview->play, &fmt, &pos))
#endif
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

static void _draw_pixbuf(cairo_t *cr, GdkPixbuf *pixbuf)
{
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
}

void
ghb_set_preview_image(signal_user_data_t *ud)
{
    GtkWidget *widget;
    gint preview_width, preview_height, target_height, width, height;

    g_debug("set_preview_button_image ()");
    gint title_id, titleindex;
    const hb_title_t *title;

    live_preview_stop(ud);

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return;
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

    ud->preview->pix =
        ghb_get_preview_image(title, ud->preview->frame, ud, &width, &height);
    if (ud->preview->pix == NULL) return;
    preview_width = gdk_pixbuf_get_width(ud->preview->pix);
    preview_height = gdk_pixbuf_get_height(ud->preview->pix);
    widget = GHB_WIDGET (ud->builder, "preview_image");
    if (preview_width != ud->preview->width ||
        preview_height != ud->preview->height)
    {
        preview_set_size(ud, preview_width, preview_height);
    }
    gtk_widget_queue_draw(widget);

    gchar *text = g_strdup_printf("%d x %d", width, height);
    widget = GHB_WIDGET (ud->builder, "preview_dims");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    g_debug("preview %d x %d", preview_width, preview_height);
    target_height = MIN(ud->preview->button_height, 200);
    height = target_height;
    width = preview_width * height / preview_height;
    if (width > 400)
    {
        width = 400;
        height = preview_height * width / preview_width;
    }

    if ((height >= 16) && (width >= 16))
    {
        GdkPixbuf *scaled_preview;
        scaled_preview = gdk_pixbuf_scale_simple (ud->preview->pix, width,
                                                height, GDK_INTERP_NEAREST);
        if (scaled_preview != NULL)
        {
            widget = GHB_WIDGET (ud->builder, "preview_button_image");
            gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled_preview);
            g_object_unref(scaled_preview);
        }
    }
}

static cairo_region_t*
curved_rect_mask(GtkWidget *widget)
{
    GdkWindow *window;
    cairo_region_t *shape;
    cairo_surface_t *surface;
    cairo_t *cr;
    double w, h;
    int radius;

    if (!gtk_widget_get_realized(widget))
        return NULL;

    window = gtk_widget_get_window(widget);
    w = gdk_window_get_width(window);
    h = gdk_window_get_height(window);
    if (w <= 50 || h <= 50)
        return NULL;
    radius = h / 4;
    surface = gdk_window_create_similar_surface(window,
                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                w, h);
    cr = cairo_create (surface);

    if (radius > w / 2)
        radius = w / 2;
    if (radius > h / 2)
        radius = h / 2;

    // fill shape with black
    cairo_save(cr);
    cairo_rectangle (cr, 0, 0, w, h);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_fill (cr);
    cairo_restore (cr);

    cairo_move_to  (cr, 0, radius);
    cairo_curve_to (cr, 0 , 0, 0 , 0, radius, 0);
    cairo_line_to (cr, w - radius, 0);
    cairo_curve_to (cr, w, 0, w, 0, w, radius);
    cairo_line_to (cr, w , h - radius);
    cairo_curve_to (cr, w, h, w, h, w - radius, h);
    cairo_line_to (cr, 0 + radius, h);
    cairo_curve_to (cr, 0, h, 0, h, 0, h - radius);

    cairo_close_path(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill(cr);

    cairo_destroy(cr);
    shape = gdk_cairo_region_create_from_surface(surface);
    cairo_surface_destroy(surface);

    return shape;
}

static void
hud_update_shape(GtkWidget *widget)
{
    cairo_region_t *shape;
    shape = curved_rect_mask(widget);
    if (shape != NULL)
    {
        gtk_widget_shape_combine_region(widget, shape);
        cairo_region_destroy(shape);
    }
}

#if defined(_ENABLE_GST)
#if GST_CHECK_VERSION(1, 0, 0)
G_MODULE_EXPORT gboolean
delayed_expose_cb(signal_user_data_t *ud)
{
    GstElement *vsink;
    GstVideoOverlay *vover;

    if (!ud->preview->live_enabled)
        return FALSE;

    g_object_get(ud->preview->play, "video-sink", &vsink, NULL);
    if (vsink == NULL)
        return FALSE;

    if (GST_IS_BIN(vsink))
        vover = GST_VIDEO_OVERLAY(gst_bin_get_by_interface(
                                GST_BIN(vsink), GST_TYPE_VIDEO_OVERLAY));
    else
        vover = GST_VIDEO_OVERLAY(vsink);
    gst_video_overlay_expose(vover);
    // This function is initiated by g_idle_add.  Must return false
    // so that it is not called again
    return FALSE;
}
#else
G_MODULE_EXPORT gboolean
delayed_expose_cb(signal_user_data_t *ud)
{
    GstElement *vsink;
    GstXOverlay *xover;

    if (!ud->preview->live_enabled)
        return FALSE;

    g_object_get(ud->preview->play, "video-sink", &vsink, NULL);
    if (vsink == NULL)
        return FALSE;

    if (GST_IS_BIN(vsink))
        xover = GST_X_OVERLAY(gst_bin_get_by_interface(
                                GST_BIN(vsink), GST_TYPE_X_OVERLAY));
    else
        xover = GST_X_OVERLAY(vsink);
    gst_x_overlay_expose(xover);
    // This function is initiated by g_idle_add.  Must return false
    // so that it is not called again
    return FALSE;
}
#endif
#endif

G_MODULE_EXPORT gboolean
preview_draw_cb(
    GtkWidget *widget,
    cairo_t *cr,
    signal_user_data_t *ud)
{
#if defined(_ENABLE_GST)
#if GST_CHECK_VERSION(1, 0, 0)
    if (ud->preview->live_enabled && ud->preview->state == PREVIEW_STATE_LIVE)
    {
        if (GST_STATE(ud->preview->play) >= GST_STATE_PAUSED)
        {
            GstElement *vsink;
            GstVideoOverlay *vover;

            g_object_get(ud->preview->play, "video-sink", &vsink, NULL);
            if (GST_IS_BIN(vsink))
                vover = GST_VIDEO_OVERLAY(gst_bin_get_by_interface(
                                    GST_BIN(vsink), GST_TYPE_VIDEO_OVERLAY));
            else
                vover = GST_VIDEO_OVERLAY(vsink);
            gst_video_overlay_expose(vover);
            // For some reason, the exposed region doesn't always get
            // cleaned up here. But a delayed gst_x_overlay_expose()
            // takes care of it.
            g_idle_add((GSourceFunc)delayed_expose_cb, ud);
        }
        return FALSE;
    }
#else
    if (ud->preview->live_enabled && ud->preview->state == PREVIEW_STATE_LIVE)
    {
        if (GST_STATE(ud->preview->play) >= GST_STATE_PAUSED)
        {
            GstElement *vsink;
            GstXOverlay *xover;

            g_object_get(ud->preview->play, "video-sink", &vsink, NULL);
            if (GST_IS_BIN(vsink))
                xover = GST_X_OVERLAY(gst_bin_get_by_interface(
                                        GST_BIN(vsink), GST_TYPE_X_OVERLAY));
            else
                xover = GST_X_OVERLAY(vsink);
            gst_x_overlay_expose(xover);
            // For some reason, the exposed region doesn't always get
            // cleaned up here. But a delayed gst_x_overlay_expose()
            // takes care of it.
            g_idle_add((GSourceFunc)delayed_expose_cb, ud);
        }
        return FALSE;
    }
#endif
#endif

    if (ud->preview->pix != NULL)
    {
        _draw_pixbuf(cr, ud->preview->pix);
    }
    return FALSE;
}

G_MODULE_EXPORT void
preview_button_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, signal_user_data_t *ud)
{
    g_debug("allocate %d x %d", allocation->width, allocation->height);
    if (ud->preview->button_width == allocation->width &&
        ud->preview->button_height == allocation->height)
    {
        // Nothing to do. Bug out.
        g_debug("nothing to do");
        return;
    }
    g_debug("prev allocate %d x %d", ud->preview->button_width,
            ud->preview->button_height);
    ud->preview->button_width = allocation->width;
    ud->preview->button_height = allocation->height;
    ghb_set_preview_image(ud);
}

void
ghb_preview_set_visible(signal_user_data_t *ud)
{
    gint title_id, titleindex;
    const hb_title_t *title;
    GtkToggleToolButton *button;
    GtkWidget *widget;
    gboolean active;

    button = GTK_TOGGLE_TOOL_BUTTON(GHB_WIDGET(ud->builder, "show_preview"));
    active = gtk_toggle_tool_button_get_active(button);

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    active &= title != NULL;
    widget = GHB_WIDGET(ud->builder, "preview_window");
    gtk_widget_set_visible(widget, active);
    if (active)
    {
        gint x, y;
        x = ghb_dict_get_int(ud->prefs, "preview_x");
        y = ghb_dict_get_int(ud->prefs, "preview_y");
        if (x >= 0 && y >= 0)
            gtk_window_move(GTK_WINDOW(widget), x, y);
    }
}

static void
update_preview_labels(signal_user_data_t *ud, gboolean active)
{
    GtkToolButton *button;

    button   = GTK_TOOL_BUTTON(GHB_WIDGET(ud->builder, "show_preview"));

    if (!active)
    {
        gtk_tool_button_set_label(button, "Show\nPreview");
    }
    else
    {
        gtk_tool_button_set_label(button, "Hide\nPreview");
    }
}

G_MODULE_EXPORT void
preview_toggled_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkCheckMenuItem *menuitem;
    gboolean          active;

    active = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(xwidget));
    ghb_preview_set_visible(ud);
    update_preview_labels(ud, active);

    menuitem = GTK_CHECK_MENU_ITEM(GHB_WIDGET(ud->builder,
                                              "show_preview_menu"));
    gtk_check_menu_item_set_active(menuitem, active);
}

G_MODULE_EXPORT void
preview_menu_toggled_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkToggleToolButton *button;
    gboolean active;

    active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(xwidget));
    button = GTK_TOGGLE_TOOL_BUTTON(GHB_WIDGET(ud->builder, "show_preview"));
    gtk_toggle_tool_button_set_active(button, active);
}

static gboolean
go_full(signal_user_data_t *ud)
{
    GtkWindow *window;
    window = GTK_WINDOW(GHB_WIDGET (ud->builder, "preview_window"));
    gtk_window_fullscreen(window);
    ghb_set_preview_image(ud);
    return FALSE;
}

G_MODULE_EXPORT void
fullscreen_clicked_cb(GtkWidget *toggle, signal_user_data_t *ud)
{
    gboolean active;
    GtkWindow *window;

    g_debug("fullscreen_clicked_cb()");
    ghb_widget_to_setting (ud->prefs, toggle);
    ghb_check_dependency(ud, toggle, NULL);
    const gchar *name = ghb_get_setting_key(toggle);
    ghb_pref_save(ud->prefs, name);

    window = GTK_WINDOW(GHB_WIDGET (ud->builder, "preview_window"));
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
    if (active)
    {
        gtk_window_set_resizable(window, TRUE);
        gtk_button_set_label(GTK_BUTTON(toggle), _("Windowed"));
        // Changing resizable property doesn't take effect immediately
        // need to delay fullscreen till after this callback returns
        // to mainloop
        g_idle_add((GSourceFunc)go_full, ud);
    }
    else
    {
        gtk_window_unfullscreen(window);
        gtk_window_set_resizable(window, FALSE);
        gtk_button_set_label(GTK_BUTTON(toggle), _("Fullscreen"));
        ghb_set_preview_image(ud);
    }
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
    GdkEvent *event,
    signal_user_data_t *ud)
{
    live_preview_stop(ud);
    widget = GHB_WIDGET(ud->builder, "show_preview");
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), FALSE);
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

static gboolean in_hud = FALSE;

G_MODULE_EXPORT gboolean
hud_enter_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
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
    widget = GHB_WIDGET(ud->builder, "preview_hud");
    if (!gtk_widget_get_visible(widget))
    {
        gtk_widget_show(widget);
    }
    hud_timeout_id = 0;
    in_hud = TRUE;
    return FALSE;
}

G_MODULE_EXPORT gboolean
hud_leave_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
{
    in_hud = FALSE;
    return FALSE;
}

G_MODULE_EXPORT gboolean
preview_leave_cb(
    GtkWidget *widget,
    GdkEventCrossing *event,
    signal_user_data_t *ud)
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
    return FALSE;
}

G_MODULE_EXPORT gboolean
preview_motion_cb(
    GtkWidget *widget,
    GdkEventMotion *event,
    signal_user_data_t *ud)
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
    widget = GHB_WIDGET(ud->builder, "preview_hud");
    if (!gtk_widget_get_visible(widget))
    {
        gtk_widget_show(widget);
    }
    if (!in_hud)
    {
        hud_timeout_id = g_timeout_add_seconds(4, (GSourceFunc)hud_timeout, ud);
    }
    return FALSE;
}

G_MODULE_EXPORT void
hud_size_alloc_cb(
    GtkWidget *widget,
    GtkAllocation *allocation,
    signal_user_data_t *ud)
{
    hud_update_shape(widget);
}

G_MODULE_EXPORT gboolean
preview_configure_cb(
    GtkWidget *widget,
    GdkEventConfigure *event,
    signal_user_data_t *ud)
{
    gint x, y;

    if (gtk_widget_get_visible(widget))
    {
        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
        ghb_dict_set_int(ud->prefs, "preview_x", x);
        ghb_dict_set_int(ud->prefs, "preview_y", y);
        ghb_pref_set(ud->prefs, "preview_x");
        ghb_pref_set(ud->prefs, "preview_y");
        ghb_prefs_store();
    }
    return FALSE;
}

