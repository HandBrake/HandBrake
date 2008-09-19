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
	pdict = plist_get_dict(presetsPlist, name);
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

	dict = plist_get_dict(presetsPlist, name);
	return preset_dict_get_value(dict, key);
}

GList*
ghb_presets_get_names()
{
	GHashTable *dict;
	GList *names, *link;
	GList *standard = NULL;
	GList *custom = NULL;

	if (presetsPlist == NULL) return NULL;
	dict = g_value_get_boxed(presetsPlist);
	link = names = g_hash_table_get_keys(dict);
	while (link)
	{
		gchar *name;
		gint ptype;

		name = (gchar*)link->data;
		ptype = ghb_value_int(preset_get_value(name, "preset_type"));
		if (ptype)
			custom = g_list_append(custom, name);
		else
			standard = g_list_append(standard, name);
		link = link->next;
	}
	custom = g_list_sort(custom, key_cmp);
	standard = g_list_sort(standard, key_cmp);
	g_list_free(names);
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

	dict = plist_get_dict(presetsPlist, name);
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
	dict = plist_get_dict(presetsPlist, name);
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

static void
store_plist(GValue *plist, const gchar *name)
{
	const gchar *dir;
	gchar *config;
	FILE *file;

	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb", dir);
	if (!g_file_test(config, G_FILE_TEST_IS_DIR))
	{
		g_mkdir (config, 0755);
	}
	g_free(config);
	config = g_strdup_printf ("%s/ghb/%s", dir, name);
	file = g_fopen(config, "w");
	g_free(config);
	ghb_plist_write(file, plist);
	fclose(file);
}

static GValue*
load_plist(const gchar *name)
{
	const gchar *dir;
	gchar *config;
	GValue *plist = NULL;

	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb/%s", dir, name);
	if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
	{
		plist = ghb_plist_parse_file(config);
	}
	g_free(config);
	return plist;
}

static void
remove_plist(const gchar *name)
{
	const gchar *dir;
	gchar *config;

	dir = g_get_user_config_dir();
	config = g_strdup_printf ("%s/ghb/%s", dir, name);
	if (g_file_test(config, G_FILE_TEST_IS_REGULAR))
	{
		g_unlink(config);
	}
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
	//if (internalPlist)
		//ghb_value_free(internalPlist);
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
	GValue *std_dict, *dict;
	GHashTableIter std_iter;

	g_debug("ghb_presets_reload()\n");
	std_dict = ghb_resource_get("standard-presets");
	if (std_dict == NULL) return;

	// Merge the keyfile contents into our presets
	gchar *name;
	GValue *orig_dict;

	ghb_dict_iter_init(&std_iter, std_dict);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&std_iter, (gpointer*)(void*)&name, (gpointer*)(void*)&orig_dict))
	{
		GHashTableIter iter;
		gchar *key;
		GValue *value;

		dict = ghb_dict_value_new();
		ghb_dict_insert(presetsPlist, g_strdup(name), dict);
		ghb_dict_iter_init(&iter, orig_dict);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
		{
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(value));
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
	ghb_dict_insert(presetsPlist, g_strdup(name), dict);
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
	if (ghb_dict_lookup(presetsPlist, name))
	{
		ghb_dict_remove(presetsPlist, name);
		presets_store();
	}
}

