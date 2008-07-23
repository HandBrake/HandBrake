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

void dump_settings(GHashTable *settings);

GObject*
debug_get_object(GtkBuilder* b, const gchar *n)
{
	g_message("name %s\n", n);
	return gtk_builder_get_object(b, n);
}

static gchar *true_strings[] =
{
	"enable",
	"yes",
	"true",
	"1"
};

static gboolean
string_is_true(const gchar *str)
{
	gint count = sizeof(true_strings) / sizeof(gchar*);
	gint ii;
	
	for (ii = 0; ii < count; ii++)
	{
		if (strcmp(str, true_strings[ii]) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

static void
delete_key(gpointer str)
{
	g_debug("delete_key (%s)\n", (gchar*)str);
	g_free(str);
}

static void
delete_value(gpointer val)
{
	g_debug("delete_value (%s)\n", ((setting_value_t*)val)->svalue);
	g_free(((setting_value_t*)val)->svalue);
	g_free(((setting_value_t*)val)->option);
	g_free(((setting_value_t*)val)->shortOpt);
	g_free(val);
}

void
ghb_free_setting_value(setting_value_t *val)
{
	delete_value((gpointer)val);
}

GHashTable*
ghb_settings_new()
{
	GHashTable* settings;
	
	g_debug("ghb_settings_new ()\n");
	settings = g_hash_table_new_full(g_str_hash, g_str_equal, 
						  delete_key, delete_value);
	return settings;
}

static void
settings_set(GHashTable *settings, const gchar *key, 
				 const gchar *str, gint val, gdouble dbl)
{
	g_debug("ghb_setting_set () key (%s), svalue (%s), ivalue %d, dvalue %.2g\n", key, str, val, dbl);
	setting_value_t *value = g_malloc(sizeof(setting_value_t));
	if (str != NULL)
		value->svalue = g_strdup(str);
	else
		value->svalue = g_strdup("");
	value->option = g_strdup(value->svalue);
	value->shortOpt = g_strdup(value->svalue);
	value->index = val;
	value->ivalue = val;
	value->dvalue = dbl;
	g_hash_table_insert(settings, g_strdup(key), value);
}

static setting_value_t*
copy_settings_value(const setting_value_t *value)
{
	setting_value_t *copy = g_malloc(sizeof(setting_value_t));
	copy->index = value->index;
	copy->ivalue = value->ivalue;
	copy->dvalue = value->dvalue;
	copy->svalue = g_strdup(value->svalue);
	copy->option = g_strdup(value->option);
	copy->shortOpt = g_strdup(value->shortOpt);
	return copy;
}

void
ghb_settings_set(GHashTable *settings, const gchar *key, setting_value_t *value)
{
	g_debug("ghb_settings_set () key (%s)\n", key);
	if ((key == NULL) || (value == NULL))
	{
		g_debug("Bad key or value\n");
		return;
	}
	g_debug("\tkey (%s) -- value (%s)\n", key, value->svalue);
	g_hash_table_insert(settings, g_strdup(key), value);
}

void
ghb_settings_set_string(GHashTable *settings, const gchar *key, const gchar *str)
{
	gdouble dvalue = 0;
	gchar *end;

	if (str == NULL) str = "";
	dvalue = g_strtod (str, &end);
	if ((end == str) && string_is_true (str))
	{
		dvalue = 1;
	}
	settings_set(settings, key, str, dvalue, dvalue);
}

void
ghb_settings_set_dbl(GHashTable *settings, const gchar *key, gdouble dvalue)
{
	setting_value_t *value;
	
	value = g_malloc(sizeof(setting_value_t));
	value->index = 0;
	value->dvalue = dvalue;
	value->option = g_strdup_printf("%.8g", dvalue);
	value->shortOpt = g_strdup(value->option);
	value->svalue = g_strdup(value->option);
	value->ivalue = dvalue;
	ghb_settings_set( settings, key, value);
}

void
ghb_settings_copy(GHashTable *settings, const gchar *key, const setting_value_t *value)
{
	setting_value_t *copy = copy_settings_value(value);
	g_hash_table_insert(settings, g_strdup(key), copy);
}

const setting_value_t*
ghb_settings_get(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get () key (%s)\n", key);
	value = g_hash_table_lookup(settings, key);
	return value;
}

gboolean
ghb_settings_get_bool(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_bool () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) 
	{
		g_debug("\tNo value found\n");
		return FALSE;
	}
	g_debug("\tvalue is %d\n", value->ivalue);
	return value->ivalue;
}

gint
ghb_settings_get_int(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_int () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return 0;
	return value->ivalue;
}

gint
ghb_settings_get_index(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_index () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return 0;
	return value->index;
}

gdouble
ghb_settings_get_dbl(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_dbl () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return 0.0;
	return value->dvalue;
}

const gchar*
ghb_settings_get_string(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_string () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return "";
	return value->svalue;
	
}

const gchar*
ghb_settings_get_option(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_option () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return "";
	g_debug("option: (%s)\n", value->option);
	return value->option;
}

const gchar*
ghb_settings_get_short_opt(GHashTable *settings, const gchar *key)
{
	const setting_value_t* value;
	g_debug("ghb_settings_get_short_opt () key (%s)\n", key);
	value = ghb_settings_get(settings, key);
	if (value == NULL) return "";
	g_debug("shrot option: (%s)\n", value->shortOpt);
	return value->shortOpt;
}

static void 
copy_key_val(gpointer key, gpointer val, gpointer settings)
{
	g_hash_table_insert((GHashTable*)settings, 
						g_strdup((gchar*)key), 
						copy_settings_value((setting_value_t*)val));
}

GHashTable*
ghb_settings_dup(GHashTable *settings)
{
	GHashTable *dup_settings;
	
	if (settings == NULL) return NULL;
	dup_settings = ghb_settings_new();
	g_hash_table_foreach (settings, copy_key_val, dup_settings);
	return dup_settings;
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

setting_value_t*
ghb_widget_value(GtkWidget *widget)
{
	setting_value_t *value;
	const gchar *name;
	GType type;
	
	if (widget == NULL)
	{
		g_debug("NULL widget\n");
		return NULL;
	}
	value = g_malloc(sizeof(setting_value_t));
	if (GTK_IS_ACTION(widget))
		name = gtk_action_get_name(GTK_ACTION(widget));
	else
		name = gtk_widget_get_name(widget);
	g_debug("ghb_widget_value widget (%s)\n", name);
	type = GTK_OBJECT_TYPE(widget);
	if (type == GTK_TYPE_ENTRY)
	{
		const gchar *str = gtk_entry_get_text((GtkEntry*)widget);
		value->option = g_strdup(str);
		value->shortOpt = g_strdup(str);
		value->svalue = g_strdup(str);
		value->dvalue = g_strtod(str, NULL);
		value->ivalue = value->dvalue;
		value->index = 0;
	}
	else if (type == GTK_TYPE_RADIO_BUTTON)
	{
		g_debug("\tradio_button");
		value->index = 0;
		if (gtk_toggle_button_get_active((GtkToggleButton*)widget))
		{
			g_debug("\tenable");
			value->option = g_strdup("enable");
			value->shortOpt = g_strdup("enable");
			value->svalue = g_strdup("enable");
			value->ivalue = 1;
			value->dvalue = 1;
		}
		else
		{
			g_debug("\tdisable");
			value->option = g_strdup("disable");
			value->shortOpt = g_strdup("disable");
			value->svalue = g_strdup("disable");
			value->ivalue = 0;
			value->dvalue = 0;
		}
	}
	else if (type == GTK_TYPE_CHECK_BUTTON)
	{
		g_debug("\tcheck_button");
		value->index = 0;
		if (gtk_toggle_button_get_active((GtkToggleButton*)widget))
		{
			g_debug("\tenable");
			value->option = g_strdup("enable");
			value->shortOpt = g_strdup("enable");
			value->svalue = g_strdup("enable");
			value->ivalue = 1;
			value->dvalue = 1;
		}
		else
		{
			g_debug("\tdisable");
			value->option = g_strdup("disable");
			value->shortOpt = g_strdup("disable");
			value->svalue = g_strdup("disable");
			value->ivalue = 0;
			value->dvalue = 0;
		}
	}
	else if (type == GTK_TYPE_TOGGLE_ACTION)
	{
		g_debug("\ttoggle action");
		value->index = 0;
		if (gtk_toggle_action_get_active((GtkToggleAction*)widget))
		{
			g_debug("\tenable");
			value->option = g_strdup("enable");
			value->shortOpt = g_strdup("enable");
			value->svalue = g_strdup("enable");
			value->ivalue = 1;
			value->dvalue = 1;
		}
		else
		{
			g_debug("\tdisable");
			value->option = g_strdup("disable");
			value->shortOpt = g_strdup("disable");
			value->svalue = g_strdup("disable");
			value->ivalue = 0;
			value->dvalue = 0;
		}
	}
	else if (type == GTK_TYPE_CHECK_MENU_ITEM)
	{
		g_debug("\tcheck_menu_item");
		value->index = 0;
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		{
			g_debug("\tenable");
			value->option = g_strdup("enable");
			value->shortOpt = g_strdup("enable");
			value->svalue = g_strdup("enable");
			value->ivalue = 1;
			value->dvalue = 1;
		}
		else
		{
			g_debug("\tdisable");
			value->option = g_strdup("disable");
			value->shortOpt = g_strdup("disable");
			value->svalue = g_strdup("disable");
			value->ivalue = 0;
			value->dvalue = 0;
		}
	}
	else if (type == GTK_TYPE_COMBO_BOX)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt, *option, *svalue;
		gint ivalue;
		gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
		{
			gtk_tree_model_get(store, &iter, 0, &option, 2, &shortOpt, 
							   3, &ivalue, 4, &svalue, -1);

			g_debug("\tcombo: index %d opt (%s) Short Opt (%s) value %d\n", index, option, shortOpt, ivalue);
			value->option = option;
			value->shortOpt = shortOpt;
			value->svalue = svalue;
			value->index = index;
			value->ivalue = ivalue;
			value->dvalue = ivalue;
		}
		else
		{
			value->option = g_strdup("");
			value->shortOpt = g_strdup("");
			value->svalue = g_strdup("");
			value->index = -1;
			value->ivalue = 0;
			value->dvalue = 0;
		}
	}
	else if (type == GTK_TYPE_SPIN_BUTTON)
	{
		value->index = 0;
		value->dvalue = gtk_spin_button_get_value_as_int((GtkSpinButton*)widget);
		value->option = g_strdup_printf("%.8g", value->dvalue);
		value->shortOpt = g_strdup_printf("%.8g", value->dvalue);
		value->svalue = g_strdup_printf("%.8g", value->dvalue);
		value->ivalue = value->dvalue;
	}
	else if (type == GTK_TYPE_HSCALE)
	{
		value->index = 0;
		value->dvalue = gtk_range_get_value((GtkRange*)widget);
		value->option = g_strdup_printf("%.8g", value->dvalue);
		value->shortOpt = g_strdup_printf("%.8g", value->dvalue);
		value->svalue = g_strdup_printf("%.8g", value->dvalue);
		value->ivalue = value->dvalue;
	}
	else if (type == GTK_TYPE_TEXT_VIEW)
	{
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		value->svalue = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		value->option = g_strdup(value->svalue);
		value->shortOpt = g_strdup(value->svalue);
		g_debug("text view (%s)\n", value->svalue);
		value->ivalue = 0;
		value->dvalue = 0;
		value->index = 0;
	}
	else if (type == GTK_TYPE_LABEL)
	{
		value->index = 0;
		value->svalue = g_strdup(gtk_label_get_text (GTK_LABEL(widget)));
		value->dvalue = g_strtod(value->svalue, NULL);
		value->option = g_strdup(value->svalue);
		value->shortOpt = g_strdup(value->svalue);
		value->ivalue = value->dvalue;
		g_debug("label (%s)\n", value->shortOpt);
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
ghb_widget_option(GtkWidget *widget)
{
	setting_value_t *value;
	gchar *str = NULL;
	
	g_debug("ghb_widget_option ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		str = g_strdup(value->option);
		ghb_free_setting_value (value);
	}
	return str;
}

gchar*
ghb_widget_short_opt(GtkWidget *widget)
{
	setting_value_t *value;
	gchar *str = NULL;
	
	g_debug("ghb_widget_short_opt ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		str = g_strdup(value->shortOpt);
		ghb_free_setting_value (value);
	}
	return str;
}

gchar*
ghb_widget_string(GtkWidget *widget)
{
	setting_value_t *value;
	gchar *str = NULL;
	
	g_debug("ghb_widget_string ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		g_debug("str (%s)\n", value->svalue);
		str = g_strdup(value->svalue);
		ghb_free_setting_value (value);
	}
	return str;
}

gdouble
ghb_widget_dbl(GtkWidget *widget)
{
	setting_value_t *value;
	gdouble dbl = 0;
	
	g_debug("ghb_widget_dbl ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		dbl = value->dvalue;
		ghb_free_setting_value (value);
	}
	return dbl;
}

gint
ghb_widget_int(GtkWidget *widget)
{
	setting_value_t *value;
	gint ivalue = 0;
	
	g_debug("ghb_widget_int ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		ivalue = value->ivalue;
		ghb_free_setting_value (value);
	}
	return ivalue;
}

gint
ghb_widget_index(GtkWidget *widget)
{
	setting_value_t *value;
	gint index = 0;
	
	g_debug("ghb_widget_index ()\n");
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		index = value->index;
		ghb_free_setting_value (value);
	}
	return index;
}

void
ghb_widget_to_setting(GHashTable *settings, GtkWidget *widget)
{
	const gchar *key = NULL;
	setting_value_t *value;
	
	g_debug("ghb_widget_to_setting ()\n");
	if (widget == NULL) return;
	// Find corresponding setting
	key = get_setting_key(widget);
	if (key == NULL) return;
	value = ghb_widget_value(widget);
	if (value != NULL)
	{
		ghb_settings_set (settings, key, value);
	}
	else
	{
		g_debug("No value found for %s\n", key);
	}
	//dump_settings(settings);
}

static void
update_widget(GtkWidget *widget, const gchar *parm_svalue, gint parm_ivalue)
{
	GType type;
	gchar *value;

	g_debug("update_widget\n");
	// make a dup of setting value because the setting hash gets 
	// modified and thus the value pointer can become invalid.
	if (parm_svalue == NULL)
	{
		value = g_strdup_printf ("%d", parm_ivalue);
	}
	else
	{
		value = g_strdup(parm_svalue);
	}
	g_debug("update widget value (%s)\n", value);
	type = GTK_OBJECT_TYPE(widget);
	if (type == GTK_TYPE_ENTRY)
	{
		g_debug("entry\n");
		gtk_entry_set_text((GtkEntry*)widget, value);
	}
	else if (type == GTK_TYPE_RADIO_BUTTON)
	{
		g_debug("radio button\n");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), string_is_true(value));
	}
	else if (type == GTK_TYPE_CHECK_BUTTON)
	{
		g_debug("check button\n");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), string_is_true(value));
	}
	else if (type == GTK_TYPE_TOGGLE_ACTION)
	{
		g_debug("toggle action\n");
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(widget), string_is_true(value));
	}
	else if (type == GTK_TYPE_CHECK_MENU_ITEM)
	{
		g_debug("check menu item\n");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), string_is_true(value));
	}
	else if (type == GTK_TYPE_COMBO_BOX)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt;
		gint ivalue;
		gboolean foundit = FALSE;

		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 2, &shortOpt, 3, &ivalue, -1);
				if (parm_svalue == NULL && ivalue == parm_ivalue)
				{
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX(widget), &iter);
					g_free(shortOpt);
					foundit = TRUE;
					break;
				}
				else if (strcmp(shortOpt, value) == 0)
				{
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX(widget), &iter);
					g_free(shortOpt);
					foundit = TRUE;
					break;
				}
				g_free(shortOpt);
			} while (gtk_tree_model_iter_next (store, &iter));
		}
		if (!foundit)
		{
			gtk_combo_box_set_active (GTK_COMBO_BOX(widget), 0);
		}
	}
	else if (type == GTK_TYPE_SPIN_BUTTON)
	{
		gdouble val;
		
		g_debug("spin\n");
		val = g_strtod(value, NULL);
		gtk_spin_button_set_value((GtkSpinButton*)widget, val);
	}
	else if (type == GTK_TYPE_HSCALE)
	{
		gdouble val;
		
		g_debug("hscale\n");
		val = g_strtod(value, NULL);
		gtk_range_set_value((GtkRange*)widget, val);
	}
	else if (type == GTK_TYPE_TEXT_VIEW)
	{
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_set_text (buffer, value, -1);
	}
	else
	{
		g_debug("Attempt to set unknown widget type\n");
	}
	g_free(value);
}

