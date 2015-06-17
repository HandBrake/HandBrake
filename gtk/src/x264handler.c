/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * x264handler.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * x264handler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <glib/gi18n.h>
#include "ghbcompat.h"
#include <string.h>
#include "settings.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "hb-backend.h"
#include "x264handler.h"
#include "videohandler.h"

gint ghb_lookup_bframes(const gchar *options);
static void x264_opt_update(signal_user_data_t *ud, GtkWidget *widget);
static gchar* sanitize_x264opts(signal_user_data_t *ud, const gchar *options);

// Flag needed to prevent x264 options processing from chasing its tail
static gboolean ignore_options_update = FALSE;

void ghb_show_hide_advanced_video( signal_user_data_t *ud )
{
    gboolean hide;

    hide = ghb_dict_get_bool(ud->prefs, "HideAdvancedVideoSettings");
    if (hide)
    {
        ghb_ui_update(ud, "x264UseAdvancedOptions", ghb_boolean_value(FALSE));
    }

    GtkWidget *widget;
    GtkWidget *at = GHB_WIDGET(ud->builder, "advanced_video_tab");

    gtk_widget_set_visible(at, !hide);
    widget = GHB_WIDGET(ud->builder, "x264UseAdvancedOptions");
    gtk_widget_set_visible(widget, !hide);
}

G_MODULE_EXPORT void
x264_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->x264_priv, widget);
    if (!ignore_options_update)
    {
        ignore_options_update = TRUE;
        x264_opt_update(ud, widget);
        ignore_options_update = FALSE;
    }
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
x264_slider_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->x264_priv, widget);

    // Lock slider values to multiples of step_increment
    GtkAdjustment * adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    gdouble step = gtk_adjustment_get_step_increment(adj);
    gdouble val = gtk_range_get_value(GTK_RANGE(widget));
    gdouble new_val = ((int)((val + step / 2) / step)) * step;
    gdouble diff = val - new_val;
    if ( diff > 0.0001 || diff < -0.0001 )
    {
        gtk_range_set_value(GTK_RANGE(widget), new_val);
    }
    else if (!ignore_options_update)
    {
        ignore_options_update = TRUE;
        x264_opt_update(ud, widget);
        ignore_options_update = FALSE;
    }
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT gchar*
x264_format_slider_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    return g_strdup_printf("%-6.6g", val);
}


G_MODULE_EXPORT void
x264_me_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint me;

    ghb_widget_to_setting(ud->x264_priv, widget);
    if (!ignore_options_update)
    {
        ignore_options_update = TRUE;
        x264_opt_update(ud, widget);
        ignore_options_update = FALSE;
    }
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    widget = GHB_WIDGET(ud->builder, "x264_merange");
    me = ghb_settings_combo_int(ud->x264_priv, "x264_me");
    if (me < 2)
    {   // me < umh
        // me_range 4 - 16
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), 4, 16);
    }
    else
    {
        // me_range 4 - 64
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), 4, 64);
    }
}

extern char *video_option_tooltip;

G_MODULE_EXPORT void
x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("x264_entry_changed_cb ()");

    if (video_option_tooltip == NULL)
    {
        GtkWidget *eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
        video_option_tooltip = gtk_widget_get_tooltip_text(eo);
    }

    if (!ignore_options_update)
    {
        GtkWidget *textview;
        const gchar *options;

        textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "x264Option"));
        ghb_widget_to_setting(ud->settings, textview);
        options = ghb_dict_get_string(ud->settings, "x264Option");

        ignore_options_update = TRUE;
        ghb_x264_parse_options(ud, options);
        if (!gtk_widget_has_focus(textview))
        {
            gchar *sopts;

            sopts = sanitize_x264opts(ud, options);
            ghb_update_x264Option(ud, sopts);
            ghb_x264_parse_options(ud, sopts);

            if (ghb_dict_get_bool(ud->settings, "x264UseAdvancedOptions"))
            {
                GtkWidget *eo;
                eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
                char * tt;
                if (sopts)
                    tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"%s\""),
                                         video_option_tooltip, sopts);
                else
                    tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"\""),
                                         video_option_tooltip);
                gtk_widget_set_tooltip_text(eo, tt);
                g_free(tt);
            }

            g_free(sopts);
        }
        ignore_options_update = FALSE;
    }
}

