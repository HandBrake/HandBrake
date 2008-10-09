/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * presets.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * presets.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 */
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "settings.h"
#include "callbacks.h"
#include "audiohandler.h"
#include "hb-backend.h"
#include "plist.h"
#include "resources.h"
#include "presets.h"
#include "values.h"

// These are flags.  One bit for each feature
enum
{
	PRESETS_CUST = 0x01,
	PRESETS_FOLDER = 0x02,
};

static GValue *presetsPlist = NULL;
static GValue *internalPlist = NULL;
static GValue *prefsPlist = NULL;

static const GValue* preset_dict_get_value(GValue *dict, const gchar *key);

static GValue*
plist_get_dict(GValue *presets, const gchar *name)
{
	if (presets == NULL || name == NULL) return NULL;
	return ghb_dict_lookup(presets, name);
}

static const gchar*
preset_get_name(GValue *dict)
{
	return g_value_get_string(ghb_dict_lookup(dict, "preset_name"));
}

gint
ghb_preset_flags(GValue *dict)
{
	const GValue *gval;
	gint ptype = 0;

	gval = preset_dict_get_value(dict, "preset_type");
	if (gval)
	{
		ptype = ghb_value_int(gval);
	}
	return ptype;
}

static GValue*
presets_get_first_dict(GValue *presets)
{
	gint count;
	
	g_debug("presets_get_first_dict ()");
	if (presets == NULL) return NULL;
	count = ghb_array_len(presets);
	if (count <= 0) return NULL;
	return ghb_array_get_nth(presets, 0);
}

static void
presets_remove_nth(GValue *presets, gint pos)
{
	GValue *dict;
	gint count;
	
	if (presets == NULL || pos < 0) return;
	count = ghb_array_len(presets);
	if (pos >= count) return;
	dict = ghb_array_get_nth(presets, pos);
	ghb_array_remove(presets, pos);
	ghb_value_free(dict);
}

gboolean
ghb_presets_remove(
	GValue *presets, 
	gint folder_pos,
	gint pos)
{
	GValue *nested;
	gint ii;

	if (folder_pos >= 0)
	{
		if (pos >= 0)
		{
			ii = pos;
			nested = ghb_array_get_nth(presets, folder_pos);
			nested = ghb_dict_lookup(nested, "preset_folder");
		}
		else
		{
			ii = folder_pos;
			nested = presets;
		}
	}
	else
	{
		ii = pos;
		nested = presets;
	}
	if (ii >= 0)
		presets_remove_nth(nested, ii);
	else
	{
		g_warning("internal preset lookup error (%d/%d)", folder_pos, pos);
		return FALSE;
	}
	return TRUE;
}

static void
ghb_presets_replace(
	GValue *presets, 
	GValue *dict,
	gint folder_pos,
	gint pos)
{
	GValue *nested;
	gint ii;

	if (folder_pos >= 0)
	{
		if (pos >= 0)
		{
			ii = pos;
			nested = ghb_array_get_nth(presets, folder_pos);
			nested = ghb_dict_lookup(nested, "preset_folder");
		}
		else
		{
			ii = folder_pos;
			nested = presets;
		}
	}
	else
	{
		ii = pos;
		nested = presets;
	}
	if (ii >= 0)
		ghb_array_replace(nested, ii, dict);
	else
	{
		g_warning("internal preset lookup error (%d/%d)", folder_pos, pos);
	}
}

static gint
presets_find_pos(GValue *presets, const gchar *name, gint type)
{
	GValue *dict;
	gint count, ii, ptype, last;
	
	if (presets == NULL || name == NULL) return -1;
	last = count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (strcasecmp(name, str) <= 0 && ptype == type)
		{
			return ii;
		}
		if (ptype == type)
			last = ii+1;
	}
	return last;
}

static gint
presets_find_folder(GValue *presets, const gchar *name)
{
	GValue *dict;
	gint count, ii, ptype;
	
	if (presets == NULL || name == NULL) return -1;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if ((ptype & PRESETS_FOLDER) && strcasecmp(name, str) == 0)
		{
			return ii;
		}
	}
	return -1;
}

static gint
presets_find_preset(GValue *presets, const gchar *name)
{
	GValue *dict;
	gint count, ii, ptype;
	
	g_debug("presets_find_preset () (%s)", name);
	if (presets == NULL || name == NULL) return -1;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (!(ptype & PRESETS_FOLDER) && strcasecmp(name, str) == 0)
		{
			return ii;
		}
	}
	return -1;
}

gboolean
ghb_presets_find(
	GValue *presets, 
	const gchar *folder, 
	const gchar *name, 
	gint *folder_pos,
	gint *pos)
{
	GValue *nested;

	*pos = -1;
	*folder_pos = -1;
	g_debug("ghb_presets_find () (%s) (%s)", folder, name);
	if (folder == NULL || folder[0] == 0)
	{
		// name could be a folder or a regular preset
		*folder_pos = presets_find_folder(presets, name);
		if (*folder_pos < 0)
		{
			*pos = presets_find_preset(presets, name);
			if (*pos < 0)
			{
				g_debug("No presets/folder (%s)", name);
				return FALSE;
			}
		}
	}
	else
	{
		*folder_pos = presets_find_folder(presets, folder);
		if (*folder_pos < 0)
		{
			g_debug("No presets folder (%s)", folder);
			return FALSE;
		}
		nested = ghb_array_get_nth(presets, *folder_pos);
		nested = ghb_dict_lookup(nested, "preset_folder");
		if (name != NULL)
		{
			*pos = presets_find_preset(nested, name);
			if (*pos < 0)
			{
				g_debug("No preset (%s/%s)", folder, name);
				return FALSE;
			}
		}
	}
	return TRUE;
}

static GValue*
presets_get_dict(GValue *presets, gint folder_pos, gint pos)
{
	g_debug("presets_get_dict () (%d) (%d)", folder_pos, pos);
	if (presets == NULL) return NULL;
	GValue *nested;
	gint ii;

	if (folder_pos >= 0)
	{
		if (pos >= 0)
		{
			ii = pos;
			nested = ghb_array_get_nth(presets, folder_pos);
			nested = ghb_dict_lookup(nested, "preset_folder");
		}
		else
		{
			ii = folder_pos;
			nested = presets;
		}
	}
	else
	{
		ii = pos;
		nested = presets;
	}
	if (ii >= 0)
		return ghb_array_get_nth(nested, ii);
	else
	{
		g_warning("internal preset lookup error (%d/%d)", folder_pos, pos);
		return NULL;
	}
}