int
ghb_ui_update(signal_user_data_t *ud, const gchar *name, const gchar *value)
{
	GObject *object;

	g_debug("ghb_ui_update ()\n");
	object = GHB_OBJECT(ud->builder, name);
	if (object == NULL)
	{
		g_debug("Failed to find widget for key: %s\n", name);
		return -1;
	}
	update_widget((GtkWidget*)object, value, 0);
	// Its possible the value hasn't changed. Since settings are only
	// updated when the value changes, I'm initializing settings here as well.
	ghb_widget_to_setting(ud->settings, (GtkWidget*)object);
	return 0;
}

int
ghb_ui_update_int(signal_user_data_t *ud, const gchar *name, gint ivalue)
{
	GObject *object;

	g_debug("ghb_ui_update ()\n");
	object = GHB_OBJECT(ud->builder, name);
	if (object == NULL)
	{
		g_debug("Failed to find widget for key: %s\n", name);
		return -1;
	}
	update_widget((GtkWidget*)object, NULL, ivalue);
	// Its possible the value hasn't changed. Since settings are only
	// updated when the value changes, I'm initializing settings here as well.
	ghb_widget_to_setting(ud->settings, (GtkWidget*)object);
	return 0;
}

static void
show_setting(gpointer key, gpointer value, gpointer user_data)
{
	printf("key (%s) -- value (%s)\n", (gchar*)key, (gchar*)value);
}