G_MODULE_EXPORT gboolean
x264_focus_out_cb(GtkWidget *widget, GdkEventFocus *event,
    signal_user_data_t *ud)
{
    const gchar *options;
    gchar *sopts;

    ghb_widget_to_setting(ud->settings, widget);
    options = ghb_dict_get_string(ud->settings, "x264Option");
    sopts = sanitize_x264opts(ud, options);
    ignore_options_update = TRUE;
    if (sopts != NULL && strcmp(sopts, options) != 0)
    {
        ghb_ui_update(ud, "x264Option", ghb_string_value(sopts));
        ghb_x264_parse_options(ud, sopts);
    }
    g_free(sopts);
    ignore_options_update = FALSE;
    return FALSE;
}

enum
{
    X264_OPT_NONE,
    X264_OPT_BOOL_NONE,
    X264_OPT_INT_NONE,
    X264_OPT_DEBLOCK,
    X264_OPT_PSY,
    X264_OPT_INT,
    X264_OPT_DOUBLE,
    X264_OPT_COMBO,
    X264_OPT_BOOL,
    X264_OPT_TRANS,
};

typedef struct
{
    gchar *x264_val;
    char *ui_val;
} trans_entry_t;

typedef struct
{
    gint count;
    gint x264_type;
    gint ui_type;
    trans_entry_t *map;
} trans_table_t;

static gchar *
trans_x264_val(trans_table_t *trans, char *val)
{
    int ii;

    if (val == NULL)
        return NULL;
    for (ii = 0; ii < trans->count; ii++)
    {
        if (strcmp(val, trans->map[ii].x264_val) == 0)
        {
            return trans->map[ii].ui_val;
        }
    }
    return NULL;
}

static gchar *
trans_ui_val(trans_table_t *trans, char *val)
{
    int ii;

    if (val == NULL)
        return NULL;
    for (ii = 0; ii < trans->count; ii++)
    {
        if (strcmp(val, trans->map[ii].ui_val) == 0)
        {
            return trans->map[ii].x264_val;
        }
    }
    return NULL;
}

struct x264_opt_map_s
{
    gchar **opt_syns;
    gchar *name;
    gchar *def_val;
    gint type;
    trans_table_t *translation;
    gboolean found;
};

static gchar *x264_ref_syns[] = {"ref", "frameref", NULL};
static gchar *x264_bframes_syns[] = {"bframes", NULL};
static gchar *x264_badapt_syns[] = {"b-adapt", "b_adapt", NULL};
static gchar *x264_direct_syns[] =
    {"direct", "direct-pred", "direct_pred", NULL};
static gchar *x264_weightp_syns[] = {"weightp", NULL};
static gchar *x264_bpyramid_syns[] = {"b-pyramid", "b_pyramid", NULL};
static gchar *x264_me_syns[] = {"me", NULL};
static gchar *x264_merange_syns[] = {"merange", "me-range", "me_range", NULL};
static gchar *x264_subme_syns[] = {"subme", "subq", NULL};
static gchar *x264_aqmode_syns[] = {"aq-mode", NULL};
static gchar *x264_analyse_syns[] = {"partitions", "analyse", NULL};
static gchar *x264_8x8dct_syns[] = {"8x8dct", NULL};
static gchar *x264_deblock_syns[] = {"deblock", "filter", NULL};
static gchar *x264_trellis_syns[] = {"trellis", NULL};
static gchar *x264_pskip_syns[] = {"no-fast-pskip", "no_fast_pskip", NULL};
static gchar *x264_psy_syns[] = {"psy-rd", "psy_rd", NULL};
static gchar *x264_aq_strength_syns[] = {"aq-strength", "aq_strength", NULL};
static gchar *x264_mbtree_syns[] = {"mbtree", NULL};
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
    {x264_bframes_syns, "x264_bframes", "3", X264_OPT_INT},
    {x264_direct_syns, "x264_direct", "spatial", X264_OPT_COMBO},
    {x264_badapt_syns, "x264_b_adapt", "1", X264_OPT_COMBO},
    {x264_weightp_syns, "x264_weighted_pframes", "2", X264_OPT_COMBO},
    {x264_bpyramid_syns, "x264_bpyramid", "normal", X264_OPT_COMBO},
    {x264_me_syns, "x264_me", "hex", X264_OPT_COMBO},
    {x264_merange_syns, "x264_merange", "16", X264_OPT_INT},
    {x264_subme_syns, "x264_subme", "7", X264_OPT_COMBO},
    {x264_aqmode_syns, "x264_aqmode", "1", X264_OPT_INT_NONE},
    {x264_analyse_syns, "x264_analyse", "p8x8,b8x8,i8x8,i4x4", X264_OPT_COMBO},
    {x264_8x8dct_syns, "x264_8x8dct", "1", X264_OPT_BOOL},
    {x264_deblock_syns, "x264_deblock_alpha", "0,0", X264_OPT_DEBLOCK},
    {x264_deblock_syns, "x264_deblock_beta", "0,0", X264_OPT_DEBLOCK},
    {x264_trellis_syns, "x264_trellis", "1", X264_OPT_COMBO},
    {x264_pskip_syns, "x264_no_fast_pskip", "0", X264_OPT_BOOL},
    {x264_decimate_syns, "x264_no_dct_decimate", "0", X264_OPT_BOOL},
    {x264_cabac_syns, "x264_cabac", "1", X264_OPT_BOOL},
    {x264_aq_strength_syns, "x264_aq_strength", "1", X264_OPT_DOUBLE},
    {x264_psy_syns, "x264_psy_rd", "1|0", X264_OPT_PSY},
    {x264_psy_syns, "x264_psy_trell", "1|0", X264_OPT_PSY},
    {x264_mbtree_syns, "x264_mbtree", "1", X264_OPT_BOOL_NONE},
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
x264_update_double(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
    gdouble dval;

    if (val == NULL) return;
    dval = g_strtod (val, NULL);
    ghb_dict_set_double(ud->x264_priv, name, dval);
}

