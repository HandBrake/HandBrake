/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * chapters.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * chapters.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * chapters.c is distributed in the hope that it will be useful,
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

#include "ghbcompat.h"
#include "hb-backend.h"
#include "callbacks.h"
#include "jobdict.h"

static void
chapter_changed_cb(GtkEditable * edit, signal_user_data_t *ud);

#if GTK_CHECK_VERSION(3, 90, 0)
static gboolean
chapter_keypress_cb(
    GtkEventController * keycon,
    guint                keyval,
    guint                keycode,
    GdkModifierType      state,
    signal_user_data_t * ud);
#else
static gboolean
chapter_keypress_cb(
    GtkWidget          * widget,
    GdkEvent           * event,
    signal_user_data_t * ud);
#endif

static GtkWidget *find_widget(GtkWidget *widget, gchar *name)
{
    const char *wname;
    GtkWidget *result = NULL;

    if (widget == NULL || name == NULL)
        return NULL;

    wname = gtk_widget_get_name(widget);
    if (wname != NULL && !strncmp(wname, name, 80))
    {
        return widget;
    }
    if (GTK_IS_CONTAINER(widget))
    {
        GList *list, *link;
        link = list = gtk_container_get_children(GTK_CONTAINER(widget));
        while (link)
        {
            result = find_widget(GTK_WIDGET(link->data), name);
            if (result != NULL)
                break;
            link = link->next;
        }
        g_list_free(list);
    }
    return result;
}

static GtkListBoxRow *
list_box_get_row(GtkWidget *widget)
{
    while (widget != NULL && G_OBJECT_TYPE(widget) != GTK_TYPE_LIST_BOX_ROW)
    {
        widget = gtk_widget_get_parent(widget);
    }
    return GTK_LIST_BOX_ROW(widget);
}

static GtkWidget *
create_chapter_row(int index, int64_t start, int64_t duration,
                   const char * name, signal_user_data_t * ud)
{
    GtkWidget          * entry;
    GtkWidget          * row;
    GtkBox             * hbox;
    GtkWidget          * label;
    gchar              * str;
    gint                 hh, mm, ss;

    row  = gtk_list_box_row_new();
    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));

    str = g_strdup_printf("%d", index);
    label = gtk_label_new(str);
    free(str);
    gtk_label_set_width_chars(GTK_LABEL(label), 5);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    ghb_box_append_child(hbox, label);

    ghb_break_duration(start, &hh, &mm, &ss);
    str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
    label = gtk_label_new(str);
    free(str);
    gtk_label_set_width_chars(GTK_LABEL(label), 10);
    gtk_label_set_xalign(GTK_LABEL(label), 1);
    ghb_box_append_child(hbox, label);

    ghb_break_duration(duration, &hh, &mm, &ss);
    str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
    label = gtk_label_new(str);
    free(str);
    gtk_label_set_width_chars(GTK_LABEL(label), 10);
    gtk_label_set_xalign(GTK_LABEL(label), 1);
    ghb_box_append_child(hbox, label);

#if GTK_CHECK_VERSION(3, 90, 0)
    entry = gtk_text_new();
#else
    entry = gtk_entry_new();
#endif
    gtk_widget_set_name(entry, "chapter_entry");
    gtk_widget_set_margin_start(entry, 12);
    gtk_widget_set_hexpand(entry, TRUE);
    ghb_editable_set_text(entry, name);
    ghb_box_append_child(hbox, entry);

#if GTK_CHECK_VERSION(3, 90, 0)
    GtkEventController * econ;

    econ = gtk_event_controller_key_new();
    gtk_widget_add_controller(entry, econ);

    g_signal_connect(econ, "key-pressed", G_CALLBACK(chapter_keypress_cb), ud);
#else
    g_signal_connect(entry, "key-press-event", G_CALLBACK(chapter_keypress_cb), ud);
#endif
    g_signal_connect(entry, "changed", G_CALLBACK(chapter_changed_cb), ud);

    gtk_container_add(GTK_CONTAINER(row), GTK_WIDGET(hbox));
