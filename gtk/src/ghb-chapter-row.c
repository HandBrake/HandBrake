/* ghb-chapter-row.c
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

#include "ghb-chapter-row.h"

struct _GhbChapterRow {
    GtkListBoxRow parent_instance;

    GtkLabel *index_label;
    GtkLabel *start_label;
    GtkLabel *duration_label;
    GtkEntry *chapter_entry;

    int idx;
    int64_t start;
    int64_t duration;
};

G_DEFINE_TYPE(GhbChapterRow, ghb_chapter_row, GTK_TYPE_LIST_BOX_ROW)

enum {
    PROP_INDEX = 1,
    PROP_START,
    PROP_DURATION,
    PROP_NAME,
    N_PROPS
};
static GParamSpec *props[N_PROPS] = {NULL};

static void ghb_chapter_row_finalize(GObject *object);
static void ghb_chapter_row_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec);
static void ghb_chapter_row_set_property(GObject *object, guint prop_id,
                                         const GValue *value, GParamSpec *pspec);
G_MODULE_EXPORT void chapter_name_changed_cb(GhbChapterRow *self,
                                             GtkEntry *chapter_entry);

static void
ghb_chapter_row_class_init (GhbChapterRowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class, "/fr/handbrake/ghb/ui/ghb-chapter-row.ui");
    gtk_widget_class_bind_template_child(widget_class, GhbChapterRow, index_label);
    gtk_widget_class_bind_template_child(widget_class, GhbChapterRow, start_label);
    gtk_widget_class_bind_template_child(widget_class, GhbChapterRow, duration_label);
    gtk_widget_class_bind_template_child(widget_class, GhbChapterRow, chapter_entry);

    props[PROP_INDEX] = g_param_spec_int("index", NULL, NULL,
                                         1, INT_MAX, 1,
                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_START] = g_param_spec_int64("start", NULL, NULL,
                                         0, INT64_MAX, 0,
                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_DURATION] = g_param_spec_int64("duration", NULL, NULL,
                                         0, INT64_MAX, 0,
                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    props[PROP_NAME] = g_param_spec_string("name", NULL, NULL,
                                         "",
                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    object_class->set_property = ghb_chapter_row_set_property;
    object_class->get_property = ghb_chapter_row_get_property;
    object_class->finalize = ghb_chapter_row_finalize;

    g_object_class_install_properties(object_class, N_PROPS, props);
}

static void
ghb_chapter_row_get_property (GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec)
{
    GhbChapterRow *self = GHB_CHAPTER_ROW(object);

    switch (prop_id)
    {
        case PROP_INDEX:
            g_value_set_int(value, ghb_chapter_row_get_index(self));
            break;

        case PROP_START:
            g_value_set_int64(value, ghb_chapter_row_get_start(self));
            break;

        case PROP_DURATION:
            g_value_set_int64(value, ghb_chapter_row_get_duration(self));
            break;

        case PROP_NAME:
            g_value_set_string(value, ghb_chapter_row_get_name(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_chapter_row_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
    GhbChapterRow *self = GHB_CHAPTER_ROW(object);
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    switch (prop_id)
    {
        case PROP_INDEX:
            ghb_chapter_row_set_index(self, g_value_get_int(value));
            break;

        case PROP_START:
            ghb_chapter_row_set_start(self, g_value_get_int64(value));
            break;

        case PROP_DURATION:
            ghb_chapter_row_set_duration(self, g_value_get_int64(value));
            break;

        case PROP_NAME:
            ghb_chapter_row_set_name(self, g_value_get_string(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_chapter_row_init (GhbChapterRow *self)
{
    gtk_widget_init_template(GTK_WIDGET (self));
}

static void
ghb_chapter_row_finalize (GObject *object)
{
    GhbChapterRow *self = GHB_CHAPTER_ROW(object);
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    g_clear_object(&self->index_label);
    g_clear_object(&self->start_label);
    g_clear_object(&self->duration_label);
    g_clear_object(&self->chapter_entry);

    G_OBJECT_CLASS(ghb_chapter_row_parent_class)->finalize (object);
}

int
ghb_chapter_row_get_index (GhbChapterRow *self)
{
    g_return_val_if_fail(GHB_IS_CHAPTER_ROW(self), 0);

    return self->idx;
}

int64_t
ghb_chapter_row_get_start (GhbChapterRow *self)
{
    g_return_val_if_fail(GHB_IS_CHAPTER_ROW(self), 0);

    return self->start;
}

int64_t
ghb_chapter_row_get_duration (GhbChapterRow *self)
{
    g_return_val_if_fail(GHB_IS_CHAPTER_ROW(self), 0);

    return self->duration;
}

const char *
ghb_chapter_row_get_name (GhbChapterRow *self)
{
    g_return_val_if_fail(GHB_IS_CHAPTER_ROW(self), 0);

    return gtk_editable_get_text(GTK_EDITABLE(self->chapter_entry));
}

GtkWidget *
ghb_chapter_row_new (int idx, int64_t start, int64_t duration,
                     const char *chapter_name)
{
    GhbChapterRow *row;

    if (chapter_name == NULL)
        chapter_name = "";

    row = g_object_new(GHB_TYPE_CHAPTER_ROW, NULL);
    ghb_chapter_row_set_index(row, idx);
    ghb_chapter_row_set_start(row, start);
    ghb_chapter_row_set_duration(row, duration);
    ghb_chapter_row_set_name(row, chapter_name);

    return GTK_WIDGET(row);
}

static void
break_duration (int64_t duration, int *hh, int *mm, int *ss)
{
    *hh = duration / (60*60);
    *mm = (duration / 60) % 60;
    *ss = duration % 60;
}

void
ghb_chapter_row_set_index (GhbChapterRow *self, int idx)
{
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    self->idx = idx;
    char *str = g_strdup_printf("%d", idx);
    gtk_label_set_label(self->index_label, str);
    g_free(str);

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_INDEX]);
}

void
ghb_chapter_row_set_start (GhbChapterRow *self, int64_t start)
{
    char *str;
    int hh, mm, ss;

    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    self->start = start;
    break_duration(start, &hh, &mm, &ss);
    str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
    gtk_label_set_label(self->start_label, str);
    g_free(str);

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_START]);
}

void
ghb_chapter_row_set_duration (GhbChapterRow *self, int64_t duration)
{
    char *str;
    int hh, mm, ss;

    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    self->duration = duration;
    break_duration(duration, &hh, &mm, &ss);
    str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
    gtk_label_set_label(self->duration_label, str);
    g_free(str);

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_DURATION]);
}

void
ghb_chapter_row_set_name (GhbChapterRow *self, const char *name)
{
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    if (name == NULL)
        name = "";

    gtk_editable_set_text(GTK_EDITABLE(self->chapter_entry), name);
}

void ghb_chapter_row_grab_focus (GhbChapterRow *self)
{
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    gtk_widget_grab_focus(GTK_WIDGET(self->chapter_entry));
}

G_MODULE_EXPORT void
chapter_name_changed_cb (GhbChapterRow *self, GtkEntry *chapter_entry)
{
    g_return_if_fail(GHB_IS_CHAPTER_ROW(self));

    g_object_notify_by_pspec(G_OBJECT(self), props[PROP_NAME]);
}