static void
x264_update_int(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
    gint ival;

    if (val == NULL) return;
    ival = g_strtod (val, NULL);
    ghb_dict_set_int(ud->x264_priv, name, ival);
}

static void
x264_update_int_setting(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
    gint ival;

    if (val == NULL) return;
    ival = g_strtod (val, NULL);
    ghb_dict_set(ud->x264_priv, name, ghb_int_value_new(ival));
    ghb_check_dependency(ud, NULL, name);
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
        ghb_dict_set_bool(ud->x264_priv, name, TRUE);
    else
        ghb_dict_set_bool(ud->x264_priv, name, str_is_true(val));
}

static void
x264_update_bool_setting(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
    if (val == NULL)
        ghb_dict_set_bool(ud->x264_priv, name, TRUE);
    else
        ghb_dict_set_bool(ud->x264_priv, name, str_is_true(val));

    ghb_check_dependency(ud, NULL, name);
}

static void
x264_update_combo(signal_user_data_t *ud, const gchar *name, const gchar *val)
{
    ghb_dict_set_string(ud->x264_priv, name, val);
}

static void
x264_update_deblock(signal_user_data_t *ud, const gchar *xval)
{
    int avalue, bvalue;
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
        avalue = (int)g_strtod(val, &end);
        if (bval != NULL)
        {
            bvalue = (int)g_strtod(bval, &end);
        }
    }
    g_free(val);
    ghb_dict_set_int(ud->x264_priv, "x264_deblock_alpha", avalue);
    ghb_dict_set_int(ud->x264_priv, "x264_deblock_beta", bvalue);
}

static void
x264_parse_psy(const gchar *psy, gdouble *psy_rd, gdouble *psy_trell)
{
    *psy_rd = 0.;
    *psy_trell = 0.;
    if (psy == NULL) return;
    if (2 == sscanf(psy, "%lf|%lf", psy_rd, psy_trell) ||
        2 == sscanf(psy, "%lf,%lf", psy_rd, psy_trell))
    {
    }
}

static void
x264_update_psy(signal_user_data_t *ud, const gchar *xval)
{
    gdouble rd_value, trell_value;

    if (xval == NULL) return;
    x264_parse_psy(xval, &rd_value, &trell_value);
    ghb_dict_set_double(ud->x264_priv, "x264_psy_rd", rd_value);
    ghb_dict_set_double(ud->x264_priv, "x264_psy_trell", trell_value);
}

