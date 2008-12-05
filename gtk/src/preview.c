/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preview.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
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
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>
#include <gst/pbutils/missing-plugins.h>
#include "settings.h"
#include "callbacks.h"
#include "hb-backend.h"
#include "preview.h"
#include "values.h"
#include "hb.h"

#define PREVIEW_STATE_IMAGE 0
#define PREVIEW_STATE_LIVE 1

struct preview_s
{
	GstElement *play;
	gint64 len;
	gint64 pos;
	gboolean seek_lock;
	gboolean progress_lock;
	gint width;
	gint height;
	GtkWidget *view;
	gulong xid;
	GdkPixbuf *pix;
	gint button_width;
	gint button_height;
	gint frame;
	gint state;
	gboolean pause;
	gboolean encoded[10];
	gint encode_frame;
	gchar *current;
};

static gboolean live_preview_cb(GstBus *bus, GstMessage *msg, gpointer data);
static GstBusSyncReply create_window(GstBus *bus, GstMessage *msg, 
				gpointer data);
gboolean preview_expose_cb(GtkWidget *widget, GdkEventExpose *event, 
				signal_user_data_t *ud);

void
ghb_screen_par(signal_user_data_t *ud, gint *par_n, gint *par_d)
{
	GValue disp_par = {0,};
	GstElement *xover;
	GObjectClass *klass;
	GParamSpec *pspec;

	g_value_init(&disp_par, GST_TYPE_FRACTION);
	gst_value_set_fraction(&disp_par, 1, 1);
	g_object_get(ud->preview->play, "video-sink", &xover, NULL);
	klass = G_OBJECT_GET_CLASS(xover);
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
}

void
ghb_par_scale(signal_user_data_t *ud, gint *width, gint *height, gint par_n, gint par_d)
{
	gint disp_par_n, disp_par_d;
	gint64 num, den;

	ghb_screen_par(ud, &disp_par_n, &disp_par_d);
	num = par_n * disp_par_d;
	den = par_d * disp_par_n;

	if (num > den)
		*width = *width * num / den;
	else
		*height = *height * den / num;
}

void
ghb_preview_init(signal_user_data_t *ud)
{
	GstBus *bus;
	GstElement *xover;

	ud->preview = g_malloc0(sizeof(preview_t));
	ud->preview->view = GHB_WIDGET(ud->builder, "preview_image");
	gtk_widget_realize(ud->preview->view);
	g_signal_connect(G_OBJECT(ud->preview->view), "expose_event",
					G_CALLBACK(preview_expose_cb), ud);
	ud->preview->xid = GDK_DRAWABLE_XID(ud->preview->view->window);

	ud->preview->play = gst_element_factory_make("playbin", "play");
	ud->preview->pause = TRUE;
	//xover = gst_element_factory_make("xvimagesink", "xover");
	xover = gst_element_factory_make("gconfvideosink", "xover");
	g_object_set(G_OBJECT(ud->preview->play), "video-sink", xover, NULL);
	//g_object_set(G_OBJECT(xover), "force-aspect-ratio", TRUE, NULL);

	bus = gst_pipeline_get_bus(GST_PIPELINE(ud->preview->play));
	gst_bus_add_watch(bus, live_preview_cb, ud);
	gst_bus_set_sync_handler(bus, create_window, ud->preview);
	gst_object_unref(bus);
}

void
ghb_preview_cleanup(signal_user_data_t *ud)
{
	if (ud->preview->current)
	{
		ud->preview->current = NULL;
		g_free(ud->preview->current);
	}
}

static GstBusSyncReply
create_window(GstBus *bus, GstMessage *msg, gpointer data)
{
	preview_t *preview = (preview_t*)data;

	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_ELEMENT:
	{
		if (!gst_structure_has_name(msg->structure, "prepare-xwindow-id"))
			return GST_BUS_PASS;
		gst_x_overlay_set_xwindow_id(
			GST_X_OVERLAY(GST_MESSAGE_SRC(msg)), preview->xid);
		gst_message_unref(msg);
		return GST_BUS_DROP;
	} break;

	default:
	{
	} break;
	}
	return GST_BUS_PASS;
}

