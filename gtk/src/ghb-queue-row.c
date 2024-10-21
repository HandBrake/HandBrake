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

#include "ghb-queue-row.h"

#include "application.h"
#include "queuehandler.h"

struct _GhbQueueRow {
    GtkListBoxRow parent_instance;
    GActionMap *actions;

    GtkLabel *dest_label;
    GtkImage *status_icon;
    GtkProgressBar *encode_progress_bar;

    GtkDragSource *drag_source;

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

static GdkContentProvider *
ghb_queue_row_drag_prepare (GtkDragSource *source, double x, double y, GhbQueueRow *self)
{
    g_return_val_if_fail(GHB_IS_QUEUE_ROW(self), NULL);

    return gdk_content_provider_new_typed(GHB_TYPE_QUEUE_ROW, self);
}

static void
ghb_queue_row_drag_end (GtkDragSource* source, GdkDrag* drag, gboolean delete_data, GhbQueueRow *self)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    GtkWidget *list_box = gtk_widget_get_parent(GTK_WIDGET(self));
    g_object_set_data(G_OBJECT(list_box), "drag-row", NULL);
}

static void
ghb_queue_row_drag_begin (GtkDragSource *source, GdkDrag *drag, GhbQueueRow *self)
{
    g_return_if_fail(GHB_IS_QUEUE_ROW(self));

    GtkWidget *list_box = gtk_widget_get_parent(GTK_WIDGET(self));
    g_object_set_data(G_OBJECT(list_box), "drag-row", self);
    if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(self)))
    {
        gtk_list_box_unselect_all(GTK_LIST_BOX(list_box));
        gtk_list_box_select_row(GTK_LIST_BOX(list_box), GTK_LIST_BOX_ROW(self));
    }
    GdkPaintable *paintable = gtk_widget_paintable_new(GTK_WIDGET(self));
    gtk_drag_source_set_icon(source, paintable, 0, 0);
    g_object_unref(paintable);
}

static void
ghb_queue_row_class_init (GhbQueueRowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-queue-row.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, dest_label);
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, status_icon);
    gtk_widget_class_bind_template_child(widget_class, GhbQueueRow, encode_progress_bar);

    props[PROP_DEST] = g_param_spec_string("dest", NULL, NULL,
            "",
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_STATUS] = g_param_spec_int("status" ,NULL, NULL,
            0, 10, 0,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_PROGRESS] = g_param_spec_double("progress", NULL, NULL,
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
            g_value_set_string(value, ghb_queue_row_get_destination(self));
            break;

        case PROP_STATUS:
            g_value_set_int(value, ghb_queue_row_get_status(self));
            break;

        case PROP_PROGRESS:
            g_value_set_double(value, ghb_queue_row_get_progress(self));
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
            ghb_queue_row_set_destination(self, g_value_get_string(value));
            break;

        case PROP_STATUS:
            ghb_queue_row_set_status(self, g_value_get_int(value));
            break;

        case PROP_PROGRESS:
            ghb_queue_row_set_progress(self, g_value_get_double(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_queue_row_init (GhbQueueRow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    self->drag_source = gtk_drag_source_new();
    gtk_drag_source_set_actions (self->drag_source, GDK_ACTION_MOVE);

    g_signal_connect(self->drag_source, "prepare", G_CALLBACK(ghb_queue_row_drag_prepare), self);
    g_signal_connect(self->drag_source, "drag-begin", G_CALLBACK(ghb_queue_row_drag_begin), self);
    g_signal_connect(self->drag_source, "drag-end", G_CALLBACK(ghb_queue_row_drag_end), self);

    gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(self->drag_source));
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
ghb_queue_row_new (const char *dest, int status)
{
    GhbQueueRow *row;

    row = g_object_new(GHB_TYPE_QUEUE_ROW, NULL);
    ghb_queue_row_set_destination(row, dest);

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
    const char *icon_name, *accessible_name;
    gboolean show_progress_bar = FALSE;
    switch (status)
    {
        case GHB_QUEUE_RUNNING:
            icon_name = "hb-running-symbolic";
            accessible_name = _("Running Queue Item");
            show_progress_bar = TRUE;
            break;
        case GHB_QUEUE_PENDING:
            icon_name = "hb-ready-symbolic";
            accessible_name = _("Pending Queue Item");
            break;
        case GHB_QUEUE_FAIL:
            icon_name = "hb-error";
            accessible_name = _("Failed Queue Item");
            break;
        case GHB_QUEUE_CANCELED:
            icon_name = "hb-cancelled";
            accessible_name = _("Cancelled Queue Item");
            break;
        case GHB_QUEUE_DONE:
            icon_name = "hb-complete";
            accessible_name = _("Completed Queue Item");
            break;
        default:
            icon_name = "hb-ready-symbolic";
            accessible_name = _("Pending Queue Item");
            break;
    }
    gtk_image_set_from_icon_name(self->status_icon, icon_name);
    gtk_accessible_update_property(GTK_ACCESSIBLE(self->status_icon),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL, accessible_name, -1);
    gtk_widget_set_visible(GTK_WIDGET(self->encode_progress_bar), show_progress_bar);
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

    ghb_queue_row_remove(self);
}
