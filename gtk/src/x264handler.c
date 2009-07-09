/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * x264handler.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * x264handler.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <gtk/gtk.h>
#include <string.h>
#include "settings.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "hb-backend.h"
#include "x264handler.h"

static void x264_opt_update(signal_user_data_t *ud, GtkWidget *widget);
static gchar* sanitize_x264opts(signal_user_data_t *ud, const gchar *options);

// Flag needed to prevent x264 options processing from chasing its tail
static gboolean ignore_options_update = FALSE;

G_MODULE_EXPORT void
x264_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
	if (!ignore_options_update)
	{
		ignore_options_update = TRUE;
		x264_opt_update(ud, widget);
		ignore_options_update = FALSE;
	}
	ghb_check_dependency(ud, widget);
	ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
x264_me_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gint me;

	ghb_widget_to_setting(ud->settings, widget);
	if (!ignore_options_update)
	{
		ignore_options_update = TRUE;
		x264_opt_update(ud, widget);
		ignore_options_update = FALSE;
	}
	ghb_check_dependency(ud, widget);
	ghb_clear_presets_selection(ud);
	widget = GHB_WIDGET(ud->builder, "x264_merange");
	me = ghb_settings_combo_int(ud->settings, "x264_me");
	if (me < 2)
	{	// me < umh
		// me_range 4 - 16
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), 4, 16);
	}
	else
	{
		// me_range 4 - 64
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), 4, 64);
	}
}

G_MODULE_EXPORT void
x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("x264_entry_changed_cb ()");
	if (!ignore_options_update)
	{
		GtkWidget *textview;
		gchar *options;

		textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "x264Option"));
		ghb_widget_to_setting(ud->settings, textview);
		options = ghb_settings_get_string(ud->settings, "x264Option");
		ignore_options_update = TRUE;
		ghb_x264_parse_options(ud, options);
		if (!GTK_WIDGET_HAS_FOCUS(textview))
		{
			gchar *sopts;

			sopts = sanitize_x264opts(ud, options);
			ghb_ui_update(ud, "x264Option", ghb_string_value(sopts));
			ghb_x264_parse_options(ud, sopts);
			g_free(sopts);
		}
		g_free(options);
		ignore_options_update = FALSE;
	}
}

G_MODULE_EXPORT gboolean
x264_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
	signal_user_data_t *ud)
{
	gchar *options, *sopts;

	ghb_widget_to_setting(ud->settings, widget);
	options = ghb_settings_get_string(ud->settings, "x264Option");
	sopts = sanitize_x264opts(ud, options);
	ignore_options_update = TRUE;
	if (sopts != NULL && strcmp(sopts, options) != 0)
	{
		ghb_ui_update(ud, "x264Option", ghb_string_value(sopts));
		ghb_x264_parse_options(ud, sopts);
	}
	g_free(options);
	g_free(sopts);
	ignore_options_update = FALSE;
	return FALSE;
}