void
dump_settings(GHashTable *settings)
{
	printf("------------------------------------\n");
	g_hash_table_foreach(settings, show_setting, NULL);
}

// This is a bit hackish, but effective
const gchar defaultSettings[] =
#include "internal_defaults.h"
;

typedef struct
{
	gchar *name;
	gchar *description;
	gboolean custom;
	gboolean defalt;
	GKeyFile *keyFile;
} presets_data_t;

static GKeyFile *standardKeyFile;
static GKeyFile *customKeyFile;
static GKeyFile *internalKeyFile;
static GKeyFile *prefsKeyFile;
static GList *presetsList;

static gint
search_group(const gchar *name, gchar **groups)
{
	gint ii;

	//g_debug("search_group\n");
	if (groups == NULL) return -1;
	for (ii = 0; groups[ii] != NULL; ii++)
	{
		//g_debug("%s cmp %s\n", name, groups[ii]);
		if (strcmp(name, groups[ii]) == 0)
		{
			return ii;
		}
	}
	return -1;
}

presets_data_t *
presets_list_search(GList *list, const gchar *name)
{
	GList *link = list;
	while (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		g_debug("search -- %s\n", data->name);
		if (strcmp(name, data->name) == 0)
		{
			return data;
		}
		link = g_list_next(link);
	}
	return NULL;
}