static void do_update(signal_user_data_t *ud, char *name, gint type, char *val)
{
    switch(type)
    {
    case X264_OPT_INT:
        x264_update_int(ud, name, val);
        break;
    case X264_OPT_DOUBLE:
        x264_update_double(ud, name, val);
        break;
    case X264_OPT_BOOL:
        x264_update_bool(ud, name, val);
        break;
    case X264_OPT_COMBO:
        x264_update_combo(ud, name, val);
        break;
    case X264_OPT_BOOL_NONE:
        x264_update_bool_setting(ud, name, val);
        break;
    case X264_OPT_INT_NONE:
        x264_update_int_setting(ud, name, val);
        break;
    }
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
                case X264_OPT_DOUBLE:
                    x264_update_double(ud, x264_opt_map[jj].name, val);
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
                case X264_OPT_BOOL_NONE:
                    x264_update_bool_setting(ud, x264_opt_map[jj].name, val);
                    break;
                case X264_OPT_INT_NONE:
                    x264_update_int_setting(ud, x264_opt_map[jj].name, val);
                    break;
                case X264_OPT_TRANS:
                    if (x264_opt_map[jj].translation == NULL)
                        break;
                    val = trans_x264_val(x264_opt_map[jj].translation, val);
                    if (val != NULL)
                    {
                        do_update(ud, x264_opt_map[jj].name,
                            x264_opt_map[jj].translation->ui_type, val);
                        // TODO un-grey the ui control
                    }
                    else
                    {
                        // TODO grey out the ui control
                    }
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
            case X264_OPT_DOUBLE:
                x264_update_double(ud, x264_opt_map[jj].name, val);
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
            case X264_OPT_BOOL_NONE:
                x264_update_bool_setting(ud, x264_opt_map[jj].name, val);
                break;
            case X264_OPT_INT_NONE:
                x264_update_int_setting(ud, x264_opt_map[jj].name, val);
                break;
            case X264_OPT_TRANS:
                if (x264_opt_map[jj].translation == NULL)
                    break;
                val = g_strdup(trans_x264_val(x264_opt_map[jj].translation, val));
                if (val != NULL)
                {
                    do_update(ud, x264_opt_map[jj].name,
                        x264_opt_map[jj].translation->ui_type, val);
                    // TODO un-grey the ui control
                }
                else
                {
                    // TODO grey out the ui control
                }
                break;
            }
            x264_opt_map[jj].found = TRUE;
            g_free(val);
        }
    }
    g_strfreev(split);
    ghb_settings_to_ui(ud, ud->x264_priv);
}

gchar*
get_deblock_val(signal_user_data_t *ud)
{
    int alpha, beta;
    gchar *result;
    alpha = ghb_dict_get_int(ud->x264_priv, "x264_deblock_alpha");
    beta = ghb_dict_get_int(ud->x264_priv, "x264_deblock_beta");
    result = g_strdup_printf("%d,%d", alpha, beta);
    return result;
}

gchar*
get_psy_val(signal_user_data_t *ud)
{
    gdouble rd, trell;
    gchar *result;
    rd = ghb_dict_get_double(ud->x264_priv, "x264_psy_rd");
    trell = ghb_dict_get_double(ud->x264_priv, "x264_psy_trell");
    result = g_strdup_printf("%g|%g", rd, trell);
    return result;
}