static GList *
get_stream_info_objects_for_type (GstElement *play, const gchar *typestr)
{
	GValueArray *info_arr = NULL;
	GList *ret = NULL;
	guint ii;

	if (play == NULL)
		return NULL;

	g_object_get(play, "stream-info-value-array", &info_arr, NULL);
	if (info_arr == NULL)
		return NULL;

	for (ii = 0; ii < info_arr->n_values; ++ii) 
	{
		GObject *info_obj;
		GValue *val;

		val = g_value_array_get_nth(info_arr, ii);
		info_obj = g_value_get_object(val);
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
	}
	g_value_array_free (info_arr);
	return g_list_reverse (ret);
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

		if (ghb_settings_get_boolean(ud->settings, "reduce_hd_preview"))
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
			gtk_widget_set_size_request(ud->preview->view, width, height);
			ud->preview->width = width;
			ud->preview->height = height;
		}
	}
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

static gboolean
live_preview_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
	signal_user_data_t *ud = (signal_user_data_t*)data;

	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_ERROR:
	{
		GError *err;
		gchar *debug;

		gst_message_parse_error(msg, &err, &debug);
		g_warning("Gstreamer Error: %s", err->message);
		g_error_free(err);
		g_free(debug);
	} break;

	case GST_MESSAGE_ELEMENT:
	{
		if (gst_is_missing_plugin_message(msg))
		{
			gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
			gchar *message, *desc;
			desc = gst_missing_plugin_message_get_description(msg);
			message = g_strdup_printf(
						"Missing GStreamer plugin\n"
						"Audio or Video may not play as expected\n\n%s",
						desc);
			ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Ok", NULL);
			g_free(message);
			gst_element_set_state(ud->preview->play, GST_STATE_PLAYING);
		}
	} break;

	case GST_MESSAGE_STATE_CHANGED:
	{
		GstState state, pending;
		gst_element_get_state(ud->preview->play, &state, &pending, 0);
		if (state == GST_STATE_PAUSED || state == GST_STATE_PLAYING)
		{
			update_stream_info(ud);
		}
	} break;

	case GST_MESSAGE_EOS:
	{
		// Done
		GtkImage *img;

		img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
		gtk_image_set_from_stock(img, "gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
		ud->preview->pause = TRUE;
		gst_element_seek(ud->preview->play, 1.0,
			GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
			GST_SEEK_TYPE_SET, 0,
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	} break;

	default:
	{
		// Ignore
	}
	}
	return TRUE;
}

void
live_preview_start(signal_user_data_t *ud)
{
	GtkImage *img;
	gchar *uri;

	img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
	if (!ud->preview->encoded[ud->preview->frame])
	{
		gtk_image_set_from_stock(img, "gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gst_element_set_state(ud->preview->play, GST_STATE_NULL);
		ud->preview->pause = TRUE;
		return;
	}

	uri = g_strdup_printf("file://%s", ud->preview->current);
	gtk_image_set_from_stock(img, "gtk-media-pause", GTK_ICON_SIZE_BUTTON);
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

	img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
	gtk_image_set_from_stock(img, "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	gst_element_set_state(ud->preview->play, GST_STATE_PAUSED);
	ud->preview->pause = TRUE;
}

void
live_preview_stop(signal_user_data_t *ud)
{
	GtkImage *img;
	GtkRange *progress;

	img = GTK_IMAGE(GHB_WIDGET(ud->builder, "live_preview_play_image"));
	gtk_image_set_from_stock(img, "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	gst_element_set_state(ud->preview->play, GST_STATE_NULL);
	ud->preview->pause = TRUE;
	ud->preview->state = PREVIEW_STATE_IMAGE;

	progress = GTK_RANGE(GHB_WIDGET(ud->builder, "live_preview_progress"));
	gtk_range_set_value(progress, 0);
}

void
ghb_live_reset(signal_user_data_t *ud)
{
	gboolean encoded;

	if (!ud->preview->pause)
		live_preview_stop(ud);
	if (ud->preview->current)
	{
		g_free(ud->preview->current);
		ud->preview->current = NULL;
	}
	encoded = ud->preview->encoded[ud->preview->frame];
	memset(ud->preview->encoded, 0, sizeof(gboolean) * 10);
	if (encoded)
		ghb_set_preview_image(ud);
}

extern void hb_get_tempory_directory(hb_handle_t *h, char path[512]);

void
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
		if (ud->preview->pause)
			live_preview_start(ud);
		else
			live_preview_pause(ud);
	}
	else
	{
		GValue *js;

		ud->preview->encode_frame = frame;
		js = ghb_value_dup(ud->settings);
		ghb_settings_set_string(js, "destination", name);
		ghb_settings_set_int(js, "start_frame", ud->preview->frame);
		ghb_settings_set_int(js, "live_duration", 15);
		ghb_add_live_job(js, 0);
		ghb_start_live_encode();
		ghb_value_free(js);
	}
}

void
ghb_live_encode_done(signal_user_data_t *ud, gboolean success)
{
	GtkWidget *widget;
	GtkWidget *prog;

	prog = GHB_WIDGET(ud->builder, "live_encode_progress");
	if (success && 
		ud->preview->encode_frame == ud->preview->frame)
	{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), "Done");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(prog), 1);
		ud->preview->encoded[ud->preview->encode_frame] = TRUE;
		live_preview_start(ud);
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