void
ghb_set_preset_default(GHashTable *settings)
{
	const gchar *preset;
	presets_data_t *data;
	
	preset = ghb_settings_get_string (settings, "default_preset");
	data = presets_list_search(presetsList, preset);
	if (data != NULL)
	{
		data->defalt = FALSE;
	}
	preset = ghb_settings_get_string (settings, "preset");
	data = presets_list_search(presetsList, preset);
	if (data != NULL)
	{
		data->defalt = TRUE;
	}
	ghb_settings_set_string(settings, "default_preset", preset);
	ghb_prefs_save(settings);
}

gint
ghb_presets_list_index(const gchar *name)
{
	GList *link = presetsList;
	int ii = 0;
	while (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		if (strcmp(name, data->name) == 0)
		{
			return ii;
		}
		link = g_list_next(link);
		ii++;
	}
	return -1;
}

gint
ghb_preset_flags(const gchar *name, gint *index)
{
	GList *link = presetsList;
	int ii = 0;
	while (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		if (strcmp(name, data->name) == 0)
		{
			gint ret = 0;
			
			*index = ii;
			ret = (data->custom ? PRESET_CUSTOM : 0);
			ret |= (data->defalt ? PRESET_DEFAULT : 0);
			return ret;
		}
		link = g_list_next(link);
		ii++;
	}
	*index = -1;
	return 0;
}