static gint
ghb_presets_get_type(
	GValue *presets, 
	gint folder_pos,
	gint pos)
{
	GValue *nested;
	GValue *dict;
	gint ii;
	gint flags = 0;

	if (folder_pos >= 0)
	{
		if (pos >= 0)
		{
			ii = pos;
			nested = ghb_array_get_nth(presets, folder_pos);
			nested = ghb_dict_lookup(nested, "preset_folder");
		}
		else
		{
			ii = folder_pos;
			nested = presets;
		}
	}
	else
	{
		ii = pos;
		nested = presets;
	}
	if (ii >= 0)
	{
		dict = ghb_array_get_nth(nested, ii);
		flags = ghb_preset_flags(dict);
	}
	else
	{
		g_warning("internal preset lookup error (%d/%d)", folder_pos, pos);
	}
	return flags;
}

void
ghb_set_preset_default(GValue *settings)
{
	gchar *preset, *folder;
	
	preset = ghb_settings_get_string (settings, "preset");
	folder = ghb_settings_get_string (settings, "folder");
	ghb_settings_set_string(settings, "default_preset", preset);
	ghb_settings_set_string(settings, "default_folder", folder);
	ghb_prefs_save(settings);
	g_free(preset);
	g_free(folder);
}

// Used for sorting dictionaries.
gint
key_cmp(gconstpointer a, gconstpointer b)
{
	gchar *stra = (gchar*)a;
	gchar *strb = (gchar*)b;

	return strcmp(stra, strb);
}

static const GValue*
preset_dict_get_value(GValue *dict, const gchar *key)
{
	const GValue *gval = NULL;

	if (dict)
	{
		gval = ghb_dict_lookup(dict, key);
	}
	if (internalPlist == NULL) return NULL;
	if (gval == NULL)
	{
		dict = plist_get_dict(internalPlist, "Presets");
		if (dict == NULL) return NULL;
		gval = ghb_dict_lookup(dict, key);
	}
	return gval;
}

const gchar*
ghb_presets_get_description(GValue *pdict)
{
	if (pdict == NULL) return g_strdup("");
	return g_value_get_string(
		preset_dict_get_value(pdict, "preset_description"));
}


static const GValue*
preset_get_value(const gchar *folder, const gchar *name, const gchar *key)
{
	GValue *dict = NULL;
	gint folder_pos, pos;

	if (ghb_presets_find(presetsPlist, folder, name, &folder_pos, &pos))
	{
		dict = presets_get_dict(presetsPlist, folder_pos, pos);
	}
	return preset_dict_get_value(dict, key);
}

static void init_settings_from_dict(
	GValue *dest, GValue *internal, GValue *dict);

static void
init_settings_from_array(
	GValue *dest, 
	GValue *internal,
	GValue *array)
{
	GValue *gval, *val;
	gint count, ii;
	
	count = ghb_array_len(array);
	// The first element of the internal version is always the 
	// template for the allowed values
	gval = ghb_array_get_nth(internal, 0);
	for (ii = 0; ii < count; ii++)
	{
		val = NULL;
		val = ghb_array_get_nth(array, ii);
		if (val == NULL)
			val = gval;
		if (G_VALUE_TYPE(gval) == ghb_dict_get_type())
		{
			GValue *new_dict;
			new_dict = ghb_dict_value_new();
			ghb_array_append(dest, new_dict);
			if (G_VALUE_TYPE(val) == ghb_dict_get_type())
				init_settings_from_dict(new_dict, gval, val);
			else
				init_settings_from_dict(new_dict, gval, gval);
		}
		else if (G_VALUE_TYPE(gval) == ghb_array_get_type())
		{
			GValue *new_array;
			new_array = ghb_array_value_new(8);
			ghb_array_append(dest, new_array);
			if (G_VALUE_TYPE(val) == ghb_array_get_type())
				init_settings_from_array(new_array, gval, val);
			else
				init_settings_from_array(new_array, gval, gval);
		}
		else
		{
			ghb_array_append(dest, val);
		}
	}
}

static void
init_settings_from_dict(
	GValue *dest, 
	GValue *internal,
	GValue *dict)
{
	GHashTableIter iter;
	gchar *key;
	GValue *gval, *val;
	
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		val = NULL;
		if (dict)
			val = ghb_dict_lookup(dict, key);
		if (val == NULL)
			val = gval;
		if (G_VALUE_TYPE(gval) == ghb_dict_get_type())
		{
			GValue *new_dict;
			new_dict = ghb_dict_value_new();
			ghb_settings_take_value(dest, key, new_dict);
			if (G_VALUE_TYPE(val) == ghb_dict_get_type())
				init_settings_from_dict(new_dict, gval, val);
			else
				init_settings_from_dict(new_dict, gval, gval);
		}
		else if (G_VALUE_TYPE(gval) == ghb_array_get_type())
		{
			GValue *new_array;
			new_array = ghb_array_value_new(8);
			ghb_settings_take_value(dest, key, new_array);
			if (G_VALUE_TYPE(val) == ghb_array_get_type())
				init_settings_from_array(new_array, gval, val);
			else
				init_settings_from_array(new_array, gval, gval);
	
		}
		else
		{
			ghb_settings_set_value(dest, key, val);
		}
	}
}

void
init_ui_from_dict(
	signal_user_data_t *ud, 
	GValue *internal,
	GValue *dict)
{
	GHashTableIter iter;
	gchar *key;
	GValue *gval, *val;
	
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		val = NULL;
		if (dict)
			val = ghb_dict_lookup(dict, key);
		if (val == NULL)
			val = gval;
		ghb_ui_update(ud, key, val);
	}
}

static void
preset_to_ui(signal_user_data_t *ud, GValue *dict)
{
	g_debug("preset_to_ui()\n");
	// Initialize the ui from presets file.
	GValue *internal;

	// Get key list from internal default presets.  This way we do not
	// load any unknown keys.
	if (internalPlist == NULL) return;
	internal = plist_get_dict(internalPlist, "Presets");
	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	init_settings_from_dict(ud->settings, internal, dict);
	init_ui_from_dict(ud, internal, dict);

	if (ghb_settings_get_boolean(ud->settings, "allow_tweaks"))
	{
		const GValue *gval;
		gval = preset_dict_get_value(dict, "deinterlace");
		if (gval)
		{
			ghb_ui_update(ud, "tweak_deinterlace", gval);
		}
		gval = preset_dict_get_value(dict, "denoise");
		if (gval)
		{
			ghb_ui_update(ud, "tweak_denoise", gval);
		}
	}
}

void
ghb_settings_to_ui(signal_user_data_t *ud, GValue *dict)
{
	init_ui_from_dict(ud, dict, dict);
}

