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

#define MAX_NESTED_PRESET 3

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
static void store_plist(GValue *plist, const gchar *name);

// This only handle limited depth
GtkTreePath*
ghb_tree_path_new_from_indices(gint *indices, gint len)
{
	switch (len)
	{
		case 1:
			return gtk_tree_path_new_from_indices(
				indices[0], -1);
		case 2:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], -1);
		case 3:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], -1);
		case 4:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], indices[3], -1);
		case 5:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], indices[3], indices[4], -1);
		default:
			return NULL;
	}
}

GValue*
ghb_parse_preset_path(const gchar *path)
{
	gchar **split;
	GValue *preset;
	gint ii;

	preset = ghb_array_value_new(MAX_NESTED_PRESET);
	split = g_strsplit(path, "#", MAX_NESTED_PRESET);
	for (ii = 0; split[ii] != NULL; ii++)
	{
		ghb_array_append(preset, ghb_string_value_new(split[ii]));
	}
	g_strfreev(split);
	return preset;
}

static GValue*
preset_path_from_indices(GValue *presets, gint *indices, gint len)
{
	gint ii;
	GValue *path;

	g_debug("preset_path_from_indices");
	path = ghb_array_value_new(MAX_NESTED_PRESET);
	for (ii = 0; ii < len; ii++)
	{
		GValue *dict;
		gint count, ptype;
		const GValue *name;

		count = ghb_array_len(presets);
		if (indices[ii] >= count) break;
		dict = ghb_array_get_nth(presets, indices[ii]);
		name = ghb_dict_lookup(dict, "preset_name");
		ghb_array_append(path, ghb_value_dup(name));
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (!(ptype & PRESETS_FOLDER))
			break;
		presets = ghb_dict_lookup(dict, "preset_folder");
	}
	return path;
}

gchar*
ghb_preset_path_string(const GValue *path)
{
	gint count, ii;
	GString *gstr;
	GValue *val;
	gchar *str;

	gstr = g_string_new("");
	if (path != NULL)
	{
		count = ghb_array_len(path);
		for (ii = 0; ii < count; ii++)
		{
			val = ghb_array_get_nth(path, ii);
			str = ghb_value_string(val);
			g_string_append(gstr, str);
			if (ii < count-1)
				g_string_append(gstr, "->");
			g_free(str);
		}
	}
	str = g_string_free(gstr, FALSE);
	return str;
}

static void
debug_show_type(GType tp)
{
	const gchar *str = "unknown";
	if (tp == G_TYPE_STRING)
	{
		str ="string";
	}
	else if (tp == G_TYPE_INT)
	{
		str ="int";
	}
	else if (tp == G_TYPE_INT64)
	{
		str ="int64";
	}
	else if (tp == G_TYPE_BOOLEAN)
	{
		str ="bool";
	}
	else if (tp == ghb_array_get_type())
	{
		str ="array";
	}
	else if (tp == ghb_dict_get_type())
	{
		str ="dict";
	}
	g_message("Type: %s", str);
}

void
dump_preset_path(const gchar *msg, const GValue *path)
{
	gchar *str;

	if (path)
		debug_show_type (G_VALUE_TYPE(path));
	str = ghb_preset_path_string(path);
	g_message("%s path: (%s)", msg, str);
	g_free(str);
}

void
dump_preset_indices(const gchar *msg, gint *indices, gint len)
{
	gint ii;

	g_message("%s indices: len %d", msg, len);
	for (ii = 0; ii < len; ii++)
	{
		printf("%d ", indices[ii]);
	}
	printf("\n");
}

#if 0
static gint
preset_path_cmp(const GValue *path1, const GValue *path2)
{
	gint count, ii;
	GValue *val;
	gchar *str1, *str2;
	gint result;

	count = ghb_array_len(path1);
	ii = ghb_array_len(path2);
	if (ii != count)
		return ii - count;
	for (ii = 0; ii < count; ii++)
	{
		val = ghb_array_get_nth(path1, ii);
		str1 = ghb_value_string(val);
		val = ghb_array_get_nth(path2, ii);
		str2 = ghb_value_string(val);
		result = strcmp(str1, str2);
		if (result != 0)
			return result;
		g_free(str1);
		g_free(str2);
	}
	return 0;
}
#endif