#if !GTK_CHECK_VERSION(3, 90, 0)
    gtk_widget_show_all(row);
#endif

    return row;
}

static void
ghb_clear_chapter_list_ui(signal_user_data_t * ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "chapters_list"));
    while ((row = gtk_list_box_get_row_at_index(lb, 0)) != NULL)
    {
        gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(row));
    }
}

static void
chapter_refresh_list_ui(signal_user_data_t *ud)
{
    GhbValue   * chapter_list;
    GtkListBox * lb;
    GtkWidget  * row;
    gint         ii, count;
    int64_t      start = 0, duration;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "chapters_list"));

    chapter_list = ghb_get_job_chapter_list(ud->settings);
    count = ghb_array_len(chapter_list);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue   * chapter_dict;
        GhbValue   * duration_dict;
        const char * name;

        chapter_dict  = ghb_array_get(chapter_list, ii);
        name          = ghb_dict_get_string(chapter_dict, "Name");
        duration_dict = ghb_dict_get(chapter_dict, "Duration");
        duration      = ghb_dict_get_int(duration_dict, "Ticks") / 90000;
        row           = create_chapter_row(ii + 1, start, duration, name, ud);
        start        += duration;

        gtk_list_box_insert(lb, row, -1);
    }
}

void
ghb_chapter_list_refresh_all(signal_user_data_t *ud)
{
    ghb_clear_chapter_list_ui(ud);
    chapter_refresh_list_ui(ud);
}

static gboolean
chapter_keypress(
    GtkWidget          * widget,
    guint                keyval,
    signal_user_data_t * ud)
{
    GtkWidget     * entry;
    GtkListBoxRow * row;
    GtkListBox    * lb;
    int             index;

    if (keyval != GDK_KEY_Return &&
        keyval != GDK_KEY_Down &&
        keyval != GDK_KEY_Up)
    {
        return FALSE;
    }

    row    = list_box_get_row(widget);
    lb     = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(row)));
    index  = gtk_list_box_row_get_index(row);
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_Down)
    {
        index++;
    }
    else if (keyval == GDK_KEY_Up && index > 0)
    {
        index--;
    }
    if (index >= 0)
    {
        row = gtk_list_box_get_row_at_index(lb, index);
        if (row != NULL)
        {
            entry  = find_widget(GTK_WIDGET(row), "chapter_entry");
            if (entry != NULL)
            {
                gtk_widget_grab_focus(entry);
                return TRUE;
            }
        }
    }
    return FALSE;
}

#if GTK_CHECK_VERSION(3, 90, 0)
static gboolean
chapter_keypress_cb(
    GtkEventController * keycon,
    guint                keyval,
    guint                keycode,
    GdkModifierType      state,
    signal_user_data_t * ud)
{
    GtkWidget     * widget;

    widget = gtk_event_controller_get_widget(keycon);
    return chapter_keypress(widget, keyval, ud);
}
#else
static gboolean
chapter_keypress_cb(
    GtkWidget          * widget,
    GdkEvent           * event,
    signal_user_data_t * ud)
{
    guint keyval;

    ghb_event_get_keyval(event, &keyval);
    return chapter_keypress(widget, keyval, ud);
}
#endif

static void
chapter_changed_cb(
    GtkEditable * edit,
    signal_user_data_t *ud)
{
    GtkListBoxRow * row;
    const char    * text;
    int             index;

    row = list_box_get_row(GTK_WIDGET(edit));
    if (row == NULL)
    {
        return;
    }
    index = gtk_list_box_row_get_index(row);
    text  = ghb_editable_get_text(edit);
    if (text == NULL)
    {
        return;
    }

    const GhbValue * chapter_list;
    GhbValue       * chapter_dict;

    chapter_list = ghb_get_job_chapter_list(ud->settings);
    chapter_dict = ghb_array_get(chapter_list, index);
    if (chapter_dict == NULL)
    {
        return;
    }
    ghb_dict_set_string(chapter_dict, "Name", text);
}