void
ghb_set_preset(signal_user_data_t *ud, const gchar *folder, const gchar *name)
{
	GValue *dict = NULL;
	gint folder_pos, pos, ptype;
	
	g_debug("ghb_set_preset() %s %s\n", folder, name);
	if (ghb_presets_find(presetsPlist, folder, name, &folder_pos, &pos))
		dict = presets_get_dict(presetsPlist, folder_pos, pos);

	if (dict == NULL)
	{
		dict = presets_get_first_dict(presetsPlist);
		folder = NULL;
		if (dict)
			name = preset_get_name(dict);
		else
			name = NULL;
		folder = "";
	}
	if (dict == NULL || name == NULL)
	{
		preset_to_ui(ud, NULL);
	}
	else
	{
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (ptype & PRESETS_FOLDER)
			preset_to_ui(ud, NULL);
		else
			preset_to_ui(ud, dict);
		ghb_settings_set_string(ud->settings, "preset", name);
		ghb_settings_set_string(ud->settings, "folder", folder);
	}
}

void
ghb_update_from_preset(
	signal_user_data_t *ud, 
	const gchar *folder, 
	const gchar *name, 
	const gchar *key)
{
	const GValue *gval;
	
	g_debug("ghb_update_from_preset() %s %s", name, key);
	if (name == NULL) return;
	gval = preset_get_value(folder, name, key);
	if (gval != NULL)
	{
		ghb_ui_update(ud, key, gval);
	}
}

gchar*
ghb_get_user_config_dir()
{
	const gchar *dir;
	gchar *config;

	dir = g_get_user_config_dir();
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
	{
		dir = g_get_home_dir();
		config = g_strdup_printf ("%s/.ghb", dir);
		if (!g_file_test(config, G_FILE_TEST_IS_DIR))
			g_mkdir (config, 0755);
	}
	else
	{
		config = g_strdup_printf ("%s/ghb", dir);
		if (!g_file_test(config, G_FILE_TEST_IS_DIR))
			g_mkdir (config, 0755);
	}
	return config;
}

static void
store_plist(GValue *plist, const gchar *name)
{
	gchar *config, *path;
	FILE *file;

	config = ghb_get_user_config_dir();
	path = g_strdup_printf ("%s/%s", config, name);
	file = g_fopen(path, "w");
	g_free(config);
	g_free(path);
	ghb_plist_write(file, plist);
	fclose(file);
}

static GValue*
load_plist(const gchar *name)
{
	gchar *config, *path;
	GValue *plist = NULL;

	config = ghb_get_user_config_dir();
	path = g_strdup_printf ("%s/%s", config, name);
	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		plist = ghb_plist_parse_file(path);
	}
	g_free(config);
	g_free(path);
	return plist;
}

static void
remove_plist(const gchar *name)
{
	gchar *config, *path;

	config = ghb_get_user_config_dir();
	path = g_strdup_printf ("%s/%s", config, name);
	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		g_unlink(path);
	}
	g_free(path);
	g_free(config);
}

static gboolean prefs_initializing = FALSE;

void
ghb_prefs_to_ui(signal_user_data_t *ud)
{
	const GValue *gval;
	gchar *key;
	gchar *str;
	GValue *internal, *dict;
	GHashTableIter iter;
	

	prefs_initializing = TRUE;

	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	internal = plist_get_dict(internalPlist, "Initialization");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_ui_update(ud, key, gval);
	}

	dict = plist_get_dict(prefsPlist, "Preferences");
	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
    {
		const GValue *value = NULL;
		if (dict)
			value = ghb_dict_lookup(dict, key);
		if (value == NULL)
			value = gval;
		ghb_settings_set_value(ud->settings, key, value);
    }
	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
   	{
		const GValue *value = NULL;
		if (dict)
			value = ghb_dict_lookup(dict, key);
		if (value == NULL)
			value = gval;
		ghb_ui_update(ud, key, value);
   	}
	const GValue *val;
	val = ghb_settings_get_value(ud->settings, "show_presets");
	ghb_ui_update(ud, "show_presets", val);
	if (ghb_settings_get_boolean(ud->settings, "hbfd_feature"))
	{
		GtkAction *action;
		val = ghb_settings_get_value(ud->settings, "hbfd");
		ghb_ui_update(ud, "hbfd", val);
		action = GHB_ACTION (ud->builder, "hbfd");
		gtk_action_set_visible(action, TRUE);
	}
	else
	{
		ghb_ui_update(ud, "hbfd", ghb_int64_value(0));
	}
	gval = ghb_settings_get_value(ud->settings, "default_source");
	ghb_settings_set_value (ud->settings, "source", gval);
	str = ghb_settings_get_string(ud->settings, "destination_dir");

	gchar *path = g_strdup_printf ("%s/new_video.mp4", str);
	ghb_ui_update(ud, "destination", ghb_string_value(path));
	g_free(str);
	g_free(path);

	prefs_initializing = FALSE;
}

void
ghb_prefs_save(GValue *settings)
{
	GValue *dict;
	GValue *pref_dict;
	GHashTableIter iter;
	gchar *key;
	const GValue *value;
	
	if (prefs_initializing) return;
	dict = plist_get_dict(internalPlist, "Preferences");
	if (dict == NULL) return;
	pref_dict = plist_get_dict(prefsPlist, "Preferences");
	if (pref_dict == NULL) return;
	ghb_dict_iter_init(&iter, dict);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
    {
	    value = ghb_settings_get_value(settings, key);
	    if (value != NULL)
	    {
			ghb_dict_insert(pref_dict, g_strdup(key), ghb_value_dup(value));
	    }
	}
    store_plist(prefsPlist, "preferences");
}

void
ghb_pref_save(GValue *settings, const gchar *key)
{
	const GValue *value;
	
	if (prefs_initializing) return;
	value = ghb_settings_get_value(settings, key);
	if (value != NULL)
	{
		GValue *dict;
		dict = plist_get_dict(prefsPlist, "Preferences");
		if (dict == NULL) return;
		ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(value));
		store_plist(prefsPlist, "preferences");
	}
}

void
ghb_settings_init(signal_user_data_t *ud)
{
	GValue *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *gval;


	g_debug("ghb_settings_init");
	prefs_initializing = TRUE;

	internalPlist = ghb_resource_get("internal-defaults");
	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	internal = plist_get_dict(internalPlist, "Initialization");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}

	internal = plist_get_dict(internalPlist, "Presets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}

	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}
	prefs_initializing = FALSE;
}

void
ghb_settings_close()
{
	if (internalPlist)
		ghb_value_free(internalPlist);
	if (presetsPlist)
		ghb_value_free(presetsPlist);
	if (prefsPlist)
		ghb_value_free(prefsPlist);
}

