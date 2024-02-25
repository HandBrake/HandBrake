/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * settings.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * settings.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * settings.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "compat.h"

#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "ghb-file-button.h"

void dump_settings(GhbValue *settings);
void ghb_pref_audio_init(signal_user_data_t *ud);

GObject*
debug_get_object(GtkBuilder* b, const gchar *n)
{
    g_debug("name %s\n", n);
    return gtk_builder_get_object(b, n);
}

gint
ghb_settings_combo_int(const GhbValue *settings, const gchar *key)
{
    return ghb_lookup_combo_int(key, ghb_dict_get_value(settings, key));
}

gdouble
ghb_settings_combo_double(const GhbValue *settings, const gchar *key)
{
    return ghb_lookup_combo_double(key, ghb_dict_get_value(settings, key));
}

gchar*
ghb_settings_combo_option(const GhbValue *settings, const gchar *key)
{
    return ghb_lookup_combo_option(key, ghb_dict_get_value(settings, key));
}

// Map widget names to setting keys
// Widgets that map to settings have names
// of this format: s_<setting key>
const gchar*
ghb_get_setting_key(GtkWidget *widget)
{
    const gchar *name;

    ghb_log_func();
    if (widget == NULL) return NULL;
    g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);
    name = gtk_buildable_get_name(GTK_BUILDABLE(widget));

    if (name == NULL || !strncmp(name, "Gtk", 3))
    {
        name = gtk_widget_get_name(widget);
    }
    if (name == NULL)
    {
        // Bad widget pointer?  Should never happen.
        g_debug("Bad widget");
        return NULL;
    }
    return name;
}

GhbValue*
ghb_widget_value(GtkWidget *widget)
{
    GhbValue *value = NULL;
    const gchar *name;
    GType type;

    if (widget == NULL)
    {
        g_debug("NULL widget");
        return NULL;
    }

    type = G_OBJECT_TYPE(widget);
    name = ghb_get_setting_key(widget);
    if (type == GTK_TYPE_ENTRY)
    {
        const gchar *str = ghb_editable_get_text(widget);
        value = ghb_string_value_new(str);
    }
    else if (type == GTK_TYPE_RADIO_BUTTON)
    {
        gboolean bval;
#if !GTK_CHECK_VERSION(4, 4, 0)
        bval = gtk_toggle_button_get_inconsistent(GTK_TOGGLE_BUTTON(widget));
        if (bval)
        {
            value = ghb_bool_value_new(FALSE);
        }
        else
#endif
        {
            bval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
            value = ghb_bool_value_new(bval);
        }
    }
    else if (type == GTK_TYPE_CHECK_BUTTON)
    {
        gboolean bval;
        bval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        value = ghb_bool_value_new(bval);
    }
    else if (type == GTK_TYPE_TOGGLE_TOOL_BUTTON)
    {
        gboolean bval;
        bval = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
        value = ghb_bool_value_new(bval);
    }
    else if (type == GTK_TYPE_TOGGLE_BUTTON)
    {
        gboolean bval;
        bval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        value = ghb_bool_value_new(bval);
    }
    else if (type == GTK_TYPE_CHECK_MENU_ITEM)
    {
        gboolean bval;
        bval = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
        value = ghb_bool_value_new(bval);
    }
    else if (type == GTK_TYPE_COMBO_BOX)
    {
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
        else if (gtk_combo_box_get_has_entry(GTK_COMBO_BOX(widget)))
        {
            const gchar *str;
            str = ghb_editable_get_text(gtk_bin_get_child(GTK_BIN(widget)));
            if (str == NULL) str = "";
            value = ghb_string_value_new(str);
        }
        else
        {
            value = ghb_string_value_new("");
        }
    }
    else if (type == GTK_TYPE_SPIN_BUTTON)
    {
        double val;
        val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
        value = ghb_double_value_new(val);
    }
    else if (type == GTK_TYPE_SCALE)
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
    else if (type == GTK_TYPE_SCALE_BUTTON)
    {
        gdouble dval;

        dval = gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));
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
    else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON)
    {
        gchar *str = NULL;
        str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(widget));
        if (str == NULL)
        {
            str = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(widget));
        }
        value = ghb_string_value_new(str);
        if (str != NULL)
            g_free(str);
    }
    else if (type == GHB_TYPE_FILE_BUTTON)
    {
        const gchar *str;
        str = ghb_file_button_get_filename (GHB_FILE_BUTTON(widget));
        value = ghb_string_value_new(str);
    }
    else
    {
        g_warning("Attempt to get unknown widget type, name %s", name);
        g_free(value);
        value = NULL;
    }
    return value;
}