gchar**
ghb_presets_get_names()
{
	gchar **result;
	GList *link = presetsList;
	int ii = 0;

	g_debug("ghb_presets_get_names()\n");
	result = g_malloc((g_list_length(presetsList)+1) * sizeof(gchar*));
	while (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		result[ii++] = g_strdup(data->name);
		link = g_list_next(link);
	}
	result[ii] = NULL;
	return result;
}

gchar**
ghb_presets_get_descriptions()
{
	gchar **result;
	GList *link = presetsList;
	int ii = 0;

	g_debug("ghb_presets_get_names()\n");
	result = g_malloc((g_list_length(presetsList)+1) * sizeof(gchar*));
	while (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		result[ii++] = g_strdup(data->description);
		link = g_list_next(link);
	}
	result[ii] = NULL;
	return result;
}

const gchar*
ghb_presets_get_name(gint index)
{
	gchar *result = NULL;
	GList *link = presetsList;
	int ii = 0;

	g_debug("ghb_presets_get_name()\n");
	while ((link != NULL) && (ii < index))
	{
		link = g_list_next(link);
		ii++;
	}
	if (link != NULL)
	{
		presets_data_t *data;
		data = (presets_data_t*)link->data;
		result = data->name;
	}
	return result;
}

static gboolean
init_presets_hash_from_key_file(signal_user_data_t *ud, const gchar *name, GKeyFile *keyFile)
{
	gchar **keys;
	gsize length;
	gchar *str;
	
	// Get key list from internal default presets.  This way we do not
	// load any unknown keys.
	keys = g_key_file_get_keys(internalKeyFile, "Presets", &length, NULL);
	if (keys != NULL)
	{
		gint ii;
		for (ii = 0; keys[ii] != NULL; ii++)
		{
			g_debug("key (%s)\n", keys[ii]);
			str = NULL;
			if (name != NULL && keyFile != NULL)
			{
				str = g_key_file_get_string(keyFile, name, keys[ii], NULL);
				g_debug("(%s, %s)\n", keys[ii], str);
			}
			if (str == NULL)
			{
				str = g_key_file_get_string(internalKeyFile, "Presets", keys[ii], NULL);
			}
			if (str != NULL)
			{
				g_debug("name (%s): key (%s) -- str (%s)\n", name, keys[ii], str);
				ghb_settings_set_string(ud->settings, keys[ii], str);
				ghb_ui_update(ud, keys[ii], str);
				g_free(str);
			}
		}
		g_strfreev(keys);
		return TRUE;
	}
	return FALSE;
}

static void
preset_to_ui(signal_user_data_t *ud, presets_data_t *data)
{
	g_debug("preset_to_settings()\n");
	// Initialize the ui from presets file.
	if (data == NULL)
	{
		// Set defaults
		init_presets_hash_from_key_file(ud, NULL, NULL);
		return;
	}
	else
	{
		g_debug("preset name (%s)\n", data->name);
		// Initialize from preset
		init_presets_hash_from_key_file(ud, data->name, data->keyFile);
	}
}

static void
preset_update_ui(signal_user_data_t *ud, presets_data_t *data, const gchar *key)
{
	gchar *str;

	g_debug("preset_update_settings()\n");
	// Initialize the ui from presets file.
	if (data == NULL) return;
	str = g_key_file_get_string(data->keyFile, data->name, key, NULL);
	if (str != NULL)
	{
		ghb_ui_update(ud, key, str);
		g_free(str);
	}
}