void
ghb_prefs_load(signal_user_data_t *ud)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *gval;
	
	g_debug("ghb_prefs_load");
	prefsPlist = load_plist("preferences");
	if (prefsPlist == NULL)
		prefsPlist = ghb_dict_value_new();
	dict = plist_get_dict(prefsPlist, "Preferences");
	internal = plist_get_dict(internalPlist, "Preferences");
    if (dict == NULL && internal)
    {
		dict = ghb_dict_value_new();
		ghb_dict_insert(prefsPlist, g_strdup("Preferences"), dict);

        // Get defaults from internal defaults 
		ghb_dict_iter_init(&iter, internal);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
        {
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(gval));
        }
		const gchar *dir = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
		if (dir == NULL)
		{
			dir = ".";
		}
		ghb_dict_insert(dict, 
			g_strdup("destination_dir"), ghb_value_dup(ghb_string_value(dir)));
		store_plist(prefsPlist, "preferences");
    }
}

void
ghb_presets_list_init(
	signal_user_data_t *ud, 
	GValue *presets, 
	const gchar *parent_name,
	GtkTreeIter *parent)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeStore *store;
	const gchar *preset;
	gchar *def_preset, *def_folder;
	const gchar *description;
	gint flags, custom;
	gboolean def;
	gint count, ii, ptype;
	GValue *dict;
	
	g_debug("ghb_presets_list_init ()");
	if (presets == NULL)
		presets = presetsPlist;
	def_folder = ghb_settings_get_string(ud->settings, "default_folder");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	count = ghb_array_len(presets);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	ii = 0;
	while (ii < count)
	{
		// Additional settings, add row
		g_debug("Adding rows");
		dict = ghb_array_get_nth(presets, ii);
		preset = preset_get_name(dict);
		def = FALSE;
		if (strcmp(preset, def_preset) == 0)
		{
			if (parent_name && strcmp(parent_name, def_folder) == 0)
				def = TRUE;
			else if (parent_name == NULL && def_folder[0] == 0)
				def = TRUE;
		}

		description = ghb_presets_get_description(dict);
		gtk_tree_store_append(store, &iter, parent);
		flags = ghb_preset_flags(dict);
		custom = flags & PRESETS_CUST;
		gtk_tree_store_set(store, &iter, 0, preset, 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, description,
						   	-1);
		if (def && parent)
		{
			GtkTreePath *path;

			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), parent);
			gtk_tree_view_expand_row(treeview, path, FALSE);
			gtk_tree_path_free(path);
		}
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (ptype & PRESETS_FOLDER)
		{
			GValue *nested;
			nested = ghb_dict_lookup(dict, "preset_folder");
			if (nested != NULL)
				ghb_presets_list_init(ud, nested, preset, &iter);
		}
		ii++;
	}
	g_free(def_preset);
	g_free(def_folder);
}

static void
presets_list_update_item(
	signal_user_data_t *ud, 
	GValue *presets,
	GtkTreeIter *iter,
	gint folder_pos,
	gint pos)
{
	GtkTreeView *treeview;
	GtkTreeStore *store;
	const gchar *preset;
	gchar *def_preset, *def_folder;
	const gchar *description;
	gint flags, custom;
	gboolean def;
	GValue *dict;
	const gchar *parent_name;
	
	g_debug("presets_list_update_item ()");
	dict = presets_get_dict(presets, folder_pos, pos);
	if (dict == NULL)
		return;
	def_folder = ghb_settings_get_string(ud->settings, "default_folder");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	// Additional settings, add row
	preset = preset_get_name(dict);
	if (pos >= 0)
	{
		GValue *parent_dict;
		parent_dict = presets_get_dict(presets, folder_pos, -1);
		parent_name = preset_get_name(parent_dict);
	}
	else
		parent_name = NULL;
	def = FALSE;
	if (strcmp(preset, def_preset) == 0)
	{
		if (parent_name && strcmp(parent_name, def_folder) == 0)
			def = TRUE;
		else if (parent_name == NULL && def_folder[0] == 0)
			def = TRUE;
	}

	description = ghb_presets_get_description(dict);
	flags = ghb_preset_flags(dict);
	custom = flags & PRESETS_CUST;
	gtk_tree_store_set(store, iter, 0, preset, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, custom ? "black" : "blue", 
						4, description,
					   	-1);
	if (flags & PRESETS_FOLDER)
	{
		presets = ghb_dict_lookup(dict, "preset_folder");
		ghb_presets_list_init(ud, presets, preset, iter);
	}
	g_free(def_preset);
}

static void
presets_list_insert(
	signal_user_data_t *ud, 
	GValue *presets,
	const gchar *parent_name,
	GtkTreeIter *parent,
	gint pos)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeStore *store;
	const gchar *preset;
	gchar *def_preset, *def_folder;
	const gchar *description;
	gint flags, custom;
	gboolean def;
	gint count;
	GValue *dict;
	
	g_debug("presets_list_insert ()");
	count = ghb_array_len(presets);
	if (pos >= count)
		return;
	def_folder = ghb_settings_get_string(ud->settings, "default_folder");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	// Additional settings, add row
	dict = ghb_array_get_nth(presets, pos);
	preset = preset_get_name(dict);
	def = FALSE;
	if (strcmp(preset, def_preset) == 0)
	{
		if (parent_name && strcmp(parent_name, def_folder) == 0)
			def = TRUE;
		else if (parent_name == NULL && def_folder[0] == 0)
			def = TRUE;
	}

	description = ghb_presets_get_description(dict);
	gtk_tree_store_insert(store, &iter, parent, pos);
	flags = ghb_preset_flags(dict);
	custom = flags & PRESETS_CUST;
	gtk_tree_store_set(store, &iter, 0, preset, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, custom ? "black" : "blue", 
						4, description,
					   	-1);
	if (flags & PRESETS_FOLDER)
	{
		presets = ghb_dict_lookup(dict, "preset_folder");
		ghb_presets_list_init(ud, presets, preset, &iter);
	}
	g_free(def_preset);
}

static void
presets_list_remove(
	signal_user_data_t *ud, 
	gint folder_pos,
	gint pos)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, piter;
	GtkTreeStore *store;
	
	g_debug("presets_list_remove ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	if (folder_pos >= 0)
	{
		if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &piter, 
			NULL, folder_pos))
		{
			if (pos >= 0)
			{
				if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, 
					&piter, pos))
				{
					gtk_tree_store_remove(store, &iter);
				}
			}
			else
			{
				gtk_tree_store_remove(store, &piter);
			}
		}
	}
	else if (pos >= 0)
	{
		if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, 
			NULL, pos))
		{
			gtk_tree_store_remove(store, &iter);
		}
	}
}

