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
#include "audiohandler.h"
#include "hb-backend.h"
#include "plist.h"
#include "resources.h"
#include "presets.h"
#include "values.h"

static GValue *presetsPlist = NULL;
static GValue *internalPlist = NULL;
static GValue *prefsPlist = NULL;

static GValue*
plist_get_dict(GValue *presets, const gchar *name)
{
	if (presets == NULL || name == NULL) return NULL;
	return ghb_dict_lookup(presets, name);
}

static GValue*
presets_get_dict(GValue *presets, const gchar *name)
{
	GValue *dict, *gval;
	gint count, ii;
	
	if (presets == NULL || name == NULL) return NULL;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		gval = ghb_dict_lookup(dict, "preset_name");
		str = g_value_get_string(gval);
		if (strcmp(str, name) == 0)
			return dict;
	}
	return NULL;
}

static gint
presets_remove(GValue *presets, const gchar *name)
{
	GValue *dict, *gval;
	gint count, ii;
	
	if (presets == NULL || name == NULL) return -1;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		gval = ghb_dict_lookup(dict, "preset_name");
		str = g_value_get_string(gval);
		if (strcmp(str, name) == 0)
		{
			ghb_array_remove(presets, ii);
			return ii;
		}
	}
	return -1;
}

void
ghb_set_preset_default(GValue *settings)
{
	gchar *preset;
	
	preset = ghb_settings_get_string (settings, "preset");
	ghb_settings_set_string(settings, "default_preset", preset);
	ghb_prefs_save(settings);
	g_free(preset);
}

// Used for sorting dictionaries.
gint
key_cmp(gconstpointer a, gconstpointer b)
{
	gchar *stra = (gchar*)a;
	gchar *strb = (gchar*)b;

	return strcmp(stra, strb);
}

gchar*
ghb_presets_get_description(const gchar *name)
{
	GValue *pdict;
	pdict = presets_get_dict(presetsPlist, name);
	if (pdict == NULL) return g_strdup("");
	return ghb_value_string(ghb_dict_lookup(pdict, "preset_description"));
}

static const GValue*
preset_dict_get_value(
	GValue *dict,
	const gchar *key)
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

static const GValue*
preset_get_value(
	const gchar *name,
	const gchar *key)
{
	GValue *dict;

	dict = presets_get_dict(presetsPlist, name);
	return preset_dict_get_value(dict, key);
}

GList*
ghb_presets_get_names()
{
	GList *names;
	GList *standard = NULL;
	GList *custom = NULL;
	gint ii, count;

	if (presetsPlist == NULL) return NULL;
	count = ghb_array_len(presetsPlist);
	for (ii = 0; ii < count; ii++)
	{
		gchar *name;
		gint ptype;
		GValue *dict;
		GValue *gval;

		dict = ghb_array_get_nth(presetsPlist, ii);
		gval = ghb_dict_lookup(dict, "preset_name");
		name = (gchar*)g_value_get_string(gval);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (ptype)
		{
			custom = g_list_append(custom, name);
		}
		else
		{
			standard = g_list_append(standard, name);
		}
	}
	custom = g_list_sort(custom, key_cmp);
	standard = g_list_sort(standard, key_cmp);
	names = g_list_concat(standard, custom);
	return names;
}

gint
ghb_preset_flags(const gchar *name)
{
	GValue *dict;
	const GValue *gval;
	gint ptype;
	gint ret = 0;

	dict = presets_get_dict(presetsPlist, name);
	gval = preset_dict_get_value(dict, "preset_type");
	if (gval)
	{
		ptype = ghb_value_int(gval);
		ret = (ptype != 0 ? PRESET_CUSTOM : 0);
	}
	return ret;
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
ghb_set_preset(signal_user_data_t *ud, const gchar *name)
{
	GValue *dict;
	
	g_debug("ghb_set_preset() %s\n", name);
	if (name == NULL)
	{
		GList *presets;
		// Try to get the first preset
		presets = ghb_presets_get_names();
		if (presets)
		{
			name = (const gchar*)presets->data;
			g_list_free(presets);
		}
	}
	dict = presets_get_dict(presetsPlist, name);
	if (dict == NULL || name == NULL)
	{
		preset_to_ui(ud, NULL);
	}
	else
	{
		preset_to_ui(ud, dict);
		ghb_settings_set_string(ud->settings, "preset", name);
	}
}

void
ghb_update_from_preset(
	signal_user_data_t *ud, 
	const gchar *name, 
	const gchar *key)
{
	const GValue *gval;
	
	g_debug("ghb_update_from_preset() %s %s", name, key);
	if (name == NULL) return;
	gval = preset_get_value(name, key);
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
		gint pos;

		std_dict = ghb_array_get_nth(std_presets, ii);
		name = g_value_get_string(ghb_dict_lookup(std_dict, "preset_name"));
		pos = presets_remove(presetsPlist, name);

		copy_dict = ghb_dict_value_new();
		if (pos >= 0)
			ghb_array_insert(presetsPlist, pos, copy_dict);
		else
			ghb_array_append(presetsPlist, copy_dict);
		ghb_dict_iter_init(&iter, std_dict);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
		{
			ghb_dict_insert(copy_dict, g_strdup(key), ghb_value_dup(value));
		}
	}
	store_plist(presetsPlist, "presets");
}