static gboolean
unlock_progress_cb(signal_user_data_t *ud)
{
	ud->preview->progress_lock = FALSE;
	// This function is initiated by g_idle_add.  Must return false
	// so that it is not called again
	return FALSE;
}

void
ghb_live_preview_progress(signal_user_data_t *ud)
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len = -1, pos = -1;

	if (ud->preview->state != PREVIEW_STATE_LIVE || ud->preview->seek_lock)
		return;

	ud->preview->progress_lock = TRUE;
	if (gst_element_query_duration(ud->preview->play, &fmt, &len))
	{
		if (len != -1 && fmt == GST_FORMAT_TIME)
		{
			ud->preview->len = len / GST_MSECOND;
		}
	}
	if (gst_element_query_position(ud->preview->play, &fmt, &pos))
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
}

static gboolean
unlock_seek_cb(signal_user_data_t *ud)
{
	ud->preview->seek_lock = FALSE;
	// This function is initiated by g_idle_add.  Must return false
	// so that it is not called again
	return FALSE;
}

void
live_preview_seek_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gdouble dval;
	gint64 pos;

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
}

void
ghb_set_preview_image(signal_user_data_t *ud)
{
	GtkWidget *widget;
	gint preview_width, preview_height, target_height, width, height;

	g_debug("set_preview_button_image ()");
	gint titleindex;

	live_preview_stop(ud);

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return;
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
		ghb_get_preview_image(titleindex, ud->preview->frame, 
								ud, TRUE, &width, &height);
	if (ud->preview->pix == NULL) return;
	preview_width = gdk_pixbuf_get_width(ud->preview->pix);
	preview_height = gdk_pixbuf_get_height(ud->preview->pix);
	widget = GHB_WIDGET (ud->builder, "preview_image");
	if (preview_width != ud->preview->width || 
		preview_height != ud->preview->height)
	{
		gtk_widget_set_size_request(widget, preview_width, preview_height);
		ud->preview->width = preview_width;
		ud->preview->height = preview_height;
	}
	gdk_draw_pixbuf(
		widget->window, NULL, ud->preview->pix, 0, 0, 0, 0,
		-1, -1, GDK_RGB_DITHER_NONE, 0, 0);

	gchar *text = g_strdup_printf("%d x %d", width, height);
	widget = GHB_WIDGET (ud->builder, "preview_dims");
	gtk_label_set_text(GTK_LABEL(widget), text);
	g_free(text);
	
	g_debug("preview %d x %d", preview_width, preview_height);
	target_height = MIN(ud->preview->button_height, 128);
	height = target_height;
	width = preview_width * height / preview_height;

	if ((height >= 16) && (width >= 16))
	{
		GdkPixbuf *scaled_preview;
		scaled_preview = gdk_pixbuf_scale_simple (ud->preview->pix, width, 
												height, GDK_INTERP_NEAREST);
		if (scaled_preview != NULL)
		{
			widget = GHB_WIDGET (ud->builder, "preview_button_image");
			gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled_preview);
			g_object_unref (scaled_preview);
		}
	}
}

static gboolean
delayed_expose_cb(signal_user_data_t *ud)
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
	// This function is initiated by g_idle_add.  Must return false
	// so that it is not called again
	return FALSE;
}

gboolean
preview_expose_cb(
	GtkWidget *widget, 
	GdkEventExpose *event, 
	signal_user_data_t *ud)
{
	if (ud->preview->state == PREVIEW_STATE_LIVE)
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
			return FALSE;
		}
		return TRUE;
	}

	gdk_draw_pixbuf(
		widget->window, NULL, ud->preview->pix, 0, 0, 0, 0,
		-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
	return TRUE;
}

void
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
preview_button_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return;
	g_debug("titleindex %d", titleindex);

	GtkWidget *widget = GHB_WIDGET (ud->builder, "preview_window");
	gtk_widget_show (widget);
}

void
preview_frame_value_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_set_preview_image(ud);
}