void
ghb_presets_reload(signal_user_data_t *ud)
{
	GValue *std_presets;
	gint count, ii;

	g_debug("ghb_presets_reload()\n");
	std_presets = ghb_resource_get("standard-presets");
	if (std_presets == NULL) return;

	// Merge the keyfile contents into our presets
	count = ghb_array_len(std_presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *name;
		GValue *std_dict;
		GValue *copy_dict;
		GHashTableIter iter;
		gchar *key;
		GValue *value;
		gint folder_pos, pos;

		std_dict = ghb_array_get_nth(std_presets, ii);
		name = preset_get_name(std_dict);
		if (ghb_presets_find(presetsPlist, NULL, name, &folder_pos, &pos))
		{
			if (ghb_presets_remove(presetsPlist, folder_pos, pos))
			{
				presets_list_remove(ud, folder_pos, pos);
			}
		}
		copy_dict = ghb_dict_value_new();
		pos = presets_find_pos(presetsPlist, name, 0);
		ghb_array_insert(presetsPlist, pos, copy_dict);
		ghb_dict_iter_init(&iter, std_dict);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
		{
			ghb_dict_insert(copy_dict, g_strdup(key), ghb_value_dup(value));
		}
		presets_list_insert(ud, presetsPlist, NULL, NULL, pos);
	}
	store_plist(presetsPlist, "presets");
}

void
ghb_save_queue(GValue *queue)
{
	store_plist(queue, "queue");
}

GValue*
ghb_load_queue()
{
	return load_plist("queue");
}

void
ghb_remove_queue_file()
{
	remove_plist("queue");
}

void
ghb_presets_load()
{
	presetsPlist = load_plist("presets");
	if (presetsPlist == NULL)
	{
		presetsPlist = ghb_value_dup(ghb_resource_get("standard-presets"));
		store_plist(presetsPlist, "presets");
	}
	if (G_VALUE_TYPE(presetsPlist) == ghb_dict_get_type())
	{ // Presets is older dictionary format. Convert to array
		GHashTableIter old_iter;
		GValue *presets;
		gchar *name;
		GValue *orig_dict;

		presets = ghb_array_value_new(32);
		ghb_dict_iter_init(&old_iter, presetsPlist);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
			&old_iter, (gpointer*)(void*)&name, (gpointer*)(void*)&orig_dict))
		{
			GHashTableIter iter;
			gchar *key;
			GValue *value, *dict;

			dict = ghb_dict_value_new();
			ghb_dict_insert(dict, g_strdup("preset_name"), 
				ghb_string_value_new(name));
			ghb_array_append(presets, dict);
			ghb_dict_iter_init(&iter, orig_dict);
			// middle (void*) cast prevents gcc warning "defreferencing 
			// type-punned pointer will break strict-aliasing rules"
			while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
			{
				ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(value));
			}
		}
		ghb_value_free(presetsPlist);
		presetsPlist = presets;
		store_plist(presetsPlist, "presets");
	}
}

static void
settings_save(signal_user_data_t *ud, const gchar *folder, const gchar *name)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *value;
	gboolean autoscale;
	gint folder_pos, pos;

	if (internalPlist == NULL) return;
	if (ghb_settings_get_boolean(ud->settings, "allow_tweaks"))
	{
		gchar *str;
		str = ghb_settings_get_string(ud->settings, "tweak_deinterlace");
		if (str)
		{
			ghb_settings_set_string(ud->settings, "deinterlace", str);
			g_free(str);
		}
		str = ghb_settings_get_string(ud->settings, "tweak_denoise");
		if (str)
		{
			ghb_settings_set_string(ud->settings, "denoise", str);
			g_free(str);
		}
	}
	autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");
	ghb_settings_set_int64(ud->settings, "preset_type", PRESETS_CUST);

	dict = ghb_dict_value_new();
	if (ghb_presets_find(presetsPlist, folder, name, &folder_pos, &pos))
	{
		if (ghb_presets_get_type(presetsPlist, folder_pos, pos) & 
			PRESETS_FOLDER)
		{
			gchar *message;
			message = g_strdup_printf(
						"%s: Folder already exists.\n"
						"You can not replace it with a preset.",
						name);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			return;
		}
		ghb_presets_replace(presetsPlist, dict, folder_pos, pos);
		pos = -1;
	}
	else
	{
		pos = presets_find_pos(presetsPlist, name, 1);
		ghb_array_insert(presetsPlist, pos, dict);
	}
	ghb_dict_insert(dict, g_strdup("preset_name"), ghb_string_value_new(name));

	internal = plist_get_dict(internalPlist, "Presets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
	{
		const GValue *gval;
		gchar *key2;

		key2 = key;
		if (!autoscale)
		{
			if (strcmp(key, "max_width") == 0)
			{
				key2 = "scale_width";
			}
			else if (strcmp(key, "max_height") == 0)
			{
				key2 = "scale_height";
			}
		}
		gval = ghb_settings_get_value(ud->settings, key2);
		if (gval == NULL)
		{
			g_debug("Setting (%s) is not in defaults\n", (gchar*)key);
			continue;
		}
		if (ghb_value_cmp(gval, value) != 0)
		{
			// Differs from default value.  Store it.
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(gval));
		}
	}
	if (pos >= 0)
		presets_list_insert(ud, presetsPlist, NULL, NULL, pos);
	store_plist(presetsPlist, "presets");
	ud->dont_clear_presets = TRUE;
	ghb_set_preset (ud, NULL, name);
	ud->dont_clear_presets = FALSE;
	return;
}

static void
folder_save(signal_user_data_t *ud, const gchar *name)
{
	GValue *dict, *folder;
	gint folder_pos, pos;


	if (ghb_presets_find(presetsPlist, name, NULL, &folder_pos, &pos))
	{
		if (!(ghb_presets_get_type(presetsPlist, folder_pos, pos) & 
			PRESETS_FOLDER))
		{
			gchar *message;
			message = g_strdup_printf(
						"%s: Preset already exists.\n"
						"You can not replace it with a folder.",
						name);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			return;
		}
		// Already exists, update its description
		dict = presets_get_dict(presetsPlist, folder_pos, pos);
		ghb_dict_insert(dict, g_strdup("preset_description"), 
			ghb_value_dup(preset_dict_get_value(
				ud->settings, "preset_description")));
		return;
	}
	else
	{
		dict = ghb_dict_value_new();
		pos = presets_find_pos(presetsPlist, name, 1);
		ghb_array_insert(presetsPlist, pos, dict);
	}
	ghb_dict_insert(dict, g_strdup("preset_description"), 
		ghb_value_dup(preset_dict_get_value(
			ud->settings, "preset_description")));
	ghb_dict_insert(dict, g_strdup("preset_name"), ghb_string_value_new(name));
	folder = ghb_array_value_new(8);
	ghb_dict_insert(dict, g_strdup("preset_folder"), folder);
	ghb_dict_insert(dict, g_strdup("preset_type"),
							ghb_int64_value_new(PRESETS_FOLDER|PRESETS_CUST));

	presets_list_insert(ud, presetsPlist, NULL, NULL, pos);
	store_plist(presetsPlist, "presets");
	ud->dont_clear_presets = TRUE;
	ghb_set_preset (ud, NULL, name);
	ud->dont_clear_presets = FALSE;
	return;
}

