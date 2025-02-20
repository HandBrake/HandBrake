/* ghb-file-button.c
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

#include "ghb-file-button.h"

#include "util.h"

struct _GhbFileButton {
    GtkButton parent_instance;

    GtkImage *icon;
    GtkLabel *label;

    char *title;
    char *accept_label;
    GtkFileChooserAction action;
    GFile *selected_file;
};

G_DEFINE_TYPE(GhbFileButton, ghb_file_button, GTK_TYPE_BUTTON)

enum {
    PROP_ACTION = 1,
    PROP_TITLE,
    PROP_FILE,
    PROP_ACCEPT_LABEL,
    N_PROPS
};
static GParamSpec *props[N_PROPS] = {NULL};

static void ghb_file_button_finalize (GObject *object);
static void ghb_file_button_get_property (GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec);
static void ghb_file_button_set_property (GObject *object, guint prop_id,
                                          const GValue *value, GParamSpec *pspec);

static void ghb_file_button_clicked(GtkButton *button);

static void
ghb_file_button_class_init (GhbFileButtonClass *class_)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class_);
    GtkButtonClass *button_class = GTK_BUTTON_CLASS(class_);
    GObjectClass *object_class = G_OBJECT_CLASS(class_);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-file-button.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbFileButton, icon);
    gtk_widget_class_bind_template_child(widget_class, GhbFileButton, label);

    props[PROP_ACTION] = g_param_spec_enum("action", NULL, NULL,
                                           GTK_TYPE_FILE_CHOOSER_ACTION,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_TITLE] = g_param_spec_string("title", NULL, NULL,
                                           _("Select a File"),
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_ACCEPT_LABEL] = g_param_spec_string("accept-label", NULL, NULL,
                                           NULL,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_FILE] = g_param_spec_string("file", NULL, NULL,
                                           "",
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    object_class->set_property = ghb_file_button_set_property;
    object_class->get_property = ghb_file_button_get_property;
    object_class->finalize = ghb_file_button_finalize;

    button_class->clicked = ghb_file_button_clicked;

    g_object_class_install_properties(object_class, N_PROPS, props);
}

static void
ghb_file_button_get_property (GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec)
{
    GhbFileButton *self = GHB_FILE_BUTTON(object);

    switch (prop_id)
    {
        case PROP_TITLE:
            g_value_set_string(value, ghb_file_button_get_title(self));
            break;

        case PROP_ACCEPT_LABEL:
            g_value_set_string(value, ghb_file_button_get_accept_label(self));
            break;

        case PROP_FILE:
            g_value_take_string(value, ghb_file_button_get_filename(self));
            break;

        case PROP_ACTION:
            g_value_set_enum(value, ghb_file_button_get_action(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_file_button_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
    GhbFileButton *self = GHB_FILE_BUTTON(object);
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    switch (prop_id)
    {
        case PROP_TITLE:
            ghb_file_button_set_title(self, g_value_get_string(value));
            break;

        case PROP_ACCEPT_LABEL:
            ghb_file_button_set_accept_label(self, g_value_get_string(value));
            break;

        case PROP_FILE:
            ghb_file_button_set_filename(self, g_value_get_string(value));
            break;

        case PROP_ACTION:
            ghb_file_button_set_action(self, g_value_get_enum(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_file_button_init (GhbFileButton *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
}

static void
ghb_file_button_finalize (GObject *object)
{
    GhbFileButton *self = GHB_FILE_BUTTON(object);
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    g_clear_object(&self->selected_file);
    g_clear_object(&self->icon);
    g_clear_object(&self->label);

    g_clear_pointer(&self->title, g_free);
    g_clear_pointer(&self->accept_label, g_free);

    G_OBJECT_CLASS(ghb_file_button_parent_class)->finalize(object);
}

const GFile *
ghb_file_button_get_file (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), NULL);

    return self->selected_file;
}

GtkFileChooserAction
ghb_file_button_get_action (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), 0);

    return self->action;
}

char *
ghb_file_button_get_filename (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), NULL);

    if (self->selected_file)
        return g_file_get_path(self->selected_file);
    else
        return NULL;
}

const char *
ghb_file_button_get_title (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), NULL);

    return self->title;
}

const char *
ghb_file_button_get_accept_label (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), NULL);

    if (self->accept_label && self->accept_label[0])
        return self->accept_label;

    switch (self->action)
    {
        case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
            return _("_Select");
        case GTK_FILE_CHOOSER_ACTION_SAVE:
            return _("_Save");
        case GTK_FILE_CHOOSER_ACTION_OPEN:
        default:
            return _("_Open");
    }
}

GhbFileButton *
ghb_file_button_new (const char *title, GtkFileChooserAction action)
{
    GhbFileButton *button = g_object_new(GHB_TYPE_FILE_BUTTON,
                                         "action", action,
                                         "title", title,
                                         NULL);
    return button;
}

static void
file_icon_query_cb (GFile *file, GAsyncResult *result, GhbFileButton *self)
{
    GIcon *icon = NULL;
    GFileInfo *info = g_file_query_info_finish(file, result, NULL);

    if (info == NULL) return;

    icon = g_file_info_get_icon(info);

    if (self->icon != NULL && icon != NULL)
    {
        gtk_image_set_from_gicon(self->icon, icon);
    }
    else if (self->icon != NULL)
    {
        gtk_image_clear(self->icon);
    }
    else if (icon != NULL)
    {
        g_object_unref(icon);
    }
    g_object_unref(info);
}

void
ghb_file_button_set_action (GhbFileButton *self, GtkFileChooserAction action)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    self->action = action;
}

void
ghb_file_button_set_file (GhbFileButton *self, GFile *file)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    if (file == NULL) return;

    if (self->selected_file)
    {
        g_object_unref(self->selected_file);
    }

    self->selected_file = g_object_ref(file);

    if (g_file_test(g_file_peek_path(file), G_FILE_TEST_EXISTS))
    {
        g_autofree char *file_base = g_file_get_basename(file);
        gtk_label_set_label(self->label, file_base);
        g_file_query_info_async(file, G_FILE_ATTRIBUTE_STANDARD_ICON,
                                G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, NULL,
                                (GAsyncReadyCallback) file_icon_query_cb, self);
    }
    else
    {
        gtk_label_set_label(self->label, _("(None)"));
        gtk_image_clear(self->icon);
    }

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_FILE]);
}

void
ghb_file_button_set_filename (GhbFileButton *self, const char *filename)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    g_autoptr(GFile) file = g_file_new_for_path(filename);
    ghb_file_button_set_file(self, file);
}

void
ghb_file_button_set_title (GhbFileButton *self, const char *filename)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    g_free(self->title);
    self->title = g_strdup(filename);
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_TITLE]);
}

void
ghb_file_button_set_accept_label (GhbFileButton *self, const char *filename)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    g_free(self->accept_label);
    self->accept_label = g_strdup(filename);
    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ACCEPT_LABEL]);
}

static void
chooser_response_cb (GtkFileChooser *chooser, GtkResponseType response,
                     GhbFileButton *self)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        GFile *file = gtk_file_chooser_get_file(chooser);
        ghb_file_button_set_file(self, file);
        g_object_unref(file);
    }
    ghb_file_chooser_destroy(chooser);
}

static void
ghb_file_button_clicked (GtkButton *button)
{
    GtkWindow *window;
    GtkFileChooser *chooser;
    GhbFileButton *self = GHB_FILE_BUTTON(button);

    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(self)));
    chooser = ghb_file_chooser_new(ghb_file_button_get_title(self), window,
                                   ghb_file_button_get_action(self),
                                   ghb_file_button_get_accept_label(self),
                                   _("_Cancel"));
    g_autofree char *selected_name = ghb_file_button_get_filename(self);
    ghb_file_chooser_set_initial_file(chooser, selected_name);

    g_signal_connect(chooser, "response", G_CALLBACK(chooser_response_cb), self);

    ghb_file_chooser_set_modal(chooser, GTK_IS_WINDOW(window));
    ghb_file_chooser_show(chooser);
}
