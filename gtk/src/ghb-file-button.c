/* ghb-file-button.c
 *
 * Copyright (C) 2024 HandBrake Team
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

#include "config.h"

#include "ghb-file-button.h"

struct _GhbFileButton {
    GtkButton parent_instance;

    GtkImage *icon;
    GtkLabel *label;

    char *title;
    char *accept_label;
    GtkFileChooserAction action;
    GFile *selected;
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

static void
ghb_file_button_class_init (GhbFileButtonClass *class_)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class_);
    GObjectClass *object_class = G_OBJECT_CLASS(class_);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-file-button.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbFileButton, icon);
    gtk_widget_class_bind_template_child(widget_class, GhbFileButton, label);

    props[PROP_ACTION] = g_param_spec_enum("action",
                                           "Action",
                                           "The selection mode for the file button",
                                           GTK_TYPE_FILE_CHOOSER_ACTION,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    props[PROP_TITLE] = g_param_spec_string("title",
                                           "Title",
                                           "The title to use for the file dialog",
                                           _("Select a File"),
                                           G_PARAM_READWRITE);
    props[PROP_ACCEPT_LABEL] = g_param_spec_string("accept-label",
                                           "Accept label",
                                           "The label to use for the accept button",
                                           NULL,
                                           G_PARAM_READWRITE);
    props[PROP_FILE] = g_param_spec_string("file",
                                           "File",
                                           "The currently selected file or folder",
                                           NULL,
                                           G_PARAM_READWRITE);

    object_class->set_property = ghb_file_button_set_property;
    object_class->get_property = ghb_file_button_get_property;
    object_class->finalize = ghb_file_button_finalize;

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
            g_value_set_string(value, self->title);
            break;

        case PROP_ACCEPT_LABEL:
            g_value_set_string(value, self->accept_label);
            break;

        case PROP_FILE:
            if (self->selected)
            {
                char *filename = g_file_get_path(self->selected);
                g_value_set_string(value, filename);
                g_free(filename);
            }
            break;

        case PROP_ACTION:
            g_value_set_enum(value, self->action);
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
            self->action = g_value_get_enum(value);
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

    g_clear_object(&self->selected);
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

    return self->selected;
}

const GtkFileChooserAction
ghb_file_button_get_action (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), 0);

    return self->action;
}

const char *
ghb_file_button_get_filename (GhbFileButton *self)
{
    g_return_val_if_fail(GHB_IS_FILE_BUTTON(self), NULL);

    if (self->selected)
        return g_file_get_path(self->selected);
    else
        return NULL;
}


GhbFileButton *
ghb_file_button_new (const char *title, GtkFileChooserAction action)
{
    GhbFileButton *button = g_object_new(GHB_TYPE_FILE_BUTTON,
                                         "action", action,
                                         NULL);

    ghb_file_button_set_title(button, title);
    return button;
}

static void
file_icon_query_cb (GFile *file, GAsyncResult *result, GhbFileButton *self)
{
    GFileInfo *info = NULL;
    GIcon *icon = NULL;

    info = g_file_query_info_finish(file, result, NULL);
    if (info != NULL)
        icon = g_file_info_get_icon(info);

    if (icon != NULL)
        gtk_image_set_from_gicon(self->icon, icon, GTK_ICON_SIZE_BUTTON);
    else
        gtk_image_clear(self->icon);

    g_object_unref(info);
}

void
ghb_file_button_set_file (GhbFileButton *self, GFile *file)
{
    char *file_base;

    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    if (file == NULL) return;

    if (self->selected)
        g_object_unref(self->selected);

    self->selected = g_file_dup(file);
    file_base = g_file_get_basename(file);
    gtk_label_set_label(self->label, file_base);
    g_free(file_base);

    g_file_query_info_async(file, G_FILE_ATTRIBUTE_STANDARD_ICON,
                            G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, NULL,
                            (GAsyncReadyCallback) file_icon_query_cb, self);

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_FILE]);
}

void
ghb_file_button_set_filename (GhbFileButton *self, const char *filename)
{
    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    GFile *file = g_file_new_for_path(filename);
    ghb_file_button_set_file(self, file);
    g_object_unref(file);
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
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

G_MODULE_EXPORT void
button_clicked_cb (GhbFileButton *self, gpointer user_data)
{
    GtkWindow *window;
    GtkFileChooserNative *chooser;

    g_return_if_fail(GHB_IS_FILE_BUTTON(self));

    window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(self)));

    chooser = gtk_file_chooser_native_new(self->title, window, self->action,
                                          self->accept_label, NULL);

    g_signal_connect(chooser, "response", G_CALLBACK(chooser_response_cb), self);

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), GTK_IS_WINDOW(window));
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}