void
ghb_set_preset(signal_user_data_t *ud, const gchar *name)
{
	presets_data_t *data;
	
	g_debug("ghb_update_from_preset() %s\n", name);
	if (name == NULL)
	{
		name = ghb_presets_get_name(0);
	}
	if (name == NULL)
	{
		preset_to_ui(ud, NULL);
	}
	else
	{
		data = presets_list_search(presetsList, name);
		preset_to_ui(ud, data);
		ghb_settings_set_string(ud->settings, "preset", name);
	}
}

void
ghb_update_from_preset(
	signal_user_data_t *ud, 
	const gchar *name, 
	const gchar *key)
{
	presets_data_t *data;
	
	g_debug("ghb_set_preset() %s\n", name);
	if (name == NULL) return;
	data = presets_list_search(presetsList, name);
	preset_update_ui(ud, data, key);
}

static void
build_presets_list(GHashTable *settings)
{
	GList *link = presetsList;
	presets_data_t *data;
	gchar **custom, **standard;
	gsize clength, slength;
	gint ii, jj;
	
	g_debug("build_presets_list ()\n");
	// First clear out the old presets list
	while (link != NULL)
	{
		data = (presets_data_t*)link->data;
		g_free(data->name);
		if (data->description != NULL)
			g_free(data->description);
		g_free(data);
		link = g_list_delete_link (link, link);
	}
	presetsList = NULL;

	// Now build up the new list
	const gchar *def_name = ghb_settings_get_string(settings, "default_preset");
	custom = g_key_file_get_groups(customKeyFile, &clength);
	standard = g_key_file_get_groups(standardKeyFile, &slength);
	if ((slength + clength) <= 0) return;
	jj = 0;
	for (ii = 0; ii < slength; ii++)
	{
		if (search_group(standard[ii], custom) < 0)
		{
			gchar *desc;
			data = g_malloc(sizeof(presets_data_t));
			data->name = g_strdup(standard[ii]);
			data->keyFile = standardKeyFile;
			data->custom = FALSE;
			data->defalt = FALSE;
			if ((def_name != NULL) && (strcmp(def_name, data->name) == 0))
			{
				data->defalt = TRUE;
			}
			desc = g_key_file_get_string(standardKeyFile, standard[ii], "preset_description", NULL);
			data->description = desc;
			presetsList = g_list_append(presetsList, data);
		}
	}
	for (ii = 0; ii < clength; ii++)
	{
		gchar *desc;
		data = g_malloc(sizeof(presets_data_t));
		data->name = g_strdup(custom[ii]);
		data->keyFile = customKeyFile;
		data->custom = TRUE;
		data->defalt = FALSE;
		if ((def_name != NULL) && (strcmp(def_name, data->name) == 0))
		{
			data->defalt = TRUE;
		}
		desc = g_key_file_get_string(customKeyFile, custom[ii], "preset_description", NULL);
		data->description = desc;
		presetsList = g_list_append(presetsList, data);
	}
	g_strfreev(custom);
	g_strfreev(standard);
}

static void
store_key_file(GKeyFile *key_file, const gchar *name)
{
	gchar *settingsString;
	const gchar *dir;
	gsize length;
	gchar *config;
	gint fd;

	g_debug("store_key_file ()\n");
	settingsString = g_key_file_to_data(key_file, &length, NULL);

	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb", dir);
	if (!g_file_test(config, G_FILE_TEST_IS_DIR))
	{
		g_mkdir (config, 0755);
	}
	g_free(config);
	config = g_strdup_printf ("%s/ghb/%s", dir, name);
	fd = g_open(config, O_RDWR|O_CREAT|O_TRUNC, 0777);
	write(fd, settingsString, length);
	close(fd);
	g_debug("prefs:\n%s\n", settingsString);
	g_free(settingsString);
}

void
ghb_prefs_to_ui(signal_user_data_t *ud)
{
	const gchar *str;
	
	str = ghb_settings_get_string(ud->settings, "default_source");
	ghb_settings_set_string (ud->settings, "source", str);
	str = ghb_settings_get_string(ud->settings, "destination_dir");

	gchar *path = g_strdup_printf ("%s/new_video.mp4", str);
	ghb_ui_update(ud, "destination", path);
	g_free(path);
}

static gboolean prefs_initializing = FALSE;

void
ghb_prefs_save(GHashTable *settings)
{
	gint ii;
	const gchar *value;
    gchar **keys;
    gsize length;
	
	if (prefs_initializing) return;
	keys = g_key_file_get_keys(internalKeyFile, "Preferences", &length, NULL);
    if (keys != NULL)
    {
	    for (ii = 0; keys[ii] != NULL; ii++)
	    {
		    value = ghb_settings_get_string(settings, keys[ii]);
		    if (value != NULL)
		    {
			    g_key_file_set_value(prefsKeyFile, "Preferences", keys[ii], value);
		    }
	    }
        g_strfreev(keys);
	    store_key_file(prefsKeyFile, "preferences");
    }
}