void
ghb_presets_list_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, citer;
	GtkTreeStore *store;
	gboolean done;
	gchar *preset;
	gchar *def_preset, *def_folder;
	gint def, weight;
	
	g_debug("ghb_presets_list_default ()");
	def_folder = ghb_settings_get_string(ud->settings, "default_folder");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		if (def_folder[0] != 0) 
		{
			gboolean found = FALSE;
			do
			{
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 
									0, &preset, 1, &weight, -1);
				if (strcmp(preset, def_folder) == 0)
				{
					if (gtk_tree_model_iter_children(
									GTK_TREE_MODEL(store), &citer, &iter))
					{
						iter = citer;
						found = TRUE;
					}
					g_free(preset);
					break;
				}
				g_free(preset);
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			} while (!done);
			if (!found) return;
		}
		do
		{
			def = FALSE;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 
								0, &preset, 1, &weight, -1);
			if (strcmp(preset, def_preset) == 0)
				def = TRUE;
			if ((!def && weight == 800) || def)
			{
				gtk_tree_store_set(store, &iter, 
							1, def ? 800 : 400, 
							2, def ? 2 : 0,
							-1);
			}
			g_free(preset);
			done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		} while (!done);
	}
	g_free(def_folder);
	g_free(def_preset);
}

void
ghb_presets_list_clear_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, piter;
	GtkTreeStore *store;
	gchar *def_preset, *def_folder;
	gint folder_pos, pos;
	gboolean found = FALSE;
	
	g_debug("ghb_presets_list_default ()");
	def_folder = ghb_settings_get_string(ud->settings, "default_folder");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	if (!ghb_presets_find(presetsPlist, def_folder, def_preset, 
		&folder_pos, &pos))
	{
		return;
	}
	// de-emphasize the current default
	if (folder_pos >= 0)
	{
		if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &piter, 
			NULL, folder_pos))
		{
			if (pos >= 0)
			{
				if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, 
					&piter, pos))
				{
					found = TRUE;
				}
			}
			else
			{
				found = TRUE;
			}
		}
	}
	else if (pos >= 0)
	{
		if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, 
			NULL, pos))
		{
			found = TRUE;
		}
	}
	if (found)
	{
		gtk_tree_store_set(store, &iter, 
					1, 400, 
					2, 0,
					-1);
	}
	g_free(def_folder);
	g_free(def_preset);
}

static void
ghb_select_preset2(
	GtkBuilder *builder, 
	gint folder_pos, 
	gint pos)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	
	g_debug("ghb_select_preset2() (%d) (%d)", folder_pos, pos);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = gtk_tree_view_get_model (treeview);
	if (folder_pos == -1)
	{
		folder_pos = pos;
		pos = -1;
	}
	path = gtk_tree_path_new_from_indices(folder_pos, pos, -1);
	if (gtk_tree_model_get_iter(store, &iter, path))
	{
		gtk_tree_selection_select_iter (selection, &iter);
	}
	else
	{
		gtk_tree_model_get_iter_first(store, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
	}
	gtk_tree_path_free(path);
}

void
ghb_select_preset(GtkBuilder *builder, const gchar *folder, const gchar *preset)
{
	gint folder_pos, pos;

	g_debug("ghb_select_preset() (%s) (%s)", folder, preset);
	if (ghb_presets_find(presetsPlist, folder, preset, &folder_pos, &pos))
	{
		ghb_select_preset2(builder, folder_pos, pos);
	}
}

static void
update_audio_presets(signal_user_data_t *ud)
{
	g_debug("update_audio_presets");
	const GValue *audio_list;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	ghb_settings_set_value(ud->settings, "pref_audio_list", audio_list);
}

void
enforce_preset_type(const gchar *name, signal_user_data_t *ud)
{
	gint folder_pos, pos;
	GtkWidget *normal, *folder;
	gint ptype;

	normal = GHB_WIDGET(ud->builder, "preset_type_normal");
	folder = GHB_WIDGET(ud->builder, "preset_type_folder");
	if (ghb_presets_find(presetsPlist, NULL, name, &folder_pos, &pos))
	{
		ptype = ghb_presets_get_type(presetsPlist, folder_pos, pos);
		if (ptype & PRESETS_FOLDER)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(folder), 
									TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(normal), 
									TRUE);
		gtk_widget_set_sensitive(folder,  ptype & PRESETS_FOLDER);
		gtk_widget_set_sensitive(normal,  !(ptype & PRESETS_FOLDER));
	}
	else
	{
		gtk_widget_set_sensitive(folder, TRUE);
		gtk_widget_set_sensitive(normal, TRUE);
	}
}

void
presets_save_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	GtkTextView *desc;
	GtkResponseType response;
	gchar *preset;

	g_debug("presets_save_clicked_cb ()");
	preset = ghb_settings_get_string (ud->settings, "preset");
	// Clear the description
	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "preset_description"));
	dialog = GHB_WIDGET(ud->builder, "preset_save_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "preset_name"));
	gtk_entry_set_text(entry, preset);
	enforce_preset_type(preset, ud);
	g_free(preset);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		g_debug("description to settings");
		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
		if (ghb_settings_get_boolean(ud->settings, "preset_type_folder"))
		{
			folder_save(ud, name);
		}
		else
		{
			// Construct the audio settings presets from the current audio list
			update_audio_presets(ud);
			settings_save(ud, NULL, name);
		}
		// Make the new preset the selected item
		ghb_select_preset(ud->builder, NULL, name);
	}
}

void
preset_type_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
}

void
preset_name_changed_cb(GtkWidget *entry, signal_user_data_t *ud)
{
	gchar *name;

	name = ghb_widget_string(entry);
	enforce_preset_type(name, ud);
}

void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	g_debug("presets_restore_clicked_cb ()");
	// Reload only the standard presets
	ghb_presets_reload(ud);
	// Updating the presets list shuffles things around
	// need to make sure the proper preset is selected
	gchar *folder = ghb_settings_get_string (ud->settings, "folder");
	gchar *preset = ghb_settings_get_string (ud->settings, "preset");
	ghb_select_preset(ud->builder, folder, preset);
	g_free(preset);
}