static void
presets_store()
{
	g_debug("presets_store ()\n");
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
		presets_store();
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
		presets_store();
	}
}

void
ghb_settings_save(signal_user_data_t *ud, const gchar *name)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *value;
	gboolean autoscale;

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
	ghb_settings_set_int64(ud->settings, "preset_type", 1);

	dict = ghb_dict_value_new();
	ghb_dict_insert(dict, g_strdup("preset_name"), ghb_string_value_new(name));
	ghb_array_append(presetsPlist, dict);
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
	presets_store();
	ud->dont_clear_presets = TRUE;
	ghb_set_preset (ud, name);
	ud->dont_clear_presets = FALSE;
}

void
ghb_presets_remove(const gchar *name)
{
	if (presets_get_dict(presetsPlist, name))
	{
		presets_remove(presetsPlist, name);
		presets_store();
	}
}

void
ghb_presets_list_update(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	GList *presets, *plink;
	gchar *preset, *def_preset;
	gchar *description;
	gint flags, custom, def;
	
	g_debug("ghb_presets_list_update ()");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	plink = presets = ghb_presets_get_names();
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			if (plink)
			{
				// Update row with settings data
				g_debug("Updating row");
				preset = (gchar*)plink->data;
				def = 0;
				if (strcmp(preset, def_preset) == 0)
					def = PRESET_DEFAULT;
				
				description = ghb_presets_get_description(preset);
				flags = ghb_preset_flags(preset);
				custom = flags & PRESET_CUSTOM;
				gtk_list_store_set(store, &iter, 
							0, preset, 
							1, def ? 800 : 400, 
							2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, description,
							-1);
				plink = plink->next;
				g_free(description);
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				// No more settings data, remove row
				g_debug("Removing row");
				done = !gtk_list_store_remove(store, &iter);
			}
		} while (!done);
	}
	while (plink)
	{
		// Additional settings, add row
		g_debug("Adding rows");
		preset = (gchar*)plink->data;
		def = 0;
		if (strcmp(preset, def_preset) == 0)
			def = PRESET_DEFAULT;

		description = ghb_presets_get_description(preset);
		gtk_list_store_append(store, &iter);
		flags = ghb_preset_flags(preset);
		custom = flags & PRESET_CUSTOM;
		gtk_list_store_set(store, &iter, 0, preset, 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, description,
						   	-1);
		plink = plink->next;
		g_free(description);
	}
	g_free(def_preset);
	g_list_free (presets);
}

void
ghb_select_preset(GtkBuilder *builder, const gchar *preset)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *tpreset;
	gboolean done;
	gboolean foundit = FALSE;
	
	g_debug("select_preset()");
	if (preset == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = gtk_tree_view_get_model (treeview);
	if (gtk_tree_model_get_iter_first(store, &iter))
	{
		do
		{
			gtk_tree_model_get(store, &iter, 0, &tpreset, -1);
			if (strcmp(preset, tpreset) == 0)
			{
				gtk_tree_selection_select_iter (selection, &iter);
				foundit = TRUE;
				g_free(tpreset);
				break;
			}
			g_free(tpreset);
			done = !gtk_tree_model_iter_next(store, &iter);
		} while (!done);
	}
	if (!foundit)
	{
		gtk_tree_model_get_iter_first(store, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
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
	g_free(preset);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		g_debug("description to settings");
		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
		// Construct the audio settings presets from the current audio list
		update_audio_presets(ud);
		ghb_settings_save(ud, name);
		ghb_presets_list_update(ud);
		// Make the new preset the selected item
		ghb_select_preset(ud->builder, name);
	}
}

void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	g_debug("presets_restore_clicked_cb ()");
	// Reload only the standard presets
	ghb_presets_reload(ud);
	ghb_presets_list_update(ud);
	// Updating the presets list shuffles things around
	// need to make sure the proper preset is selected
	gchar *preset = ghb_settings_get_string (ud->settings, "preset");
	ghb_select_preset(ud->builder, preset);
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

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
								GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
								"Confirm deletion of preset %s.", preset);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		if (response == GTK_RESPONSE_YES)
		{
			GtkTreeIter nextIter = iter;
			gchar *nextPreset = NULL;
			if (!gtk_tree_model_iter_next(store, &nextIter))
			{
				if (gtk_tree_model_get_iter_first(store, &nextIter))
				{
					gtk_tree_model_get(store, &nextIter, 0, &nextPreset, -1);
				}
			}
			else
			{
				gtk_tree_model_get(store, &nextIter, 0, &nextPreset, -1);
			}
			// Remove the selected item
			// First unselect it so that selecting the new item works properly
			gtk_tree_selection_unselect_iter (selection, &iter);
			ghb_presets_remove(preset);
			ghb_presets_list_update(ud);
			ghb_select_preset(ud->builder, nextPreset);
		}
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
	GtkTreeIter iter;
	gchar *preset;
	ghb_title_info_t tinfo;
	GtkWidget *widget;
	
	g_debug("presets_list_selection_changed_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gtk_tree_model_get(store, &iter, 0, &preset, -1);
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
		ghb_set_preset(ud, preset);
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
	ghb_set_preset_default(ud->settings);
	ghb_presets_list_update(ud);
}

