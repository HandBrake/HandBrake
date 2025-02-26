/* ghb-string-list.c
 *
 * Copyright (C) 2025 HandBrake Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ghb-string-list.h"

struct _GhbStringList {
    GtkBox parent_instance;
    GActionMap *actions;

    GtkListBox *list_box;

    gboolean is_editable;
};

G_DEFINE_TYPE(GhbStringList, ghb_string_list, GTK_TYPE_BOX)

enum {
    PROP_ITEMS = 1,
    PROP_IS_EDITABLE,
    N_PROPS
};
static GParamSpec *props[N_PROPS] = { NULL };

static void string_list_add_action(GSimpleAction *action, GVariant *param, gpointer self);
static void string_list_remove_action(GSimpleAction *action, GVariant *param, gpointer self);

static GActionEntry action_entries[] = {
    {"add", string_list_add_action, NULL, NULL, NULL},
    {"remove", string_list_remove_action, NULL, NULL, NULL},
};

static void ghb_string_list_finalize(GObject *object);
static void ghb_string_list_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec);
static void ghb_string_list_set_property(GObject *object, guint prop_id,
                                         const GValue *value, GParamSpec *pspec);

static void
ghb_string_list_class_init (GhbStringListClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-string-list.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbStringList, list_box);

    props[PROP_ITEMS] = g_param_spec_boxed("items", NULL, NULL,
                                           G_TYPE_STRV,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_IS_EDITABLE] = g_param_spec_boolean("is-editable", NULL, NULL,
                                           TRUE,
                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    object_class->set_property = ghb_string_list_set_property;
    object_class->get_property = ghb_string_list_get_property;
    object_class->finalize = ghb_string_list_finalize;

    g_object_class_install_properties(object_class, N_PROPS, props);
}

static void
ghb_string_list_get_property (GObject *object, guint prop_id,
                                   GValue *value, GParamSpec *pspec)
{
    GhbStringList *self = GHB_STRING_LIST(object);
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    switch (prop_id)
    {
        case PROP_ITEMS:
            g_value_take_boxed(value, ghb_string_list_get_items(self));
            break;
        case PROP_IS_EDITABLE:
            g_value_set_boolean(value, self->is_editable);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_string_list_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
    GhbStringList *self = GHB_STRING_LIST(object);
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    switch (prop_id)
    {
        case PROP_ITEMS:
            ghb_string_list_set_items(self, g_value_get_boxed(value));
            break;
        case PROP_IS_EDITABLE:
            self->is_editable = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_string_list_init (GhbStringList *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    self->actions = G_ACTION_MAP(g_simple_action_group_new());
    g_action_map_add_action_entries(self->actions, action_entries,
                                    G_N_ELEMENTS(action_entries), self);

    gtk_widget_insert_action_group(GTK_WIDGET(self), "string-list", G_ACTION_GROUP(self->actions));
}

static void
ghb_string_list_finalize (GObject *object)
{
    GhbStringList *self = GHB_STRING_LIST(object);
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    G_OBJECT_CLASS(ghb_string_list_parent_class)->finalize (object);
}

GtkWidget *
ghb_string_list_new (gboolean editable)
{
    GhbStringList *view;

    view = g_object_new(GHB_TYPE_STRING_LIST, NULL);

    return GTK_WIDGET(view);
}

static void
row_text_update (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
    GhbStringList *self = user_data;
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
}

static GtkWidget *
ghb_string_list_row_new (GhbStringList *list, const char *text)
{
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_editable_label_new(text);
    gtk_editable_set_editable(GTK_EDITABLE(label), list->is_editable);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_signal_connect(label, "notify::text", G_CALLBACK(row_text_update), list);
    return row;
}

void
ghb_string_list_remove_item_at_index (GhbStringList *self, int idx)
{
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkListBoxRow *row = gtk_list_box_get_row_at_index(self->list_box, idx);
    gtk_list_box_remove(self->list_box, GTK_WIDGET(row));
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
}

void
ghb_string_list_append (GhbStringList *self, const char *name)
{
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkWidget *row = ghb_string_list_row_new(self, name);
    gtk_list_box_append(self->list_box, row);
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
}

void
ghb_string_list_update_item_at_index (GhbStringList *self, int idx, const char *name)
{
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkListBoxRow *row = gtk_list_box_get_row_at_index(self->list_box, idx);
    GtkEditable *label = GTK_EDITABLE(gtk_list_box_row_get_child(row));
    gtk_editable_set_text(label, name);
}

char **
ghb_string_list_get_items (GhbStringList *self)
{
    g_return_val_if_fail(GHB_IS_STRING_LIST(self), NULL);

    GStrvBuilder *builder = g_strv_builder_new();

    GtkListBoxRow *row;
    for (int i = 0; (row = gtk_list_box_get_row_at_index(self->list_box, i)); i++)
    {
        GtkEditable *label = GTK_EDITABLE(gtk_list_box_row_get_child(row));
        g_strv_builder_add(builder, gtk_editable_get_text(label));
    }

    return g_strv_builder_end(builder);
}

void
ghb_string_list_set_items (GhbStringList *self, const char **names)
{
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    g_object_freeze_notify(G_OBJECT(self));
    ghb_string_list_clear(self);
    for (int i = 0; i < g_strv_length((char **)names); i++)
    {
        ghb_string_list_append(self, names[i]);
    }
    g_object_thaw_notify(G_OBJECT(self));
}

void
ghb_string_list_clear (GhbStringList *self)
{
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkListBoxRow *row;
    while ((row = gtk_list_box_get_row_at_index(self->list_box, 0)))
    {
        gtk_list_box_remove(self->list_box, GTK_WIDGET(row));
    }
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
}

int
ghb_string_list_get_selected_index (GhbStringList *self)
{
    g_return_val_if_fail(GHB_IS_STRING_LIST(self), -1);

    GtkListBoxRow *row = gtk_list_box_get_selected_row(self->list_box);
    if (!row)
        return -1;

    return gtk_list_box_row_get_index(row);
}

char *
ghb_string_list_get_selected_string (GhbStringList *self)
{
    g_return_val_if_fail(GHB_IS_STRING_LIST(self), NULL);

    GtkListBoxRow *row = gtk_list_box_get_selected_row(self->list_box);
    if (!row)
        return NULL;

    GtkEditable *label = GTK_EDITABLE(gtk_list_box_row_get_child(row));
    return g_strdup(gtk_editable_get_text(label));
}

static void
string_list_add_action (GSimpleAction *action, GVariant *param, gpointer data)
{
    GhbStringList *self = data;
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkWidget *row = ghb_string_list_row_new(self, "extension");

    // We have to add it at the top as GtkListBox does not scroll down reliably
    gtk_list_box_insert(self->list_box, row, 0);
    gtk_list_box_select_row(self->list_box, GTK_LIST_BOX_ROW(row));

    GtkEditableLabel *label = GTK_EDITABLE_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row)));
    gtk_editable_label_start_editing(label);
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
}

G_MODULE_EXPORT void
string_list_remove_action (GSimpleAction *action, GVariant *param, gpointer data)
{
    GhbStringList *self = data;
    g_return_if_fail(GHB_IS_STRING_LIST(self));

    GtkListBoxRow *row = gtk_list_box_get_selected_row(self->list_box);
    if (row)
    {
        gtk_list_box_remove(self->list_box, GTK_WIDGET(row));
        g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ITEMS]);
    }
}