static GValue*
presets_get_dict(GValue *presets, gint *indices, gint len)
{
	gint ii, count, ptype;
	GValue *dict = NULL;

	g_debug("presets_get_dict ()");
	for (ii = 0; ii < len; ii++)
	{
		count = ghb_array_len(presets);
		if (indices[ii] >= count) return NULL;
		dict = ghb_array_get_nth(presets, indices[ii]);
		if (ii < len-1)
		{
			ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
			if (!(ptype & PRESETS_FOLDER))
				return NULL;
			presets = ghb_dict_lookup(dict, "preset_folder");
		}
	}
	if (ii < len)
		return NULL;
	return dict;
}

static GValue*
presets_get_folder(GValue *presets, gint *indices, gint len)
{
	gint ii, count, ptype;
	GValue *dict;

	g_debug("presets_get_folder ()");
	for (ii = 0; ii < len; ii++)
	{
		count = ghb_array_len(presets);
		if (indices[ii] >= count) return NULL;
		dict = ghb_array_get_nth(presets, indices[ii]);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (!(ptype & PRESETS_FOLDER))
			break;
		presets = ghb_dict_lookup(dict, "preset_folder");
	}
	if (ii < len)
		return NULL;
	return presets;
}

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
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		presets_remove_nth(folder, indices[len-1]);
	else
	{
		g_warning("ghb_presets_remove ()");
		g_warning("internal preset lookup error");
		return FALSE;
	}
	return TRUE;
}

static void
ghb_presets_replace(
	GValue *presets, 
	GValue *dict,
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		ghb_array_replace(folder, indices[len-1], dict);
	else
	{
		g_warning("ghb_presets_replace ()");
		g_warning("internal preset lookup error");
	}
}

static void
ghb_presets_insert(
	GValue *presets, 
	GValue *dict,
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		ghb_array_insert(folder, indices[len-1], dict);
	else
	{
		g_warning("ghb_presets_insert ()");
		g_warning("internal preset lookup error");
	}
}

static gint
presets_find_element(GValue *presets, const gchar *name)
{
	GValue *dict;
	gint count, ii;
	
	g_debug("presets_find_element () (%s)", name);
	if (presets == NULL || name == NULL) return -1;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		if (strcmp(name, str) == 0)
		{
			return ii;
		}
	}
	return -1;
}

static gint
single_find_pos(GValue *presets, const gchar *name, gint type)
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

static gint*
presets_find_pos(const GValue *path, gint type, gint *len)
{
	GValue *nested;
	GValue *val;
	gint count, ii, ptype;
	gint *indices = NULL;
	const gchar *name;
	GValue *dict;

	g_debug("presets_find_pos () ");
	nested = presetsPlist;
	count = ghb_array_len(path);
	indices = g_malloc(MAX_NESTED_PRESET * sizeof(gint));
	for (ii = 0; ii < count-1; ii++)
	{
		val = ghb_array_get_nth(path, ii);
		name = g_value_get_string(val);
		indices[ii] = presets_find_element(nested, name);
		if (indices[ii] == -1) return NULL;
		dict = ghb_array_get_nth(nested, indices[ii]);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		nested = NULL;
		if (!(ptype & PRESETS_FOLDER))
			break;
		nested = ghb_dict_lookup(dict, "preset_folder");
	}
	if (nested)
	{
		const gchar *name;

		name = g_value_get_string(ghb_array_get_nth(path, count-1));
		indices[ii] = single_find_pos(nested, name, type);
		ii++;
	}
	*len = ii;
	return indices;
}

static gint
preset_tree_depth(GValue *dict)
{
	gint ptype;

	ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
	if (ptype & PRESETS_FOLDER)
	{
		gint depth = 0;
		gint count, ii;
		GValue *presets;

		presets = ghb_dict_lookup(dict, "preset_folder");
		count = ghb_array_len(presets);
		for (ii = 0; ii < count; ii++)
		{
			gint tmp;

			dict = ghb_array_get_nth(presets, ii);
			tmp = preset_tree_depth(dict);
			depth = MAX(depth, tmp);
		}
		return depth + 1;
	}
	else
	{
		return 1;
	}
}

static gboolean
preset_is_default(GValue *dict)
{
	const GValue *val;

	val = preset_dict_get_value(dict, "Default");
	return g_value_get_boolean(val);
}

static gint*
presets_find_default2(GValue *presets, gint *len)
{
	gint count, ii;
	gint *indices;

	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		GValue *dict;
		gint ptype;

		dict = ghb_array_get_nth(presets, ii);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (ptype & PRESETS_FOLDER)
		{
			GValue *nested;
			gint pos = *len;

			nested = ghb_dict_lookup(dict, "preset_folder");
			(*len)++;
			indices = presets_find_default2(nested, len);
			if (indices)
			{
				indices[pos] = ii;
				return indices;
			}
			else
				*len = pos;
		}
		else
		{
			if (preset_is_default(dict))
			{
				indices = malloc(MAX_NESTED_PRESET * sizeof(gint));
				indices[*len] = ii;
				(*len)++;
				return indices;
			}
		}
	}
	return NULL;
}

