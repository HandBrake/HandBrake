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

void
ghb_settings_set_combo(
	GValue *settings, 
	const gchar *key, 
	gint index,
	const gchar *option,
	const gchar *shortOpt,
	const gchar *svalue,
	gint ivalue)
{
	GValue *value;
	value = ghb_combo_value_new(index, option, shortOpt, svalue, ivalue);
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
ghb_settings_get_combo_index(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return 0;
	ghb_combodata_t *cd;
	if (G_VALUE_TYPE(value) != ghb_combodata_get_type())
		return 0;
	cd = g_value_get_boxed(value);
	return cd->index;
}

gchar*
ghb_settings_get_combo_option(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return g_strdup("");
	ghb_combodata_t *cd;
	if (G_VALUE_TYPE(value) != ghb_combodata_get_type())
		return g_strdup("");
	cd = g_value_get_boxed(value);
	return g_strdup(cd->option);
}

gchar*
ghb_settings_get_combo_string(GValue *settings, const gchar *key)
{
	const GValue* value;
	value = ghb_settings_get_value(settings, key);
	if (value == NULL) return g_strdup("");
	ghb_combodata_t *cd;
	if (G_VALUE_TYPE(value) != ghb_combodata_get_type())
		return g_strdup("");
	cd = g_value_get_boxed(value);
	return g_strdup(cd->svalue);
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
		gchar *shortOpt, *option, *svalue;
		gint index, ivalue;

		index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
		{
			gtk_tree_model_get(store, &iter, 0, &option, 2, &shortOpt, 
							   3, &ivalue, 4, &svalue, -1);
			value = ghb_combo_value_new(index, option, shortOpt, 
										svalue, ivalue);
		}
		else
		{
			value = ghb_combo_value_new(-1, "", "", "", 0);
		}
		g_debug("\tdone");
	}
	else if (type == GTK_TYPE_COMBO_BOX_ENTRY)
	{
		GtkTreeModel *store;
		GtkTreeIter iter;
		gchar *shortOpt, *option, *svalue;
		gint index, ivalue;

		index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
		store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
		{
			gtk_tree_model_get(store, &iter, 0, &option, 2, &shortOpt, 
							   3, &ivalue, 4, &svalue, -1);
			value = ghb_combo_value_new(index, option, shortOpt, 
										svalue, ivalue);
		}
		else
		{
			const gchar *str;
			str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
			if (str == NULL) str = "";
			value = ghb_combo_value_new(-1, str, str, str, -1);
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
		dval = gtk_range_get_value(GTK_RANGE(widget));
		value = ghb_double_value_new(dval);
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
ghb_widget_index(GtkWidget *widget)
{
	GValue *value;
	gint index = 0;
	
	value = ghb_widget_value(widget);
	if (value == NULL) return 0;
	ghb_combodata_t *cd;
	if (G_VALUE_TYPE(value) != ghb_combodata_get_type())
		return 0;
	cd = g_value_get_boxed(value);
	index = cd->index;
	ghb_value_free(value);
	return index;
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

enum
{
	X264_OPT_DEBLOCK,
	X264_OPT_INT,
	X264_OPT_COMBO,
	X264_OPT_BOOL,
};

struct x264_opt_map_s
{
	gchar **opt_syns;
	gchar *name;
	gchar *def_val;
	gint type;
	gboolean found;
};

static gchar *x264_ref_syns[] = {"ref", "frameref", NULL};
static gchar *x264_mixed_syns[] = {"mixed-refs", NULL};
static gchar *x264_bframes_syns[] = {"bframes", NULL};
static gchar *x264_direct_syns[] = {"direct", "direct-pred", NULL};
static gchar *x264_weightb_syns[] = {"weightb", "weight-b", NULL};
static gchar *x264_brdo_syns[] = {"brdo", "b-rdo", NULL};
static gchar *x264_bime_syns[] = {"bime", NULL};
static gchar *x264_bpyramid_syns[] = {"b-pyramid", NULL};
static gchar *x264_me_syns[] = {"me", NULL};
static gchar *x264_merange_syns[] = {"merange", "me-range", NULL};
static gchar *x264_subme_syns[] = {"subme", "subq", NULL};
static gchar *x264_analyse_syns[] = {"analyse", "partitions", NULL};
static gchar *x264_8x8dct_syns[] = {"8x8dct", NULL};
static gchar *x264_deblock_syns[] = {"deblock", "filter", NULL};
static gchar *x264_trellis_syns[] = {"trellis", NULL};
static gchar *x264_pskip_syns[] = {"no-fast-pskip", NULL};
static gchar *x264_decimate_syns[] = {"no-dct-decimate", NULL};
static gchar *x264_cabac_syns[] = {"cabac", NULL};

static gint
find_syn_match(const gchar *opt, gchar **syns)
{
	gint ii;
	for (ii = 0; syns[ii] != NULL; ii++)
	{
		if (strcmp(opt, syns[ii]) == 0)
			return ii;
	}
	return -1;
}

struct x264_opt_map_s x264_opt_map[] =
{
	{x264_ref_syns, "x264_refs", "1", X264_OPT_INT},
	{x264_mixed_syns, "x264_mixed_refs", "0", X264_OPT_BOOL},
	{x264_bframes_syns, "x264_bframes", "0", X264_OPT_INT},
	{x264_direct_syns, "x264_direct", "spatial", X264_OPT_COMBO},
	{x264_weightb_syns, "x264_weighted_bframes", "0", X264_OPT_BOOL},
	{x264_brdo_syns, "x264_brdo", "0", X264_OPT_BOOL},
	{x264_bime_syns, "x264_bime", "0", X264_OPT_BOOL},
	{x264_bpyramid_syns, "x264_bpyramid", "0", X264_OPT_BOOL},
	{x264_me_syns, "x264_me", "hex", X264_OPT_COMBO},
	{x264_merange_syns, "x264_merange", "16", X264_OPT_INT},
	{x264_subme_syns, "x264_subme", "5", X264_OPT_COMBO},
	{x264_analyse_syns, "x264_analyse", "some", X264_OPT_COMBO},
	{x264_8x8dct_syns, "x264_8x8dct", "0", X264_OPT_BOOL},
	{x264_deblock_syns, "x264_deblock_alpha", "0,0", X264_OPT_DEBLOCK},
	{x264_deblock_syns, "x264_deblock_beta", "0,0", X264_OPT_DEBLOCK},
	{x264_trellis_syns, "x264_trellis", "0", X264_OPT_COMBO},
	{x264_pskip_syns, "x264_no_fast_pskip", "0", X264_OPT_BOOL},
	{x264_decimate_syns, "x264_no_dct_decimate", "0", X264_OPT_BOOL},
	{x264_cabac_syns, "x264_cabac", "1", X264_OPT_BOOL},
};
#define X264_OPT_MAP_SIZE (sizeof(x264_opt_map)/sizeof(struct x264_opt_map_s))

static const gchar*
x264_opt_get_default(const gchar *opt)
{
	gint jj;
	for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
	{
		if (find_syn_match(opt, x264_opt_map[jj].opt_syns) >= 0)
		{
			return x264_opt_map[jj].def_val;
		}
	}
	return "";
}

static void
x264_update_int(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
	gint ival;

	if (val == NULL) return;
	ival = g_strtod (val, NULL);
	ghb_ui_update(ud, name, ghb_int64_value(ival));
}

static gchar *true_str[] =
{
	"true",
	"yes",
	"1",
	NULL
};

static gboolean 
str_is_true(const gchar *str)
{
	gint ii;
	for (ii = 0; true_str[ii]; ii++)
	{
		if (g_ascii_strcasecmp(str, true_str[ii]) == 0)
			return TRUE;
	}
	return FALSE;
}

static void
x264_update_bool(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
	if (val == NULL)
		ghb_ui_update(ud, name, ghb_boolean_value(1));
	else
		ghb_ui_update(ud, name, ghb_boolean_value(str_is_true(val)));
}

static void
x264_update_combo(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *shortOpt;
	gint ivalue;
	gboolean foundit = FALSE;
	GtkWidget *widget;

	if (val == NULL) return;
	widget = GHB_WIDGET(ud->builder, name);
	if (widget == NULL)
	{
		g_debug("Failed to find widget for key: %s\n", name);
		return;
	}
	store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	if (gtk_tree_model_get_iter_first (store, &iter))
	{
		do
		{
			gtk_tree_model_get(store, &iter, 2, &shortOpt, 3, &ivalue, -1);
			if (strcmp(shortOpt, val) == 0)
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
		if (gtk_tree_model_get_iter_first (store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 2, &shortOpt, 3, &ivalue, -1);
				if (strcmp(shortOpt, "custom") == 0)
				{
					gtk_list_store_set(GTK_LIST_STORE(store), &iter, 4, val, -1);
					gtk_combo_box_set_active_iter (GTK_COMBO_BOX(widget), &iter);
					g_free(shortOpt);
					foundit = TRUE;
					break;
				}
				g_free(shortOpt);
			} while (gtk_tree_model_iter_next (store, &iter));
		}
	}
	// Its possible the value hasn't changed. Since settings are only
	// updated when the value changes, I'm initializing settings here as well.
	ghb_widget_to_setting(ud->settings, widget);
}

static void
x264_update_deblock(signal_user_data_t *ud, const gchar *xval)
{
	gdouble avalue, bvalue;
	gchar *end;
	gchar *val;
	gchar *bval = NULL;

	if (xval == NULL) return;
	val = g_strdup(xval);
	bvalue = avalue = 0;
	if (val != NULL) 
	{
		gchar *pos = strchr(val, ',');
		if (pos != NULL)
		{
			bval = pos + 1;
			*pos = 0;
		}
		avalue = g_strtod (val, &end);
		if (bval != NULL)
		{
			bvalue = g_strtod (bval, &end);
		}
	}
	g_free(val);
	ghb_ui_update(ud, "x264_deblock_alpha", ghb_int64_value(avalue));
	ghb_ui_update(ud, "x264_deblock_beta", ghb_int64_value(bvalue));
}

void
ghb_x264_parse_options(signal_user_data_t *ud, const gchar *options)
{
	gchar **split = g_strsplit(options, ":", -1);
	if (split == NULL) return;

	gint ii;
	gint jj;

	for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
		x264_opt_map[jj].found = FALSE;

	for (ii = 0; split[ii] != NULL; ii++)
	{
		gchar *val = NULL;
		gchar *pos = strchr(split[ii], '=');
		if (pos != NULL)
		{
			val = pos + 1;
			*pos = 0;
		}
		for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
		{
			if (find_syn_match(split[ii], x264_opt_map[jj].opt_syns) >= 0)
			{
				x264_opt_map[jj].found = TRUE;
				switch(x264_opt_map[jj].type)
				{
				case X264_OPT_INT:
					x264_update_int(ud, x264_opt_map[jj].name, val);
					break;
				case X264_OPT_BOOL:
					x264_update_bool(ud, x264_opt_map[jj].name, val);
					break;
				case X264_OPT_COMBO:
					x264_update_combo(ud, x264_opt_map[jj].name, val);
					break;
				case X264_OPT_DEBLOCK:
					// dirty little hack.  mark deblock_beta found as well
					x264_opt_map[jj+1].found = TRUE;
					x264_update_deblock(ud, val);
					break;
				}
				break;
			}
		}
	}
	// For any options not found in the option string, set ui to
	// default values
	for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
	{
		if (!x264_opt_map[jj].found)
		{
			gchar *val = strdup(x264_opt_map[jj].def_val);
			switch(x264_opt_map[jj].type)
			{
			case X264_OPT_INT:
				x264_update_int(ud, x264_opt_map[jj].name, val);
				break;
			case X264_OPT_BOOL:
				x264_update_bool(ud, x264_opt_map[jj].name, val);
				break;
			case X264_OPT_COMBO:
				x264_update_combo(ud, x264_opt_map[jj].name, val);
				break;
			case X264_OPT_DEBLOCK:
				x264_update_deblock(ud, val);
				break;
			}
			x264_opt_map[jj].found = TRUE;
			g_free(val);
		}
	}
	g_strfreev(split);
}

gchar*
get_deblock_val(signal_user_data_t *ud)
{
	gchar *alpha, *beta;
	gchar *result;
	alpha = ghb_settings_get_string(ud->settings, "x264_deblock_alpha");
	beta = ghb_settings_get_string(ud->settings, "x264_deblock_beta");
	result = g_strdup_printf("%s,%s", alpha, beta);
	g_free(alpha);
	g_free(beta);
	return result;
}

void
ghb_x264_opt_update(signal_user_data_t *ud, GtkWidget *widget)
{
	gint jj;
	const gchar *name = gtk_widget_get_name(widget);
	gchar **opt_syns = NULL;
	const gchar *def_val = NULL;
	gint type;

	for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
	{
		if (strcmp(name, x264_opt_map[jj].name) == 0)
		{
			// found the options that needs updating
			opt_syns = x264_opt_map[jj].opt_syns;
			def_val = x264_opt_map[jj].def_val;
			type = x264_opt_map[jj].type;
			break;
		}
	}
	if (opt_syns != NULL)
	{
		GString *x264opts = g_string_new("");
		gchar *options;
		gchar **split = NULL;
		gint ii;
		gboolean foundit = FALSE;

		options = ghb_settings_get_string(ud->settings, "x264_options");
		if (options)
		{
			split = g_strsplit(options, ":", -1);
			g_free(options);
		}
		for (ii = 0; split && split[ii] != NULL; ii++)
		{
			gint syn;
			gchar *val = NULL;
			gchar *pos = strchr(split[ii], '=');
			if (pos != NULL)
			{
				val = pos + 1;
				*pos = 0;
			}
			syn = find_syn_match(split[ii], opt_syns);
			if (syn >= 0)
			{ // Updating this option
				gchar *val;
				foundit = TRUE;
				if (type == X264_OPT_DEBLOCK)
					val = get_deblock_val(ud);
				else
				{
					GValue *gval;
					gval = ghb_widget_value(widget);
					if (G_VALUE_TYPE(gval) == G_TYPE_BOOLEAN)
					{
						if (ghb_value_boolean(gval))
							val = g_strdup("1");
						else
							val = g_strdup("0");
					}
					else
					{
						val = ghb_widget_string(widget);
					}
					ghb_value_free(gval);
				}
				if (strcmp(def_val, val) != 0)
				{
					g_string_append_printf(x264opts, "%s=%s:", opt_syns[syn], val);
				}
				g_free(val);
			}
			else if (val != NULL)
				g_string_append_printf(x264opts, "%s=%s:", split[ii], val);
			else
				g_string_append_printf(x264opts, "%s:", split[ii]);

		}
		if (split) g_strfreev(split);
		if (!foundit)
		{
			gchar *val;
			if (type == X264_OPT_DEBLOCK)
				val = get_deblock_val(ud);
			else
			{
				GValue *gval;
				gval = ghb_widget_value(widget);
				if (G_VALUE_TYPE(gval) == G_TYPE_BOOLEAN)
				{
					if (ghb_value_boolean(gval))
						val = g_strdup("1");
					else
						val = g_strdup("0");
				}
				else
				{
					val = ghb_widget_string(widget);
				}
				ghb_value_free(gval);
			}
			if (strcmp(def_val, val) != 0)
			{
				g_string_append_printf(x264opts, "%s=%s:", opt_syns[0], val);
			}
			g_free(val);
		}
		// Update the options value
		// strip the trailing ":"
		gchar *result;
		gint len;
		result = g_string_free(x264opts, FALSE);
		len = strlen(result);
		if (len > 0) result[len - 1] = 0;
		gchar *sopts;
		sopts = ghb_sanitize_x264opts(ud, result);
		ghb_ui_update(ud, "x264_options", ghb_string_value(sopts));
		ghb_x264_parse_options(ud, sopts);
		g_free(sopts);
		g_free(result);
	}
}

static void
x264_remove_opt(gchar **opts, gchar **opt_syns)
{
	gint ii;
	for (ii = 0; opts[ii] != NULL; ii++)
	{
		gchar *opt;
		opt = g_strdup(opts[ii]);
		gchar *pos = strchr(opt, '=');
		if (pos != NULL)
		{
			*pos = 0;
		}
		if (find_syn_match(opt, opt_syns) >= 0)
		{
			// Mark as deleted
			opts[ii][0] = 0;
		}
		g_free(opt);
	}
}

// Construct the x264 options string
// The result is allocated, so someone must free it at some point.
gchar*
ghb_sanitize_x264opts(signal_user_data_t *ud, const gchar *options)
{
	GString *x264opts = g_string_new("");
	gchar **split = g_strsplit(options, ":", -1);

	// Remove entries that match the defaults
	gint ii;
	for (ii = 0; split[ii] != NULL; ii++)
	{
		gchar *val = NULL;
		gchar *opt = g_strdup(split[ii]);
		gchar *pos = strchr(opt, '=');
		if (pos != NULL)
		{
			val = pos + 1;
			*pos = 0;
		}
		else
		{
			val = "1";
		}
		const gchar *def_val = x264_opt_get_default(opt);
		if (strcmp(val, def_val) == 0)
		{
			// Matches the default, so remove it
			split[ii][0] = 0;
		}
		g_free(opt);
	}
	gint refs = ghb_settings_get_int(ud->settings, "x264_refs");
	if (refs <= 1)
	{
		x264_remove_opt(split, x264_mixed_syns);
	}
	gint subme = ghb_settings_get_int(ud->settings, "x264_subme");
	if (subme < 6)
	{
		x264_remove_opt(split, x264_brdo_syns);
	}
	gint bframes = ghb_settings_get_int(ud->settings, "x264_bframes");
	if (bframes == 0)
	{
		x264_remove_opt(split, x264_weightb_syns);
		x264_remove_opt(split, x264_brdo_syns);
		x264_remove_opt(split, x264_bime_syns);
	}
	if (bframes <= 1)
	{
		x264_remove_opt(split, x264_bpyramid_syns);
	}
	gchar *me = ghb_settings_get_string(ud->settings, "x264_me");
	if (!(strcmp(me, "umh") == 0 || strcmp(me, "esa") == 0))
	{
		x264_remove_opt(split, x264_merange_syns);
	}
	g_free(me);
	if (!ghb_settings_get_boolean(ud->settings, "x264_cabac"))
	{
		x264_remove_opt(split, x264_trellis_syns);
	}
	gint analyse = ghb_settings_get_int(ud->settings, "x264_analyse");
	if (analyse == 1)
	{
		x264_remove_opt(split, x264_direct_syns);
	}
	for (ii = 0; split[ii] != NULL; ii++)
	{
		if (split[ii][0] != 0)
			g_string_append_printf(x264opts, "%s:", split[ii]);
	}
	g_strfreev(split);
	// strip the trailing ":"
	gchar *result;
	gint len;
	result = g_string_free(x264opts, FALSE);
	len = strlen(result);
	if (len > 0) result[len - 1] = 0;
	return result;
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

