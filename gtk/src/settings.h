/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#if !defined(_SETTINGS_H_)
#define _SETTINGS_H_

#include <gtk/gtk.h>

#define GHB_WIDGET(b,n)	GTK_WIDGET(gtk_builder_get_object ((b), (n)))
//#define GHB_WIDGET(b,n)	GTK_WIDGET(debug_get_object((b), (n)))
#define GHB_ACTION(b,n)	GTK_ACTION(gtk_builder_get_object ((b), (n)))
#define GHB_OBJECT(b,n)	gtk_builder_get_object ((b), (n))

GObject* debug_get_object(GtkBuilder *b, const gchar *n);

enum
{
	GHB_STATE_IDLE 		= 0x00,
	GHB_STATE_SCANNING 	= 0x02,
	GHB_STATE_SCANDONE 	= 0x04,
	GHB_STATE_WORKING 	= 0x08,
	GHB_STATE_WORKDONE 	= 0x10,
	GHB_STATE_PAUSED 	= 0x20,
	GHB_STATE_MUXING 	= 0x40,
};

typedef struct
{
	gchar *current_dvd_device;
	gboolean debug;
	gboolean dont_clear_presets;
	gboolean cancel_encode;
	GtkBuilder *builder;
	GValue *settings;
	GValue *queue;
	GValue *current_job;
	GIOChannel *activity_log;
	GIOChannel *job_activity_log;
	gchar *appcast;
	gint appcast_len;
} signal_user_data_t;

enum
{
	GHB_QUEUE_PENDING,
	GHB_QUEUE_RUNNING,
	GHB_QUEUE_CANCELED,
	GHB_QUEUE_DONE,
};

GValue* ghb_settings_new(void);
void ghb_settings_take_value(
	GValue *settings, const gchar *key, GValue *value);
void ghb_settings_set_value(
	GValue *settings, const gchar *key, const GValue *value);
void ghb_settings_set_string(
	GValue *settings, const gchar *key, const gchar *sval);
void ghb_settings_set_double(GValue *settings, const gchar *key, gdouble dval);
void ghb_settings_set_int64(GValue *settings, const gchar *key, gint64 ival);
void ghb_settings_set_int(GValue *settings, const gchar *key, gint ival);
void ghb_settings_set_boolean(
	GValue *settings, const gchar *key, gboolean bval);
void ghb_settings_copy(
	GValue *settings, const gchar *key, const GValue *value);
GValue* ghb_settings_get_value(GValue *settings, const gchar *key);
gboolean ghb_settings_get_boolean(GValue *settings, const gchar *key);
gint64 ghb_settings_get_int64(GValue *settings, const gchar *key);
gint ghb_settings_get_int(GValue *settings, const gchar *key);
gdouble ghb_settings_get_double(GValue *settings, const gchar *key);
gchar* ghb_settings_get_string(GValue *settings, const gchar *key);
gint ghb_settings_combo_int(GValue *settings, const gchar *key);
const gchar* ghb_settings_combo_option(GValue *settings, const gchar *key);

GValue* ghb_widget_value(GtkWidget *widget);
gchar* ghb_widget_string(GtkWidget *widget);
gdouble ghb_widget_double(GtkWidget *widget);
gint64 ghb_widget_int64(GtkWidget *widget);
gint ghb_widget_int(GtkWidget *widget);
gint ghb_widget_boolean(GtkWidget *widget);

void ghb_widget_to_setting(GValue *settings, GtkWidget *widget);
int ghb_ui_update(
	signal_user_data_t *ud, const gchar *name, const GValue *value);

#endif // _SETTINGS_H_