static gint*
presets_find_default(gint *len)
{
	*len = 0;
	return presets_find_default2(presetsPlist, len);
}

gint*
ghb_preset_indices_from_path(
	GValue *presets, 
	const GValue *path,
	gint *len)
{
	GValue *nested;
	GValue *val;
	gint count, ii, ptype;
	gint *indices = NULL;
	const gchar *name;
	GValue *dict;

	g_debug("ghb_preset_indices_from_path () ");
	nested = presets;
	count = ghb_array_len(path);
	if (count)
		indices = g_malloc(MAX_NESTED_PRESET * sizeof(gint));
	*len = 0;
	for (ii = 0; ii < count; ii++)
	{
		val = ghb_array_get_nth(path, ii);
		name = g_value_get_string(val);
		indices[ii] = presets_find_element(nested, name);
		if (indices[ii] == -1)
		{
			g_free(indices);
			return NULL;
		}
		if (ii < count-1)
		{
			dict = ghb_array_get_nth(nested, indices[ii]);
			ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
			if (!(ptype & PRESETS_FOLDER))
			{
				g_free(indices);
				return NULL;
			}
			nested = ghb_dict_lookup(dict, "preset_folder");
		}
	}
	*len = ii;
	return indices;
}

static gint
ghb_presets_get_type(
	GValue *presets, 
	gint *indices,
	gint len)
{
	GValue *dict;
	gint flags = 0;

	dict = presets_get_dict(presets, indices, len);
	if (dict)
	{
		flags = ghb_preset_flags(dict);
	}
	else
	{
		g_warning("ghb_presets_get_type ()");
		g_warning("internal preset lookup error");
	}
	return flags;
}

void
presets_set_default(gint *indices, gint len)
{
	GValue *dict;
	gint *curr_indices, curr_len;
	
	g_debug("presets_set_default ()");
	curr_indices = presets_find_default(&curr_len);
	if (curr_indices)
	{
		dict = presets_get_dict(presetsPlist, curr_indices, curr_len);
		if (dict)
		{
			ghb_dict_insert(dict, g_strdup("Default"), 
							ghb_boolean_value_new(FALSE));
		}
	}
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict)
	{
		ghb_dict_insert(dict, g_strdup("Default"), ghb_boolean_value_new(TRUE));
	}
	store_plist(presetsPlist, "presets");
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
	const gchar *desc;

	if (pdict == NULL) return NULL;
	desc = g_value_get_string(
			preset_dict_get_value(pdict, "preset_description"));
	if (desc[0] == 0) return NULL;
	return desc;
}