enum
{
	X264_OPT_DEBLOCK,
	X264_OPT_PSY,
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
static gchar *x264_mixed_syns[] = {"mixed-refs", "mixed_refs", NULL};
static gchar *x264_bframes_syns[] = {"bframes", NULL};
static gchar *x264_badapt_syns[] = {"b-adapt", "b_adapt", NULL};
static gchar *x264_direct_syns[] = 
	{"direct", "direct-pred", "direct_pred", NULL};
static gchar *x264_weightb_syns[] = {"weightb", "weight-b", "weight_b", NULL};
static gchar *x264_bpyramid_syns[] = {"b-pyramid", "b_pyramid", NULL};
static gchar *x264_me_syns[] = {"me", NULL};
static gchar *x264_merange_syns[] = {"merange", "me-range", "me_range", NULL};
static gchar *x264_subme_syns[] = {"subme", "subq", NULL};
static gchar *x264_analyse_syns[] = {"analyse", "partitions", NULL};
static gchar *x264_8x8dct_syns[] = {"8x8dct", NULL};
static gchar *x264_deblock_syns[] = {"deblock", "filter", NULL};
static gchar *x264_trellis_syns[] = {"trellis", NULL};
static gchar *x264_pskip_syns[] = {"no-fast-pskip", "no_fast_pskip", NULL};
static gchar *x264_psy_syns[] = {"psy-rd", "psy_rd", NULL};
static gchar *x264_decimate_syns[] = 
	{"no-dct-decimate", "no_dct_decimate", NULL};
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
	{x264_ref_syns, "x264_refs", "3", X264_OPT_INT},
	{x264_mixed_syns, "x264_mixed_refs", "1", X264_OPT_BOOL},
	{x264_bframes_syns, "x264_bframes", "3", X264_OPT_INT},
	{x264_direct_syns, "x264_direct", "spatial", X264_OPT_COMBO},
	{x264_badapt_syns, "x264_b_adapt", "1", X264_OPT_COMBO},
	{x264_weightb_syns, "x264_weighted_bframes", "1", X264_OPT_BOOL},
	{x264_bpyramid_syns, "x264_bpyramid", "0", X264_OPT_BOOL},
	{x264_me_syns, "x264_me", "hex", X264_OPT_COMBO},
	{x264_merange_syns, "x264_merange", "16", X264_OPT_INT},
	{x264_subme_syns, "x264_subme", "7", X264_OPT_COMBO},
	{x264_analyse_syns, "x264_analyse", "some", X264_OPT_COMBO},
	{x264_8x8dct_syns, "x264_8x8dct", "1", X264_OPT_BOOL},
	{x264_deblock_syns, "x264_deblock_alpha", "0,0", X264_OPT_DEBLOCK},
	{x264_deblock_syns, "x264_deblock_beta", "0,0", X264_OPT_DEBLOCK},
	{x264_trellis_syns, "x264_trellis", "1", X264_OPT_COMBO},
	{x264_pskip_syns, "x264_no_fast_pskip", "0", X264_OPT_BOOL},
	{x264_decimate_syns, "x264_no_dct_decimate", "0", X264_OPT_BOOL},
	{x264_cabac_syns, "x264_cabac", "1", X264_OPT_BOOL},
	{x264_psy_syns, "x264_psy_rd", "1,0", X264_OPT_PSY},
	{x264_psy_syns, "x264_psy_trell", "1,0", X264_OPT_PSY},
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
	gdouble ivalue;
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

static void
x264_parse_psy(const gchar *psy, gdouble *psy_rd, gdouble *psy_trell)
{
	gchar *val;
	gchar *trell_val = NULL;
	gchar *end;

	*psy_rd = 0.;
	*psy_trell = 0.;
	if (psy == NULL) return;
	val = g_strdup(psy);
	gchar *pos = strchr(val, ',');
	if (pos != NULL)
	{
		trell_val = pos + 1;
		*pos = 0;
	}
	*psy_rd = g_strtod (val, &end);
	if (trell_val != NULL)
	{
		*psy_trell = g_strtod (trell_val, &end);
	}
	g_free(val);
}

static void
x264_update_psy(signal_user_data_t *ud, const gchar *xval)
{
	gdouble rd_value, trell_value;

	if (xval == NULL) return;
	x264_parse_psy(xval, &rd_value, &trell_value);
	ghb_ui_update(ud, "x264_psy_rd", ghb_double_value(rd_value));
	ghb_ui_update(ud, "x264_psy_trell", ghb_double_value(trell_value));
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
				case X264_OPT_PSY:
					// dirty little hack.  mark psy_trell found as well
					x264_opt_map[jj+1].found = TRUE;
					x264_update_psy(ud, val);
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
			case X264_OPT_PSY:
				x264_update_psy(ud, val);
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

gchar*
get_psy_val(signal_user_data_t *ud)
{
	gdouble rd, trell;
	gchar *result;
	rd = ghb_settings_get_double(ud->settings, "x264_psy_rd");
	trell = ghb_settings_get_double(ud->settings, "x264_psy_trell");
	result = g_strdup_printf("%g,%g", rd, trell);
	return result;
}

static void
x264_opt_update(signal_user_data_t *ud, GtkWidget *widget)
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

		options = ghb_settings_get_string(ud->settings, "x264Option");
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
				else if (type == X264_OPT_PSY)
					val = get_psy_val(ud);
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
			else if (type == X264_OPT_PSY)
				val = get_psy_val(ud);
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
		sopts = sanitize_x264opts(ud, result);
		ghb_ui_update(ud, "x264Option", ghb_string_value(sopts));
		ghb_x264_parse_options(ud, sopts);
		g_free(sopts);
		g_free(result);
	}
}

