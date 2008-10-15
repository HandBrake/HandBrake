/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * settings.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * settings.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 */
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "settings.h"
#include "hb-backend.h"
#include "values.h"

void dump_settings(GValue *settings);
void ghb_pref_audio_init(signal_user_data_t *ud);

GObject*
debug_get_object(GtkBuilder* b, const gchar *n)
{
	g_message("name %s\n", n);
	return gtk_builder_get_object(b, n);
}

GValue*
ghb_settings_new()
{
	return ghb_dict_value_new();
}

void
ghb_settings_set_value(
	GValue *settings, 
	const gchar *key, 
	const GValue *value)
{
	if (key == NULL || value == NULL)
		return;
	ghb_dict_insert(settings, g_strdup(key), ghb_value_dup(value));
}

void
ghb_settings_take_value(GValue *settings, const gchar *key, GValue *value)
{
	ghb_dict_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_string(
	GValue *settings, 
	const gchar *key, 
	const gchar *sval)
{
	GValue *value;
	value = ghb_string_value_new(sval);
	ghb_dict_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_double(GValue *settings, const gchar *key, gdouble dval)
{
	GValue *value;
	value = ghb_double_value_new(dval);
	ghb_dict_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_int64(GValue *settings, const gchar *key, gint64 ival)
{
	GValue *value;
	value = ghb_int64_value_new(ival);
	ghb_dict_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_int(GValue *settings, const gchar *key, gint ival)
{
	GValue *value;
	value = ghb_int64_value_new((gint64)ival);
	ghb_dict_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_boolean(GValue *settings, const gchar *key, gboolean bval)
{
	GValue *value;
	value = ghb_boolean_value_new(bval);
	ghb_dict_insert(settings, g_strdup(key), value);
}

GValue*
ghb_settings_get_value(GValue *settings, const gchar *key)
{
	GValue *value;
	value = ghb_dict_lookup(settings, key);
	if (value == NULL)
		g_warning("returning null (%s)", key);
	return value;
}

gboolean
ghb_settings_get_boolean(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return FALSE;
	return ghb_value_boolean(value);
}

gint64
ghb_settings_get_int64(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return 0;
	return ghb_value_int64(value);
}

gint
ghb_settings_get_int(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return 0;
	return ghb_value_int(value);
}

gdouble
ghb_settings_get_double(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return 0;
	return ghb_value_double(value);
}

gchar*
ghb_settings_get_string(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return g_strdup("");
	return ghb_value_string(value);
}

gint
ghb_settings_combo_int(GValue *settings, const gchar *key)
{
	return ghb_lookup_combo_int(key, ghb_settings_get_value(settings, key));
}

const gchar*
ghb_settings_combo_option(GValue *settings, const gchar *key)
{
	return ghb_lookup_combo_option(key, ghb_settings_get_value(settings, key));
}

// Map widget names to setting keys
// Widgets that map to settings have names
// of this format: s_<setting key>
static const gchar*
get_setting_key(GtkWidget *widget)
{
	const gchar *name;
	
	g_debug("get_setting_key ()\n");
	if (widget == NULL) return NULL;
	if (GTK_IS_ACTION(widget))
		name = gtk_action_get_name(GTK_ACTION(widget));
	else
		name = gtk_widget_get_name(widget);
		
	if (name == NULL)
	{
		// Bad widget pointer?  Should never happen.
		g_debug("Bad widget\n");
		return NULL;
	}
	return name;
}

GValue*
ghb_widget_value(GtkWidget *widget)
{
	GValue *value = NULL;
	const gchar *name;
	GType type;
	
	if (widget == NULL)
	{
		g_debug("NULL widget\n");
		return NULL;
	}

	type = GTK_WIDGET_TYPE(widget);
	if (GTK_IS_ACTION(widget))
		name = gtk_action_get_name(GTK_ACTION(widget));
	else
		name = gtk_widget_get_name(widget);
	g_debug("ghb_widget_value widget (%s)\n", name);
	if (type == GTK_TYPE_ENTRY)
	{
		const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
		value = ghb_string_value_new(str);
	}
	else if (type == GTK_TYPE_RADIO_BUTTON)
	{
		g_debug("\tradio_button");
		gboolean bval;
		bval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		value = ghb_boolean_value_new(bval);
	}
	else if (type == GTK_TYPE_CHECK_BUTTON)
	{
		g_debug("\tcheck_button");
		gboolean bval;
		bval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		value = ghb_boolean_value_new(bval);
	}
	else if (type == GTK_TYPE_TOGGLE_ACTION)
	{
		g_debug("\ttoggle action");
		gboolean bval;
		bval = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget));
		value = ghb_boolean_value_new(bval);
	}
	else if (type == GTK_TYPE_CHECK_MENU_ITEM)
	{
		g_debug("\tcheck_menu_item");
		gboolean bval;
		bval = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
		value = ghb_boolean_value_new(bval);
	}
	else if (type == GTK_TYPE_COMBO_BOX)
	{
		g_debug("\tcombo_box");
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt;

		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
		{
			gtk_tree_model_get(store, &iter, 2, &shortOpt, -1);
			value = ghb_string_value_new(shortOpt);
			g_free(shortOpt);
		}
		else
		{
			value = ghb_string_value_new("");
		}
	}
	else if (type == GTK_TYPE_COMBO_BOX_ENTRY)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt;

		g_debug("\tcombo_box_entry");
		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
		{
			gtk_tree_model_get(store, &iter, 2, &shortOpt, -1);
			value = ghb_string_value_new(shortOpt);
			g_free(shortOpt);
		}
		else
		{
			const gchar *str;
			str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
			if (str == NULL) str = "";
			value = ghb_string_value_new(str);
		}
	}
	else if (type == GTK_TYPE_SPIN_BUTTON)
	{
		gint ival;
		ival = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
		value = ghb_int64_value_new(ival);
	}
	else if (type == GTK_TYPE_HSCALE)
	{
		gdouble dval;
		gint digits;

		digits = gtk_scale_get_digits(GTK_SCALE(widget));
		dval = gtk_range_get_value(GTK_RANGE(widget));
		if (digits)
		{
			value = ghb_double_value_new(dval);
		}
		else
		{
			value = ghb_int_value_new(dval);
		}
	}
	else if (type == GTK_TYPE_TEXT_VIEW)
	{
		GtkTextBuffer *buffer;
		GtkTextIter start, end;
		gchar *str;

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		str = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		value = ghb_string_value_new(str);
		g_free(str);
	}
	else if (type == GTK_TYPE_LABEL)
	{
		const gchar *str;
		str = gtk_label_get_text (GTK_LABEL(widget));
		value = ghb_string_value_new(str);
	}
	else
	{
		g_debug("Attempt to set unknown widget type: %s\n", name);
		g_free(value);
		value = NULL;
	}
	return value;
}

gchar*
ghb_widget_string(GtkWidget *widget)
{
	GValue *value;
	gchar *sval;
	
	value = ghb_widget_value(widget);
	sval = ghb_value_string(value);
	ghb_value_free(value);
	return sval;
}

gdouble
ghb_widget_double(GtkWidget *widget)
{
	GValue *value;
	gdouble dval;
	
	value = ghb_widget_value(widget);
	dval = ghb_value_double(value);
	ghb_value_free(value);
	return dval;
}

gint64
ghb_widget_int64(GtkWidget *widget)
{
	GValue *value;
	gint64 ival;
	
	value = ghb_widget_value(widget);
	ival = ghb_value_int64(value);
	ghb_value_free(value);
	return ival;
}

gint
ghb_widget_int(GtkWidget *widget)
{
	GValue *value;
	gint ival;
	
	value = ghb_widget_value(widget);
	ival = (gint)ghb_value_int64(value);
	ghb_value_free(value);
	return ival;
}

gint
ghb_widget_boolean(GtkWidget *widget)
{
	GValue *value;
	gboolean bval;
	
	value = ghb_widget_value(widget);
	bval = ghb_value_boolean(value);
	ghb_value_free(value);
	return bval;
}

void
ghb_widget_to_setting(GValue *settings, GtkWidget *widget)
{
	const gchar *key = NULL;
	GValue *value;
	
	if (widget == NULL) return;
	g_debug("ghb_widget_to_setting");
	// Find corresponding setting
	key = get_setting_key(widget);
	if (key == NULL) return;
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		ghb_settings_take_value(settings, key, value);
	}
	else
	{
		g_debug("No value found for %s\n", key);
	}
}

static void
update_widget(GtkWidget *widget, const GValue *value)
{
	GType type;
	gchar *str;
	gint ival;
	gdouble dval;

	g_debug("update_widget");
	type = G_VALUE_TYPE(value);
	if (type == ghb_array_get_type() || type == ghb_dict_get_type())
		return;
	if (value == NULL) return;
	str = ghb_value_string(value);
	ival = ghb_value_int(value);
	dval = ghb_value_double(value);
	type = GTK_OBJECT_TYPE(widget);
	if (type == GTK_TYPE_ENTRY)
	{
		g_debug("entry");
		gtk_entry_set_text((GtkEntry*)widget, str);
	}
	else if (type == GTK_TYPE_RADIO_BUTTON)
	{
		g_debug("radio button");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), ival);
	}
	else if (type == GTK_TYPE_CHECK_BUTTON)
	{
		g_debug("check button");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), ival);
	}
	else if (type == GTK_TYPE_TOGGLE_ACTION)
	{
		g_debug("toggle action");
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(widget), ival);
	}
	else if (type == GTK_TYPE_CHECK_MENU_ITEM)
	{
		g_debug("check menu item");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), ival);
	}
	else if (type == GTK_TYPE_COMBO_BOX)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt;
		gint ivalue;
		gboolean foundit = FALSE;

		g_debug("combo (%s)", str);
		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 2, &shortOpt, -1);
				if (strcmp(shortOpt, str) == 0)
				{
					gtk_combo_box_set_active_iter (
						GTK_COMBO_BOX(widget), &iter);
					g_free(shortOpt);
					foundit = TRUE;
					break;
				}
				g_free(shortOpt);
			} while (gtk_tree_model_iter_next (store, &iter));
		}
		if (!foundit && gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 3, &ivalue, -1);
				if (ivalue == ival)
				{
					gtk_combo_box_set_active_iter (
						GTK_COMBO_BOX(widget), &iter);
					foundit = TRUE;
					break;
				}
			} while (gtk_tree_model_iter_next (store, &iter));
		}
		if (!foundit)
		{
			gtk_combo_box_set_active (GTK_COMBO_BOX(widget), 0);
		}
	}
	else if (type == GTK_TYPE_COMBO_BOX_ENTRY)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt;
		gint ivalue;
		gboolean foundit = FALSE;

		g_debug("GTK_COMBO_BOX_ENTRY");
		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 2, &shortOpt, -1);
				if (strcmp(shortOpt, str) == 0)
				{
					gtk_combo_box_set_active_iter (
						GTK_COMBO_BOX(widget), &iter);
					g_free(shortOpt);
					foundit = TRUE;
					break;
				}
				g_free(shortOpt);
			} while (gtk_tree_model_iter_next (store, &iter));
		}
		if (!foundit && gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 3, &ivalue, -1);
				if (ivalue == ival)
				{
					gtk_combo_box_set_active_iter (
						GTK_COMBO_BOX(widget), &iter);
					foundit = TRUE;
					break;
				}
			} while (gtk_tree_model_iter_next (store, &iter));
		}
		if (!foundit)
		{
			GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget)));
			if (entry)
			{
				gtk_entry_set_text (entry, str);
			}
		}
	}
	else if (type == GTK_TYPE_SPIN_BUTTON)
	{
		g_debug("spin (%s)", str);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), dval);
	}
	else if (type == GTK_TYPE_HSCALE)
	{
		g_debug("hscale");
		gtk_range_set_value(GTK_RANGE(widget), dval);
	}
	else if (type == GTK_TYPE_TEXT_VIEW)
	{
		g_debug("textview (%s)", str);
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(
												GTK_TEXT_VIEW(widget));
		gtk_text_buffer_set_text (buffer, str, -1);
	}
	else if (type == GTK_TYPE_LABEL)
	{
		gtk_label_set_text (GTK_LABEL(widget), str);
	}
	else
	{
		g_debug("Attempt to set unknown widget type");
	}
	g_free(str);
}

