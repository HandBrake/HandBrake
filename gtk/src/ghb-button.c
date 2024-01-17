/* ghb-button.c
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

#include "ghb-button.h"

struct _GhbButton
{
    GtkButton parent_instance;

    GtkBox   *layout_box;
    GtkImage *icon;
    GtkLabel *label;
    GtkLabel *indicator;

    GtkOrientation orientation;
};

G_DEFINE_TYPE (GhbButton, ghb_button, GTK_TYPE_BUTTON)

enum {
    PROP_LABEL = 1,
    PROP_ICON_NAME,
    PROP_INDICATOR,
    PROP_ORIENTATION,
    N_PROPS
};
static GParamSpec *props[N_PROPS] = {NULL};

static void ghb_button_dispose (GObject *object);
static void ghb_button_finalize (GObject *object);
static void ghb_button_get_property (GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec);
static void ghb_button_set_property (GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec);

static void
ghb_button_class_init (GhbButtonClass *class_)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class_);
    GObjectClass *object_class = G_OBJECT_CLASS(class_);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-button.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbButton, layout_box);
    gtk_widget_class_bind_template_child(widget_class, GhbButton, icon);
    gtk_widget_class_bind_template_child(widget_class, GhbButton, label);
    gtk_widget_class_bind_template_child(widget_class, GhbButton, indicator);

    props[PROP_LABEL] = g_param_spec_string("label", NULL, NULL,
                                           "",
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_ICON_NAME] = g_param_spec_string("icon-name", NULL, NULL,
                                           NULL,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_INDICATOR] = g_param_spec_string("indicator-label", NULL, NULL,
                                           NULL,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_ORIENTATION] = g_param_spec_enum("orientation", NULL, NULL,
                                           GTK_TYPE_ORIENTATION,
                                           GTK_ORIENTATION_VERTICAL,
                                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    object_class->set_property = ghb_button_set_property;
    object_class->get_property = ghb_button_get_property;
    object_class->finalize = ghb_button_finalize;
    object_class->dispose = ghb_button_dispose;

    g_object_class_install_properties(object_class, N_PROPS, props);
}

static void
ghb_button_get_property (GObject *object, guint prop_id,
                         GValue *value, GParamSpec *pspec)
{
    GhbButton *self = GHB_BUTTON(object);

    switch (prop_id)
    {
        case PROP_LABEL:
            g_value_set_string(value, ghb_button_get_label(self));
            break;

        case PROP_ICON_NAME:
            g_value_set_string(value, ghb_button_get_icon_name(self));
            break;

        case PROP_INDICATOR:
            g_value_set_string(value, ghb_button_get_indicator_label(self));
            break;

        case PROP_ORIENTATION:
            g_value_set_enum(value, ghb_button_get_orientation(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_button_set_property (GObject *object, guint prop_id,
                         const GValue *value, GParamSpec *pspec)
{
    GhbButton *self = GHB_BUTTON (object);
    g_return_if_fail(GHB_IS_BUTTON(self));

    switch (prop_id)
    {
        case PROP_LABEL:
            ghb_button_set_label(self, g_value_get_string(value));
            break;

        case PROP_ICON_NAME:
            ghb_button_set_icon_name(self, g_value_get_string(value));
            break;

        case PROP_INDICATOR:
            ghb_button_set_indicator_label(self, g_value_get_string(value));
            break;

        case PROP_ORIENTATION:
            ghb_button_set_orientation(self, g_value_get_enum(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_button_init (GhbButton *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
    g_object_bind_property(self, "orientation", self->layout_box, "orientation",
                           G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
ghb_button_dispose (GObject *object)
{
    G_OBJECT_CLASS(ghb_button_parent_class)->dispose(object);
}

static void
ghb_button_finalize (GObject *object)
{
    GhbButton *self = GHB_BUTTON(object);
    g_return_if_fail(GHB_IS_BUTTON(self));

    g_clear_object(&self->icon);
    g_clear_object(&self->label);

    G_OBJECT_CLASS(ghb_button_parent_class)->finalize(object);
}

void
ghb_button_set_icon_name (GhbButton *self, const char *icon_name)
{
    g_return_if_fail(GHB_IS_BUTTON(self));

    gtk_image_set_from_icon_name(self->icon, icon_name);
}

void
ghb_button_set_label (GhbButton *self, const char *label)
{
    g_return_if_fail(GHB_IS_BUTTON(self));

    gtk_label_set_label(self->label, label);
}

void
ghb_button_set_indicator_label (GhbButton *self, const char *label)
{
    g_return_if_fail(GHB_IS_BUTTON(self));

    gtk_widget_set_visible(GTK_WIDGET(self->indicator),
                           (label != NULL && label[0] != '\0'));

    if (label == NULL) label = "";
    gtk_label_set_label(self->indicator, label);
}

void
ghb_button_set_orientation (GhbButton *self, GtkOrientation orientation)
{
    g_return_if_fail(GHB_IS_BUTTON(self));

    self->orientation = orientation;
}

const char *
ghb_button_get_label (GhbButton *self)
{
    g_return_val_if_fail(GHB_IS_BUTTON(self), NULL);

    return gtk_label_get_label(self->label);
}

const char *
ghb_button_get_indicator_label (GhbButton *self)
{
    g_return_val_if_fail(GHB_IS_BUTTON(self), NULL);

    return gtk_label_get_label(self->indicator);
}

const char *
ghb_button_get_icon_name (GhbButton *self)
{
    g_return_val_if_fail(GHB_IS_BUTTON(self), NULL);

    return gtk_image_get_icon_name(self->icon);
}

GtkOrientation
ghb_button_get_orientation (GhbButton *self)
{
    g_return_val_if_fail(GHB_IS_BUTTON(self), 0);

    return self->orientation;
}

GhbButton *
ghb_button_new (void)
{
    GhbButton *button = g_object_new(GHB_TYPE_BUTTON, NULL);
    return button;
}