void
presets_remove_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *preset;
	GtkResponseType response;

	g_debug("presets_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkWidget *dialog;
		GtkTreePath *path;
		gint *indices;
		gint folder_pos, pos, ptype;

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		path = gtk_tree_model_get_path(store, &iter);
		indices = gtk_tree_path_get_indices(path);
		folder_pos = indices[0];
		pos = -1;
		if (gtk_tree_path_get_depth(path) > 1)
			pos = indices[1];
		gtk_tree_path_free(path);
		ptype = ghb_presets_get_type(presetsPlist, folder_pos, pos);
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
							GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
							"Confirm deletion of %s:\n\n%s", 
							(ptype & PRESETS_FOLDER) ? "folder" : "preset",
							preset);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		if (response == GTK_RESPONSE_YES)
		{
			GtkTreeIter nextIter = iter;
			gboolean valid = TRUE;
			if (!gtk_tree_model_iter_next(store, &nextIter))
			{
				if (!gtk_tree_model_iter_parent(store, &nextIter, &iter))
				{
					valid = FALSE;
				}
			}
			// Remove the selected item
			// First unselect it so that selecting the new item works properly
			gtk_tree_selection_unselect_iter (selection, &iter);
			if (ghb_presets_remove(presetsPlist, folder_pos, pos))
			{
				store_plist(presetsPlist, "presets");
				presets_list_remove(ud, folder_pos, pos);
			}
			if (!valid)
				valid = gtk_tree_model_get_iter_first(store, &nextIter);
			if (valid)
			{
				path = gtk_tree_model_get_path(store, &nextIter);
				indices = gtk_tree_path_get_indices(path);
				folder_pos = indices[0];
				pos = -1;
				if (gtk_tree_path_get_depth(path) > 1)
					pos = indices[1];
				gtk_tree_path_free(path);
				ghb_select_preset2(ud->builder, folder_pos, pos);
			}
		}
		g_free(preset);
	}
}