static const GValue*
preset_get_value(const GValue *preset, const gchar *key)
{
	GValue *dict = NULL;
	gint *indices, len;


	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	if (indices)
	{
		dict = presets_get_dict(presetsPlist, indices, len);
		g_free(indices);
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
ghb_set_preset_from_indices(signal_user_data_t *ud, gint *indices, gint len)
{
	GValue *dict = NULL;
	gint fallback[2] = {0, -1};

	if (indices)
		dict = presets_get_dict(presetsPlist, indices, len);
	if (dict == NULL)
	{
		indices = fallback;
		len = 1;
		dict = presets_get_dict(presetsPlist, indices, len);
	}
	if (dict == NULL)
	{
		preset_to_ui(ud, NULL);
	}
	else
	{
		gint ptype;
		GValue *path;

		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (ptype & PRESETS_FOLDER)
			preset_to_ui(ud, NULL);
		else
			preset_to_ui(ud, dict);
		path = preset_path_from_indices(presetsPlist, indices, len);
		ghb_settings_set_value(ud->settings, "preset", path);
		ghb_value_free(path);
	}
}

void
ghb_set_preset(signal_user_data_t *ud, const GValue *path)
{
	gint *indices, len;
	
	g_debug("ghb_set_preset()");
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	ghb_set_preset_from_indices(ud, indices, len);
	if (indices) g_free(indices);
}

void
ghb_update_from_preset(
	signal_user_data_t *ud, 
	const GValue *preset, 
	const gchar *key)
{
	const GValue *gval;
	
	g_debug("ghb_update_from_preset() %s", key);
	if (preset == NULL) return;
	gval = preset_get_value(preset, key);
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
	

	g_debug("ghb_prefs_to_ui");
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
	GValue *gval, *path;
	
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
	// Read legacy default_preset preference and update accordingly
	path = ghb_dict_lookup(dict, "default_preset");
	if (path)
	{
		gint *indices, len;

		if (G_VALUE_TYPE(path) == G_TYPE_STRING)
		{
			GValue *str = path;

			path = ghb_array_value_new(1);
			ghb_array_append(path, ghb_value_dup(str));
			indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
			ghb_value_free(path);
		}
		else
			indices = ghb_preset_indices_from_path(presetsPlist, path, &len);

		if (indices)
		{
			presets_set_default(indices, len);
			g_free(indices);
		}
		ghb_dict_remove(dict, "default_preset");
		store_plist(prefsPlist, "preferences");
	}
}

static const gchar*
get_preset_color(gint flags)
{
	const gchar *color;

	if (flags & PRESETS_CUST)
	{
		color = "DimGray";
		if (flags & PRESETS_FOLDER)
		{
			color = "black";
		}
	}
	else
	{
		color = "blue";
		if (flags & PRESETS_FOLDER)
		{
			color = "Navy";
		}
	}
	return color;
}

void
ghb_presets_list_init(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, titer, *piter;
	
	GtkTreeStore *store;
	const gchar *preset;
	GtkTreePath *parent_path;
	const gchar *description;
	gint flags;
	gboolean def;
	gint count, ii;
	GValue *dict;
	gint *more_indices;
	GValue *presets = NULL;
	
	g_debug("ghb_presets_list_init ()");
	more_indices = g_malloc((len+1)*sizeof(gint));
	memcpy(more_indices, indices, len*sizeof(gint));
	presets = presets_get_folder(presetsPlist, indices, len);
	if (presets == NULL)
	{
		g_warning("Failed to find parent folder when adding child.");
		return;
	}
	count = ghb_array_len(presets);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	parent_path = ghb_tree_path_new_from_indices(indices, len);
	if (parent_path)
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &titer, parent_path);
		piter = &titer;
		gtk_tree_path_free(parent_path);
	}
	else
	{
		piter = NULL;
	}
	for (ii = 0; ii < count; ii++)
	{
		const gchar *color;

		// Additional settings, add row
		dict = ghb_array_get_nth(presets, ii);
		preset = preset_get_name(dict);
		more_indices[len] = ii;
		def = preset_is_default(dict);

		description = ghb_presets_get_description(dict);
		gtk_tree_store_append(store, &iter, piter);
		flags = ghb_preset_flags(dict);
		color = get_preset_color(flags);
		gtk_tree_store_set(store, &iter, 0, preset, 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, color, 
							4, description,
						   	-1);
		if (def && piter)
		{
			GtkTreePath *path;
			GtkTreeIter ppiter;

			if (gtk_tree_model_iter_parent(
				GTK_TREE_MODEL(store), &ppiter, piter))
			{
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &ppiter);
				gtk_tree_view_expand_row(treeview, path, FALSE);
				gtk_tree_path_free(path);
			}
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), piter);
			gtk_tree_view_expand_row(treeview, path, FALSE);
			gtk_tree_path_free(path);
		}
		if (flags & PRESETS_FOLDER)
		{
			ghb_presets_list_init(ud, more_indices, len+1);
		}
	}
	g_free(more_indices);
}

static void
presets_list_update_item(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *treepath;
	const gchar *name;
	const gchar *description;
	gint flags;
	gboolean def;
	GValue *dict;
	const gchar *color;
	
	g_debug("presets_list_update_item ()");
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict == NULL)
		return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	treepath = ghb_tree_path_new_from_indices(indices, len);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
	// Additional settings, add row
	name = preset_get_name(dict);
	def = preset_is_default(dict);

	description = ghb_presets_get_description(dict);
	flags = ghb_preset_flags(dict);
	color = get_preset_color(flags);
	gtk_tree_store_set(store, &iter, 0, name, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, color,
						4, description,
					   	-1);
	if (flags & PRESETS_FOLDER)
	{
		ghb_presets_list_init(ud, indices, len);
	}
}

