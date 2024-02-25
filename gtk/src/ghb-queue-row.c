/* ghb-queue-row.c
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

#include "compat.h"
#include "ghb-queue-row.h"
#include "queuehandler.h"

struct _GhbQueueRow {
    GtkListBoxRow parent_instance;
    GActionMap *actions;

    signal_user_data_t *ud;

    GtkLabel *dest_label;
    GtkImage *status_icon;
    GtkProgressBar *encode_progress_bar;

    char *destination;
    int status;
};

G_DEFINE_TYPE(GhbQueueRow, ghb_queue_row, GTK_TYPE_LIST_BOX_ROW)

enum {
    PROP_DEST = 1,
    PROP_STATUS,
    PROP_PROGRESS,
    N_PROPS
};
static GParamSpec *props[N_PROPS] = { NULL };

static void ghb_queue_row_delete_action(GSimpleAction *action, GVariant *param, gpointer data);
static void ghb_queue_row_dispose(GObject *object);
static void ghb_queue_row_finalize(GObject *object);
static void ghb_queue_row_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec);
static void ghb_queue_row_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec);

static GActionEntry action_entries[] = {
    {"delete", ghb_queue_row_delete_action, NULL, NULL, NULL},
};

static void
ghb_queue_row_class_init (GhbQueueRowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-queue-row.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, dest_label);
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, status_icon);
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, encode_progress_bar);

    props[PROP_DEST] = g_param_spec_string("dest",
            "Destination",
            "The destination file name",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
    props[PROP_STATUS] = g_param_spec_int("status",
            "Status",
            "The status of the queue item",
            0, 10, 0,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_PROGRESS] = g_param_spec_double("progress",
            "Progress",
            "The progress of the encode in percent",
            0, 100.0f, 0,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    object_class->set_property = ghb_queue_row_set_property;
    object_class->get_property = ghb_queue_row_get_property;
    object_class->dispose = ghb_queue_row_dispose;
    object_class->finalize = ghb_queue_row_finalize;

    g_object_class_install_properties(object_class, N_PROPS, props);
}

static void
ghb_queue_row_get_property (GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
    GhbQueueRow *self = GHB_QUEUE_ROW(object);

    switch (prop_id)
    {
        case PROP_DEST:
            g_value_set_string(value, self->destination);
            break;

        case PROP_STATUS:
            break;

        case PROP_PROGRESS:
            g_value_set_double(value, gtk_progress_bar_get_fraction(self->encode_progress_bar));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_queue_row_set_property (GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
    GhbQueueRow *self = GHB_QUEUE_ROW(object);
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    switch (prop_id)
    {
        case PROP_DEST:
            self->destination = g_strdup(g_value_get_string(value));
            break;

        case PROP_STATUS:
            break;

        case PROP_PROGRESS:
            gtk_progress_bar_set_fraction(self->encode_progress_bar, g_value_get_double(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_queue_row_init (GhbQueueRow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
}

static void
ghb_queue_row_dispose (GObject *object)
{
    GhbQueueRow *self = GHB_QUEUE_ROW(object);
    g_clear_object(&self->actions);

    G_OBJECT_CLASS(ghb_queue_row_parent_class)->dispose(object);
}

static void
ghb_queue_row_finalize (GObject *object)
{
    GhbQueueRow *self = GHB_QUEUE_ROW(object);
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    g_clear_object(&self->dest_label);
    g_clear_object(&self->encode_progress_bar);
    g_clear_object(&self->status_icon);
    g_free(self->destination);

    G_OBJECT_CLASS(ghb_queue_row_parent_class)->finalize(object);
}

const char *
ghb_queue_row_get_destination (GhbQueueRow *self)
{
    g_return_val_if_fail(GHB_IS_QUEUE_ROW(self), 0);

    return self->destination;
}

GtkWidget *
ghb_queue_row_new (const char *dest, int status, signal_user_data_t *ud)
{
    GhbQueueRow *row;

    row = g_object_new(GHB_TYPE_QUEUE_ROW, NULL);
    ghb_queue_row_set_destination(row, dest);

    row->ud = ud;

    row->actions = G_ACTION_MAP(g_simple_action_group_new());
    g_action_map_add_action_entries(row->actions, action_entries,
                                    G_N_ELEMENTS(action_entries), row);

    gtk_widget_insert_action_group(GTK_WIDGET(row), "queue",
                                   G_ACTION_GROUP(row->actions));

    return GTK_WIDGET(row);
}

double
ghb_queue_row_get_progress (GhbQueueRow *self)
{
    g_return_val_if_fail(GHB_IS_QUEUE_ROW(self), 0.0);

    return gtk_progress_bar_get_fraction(self->encode_progress_bar);
}

void
ghb_queue_row_set_status (GhbQueueRow *self, int status)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    self->status = status;
    const char *icon_name;
    switch (status)
    {
        case GHB_QUEUE_RUNNING:
            icon_name = "hb-start";
            break;
        case GHB_QUEUE_PENDING:
            icon_name = "hb-source";
            break;
        case GHB_QUEUE_FAIL:
        case GHB_QUEUE_CANCELED:
            icon_name = "hb-stop";
            break;
        case GHB_QUEUE_DONE:
            icon_name = "hb-complete";
            break;
        default:
            icon_name = "document-edit";
            break;
    }

    ghb_image_set_from_icon_name(self->status_icon, icon_name,
                                 GHB_ICON_SIZE_BUTTON);
}

int
ghb_queue_row_get_status (GhbQueueRow *self)
{
    g_return_val_if_fail(GHB_IS_QUEUE_ROW(self), 0.0);

    return self->status;
}

void
ghb_queue_row_set_progress (GhbQueueRow *self, double fraction)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    gtk_progress_bar_set_fraction(self->encode_progress_bar, fraction);
}

void
ghb_queue_row_set_progress_bar_visible (GhbQueueRow *self, gboolean visible)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    gtk_widget_set_visible(GTK_WIDGET(self->encode_progress_bar), visible);
}

void
ghb_queue_row_set_destination (GhbQueueRow *self, const char *dest)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    if (dest == NULL) dest = "";

    g_autofree char *basename = g_path_get_basename(dest);
    self->destination = g_strdup(dest);
    gtk_label_set_text(self->dest_label, basename);
}

static void
ghb_queue_row_delete_action (GSimpleAction *action, GVariant *param,
                             gpointer data)
{
    GhbQueueRow *self = data;

    g_return_if_fail(GHB_IS_QUEUE_ROW(self));
    ghb_queue_row_remove(self, self->ud);
}