// controls where valid drop locations are
gboolean
presets_drag_motion_cb(
	GtkTreeView *tv,
	GdkDragContext *ctx,
	gint x,
	gint y,
	guint time,
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition drop_pos;
	gint *indices, folder_pos, pos;
	GtkTreeIter iter;
	GtkTreeView *srctv;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	gint src_ptype, dst_ptype;

	// Get the type of the object being dragged
	srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
	select = gtk_tree_view_get_selection (srctv);
	gtk_tree_selection_get_selected (select, &model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices(path);
	folder_pos = indices[0];
	pos = -1;
	if (gtk_tree_path_get_depth(path) > 1)
		pos = indices[1];
	gtk_tree_path_free(path);
	src_ptype = ghb_presets_get_type(presetsPlist, folder_pos, pos);

	// The rest checks that the destination is a valid position
	// in the list.
	gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &drop_pos);
	if (path == NULL)
	{
		gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
		return TRUE;
	}
	indices = gtk_tree_path_get_indices(path);
	folder_pos = indices[0];
	pos = -1;
	if (gtk_tree_path_get_depth(path) > 1)
		pos = indices[1];
	dst_ptype = ghb_presets_get_type(presetsPlist, folder_pos, pos);

	// Don't allow *drop into* if the source is a folder
	if (((src_ptype & PRESETS_FOLDER) || (!(dst_ptype & PRESETS_FOLDER))) && 
		drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
		drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
	if (((src_ptype & PRESETS_FOLDER) || (!(dst_ptype & PRESETS_FOLDER))) && 
		drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
		drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	// Don't allow droping folders into child items
	if ((src_ptype & PRESETS_FOLDER) && gtk_tree_path_get_depth(path) > 1)
	{
		gtk_tree_path_up(path);
		drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	}
	gtk_tree_view_set_drag_dest_row(tv, path, drop_pos);
	gtk_tree_path_free(path);
	gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
	return TRUE;
}

void 
presets_drag_cb(
	GtkTreeView *dstwidget, 
	GdkDragContext *dc, 
	gint x, gint y, 
	GtkSelectionData *selection_data, 
	guint info, guint t, 
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition drop_pos;
	GtkTreeIter dstiter, srciter;
	gint *indices;
	gint dst_folder_pos, dst_pos, src_folder_pos, src_pos;
	gint src_ptype, dst_ptype;
	
	GtkTreeModel *dstmodel = gtk_tree_view_get_model(dstwidget);
			
	g_debug("preset_drag_cb ()");
	// This doesn't work here for some reason...
	// gtk_tree_view_get_drag_dest_row(dstwidget, &path, &drop_pos);
	gtk_tree_view_get_dest_row_at_pos (dstwidget, x, y, &path, &drop_pos);
	// This little hack is needed because attempting to drop after
	// the last item gives us no path or drop_pos.
	if (path == NULL)
	{
		gint n_children;

		n_children = gtk_tree_model_iter_n_children(dstmodel, NULL);
		if (n_children)
		{
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
			path = gtk_tree_path_new_from_indices(n_children-1, -1);
		}
		else
		{
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			path = gtk_tree_path_new_from_indices(0, -1);
		}
	}
	if (path)
	{
		GtkTreeView *srcwidget;
		GtkTreeModel *srcmodel;
		GtkTreeSelection *select;
		GtkTreePath *srcpath = NULL;
		GValue *dict, *presets;
		GValue *preset;

		srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
		select = gtk_tree_view_get_selection (srcwidget);
		gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

		srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
		indices = gtk_tree_path_get_indices(srcpath);
		src_folder_pos = indices[0];
		src_pos = -1;
		if (gtk_tree_path_get_depth(srcpath) > 1)
			src_pos = indices[1];
		src_ptype = ghb_presets_get_type(presetsPlist, src_folder_pos, src_pos);
		preset = presets_get_dict(presetsPlist, src_folder_pos, src_pos);
		gtk_tree_path_free(srcpath);

		indices = gtk_tree_path_get_indices(path);
		dst_folder_pos = indices[0];
		dst_pos = -1;
		if (gtk_tree_path_get_depth(path) > 1)
			dst_pos = indices[1];
		dst_ptype = ghb_presets_get_type(presetsPlist, dst_folder_pos, dst_pos);

		if ((src_ptype & PRESETS_FOLDER) && gtk_tree_path_get_depth(path) > 1)
			gtk_tree_path_up(path);

		if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
		{
			GtkTreeIter iter;
			GtkTreePath *dstpath = NULL;

			if ((src_ptype & PRESETS_FOLDER) || 
				gtk_tree_path_get_depth(path) > 1)
			{
				switch (drop_pos)
				{
					case GTK_TREE_VIEW_DROP_BEFORE:
					case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
						gtk_tree_store_insert_before(GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
						break;

					case GTK_TREE_VIEW_DROP_AFTER:
					case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
						gtk_tree_store_insert_after(GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
						break;

					default:
						break;
				}
			}
			else
			{
				switch (drop_pos)
				{
					case GTK_TREE_VIEW_DROP_BEFORE:
						gtk_tree_store_insert_before(GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
						break;

					case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
						gtk_tree_store_insert(GTK_TREE_STORE (dstmodel), 
													&iter, &dstiter, 0);
						break;

					case GTK_TREE_VIEW_DROP_AFTER:
						gtk_tree_store_insert_after(GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
						break;

					case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
						gtk_tree_store_insert_after(GTK_TREE_STORE (dstmodel), 
													&iter, &dstiter, 0);
						break;

					default:
						break;
				}
			}
			presets_list_update_item(ud, presetsPlist, &iter, 
				src_folder_pos, src_pos);

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			indices = gtk_tree_path_get_indices(dstpath);
			dst_folder_pos = indices[0];
			dst_pos = -1;
			if (gtk_tree_path_get_depth(dstpath) > 1)
				dst_pos = indices[1];
			gtk_tree_path_free(dstpath);
			if (dst_pos != -1)
			{
				dict = presets_get_dict(presetsPlist, dst_folder_pos, -1);
				presets = ghb_dict_lookup(dict, "preset_folder");
				ghb_array_insert(presets, dst_pos, preset);
			}
			else
			{
				ghb_array_insert(presetsPlist, dst_folder_pos, preset);
			}

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			indices = gtk_tree_path_get_indices(srcpath);
			src_folder_pos = indices[0];
			src_pos = -1;
			if (gtk_tree_path_get_depth(srcpath) > 1)
				src_pos = indices[1];
			gtk_tree_path_free(srcpath);
			if (src_pos != -1)
			{
				dict = presets_get_dict(presetsPlist, src_folder_pos, -1);
				presets = ghb_dict_lookup(dict, "preset_folder");
				ghb_array_remove(presets, src_pos);
			}
			else
			{
				ghb_array_remove(presetsPlist, src_folder_pos);
			}
			gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);
			store_plist(presetsPlist, "presets");
		}
		gtk_tree_path_free(path);
	}
}

static void
preset_update_title_deps(signal_user_data_t *ud, ghb_title_info_t *tinfo)
{
	GtkWidget *widget;

	ghb_ui_update(ud, "scale_width", 
			ghb_int64_value(tinfo->width - tinfo->crop[2] - tinfo->crop[3]));
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect, anamorphic;
	keep_aspect = ghb_settings_get_boolean(ud->settings, "keep_aspect");
	anamorphic = ghb_settings_get_boolean(ud->settings, "anamorphic");
	if (!(keep_aspect || anamorphic))
	{
		ghb_ui_update(ud, "scale_height", 
			ghb_int64_value(tinfo->height - tinfo->crop[0] - tinfo->crop[1]));
	}

	// Set the limits of cropping.  hb_set_anamorphic_size crashes if
	// you pass it a cropped width or height == 0.
	gint bound;
	bound = tinfo->height / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_top");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_bottom");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	bound = tinfo->width / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_left");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_right");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	if (ghb_settings_get_boolean(ud->settings, "autocrop"))
	{
		ghb_ui_update(ud, "crop_top", ghb_int64_value(tinfo->crop[0]));
		ghb_ui_update(ud, "crop_bottom", ghb_int64_value(tinfo->crop[1]));
		ghb_ui_update(ud, "crop_left", ghb_int64_value(tinfo->crop[2]));
		ghb_ui_update(ud, "crop_right", ghb_int64_value(tinfo->crop[3]));
	}
}

void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter, piter;
	gchar *preset;
	gchar *folder = NULL;
	ghb_title_info_t tinfo;
	GtkWidget *widget;
	
	g_debug("presets_list_selection_changed_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		if (gtk_tree_model_iter_parent(store, &piter, &iter))
		{
			gtk_tree_model_get(store, &piter, 0, &folder, -1);
		}
		ud->dont_clear_presets = TRUE;
		// Temporarily set the video_quality range to (0,100)
		// This is needed so the video_quality value does not get
		// truncated when set.  The range will be readjusted below
		GtkWidget *qp = GHB_WIDGET(ud->builder, "video_quality");
		gtk_range_set_range (GTK_RANGE(qp), 0, 100);
		// Clear the audio list prior to changing the preset.  Existing audio
		// can cause the container extension to be automatically changed when
		// it shouldn't be
		ghb_clear_audio_list(ud);
		ghb_set_preset(ud, folder, preset);
		gint titleindex;
		titleindex = ghb_settings_combo_int(ud->settings, "title");
		ghb_set_pref_audio(titleindex, ud);
		ghb_settings_set_boolean(ud->settings, "preset_modified", FALSE);
		ud->dont_clear_presets = FALSE;
		if (ghb_get_title_info (&tinfo, titleindex))
		{
			preset_update_title_deps(ud, &tinfo);
		}
		ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);

		gint vqmin, vqmax;
		ghb_vquality_range(ud, &vqmin, &vqmax);
		gtk_range_set_range (GTK_RANGE(qp), vqmin, vqmax);
		gtk_widget_set_sensitive(widget, TRUE);
		g_free(preset);
		if (folder) g_free(folder);
	}
	else
	{
		g_debug("No selection???  Perhaps unselected.");
		gtk_widget_set_sensitive(widget, FALSE);
	}
}

void
ghb_clear_presets_selection(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	
	if (ud->dont_clear_presets) return;
	g_debug("ghb_clear_presets_selection()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_unselect_all (selection);
	ghb_settings_set_boolean(ud->settings, "preset_modified", TRUE);
}

void
presets_frame_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *path;
		path = gtk_tree_model_get_path (store, &iter);
		// Make the parent visible in scroll window if it is not.
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
}

void
presets_default_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	gchar *folder, *preset;
	gint folder_pos, pos;

	folder = ghb_settings_get_string(ud->settings, "folder");
	preset = ghb_settings_get_string(ud->settings, "preset");
	if (ghb_presets_find(presetsPlist, folder, preset, &folder_pos, &pos))
	{
		if (!(ghb_presets_get_type(presetsPlist, folder_pos, pos) & 
			PRESETS_FOLDER))
		{
			ghb_presets_list_clear_default(ud);
			ghb_set_preset_default(ud->settings);
			ghb_presets_list_default(ud);
		}
	}
	g_free(folder);
	g_free(preset);
}