static void
presets_list_insert(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, titer, *piter;
	GtkTreeStore *store;
	const gchar *preset;
	const gchar *description;
	gint flags;
	gboolean def;
	gint count;
	GValue *presets;
	GtkTreePath *parent_path;
	GValue *dict;
	const gchar *color;
	
	g_debug("presets_list_insert ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	presets = presets_get_folder(presetsPlist, indices, len-1);
	if (presets == NULL)
	{
		g_warning("Failed to find parent folder while adding child.");
		return;
	}
	parent_path = ghb_tree_path_new_from_indices(indices, len-1);
	if (parent_path)
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &titer, parent_path);
		piter = &titer;
		gtk_tree_path_free(parent_path);
	}
	else
	{
		piter = NULL;
	}
	count = ghb_array_len(presets);
	if (indices[len-1] >= count)
		return;
	// Additional settings, add row
	dict = ghb_array_get_nth(presets, indices[len-1]);
	preset = preset_get_name(dict);
	def = preset_is_default(dict);

	description = ghb_presets_get_description(dict);
	gtk_tree_store_insert(store, &iter, piter, indices[len-1]);
	flags = ghb_preset_flags(dict);
	color = get_preset_color(flags);
	gtk_tree_store_set(store, &iter, 0, preset, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, color,
						4, description,
					   	-1);
	if (flags & PRESETS_FOLDER)
	{
		ghb_presets_list_init(ud, indices, len);
	}
}

static void
presets_list_remove(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	
	g_debug("presets_list_remove ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
			gtk_tree_store_remove(store, &iter);
		gtk_tree_path_free(treepath);
	}
}

static void
remove_std_presets(signal_user_data_t *ud)
{
	gint count, ii;
	gint indices = 0;

	count = ghb_array_len(presetsPlist);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *dict;
		gint ptype;

		dict = ghb_array_get_nth(presetsPlist, ii);
		ptype = ghb_value_int(preset_dict_get_value(dict, "preset_type"));
		if (!(ptype & PRESETS_CUST))
		{
			if (ghb_presets_remove(presetsPlist, &indices, 1))
			{
				presets_list_remove(ud, &indices, 1);
			}
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

	remove_std_presets(ud);
	// Merge the keyfile contents into our presets
	count = ghb_array_len(std_presets);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *std_dict;
		GValue *copy_dict;
		GHashTableIter iter;
		gchar *key;
		GValue *value;
		gint indices = 0;

		std_dict = ghb_array_get_nth(std_presets, ii);
		copy_dict = ghb_dict_value_new();
		ghb_presets_insert(presetsPlist, copy_dict, &indices, 1);
		ghb_dict_iter_init(&iter, std_dict);
		// middle (void*) cast prevents gcc warning "defreferencing 
		// type-punned pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
		{
			ghb_dict_insert(copy_dict, g_strdup(key), ghb_value_dup(value));
		}
		presets_list_insert(ud, &indices, 1);
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
settings_save(signal_user_data_t *ud, const GValue *path)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *value;
	gboolean autoscale;
	gint *indices, len, count;
	const gchar *name;
	gboolean replace = FALSE;

	g_debug("settings_save");
	if (internalPlist == NULL) return;
	count = ghb_array_len(path);
	name = g_value_get_string(ghb_array_get_nth(path, count-1));
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		if (ghb_presets_get_type(presetsPlist, indices, len) & 
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
		dict = ghb_dict_value_new();
		ghb_presets_replace(presetsPlist, dict, indices, len);
		replace = TRUE;
	}
	else
	{
		indices = presets_find_pos(path, PRESETS_CUST, &len);
		if (indices)
		{
			dict = ghb_dict_value_new();
			ghb_presets_insert(presetsPlist, dict, indices, len);
		}
		else
		{
			g_warning("failed to find insert path");
			return;
		}
	}
	ghb_dict_insert(dict, g_strdup("preset_name"), ghb_string_value_new(name));

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
	if (replace)
		presets_list_update_item(ud, indices, len);
	else
	{
		ghb_dict_insert(dict, g_strdup("Default"), 
						ghb_boolean_value_new(FALSE));
		presets_list_insert(ud, indices, len);
	}
	g_free(indices);
	store_plist(presetsPlist, "presets");
	ud->dont_clear_presets = TRUE;
	ghb_set_preset(ud, path);
	ud->dont_clear_presets = FALSE;
	return;
}

static void
folder_save(signal_user_data_t *ud, const GValue *path)
{
	GValue *dict, *folder;
	gint *indices, len, count;
	const gchar *name;

	count = ghb_array_len(path);
	name = g_value_get_string(ghb_array_get_nth(path, count-1));
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		if (!(ghb_presets_get_type(presetsPlist, indices, len) & 
			PRESETS_FOLDER))
		{
			gchar *message;
			message = g_strdup_printf(
						"%s: Preset already exists.\n"
						"You can not replace it with a folder.",
						name);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			g_free(indices);
			return;
		}
		// Already exists, update its description
		dict = presets_get_dict(presetsPlist, indices, len);
		ghb_dict_insert(dict, g_strdup("preset_description"), 
			ghb_value_dup(preset_dict_get_value(
				ud->settings, "preset_description")));
		g_free(indices);
		return;
	}
	else
	{
		indices = presets_find_pos(path, PRESETS_CUST, &len);
		if (indices)
		{
			dict = ghb_dict_value_new();
			ghb_presets_insert(presetsPlist, dict, indices, len);
		}
		else
		{
			g_warning("failed to find insert path");
			return;
		}
	}
	ghb_dict_insert(dict, g_strdup("preset_description"), 
		ghb_value_dup(preset_dict_get_value(
			ud->settings, "preset_description")));
	ghb_dict_insert(dict, g_strdup("preset_name"), ghb_string_value_new(name));
	folder = ghb_array_value_new(8);
	ghb_dict_insert(dict, g_strdup("preset_folder"), folder);
	ghb_dict_insert(dict, g_strdup("preset_type"),
							ghb_int64_value_new(PRESETS_FOLDER|PRESETS_CUST));

	presets_list_insert(ud, indices, len);
	g_free(indices);
	store_plist(presetsPlist, "presets");
	ud->dont_clear_presets = TRUE;
	ghb_set_preset(ud, path);
	ud->dont_clear_presets = FALSE;
	return;
}