gchar*
ghb_widget_string(GtkWidget *widget)
{
    GhbValue *value;
    gchar *sval;

    value = ghb_widget_value(widget);
    sval = ghb_value_get_string_xform(value);
    ghb_value_free(&value);
    return sval;
}

gdouble
ghb_widget_double(GtkWidget *widget)
{
    GhbValue *value;
    gdouble dval;

    value = ghb_widget_value(widget);
    dval = ghb_value_get_double(value);
    ghb_value_free(&value);
    return dval;
}

gint64
ghb_widget_int64(GtkWidget *widget)
{
    GhbValue *value;
    gint64 ival;

    value = ghb_widget_value(widget);
    ival = ghb_value_get_int(value);
    ghb_value_free(&value);
    return ival;
}

gint
ghb_widget_int(GtkWidget *widget)
{
    GhbValue *value;
    gint ival;

    value = ghb_widget_value(widget);
    ival = (gint)ghb_value_get_int(value);
    ghb_value_free(&value);
    return ival;
}

gint
ghb_widget_boolean(GtkWidget *widget)
{
    GhbValue *value;
    gboolean bval;

    value = ghb_widget_value(widget);
    bval = ghb_value_get_bool(value);
    ghb_value_free(&value);
    return bval;
}

void
ghb_widget_to_setting(GhbValue *settings, GtkWidget *widget)
{
    const gchar *key = NULL;
    GhbValue *value;

    if (widget == NULL) return;
    ghb_log_func();
    // Find corresponding setting
    key = ghb_get_setting_key(widget);
    if (key == NULL) return;
    value = ghb_widget_value(widget);
    if (value != NULL)
    {
        ghb_dict_set(settings, key, value);
    }
    else
    {
        g_debug("No value found for %s", key);
    }
}