static void
x264_opt_update(signal_user_data_t *ud, GtkWidget *widget)
{
    gint jj;
    const gchar *name = ghb_get_setting_key(widget);
    gchar **opt_syns = NULL;
    const gchar *def_val = NULL;
    gint type;
    trans_table_t *trans;

    for (jj = 0; jj < X264_OPT_MAP_SIZE; jj++)
    {
        if (strcmp(name, x264_opt_map[jj].name) == 0)
        {
            // found the options that needs updating
            opt_syns = x264_opt_map[jj].opt_syns;
            def_val = x264_opt_map[jj].def_val;
            type = x264_opt_map[jj].type;
            trans = x264_opt_map[jj].translation;
            break;
        }
    }
    if (opt_syns != NULL)
    {
        GString *x264opts = g_string_new("");
        const gchar *options;
        gchar **split = NULL;
        gint ii;
        gboolean foundit = FALSE;

        options = ghb_dict_get_string(ud->settings, "x264Option");
        if (options)
        {
            split = g_strsplit(options, ":", -1);
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
                    GhbValue *gval;
                    gval = ghb_widget_value(widget);
                    if (ghb_value_type(gval) == GHB_BOOL)
                    {
                        if (ghb_value_get_bool(gval))
                            val = g_strdup("1");
                        else
                            val = g_strdup("0");
                    }
                    else
                    {
                        val = ghb_widget_string(widget);
                    }
                    ghb_value_free(&gval);
                }
                if (type == X264_OPT_TRANS)
                {
                    gchar *tmp;
                    tmp = g_strdup(trans_ui_val(trans, val));
                    if (tmp)
                    {
                        g_free(val);
                        val = tmp;
                    }
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
                GhbValue *gval;
                gval = ghb_widget_value(widget);
                if (ghb_value_type(gval) == GHB_BOOL)
                {
                    if (ghb_value_get_bool(gval))
                        val = g_strdup("1");
                    else
                        val = g_strdup("0");
                }
                else
                {
                    val = ghb_widget_string(widget);
                }
                ghb_value_free(&gval);
            }
            if (type == X264_OPT_TRANS)
            {
                gchar *tmp;
                tmp = g_strdup(trans_ui_val(trans, val));
                if (tmp)
                {
                    g_free(val);
                    val = tmp;
                }
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
        ghb_update_x264Option(ud, sopts);
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
ghb_lookup_badapt(const gchar *options)
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

gint
ghb_lookup_aqmode(const gchar *options)
{
    gint ret = 0;
    gchar *result;
    gchar **split;

    if (options == NULL)
        options = "";

    split = g_strsplit(options, ":", -1);

    result = x264_lookup_value(split, x264_aqmode_syns);
    g_strfreev(split);
    if (result != NULL)
    {
        ret = g_strtod(result, NULL);
        g_free(result);
    }
    return ret;
}

gint
ghb_lookup_bframes(const gchar *options)
{
    gint ret = 0;
    gchar *result;
    gchar **split;

    if (options == NULL)
        options = "";

    split = g_strsplit(options, ":", -1);

    result = x264_lookup_value(split, x264_bframes_syns);
    g_strfreev(split);
    if (result != NULL)
    {
        ret = g_strtod(result, NULL);
        g_free(result);
    }
    return ret;
}

gint
ghb_lookup_mbtree(const gchar *options)
{
    gint ret = ghb_lookup_bframes(options) != 0;
    gchar *result;
    gchar **split;

    if (options == NULL)
        options = "";

    split = g_strsplit(options, ":", -1);

    result = x264_lookup_value(split, x264_mbtree_syns);
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
    gint subme = ghb_settings_combo_int(ud->x264_priv, "x264_subme");
    if (subme < 6)
    {
        x264_remove_opt(split, x264_psy_syns);
    }
    gint trell = ghb_settings_combo_int(ud->x264_priv, "x264_trellis");
    if (subme >= 10)
    {
        gint aqmode = ghb_lookup_aqmode(options);
        if (trell != 2 || aqmode == 0)
        {
            gint pos = x264_find_opt(split, x264_subme_syns);
            g_free(split[pos]);
            split[pos] = g_strdup_printf("subme=9");
        }
    }
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
            split[psy] = g_strdup_printf("psy-rd=%g|0", psy_rd);
        }
    }
    gint bframes = ghb_dict_get_int(ud->x264_priv, "x264_bframes");
    if (bframes == 0)
    {
        x264_remove_opt(split, x264_direct_syns);
        x264_remove_opt(split, x264_badapt_syns);
    }
    if (bframes <= 1)
    {
        x264_remove_opt(split, x264_bpyramid_syns);
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
        const gchar *def_val;
        def_val = x264_opt_get_default(opt);
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

void
ghb_x264_init(signal_user_data_t *ud)
{
    ud->x264_priv = ghb_dict_new();
}

gboolean
ghb_background_refresh_x264Option(signal_user_data_t *ud)
{
    const char *opt;
    opt = ghb_dict_get_string(ud->settings, "x264Option");

    GtkWidget *widget;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    gchar *str;

    widget = GHB_WIDGET(ud->builder, "x264Option");
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    str = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // If the value has changed, then update it
    if (opt != NULL)
    {
        if (str == NULL || strcmp(str, opt))
        {
            ud->dont_clear_presets = 1;
            ghb_ui_update(ud, "x264Option", ghb_string_value(opt));
            ud->dont_clear_presets = 0;
        }
    }
    free(str);
    return FALSE;
}

void ghb_update_x264Option(signal_user_data_t *ud, const char *opt)
{
    ghb_dict_set_string(ud->settings, "x264Option", opt);
    g_idle_add((GSourceFunc)ghb_background_refresh_x264Option, ud);
}