void
ghb_presets_list_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gint *indices, len;
	
	g_debug("ghb_presets_list_default ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	indices = presets_find_default(&len);
	if (indices == NULL) return;
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
		{
			gtk_tree_store_set(store, &iter, 
						1, 800, 
						2, 2 ,
						-1);
		}
		gtk_tree_path_free(treepath);
	}
	g_free(indices);
}

void
ghb_presets_list_clear_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gint *indices, len;
	
	g_debug("ghb_presets_list_clear_default ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	indices = presets_find_default(&len);
	if (indices == NULL) return;
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
		{
			gtk_tree_store_set(store, &iter, 
						1, 400, 
						2, 0 ,
						-1);
		}
		gtk_tree_path_free(treepath);
	}
	g_free(indices);
}

static void
ghb_select_preset2(
	GtkBuilder *builder, 
	gint *indices, 
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	
	g_debug("ghb_select_preset2()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = gtk_tree_view_get_model (treeview);
	path = ghb_tree_path_new_from_indices(indices, len);
	if (path)
	{
		if (gtk_tree_model_get_iter(store, &iter, path))
		{
			gtk_tree_selection_select_iter (selection, &iter);
		}
		else
		{
			if (gtk_tree_model_get_iter_first(store, &iter))
				gtk_tree_selection_select_iter (selection, &iter);
		}
		gtk_tree_path_free(path);
	}
}

void
ghb_select_preset(GtkBuilder *builder, const GValue *path)
{
	gint *indices, len;

	g_debug("ghb_select_preset()");
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		ghb_select_preset2(builder, indices, len);
		g_free(indices);
	}
}