void
ghb_pref_save(GHashTable *settings, const gchar *key)
{
	const gchar *value;
	
	if (prefs_initializing) return;
	value = ghb_settings_get_string(settings, key);
	if (value != NULL)
	{
		g_key_file_set_value(prefsKeyFile, "Preferences", key, value);
		store_key_file(prefsKeyFile, "preferences");
	}
}

#if 0
static void
dump_key_file(GKeyFile *keyFile, const gchar *section)
{
	gint ii;
    gchar **keys;
    gsize length;

    // Get defaults from internal defaults 
	keys = g_key_file_get_keys(keyFile, section, &length, NULL);
    if (keys != NULL)
    {
        for (ii = 0; keys[ii] != NULL; ii++)
        {
            gchar *str;

			str = g_key_file_get_string(keyFile, section, keys[ii], NULL);
			if (str != NULL)
			{
				g_message("Preference: key (%s) -- str (%s)\n", keys[ii], str);
				g_free(str);
			}
			else
			{
				g_message("Preference: key (%s) -- str **none**\n", keys[ii]);
			}
        }
        g_strfreev(keys);
    }
	else
	{
		g_message("no keys");
	}
}
#endif

void
ghb_prefs_load(signal_user_data_t *ud)
{
	gint ii;
	const gchar *dir;
	gchar *config;
	gchar *value;
    gchar **keys;
    gsize length;
	gboolean res;
	
	prefs_initializing = TRUE;
	internalKeyFile = g_key_file_new();
	res = g_key_file_load_from_data( internalKeyFile, defaultSettings, 
							  sizeof(defaultSettings), G_KEY_FILE_NONE, NULL);
	if (!res)
		g_warning("Failed to initialize internal defaults\n");

	keys = g_key_file_get_keys(internalKeyFile, "Initialization", &length, NULL);
	if (keys != NULL)
	{
		gint ii;
		for (ii = 0; keys[ii] != NULL; ii++)
		{
			gchar *str;
			
			g_debug("key (%s)\n", keys[ii]);
			str = g_key_file_get_string(internalKeyFile, "Initialization", keys[ii], NULL);
			if (str != NULL)
			{
				g_debug("Initialization: key (%s) -- str (%s)\n", keys[ii], str);
				ghb_settings_set_string(ud->settings, keys[ii], str);
				ghb_ui_update(ud, keys[ii], str);
				g_free(str);
			}
		}
		g_strfreev(keys);
	}
	prefsKeyFile = g_key_file_new();
	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb/preferences", dir);
	if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
	{
		g_key_file_load_from_file( prefsKeyFile, config, G_KEY_FILE_KEEP_COMMENTS, NULL);
	}
	value = g_key_file_get_value(prefsKeyFile, "Preferences", "version", NULL);
    if (value == NULL)
    {
        gint ii;

        // Get defaults from internal defaults 
	    keys = g_key_file_get_keys(internalKeyFile, "Preferences", &length, NULL);
        if (keys != NULL)
        {
            for (ii = 0; keys[ii] != NULL; ii++)
            {
                gchar *str;

			    str = g_key_file_get_string(internalKeyFile, "Preferences", keys[ii], NULL);
			    if (str != NULL)
			    {
				    g_debug("Preference: key (%s) -- str (%s)\n", keys[ii], str);
		            g_key_file_set_value(prefsKeyFile, "Preferences", keys[ii], str);
				    g_free(str);
			    }
            }
            g_strfreev(keys);
        }
		const gchar *dir = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
		g_key_file_set_value(prefsKeyFile, "Preferences", "destination_dir", dir);
		store_key_file(prefsKeyFile, "preferences");
    }
	g_free(config);
	keys = g_key_file_get_keys(internalKeyFile, "Preferences", &length, NULL);
    if (keys != NULL)
    {
	    for (ii = 0; keys[ii] != NULL; ii++)
	    {
		    value = g_key_file_get_value(prefsKeyFile, "Preferences", keys[ii], NULL);
		    if (value != NULL)
		    {
			    ghb_settings_set_string(ud->settings, keys[ii], value);
				ghb_ui_update(ud, keys[ii], value);
			    g_free(value);
		    }
            else
            {
		        value = g_key_file_get_value(internalKeyFile, "Preferences", keys[ii], NULL);
		        if (value != NULL)
		        {
			        ghb_settings_set_string(ud->settings, keys[ii], value);
					ghb_ui_update(ud, keys[ii], value);
			        g_free(value);
		        }
            }
	    }
        g_strfreev(keys);
    }
	gint bval = ghb_settings_get_int(ud->settings, "show_presets");
	ghb_ui_update_int(ud, "show_presets", bval);
	if (ghb_settings_get_bool(ud->settings, "hbfd_feature"))
	{
		GtkAction *action;
		bval = ghb_settings_get_int(ud->settings, "hbfd");
		ghb_ui_update_int(ud, "hbfd", bval);
		action = GHB_ACTION (ud->builder, "hbfd");
		gtk_action_set_visible(action, TRUE);
	}
	else
	{
		ghb_ui_update_int(ud, "hbfd", 0);
	}
	prefs_initializing = FALSE;
}