int
ghb_ui_update(signal_user_data_t *ud, const gchar *name, const GValue *value)
{
	GObject *object;

	g_debug("ghb_ui_update() %s", name);
	if (name == NULL || value == NULL)
		return 0;
	object = GHB_OBJECT(ud->builder, name);
	if (object == NULL)
	{
		g_debug("Failed to find widget for key: %s\n", name);
		return -1;
	}
	update_widget((GtkWidget*)object, value);
	// Its possible the value hasn't changed. Since settings are only
	// updated when the value changes, I'm initializing settings here as well.
	ghb_widget_to_setting(ud->settings, (GtkWidget*)object);
	return 0;
}

gint
ghb_pref_acount(GValue *settings)
{
	GValue *acodec;
	acodec = ghb_settings_get_value(settings, "pref_audio_codec");
	return ghb_array_len(acodec);
}

gint
ghb_pref_acodec(GValue *settings, gint index)
{
	GValue *acodec;
	gint count;

	acodec = ghb_settings_get_value(settings, "pref_audio_codec");
	count = ghb_array_len(acodec);
	if (index >= count)
		return 0;
	return ghb_value_int(ghb_array_get_nth(acodec, index));
}

gint
ghb_pref_bitrate(GValue *settings, gint index)
{
	GValue *bitrate;
	gint count;

	bitrate = ghb_settings_get_value(settings, "pref_audio_bitrate");
	count = ghb_array_len(bitrate);
	if (index >= count)
		return 0;
	return ghb_value_int(ghb_array_get_nth(bitrate, index));
}

gint
ghb_pref_rate(GValue *settings, gint index)
{
	GValue *rate;
	gint count;

	rate = ghb_settings_get_value(settings, "pref_audio_rate");
	count = ghb_array_len(rate);
	if (index >= count)
		return 0;
	return ghb_value_int(ghb_array_get_nth(rate, index));
}

gint
ghb_pref_mix(GValue *settings, gint index)
{
	GValue *mix;
	gint count;

	mix = ghb_settings_get_value(settings, "pref_audio_mix");
	count = ghb_array_len(mix);
	if (index >= count)
		return 0;
	return ghb_value_int(ghb_array_get_nth(mix, index));
}

gdouble
ghb_pref_drc(GValue *settings, gint index)
{
	GValue *drc;
	gint count;

	drc = ghb_settings_get_value(settings, "pref_audio_drc");
	count = ghb_array_len(drc);
	if (index >= count)
		return 0;
	return ghb_value_double(ghb_array_get_nth(drc, index));
}