void
ghb_update_widget(GtkWidget *widget, const GhbValue *value)
{
    GType type;
    gchar *str, *tmp;
    gint ival;
    gdouble dval;

    const char *name = ghb_get_setting_key(widget);
    type = ghb_value_type(value);
    if (type == GHB_ARRAY || type == GHB_DICT)
        return;
    if (value == NULL) return;
    str = tmp = ghb_value_get_string_xform(value);
    ival = ghb_value_get_int(value);
    dval = ghb_value_get_double(value);
    type = G_OBJECT_TYPE(widget);

    if (str == NULL)
        str = "";

    if (type == GTK_TYPE_ENTRY)
    {
        ghb_editable_set_text(widget, str);
    }
    else if (type == GTK_TYPE_RADIO_BUTTON)
    {
        if (ival)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !!ival);
    }
    else if (type == GTK_TYPE_CHECK_BUTTON)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), ival);
    }
    else if (type == GTK_TYPE_TOGGLE_TOOL_BUTTON)
    {
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), ival);
    }
    else if (type == GTK_TYPE_TOGGLE_BUTTON)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), ival);
    }
    else if (type == GTK_TYPE_CHECK_MENU_ITEM)
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), ival);
    }
    else if (type == GTK_TYPE_COMBO_BOX)
    {
        GtkTreeModel *store;
        GtkTreeIter iter;
        gchar *shortOpt;
        gdouble ivalue;
        gboolean foundit = FALSE;

        store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
        if (gtk_tree_model_get_iter_first (store, &iter))
        {
            do
            {
                gtk_tree_model_get(store, &iter, 2, &shortOpt, -1);
                if (strcasecmp(shortOpt, str) == 0)
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
                if ((gint)ivalue == ival || ivalue == dval)
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
            if (gtk_combo_box_get_has_entry(GTK_COMBO_BOX(widget)))
            {
                GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget)));
                if (entry)
                {
                    ghb_editable_set_text(entry, str);
                }
                else
                {
                    gtk_combo_box_set_active (GTK_COMBO_BOX(widget), 0);
                }
            }
            else
            {
                gtk_combo_box_set_active (GTK_COMBO_BOX(widget), 0);
            }
        }
    }
    else if (type == GTK_TYPE_SPIN_BUTTON)
    {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), dval);
    }
    else if (type == GTK_TYPE_SCALE)
    {
        gtk_range_set_value(GTK_RANGE(widget), dval);
    }
    else if (type == GTK_TYPE_SCALE_BUTTON)
    {
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(widget), dval);
    }
    else if (type == GTK_TYPE_TEXT_VIEW)
    {
        static int text_view_busy = 0;
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(
                                                GTK_TEXT_VIEW(widget));
        if (!text_view_busy)
        {
            text_view_busy = 1;
            gtk_text_buffer_set_text (buffer, str, -1);
            text_view_busy = 0;
        }
    }
    else if (type == GTK_TYPE_LABEL)
    {
        gtk_label_set_markup (GTK_LABEL(widget), str);
    }
    else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON)
    {
        if (str[0] != 0)
        {
            ghb_file_chooser_set_initial_file(GTK_FILE_CHOOSER(widget), str);
        }
    }
    else if (type == GHB_TYPE_FILE_BUTTON)
    {
        GtkFileChooserAction act;
        act = ghb_file_button_get_action(GHB_FILE_BUTTON(widget));

        if (str[0] == 0)
        {
            // Do nothing
            ;
        }
        else if (act == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ||
                 act == GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER ||
                 act == GTK_FILE_CHOOSER_ACTION_SAVE)
        {
            ghb_file_button_set_filename (GHB_FILE_BUTTON(widget), str);
        }
        else
        {
            if (g_file_test(str, G_FILE_TEST_IS_DIR) ||
                g_file_test(str, G_FILE_TEST_EXISTS))
            {
                ghb_file_button_set_filename (GHB_FILE_BUTTON(widget), str);
            }
            else
            {
                gchar * dirname;

                dirname = g_path_get_dirname(str);
                ghb_file_button_set_filename(
                    GHB_FILE_BUTTON(widget), dirname);
                g_free(dirname);
            }
        }
    }
    else
    {
        g_warning("Attempt to set unknown widget type, name %s", name);
    }
    g_free(tmp);
}

int
ghb_ui_update_from_settings(signal_user_data_t *ud, const gchar *name, const GhbValue *settings)
{
    GObject *object;
    GhbValue * value;

    ghb_log_func_str(name);
    if (name == NULL)
        return 0;
    value = ghb_dict_get_value(settings, name);
    if (value == NULL)
        return 0;
    object = GHB_OBJECT(ud->builder, name);
    if (object == NULL)
    {
        g_debug("Failed to find widget for key: %s", name);
        return -1;
    }
    ghb_update_widget((GtkWidget*)object, value);
    // Its possible the value hasn't changed. Since settings are only
    // updated when the value changes, I'm initializing settings here as well.
    ghb_widget_to_setting(ud->settings, (GtkWidget*)object);
    return 0;
}

int
ghb_ui_update(signal_user_data_t *ud, const gchar *name, const GhbValue *value)
{
    GObject *object;

    ghb_log_func_str(name);
    if (name == NULL || value == NULL)
        return 0;
    object = GHB_OBJECT(ud->builder, name);
    if (object == NULL)
    {
        g_debug("Failed to find widget for key: %s", name);
        return -1;
    }
    ghb_update_widget((GtkWidget*)object, value);
    // Its possible the value hasn't changed. Since settings are only
    // updated when the value changes, I'm initializing settings here as well.
    ghb_widget_to_setting(ud->settings, (GtkWidget*)object);
    return 0;
}

int
ghb_ui_settings_update(
    signal_user_data_t *ud,
    GhbValue *settings,
    const gchar *name,
    const GhbValue *value)
{
    GObject *object;

    ghb_log_func_str(name);
    if (name == NULL || value == NULL)
        return 0;
    object = GHB_OBJECT(ud->builder, name);
    if (object == NULL)
    {
        g_debug("Failed to find widget for key: %s", name);
        return -1;
    }
    ghb_update_widget((GtkWidget*)object, value);
    // Its possible the value hasn't changed. Since settings are only
    // updated when the value changes, I'm initializing settings here as well.
    ghb_widget_to_setting(settings, (GtkWidget*)object);
    return 0;
}