void
ghb_select_default_preset(GtkBuilder *builder)
{
	gint *indices, len;

	g_debug("ghb_select_default_preset()");
	indices = presets_find_default(&len);
	if (indices)
	{
		ghb_select_preset2(builder, indices, len);
		g_free(indices);
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
enforce_preset_type(signal_user_data_t *ud, const GValue *path)
{
	gint *indices, len;
	GtkWidget *normal, *folder;
	gint ptype;

	normal = GHB_WIDGET(ud->builder, "preset_type_normal");
	folder = GHB_WIDGET(ud->builder, "preset_type_folder");
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		ptype = ghb_presets_get_type(presetsPlist, indices, len);
		if (ptype & PRESETS_FOLDER)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(folder), 
									TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(normal), 
									TRUE);
		gtk_widget_set_sensitive(folder,  ptype & PRESETS_FOLDER);
		gtk_widget_set_sensitive(normal,  !(ptype & PRESETS_FOLDER));
		g_free(indices);
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
	GValue *preset;
	const gchar *name = "";
	gint count, *indices, len;

	g_debug("presets_save_clicked_cb ()");
	preset = ghb_settings_get_value (ud->settings, "preset_selection");

	count = ghb_array_len(preset);
	if (count > 0)
		name = g_value_get_string(ghb_array_get_nth(preset, count-1));
	else
		count = 1;
	// Clear the description
	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "preset_description"));
	dialog = GHB_WIDGET(ud->builder, "preset_save_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "preset_name"));
	gtk_entry_set_text(entry, name);
	enforce_preset_type(ud, preset);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		GValue *dest;

		if (ghb_settings_get_boolean(ud->settings, "preset_type_folder"))
		{
			if (count > MAX_NESTED_PRESET-1)
			{
				count = MAX_NESTED_PRESET-1;
			}
		}
		dest = ghb_array_value_new(MAX_NESTED_PRESET);
		indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
		if (indices)
		{
			gint ptype;

			ptype = ghb_presets_get_type(presetsPlist, indices, len);
			if (ptype & PRESETS_CUST)
			{
				ghb_array_copy(dest, preset, count-1);
			}
		}
		ghb_array_append(dest, ghb_string_value_new(name));

		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
		if (ghb_settings_get_boolean(ud->settings, "preset_type_folder"))
		{
			folder_save(ud, dest);
		}
		else
		{
			// Construct the audio settings presets from the current audio list
			update_audio_presets(ud);
			settings_save(ud, dest);
		}
		// Make the new preset the selected item
		ghb_select_preset(ud->builder, dest);
		ghb_value_free(dest);
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
	GValue *preset, *dest;
	gint count;

	preset = ghb_settings_get_value (ud->settings, "preset_selection");
	name = ghb_widget_string(entry);
	dest = ghb_value_dup(preset);
	count = ghb_array_len(dest);
	ghb_array_replace(dest, count-1, ghb_string_value_new(name));
	enforce_preset_type(ud, dest);
	ghb_value_free(dest);
}