static gint
x264_find_opt(gchar **opts, gchar **opt_syns)
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
			g_free(opt);
			return ii;
		}
		g_free(opt);
	}
	return -1;
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

static gchar*
x264_lookup_value(gchar **opts, gchar **opt_syns)
{
	gchar *ret = NULL;
	gint pos;

	const gchar *def_val = x264_opt_get_default(opt_syns[0]);

	pos = x264_find_opt(opts, opt_syns);
	if (pos >= 0)
	{
		gchar *cpos = strchr(opts[pos], '=');
		if (cpos != NULL)
		{
			ret = g_strdup(cpos+1);
		}
		else
		{
			ret = g_strdup("");
		}
	}
	else if (def_val != NULL)
	{
		ret = g_strdup(def_val);
	}
	return ret;
}

gint
ghb_lookup_badapt(gchar *options)
{
	gint ret = 0;
	gchar *result;
	gchar **split;
	
	if (options == NULL)
		options = "";

	split = g_strsplit(options, ":", -1);

	result = x264_lookup_value(split, x264_badapt_syns);
	g_strfreev(split);
	if (result != NULL)
	{
		ret = g_strtod(result, NULL);
		g_free(result);
	}
	return ret;
}

// Construct the x264 options string
// The result is allocated, so someone must free it at some point.
static gchar*
sanitize_x264opts(signal_user_data_t *ud, const gchar *options)
{
	GString *x264opts = g_string_new("");
	gchar **split = g_strsplit(options, ":", -1);
	gint ii;

	// Fix up option dependencies
	gint subme = ghb_settings_combo_int(ud->settings, "x264_subme");
	if (subme < 6)
	{
		x264_remove_opt(split, x264_psy_syns);
	}
	gint trell = ghb_settings_combo_int(ud->settings, "x264_trellis");
	if (trell < 1)
	{
		gint psy;
		gdouble psy_rd = 0., psy_trell;

		psy = x264_find_opt(split, x264_psy_syns);
		if (psy >= 0)
		{
			gchar *pos = strchr(split[psy], '=');
			if (pos != NULL)
			{
				x264_parse_psy(pos+1, &psy_rd, &psy_trell);
			}
			g_free(split[psy]);
			split[psy] = g_strdup_printf("psy-rd=%g,0", psy_rd);
		}
	}
	gint refs = ghb_settings_get_int(ud->settings, "x264_refs");
	if (refs <= 1)
	{
		x264_remove_opt(split, x264_mixed_syns);
	}
	gint bframes = ghb_settings_get_int(ud->settings, "x264_bframes");
	if (bframes == 0)
	{
		x264_remove_opt(split, x264_weightb_syns);
		x264_remove_opt(split, x264_direct_syns);
		x264_remove_opt(split, x264_badapt_syns);
	}
	if (bframes <= 1)
	{
		x264_remove_opt(split, x264_bpyramid_syns);
	}
	if (!ghb_settings_get_boolean(ud->settings, "x264_cabac"))
	{
		x264_remove_opt(split, x264_trellis_syns);
	}
	// Remove entries that match the defaults
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