void
ghb_presets_load(signal_user_data_t *ud)
{
	const gchar *dir;
	gchar *config;
	GHashTable *settings = ud->settings;

	g_debug("ghb_presets_load()\n");
	customKeyFile = g_key_file_new();
	standardKeyFile = g_key_file_new();
	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb/custom_presets", dir);
	if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
	{
		g_key_file_load_from_file( customKeyFile, config, 
								  G_KEY_FILE_KEEP_COMMENTS, NULL);
	}
	g_free(config);
	// Try current dir first. Makes testing prior to installation easier
	if (g_file_test("./standard_presets", G_FILE_TEST_IS_REGULAR))
	{
		g_key_file_load_from_file( standardKeyFile, "./standard_presets", 
								  G_KEY_FILE_KEEP_COMMENTS, NULL);
	}
	else
	{
		// Try users config dir
		config = g_strdup_printf ("%s/ghb/standard_presets", dir);
		if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
		{
			g_key_file_load_from_file( standardKeyFile, config, 
									  G_KEY_FILE_KEEP_COMMENTS, NULL);
			g_free(config);
		}
		else
		{
			const gchar* const *dirs;
			gint ii;
			g_free(config);
			dirs = g_get_system_data_dirs();
			if (dirs != NULL)
			{
				for (ii = 0; dirs[ii] != NULL; ii++)
				{
					config = g_strdup_printf("%s/ghb/standard_presets", dirs[ii]);
					if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
					{
						g_key_file_load_from_file( standardKeyFile, config, 
												  G_KEY_FILE_KEEP_COMMENTS, NULL);
						break;
					}
					g_free(config);
				}
			}
		}
	}
	build_presets_list(settings);
}

static void
presets_store()
{
	g_debug("presets_store ()\n");
	store_key_file(customKeyFile, "custom_presets");
}

typedef struct
{
	const gchar *name;
	GKeyFile *keyFile;
	gboolean autoscale;
} store_key_info_t;

static void
store_to_key_file(gpointer xkey, gpointer xvalue, gpointer xski)
{
	store_key_info_t *ski = (store_key_info_t*)xski;
	setting_value_t *value = (setting_value_t *)xvalue;
	gchar *key = (gchar*)xkey;
	gchar *str;

	if (!ski->autoscale)
	{
		if (strcmp(key, "scale_width"))
		{
			key = "max_width";
		}
		if (strcmp(key, "scale_height"))
		{
			key = "max_height";
		}
	}
	str = g_key_file_get_string(internalKeyFile, "Presets", key, NULL);
	if (str == NULL)
	{
		g_debug("Setting (%s) is not in defaults\n", (gchar*)key);
		return;
	}
	g_debug("comparing: key (%s) -- (%s) == (%s)\n", (gchar*)key, str, value->svalue);
	if (strcmp(str, value->shortOpt) != 0)
	{
		// Differs from default value.  Store it.
		g_debug("storing: key (%s) -- (%s)\n", (gchar*)key, value->shortOpt);
		gchar *tmp = g_strescape (value->shortOpt, NULL);
		g_key_file_set_value(ski->keyFile, ski->name, (gchar*)key, tmp);
		g_free(tmp);
	}
	else
	{
		// Remove it if it exists already in keyfile
		g_key_file_remove_key (ski->keyFile, ski->name, (gchar*)key, NULL);
	}
	g_free(str);
}

void
ghb_settings_save(signal_user_data_t *ud, const gchar *name)
{
	store_key_info_t ski;

	g_debug("ghb_settings_save ()\n");
	ski.name = name;
	ski.keyFile = customKeyFile;
	ski.autoscale = ghb_settings_get_bool (ud->settings, "autoscale");
	g_hash_table_foreach(ud->settings, store_to_key_file, &ski);
	presets_store();
	build_presets_list(ud->settings);
	ud->dont_clear_presets = TRUE;
	ghb_set_preset (ud, name);
	ud->dont_clear_presets = FALSE;
}

// Checks to see if the preset is in standard presets
// I allow standard to be overridden by adding a preset with the
// same name to the custom list.  So to determine if the named 
// preset is standard, I must first check to see if is in the
// custom list.
gboolean
ghb_presets_is_standard(const gchar *name)
{
	g_debug("ghb_presets_is_standard()\n");
	if (g_key_file_has_group(customKeyFile, name))
	{
		// The preset is in the custom list, so it
		// can not be a standard.
		return FALSE;
	}
	return g_key_file_has_group(standardKeyFile, name);
}

// This function will not remove presets from the standard preset list.
// Return false if attempt is made.
gboolean
ghb_presets_remove(GHashTable *settings, const gchar *name)
{
	g_debug("ghb_presets_remove()\n");
	if (g_key_file_has_group(customKeyFile, name))
	{
		g_debug("\t removing %s\n", name);
		g_key_file_remove_group(customKeyFile, name, NULL);
		presets_store();
		build_presets_list(settings);
		return TRUE;
	}
	return FALSE;
}

