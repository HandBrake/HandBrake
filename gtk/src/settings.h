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

#define PRESET_CUSTOM	1
#define PRESET_DEFAULT	2

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
	GtkBuilder *builder;
	GHashTable *settings;
	GSList *audio_settings;
	gchar **chapter_list;
	GSList *queue;
	GIOChannel *activity_log;
} signal_user_data_t;

enum
{
	GHB_QUEUE_PENDING,
	GHB_QUEUE_RUNNING,
	GHB_QUEUE_CANCELED,
	GHB_QUEUE_DONE,
};

typedef struct
{
	gint unique_id;
	gint status;
	GHashTable *settings;
	GSList *audio_settings;
	gchar **chapter_list;
} job_settings_t;

typedef struct
{
	gint index;
	gchar *option;
	gchar *shortOpt;
	gint ivalue;
	gdouble dvalue;
	gchar *svalue;
} setting_value_t;

GHashTable* ghb_settings_new();
void ghb_settings_set(GHashTable *settings, const gchar *key, 
					  setting_value_t *value);
void ghb_settings_set_string(GHashTable *settings, const gchar *key, const gchar *str);
void ghb_settings_set_dbl(GHashTable *settings, const gchar *key, gdouble dvalue);
void ghb_settings_copy(GHashTable *settings, const gchar *key, const setting_value_t *value);
const setting_value_t* ghb_settings_get(GHashTable *settings, const gchar *key);
gboolean ghb_settings_get_bool(GHashTable *settings, const gchar *key);
gint ghb_settings_get_index(GHashTable *settings, const gchar *key);
gint ghb_settings_get_int(GHashTable *settings, const gchar *key);
gdouble ghb_settings_get_dbl(GHashTable *settings, const gchar *key);
setting_value_t* ghb_widget_value(GtkWidget *widget);
gchar* ghb_widget_short_opt(GtkWidget *widget);
gchar* ghb_widget_option(GtkWidget *widget);
gchar* ghb_widget_string(GtkWidget *widget);
gint ghb_widget_int(GtkWidget *widget);
gdouble ghb_widget_dbl(GtkWidget *widget);
gint ghb_widget_index(GtkWidget *widget);

const gchar* ghb_settings_get_string(GHashTable *settings, const gchar *key);
const gchar* ghb_settings_get_option(GHashTable *settings, const gchar *name);
const gchar* ghb_settings_get_short_opt(GHashTable *settings, const gchar *key);
GHashTable* ghb_settings_dup(GHashTable *settings);
void ghb_widget_to_setting(GHashTable *settings, GtkWidget *widget);
int ghb_ui_update(signal_user_data_t *ud, const gchar *key, const gchar *value);
int ghb_ui_update_int(signal_user_data_t *ud, const gchar *key, gint ivalue);
void ghb_settings_save(signal_user_data_t *ud, const gchar *name);
void ghb_presets_load(signal_user_data_t *ud);
void ghb_set_preset(signal_user_data_t *ud, const gchar *name);
void ghb_update_from_preset( signal_user_data_t *ud, 
							const gchar *name, const gchar *key);
gchar** ghb_presets_get_names();
gchar** ghb_presets_get_descriptions();
const gchar* ghb_presets_get_name(gint index);
gboolean ghb_presets_is_standard(const gchar *name);
gboolean ghb_presets_remove(GHashTable *settings, const gchar *name);
void ghb_presets_revert(signal_user_data_t *ud, const gchar *name);
GdkColor* ghb_presets_color(gboolean modified);
const gchar* ghb_presets_current_name();
gint ghb_presets_list_index(const gchar *name);
gint ghb_preset_flags(const gchar *name, gint *index);
void ghb_prefs_load(signal_user_data_t *ud);
void ghb_prefs_to_ui(signal_user_data_t *ud);
void ghb_prefs_save(GHashTable *settings);
void ghb_pref_save(GHashTable *settings, const gchar *key);
void ghb_set_preset_default(GHashTable *settings);
void ghb_x264_parse_options(signal_user_data_t *ud, const gchar *options);
void ghb_x264_opt_update(signal_user_data_t *ud, GtkWidget *widget);
gchar* ghb_sanitize_x264opts(signal_user_data_t *ud, const gchar *options);
gint ghb_pref_acount();
gint ghb_pref_acodec(gint index);
gint ghb_pref_bitrate(gint index);
gint ghb_pref_rate(gint index);
gint ghb_pref_mix(gint index);
gdouble ghb_pref_drc(gint index);

#endif // _SETTINGS_H_