void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GValue *preset;

	g_debug("presets_restore_clicked_cb ()");
	// Reload only the standard presets
	ghb_presets_reload(ud);
	// Updating the presets list shuffles things around
	// need to make sure the proper preset is selected
	preset = ghb_settings_get_value (ud->settings, "preset");
	ghb_select_preset(ud->builder, preset);
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
		gint *indices, len;
		gint ptype;

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		path = gtk_tree_model_get_path(store, &iter);
		indices = gtk_tree_path_get_indices(path);
		len = gtk_tree_path_get_depth(path);

		ptype = ghb_presets_get_type(presetsPlist, indices, len);
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
			if (ghb_presets_remove(presetsPlist, indices, len))
			{
				store_plist(presetsPlist, "presets");
				presets_list_remove(ud, indices, len);
			}
			if (!valid)
				valid = gtk_tree_model_get_iter_first(store, &nextIter);
			if (valid)
			{
				gtk_tree_path_free(path);
				path = gtk_tree_model_get_path(store, &nextIter);
				indices = gtk_tree_path_get_indices(path);
				len = gtk_tree_path_get_depth(path);
				ghb_select_preset2(ud->builder, indices, len);
			}
		}
		g_free(preset);
		gtk_tree_path_free(path);
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
	gint *indices, len;
	GtkTreeIter iter;
	GtkTreeView *srctv;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	gint src_ptype, dst_ptype;
	GValue *preset;
	gint tree_depth, ii;

	// Get the type of the object being dragged
	srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
	select = gtk_tree_view_get_selection (srctv);
	gtk_tree_selection_get_selected (select, &model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices(path);
	len = gtk_tree_path_get_depth(path);

	preset = presets_get_dict(presetsPlist, indices, len);
	tree_depth = preset_tree_depth(preset);

	src_ptype = ghb_presets_get_type(presetsPlist, indices, len);
	gtk_tree_path_free(path);

	if ((src_ptype & PRESETS_FOLDER) && tree_depth == 1)
		tree_depth = 2;

	// The rest checks that the destination is a valid position
	// in the list.
	gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &drop_pos);
	if (path == NULL)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	// Don't allow repositioning of builtin presets
	if (!(src_ptype & PRESETS_CUST))
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	len = gtk_tree_path_get_depth(path);
	if (len+tree_depth-1 >= MAX_NESTED_PRESET)
	{
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	}
	for (ii = len+tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
		gtk_tree_path_up(path);
	indices = gtk_tree_path_get_indices(path);
	len = gtk_tree_path_get_depth(path);
	dst_ptype = ghb_presets_get_type(presetsPlist, indices, len);
	// Don't allow mixing custom presets in the builtins
	if (!(dst_ptype & PRESETS_CUST))
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	// Only allow *drop into* for folders
	if (!(dst_ptype & PRESETS_FOLDER))
	{ 
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	}

	len = gtk_tree_path_get_depth(path);
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
	gint *dst_indices, dst_len, *src_indices, src_len;
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
		GValue *preset;
		gint tree_depth, ii;

		srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
		select = gtk_tree_view_get_selection (srcwidget);
		gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

		srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
		src_indices = gtk_tree_path_get_indices(srcpath);
		src_len = gtk_tree_path_get_depth(srcpath);
		src_ptype = ghb_presets_get_type(presetsPlist, src_indices, src_len);
		preset = ghb_value_dup(
					presets_get_dict(presetsPlist, src_indices, src_len));
		gtk_tree_path_free(srcpath);

		// Don't allow repositioning of builtin presets
		if (!(src_ptype & PRESETS_CUST))
			return;

		tree_depth = preset_tree_depth(preset);
		if ((src_ptype & PRESETS_FOLDER) && tree_depth == 1)
			tree_depth = 2;

		dst_len = gtk_tree_path_get_depth(path);
		if (dst_len+tree_depth-1 >= MAX_NESTED_PRESET)
		{
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
				drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
				drop_pos = GTK_TREE_VIEW_DROP_AFTER;
		}

		for (ii = dst_len+tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
			gtk_tree_path_up(path);
		dst_indices = gtk_tree_path_get_indices(path);
		dst_len = gtk_tree_path_get_depth(path);
		dst_ptype = ghb_presets_get_type(presetsPlist, dst_indices, dst_len);
		// Only allow *drop into* for folders
		if (!(dst_ptype & PRESETS_FOLDER))
		{ 
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
				drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
				drop_pos = GTK_TREE_VIEW_DROP_AFTER;
		}
		if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
		{
			GtkTreeIter iter;
			GtkTreePath *dstpath = NULL;

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

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			dst_indices = gtk_tree_path_get_indices(dstpath);
			dst_len = gtk_tree_path_get_depth(dstpath);
			ghb_presets_insert(presetsPlist, preset, dst_indices, dst_len);
			gtk_tree_path_free(dstpath);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			src_indices = gtk_tree_path_get_indices(srcpath);
			src_len = gtk_tree_path_get_depth(srcpath);
			ghb_presets_remove(presetsPlist, src_indices, src_len);
			gtk_tree_path_free(srcpath);

			gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			dst_indices = gtk_tree_path_get_indices(dstpath);
			dst_len = gtk_tree_path_get_depth(dstpath);
			presets_list_update_item(ud, dst_indices, dst_len);
			gtk_tree_path_free(dstpath);

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
	else
	{
		ghb_ui_update(ud, "crop_top", ghb_int64_value(0));
		ghb_ui_update(ud, "crop_bottom", ghb_int64_value(0));
		ghb_ui_update(ud, "crop_left", ghb_int64_value(0));
		ghb_ui_update(ud, "crop_right", ghb_int64_value(0));
	}
}

void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	ghb_title_info_t tinfo;
	GtkWidget *widget;
	
	g_debug("presets_list_selection_changed_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *treepath;
		gint *indices, len, ptype;
		GValue *path;

		treepath = gtk_tree_model_get_path(store, &iter);
		indices = gtk_tree_path_get_indices(treepath);
		len = gtk_tree_path_get_depth(treepath);

		path = preset_path_from_indices(presetsPlist, indices, len);
		ghb_settings_take_value(ud->settings, "preset_selection", path);

		ptype = ghb_presets_get_type(presetsPlist, indices, len);
		if (!(ptype & PRESETS_FOLDER))
		{
			ud->dont_clear_presets = TRUE;
			// Temporarily set the video_quality range to (0,100)
			// This is needed so the video_quality value does not get
			// truncated when set.  The range will be readjusted below
			GtkWidget *qp = GHB_WIDGET(ud->builder, "video_quality");
			gtk_range_set_range (GTK_RANGE(qp), 0, 100);
			// Clear the audio list prior to changing the preset.  Existing 
			// audio can cause the container extension to be automatically 
			// changed when it shouldn't be
			ghb_clear_audio_list(ud);
			ghb_set_preset_from_indices(ud, indices, len);
			gtk_tree_path_free(treepath);
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
	GValue *preset;
	gint *indices, len;

	g_debug("presets_default_clicked_cb ()");
	preset = ghb_settings_get_value(ud->settings, "preset_selection");
	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	if (indices)
	{
		if (!(ghb_presets_get_type(presetsPlist, indices, len) & 
			PRESETS_FOLDER))
		{
			ghb_presets_list_clear_default(ud);
			presets_set_default(indices, len);
			ghb_presets_list_default(ud);
		}
		g_free(indices);
	}
}

