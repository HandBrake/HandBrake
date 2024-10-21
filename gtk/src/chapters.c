/* chapters.c
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "chapters.h"

#include "application.h"
#include "callbacks.h"
#include "ghb-chapter-row.h"
#include "hb-backend.h"
#include "jobdict.h"
#include "presets.h"
#include "util.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <math.h>

static gboolean chapter_keypress_cb (GtkEventController * keycon, guint keyval,
                                     guint keycode, GdkModifierType state,
                                     gpointer user_data);
static void chapter_changed_cb (GhbChapterRow * row, GParamSpec *pspec,
                                signal_user_data_t *ud);

static GtkWidget *
create_chapter_row (int index, gint64 start, gint64 duration,
                    const char * name, signal_user_data_t * ud)
{
    GtkWidget *row = ghb_chapter_row_new(index, start, duration, name);
    g_signal_connect(row, "notify::name", G_CALLBACK(chapter_changed_cb), ud);

    GtkEventController *econ = gtk_event_controller_key_new();
    gtk_widget_add_controller(row, econ);
    g_signal_connect(econ, "key-pressed", G_CALLBACK(chapter_keypress_cb), ud);

    gtk_widget_show(row);
    return row;
}

static gboolean
chapter_keypress (GtkWidget *widget, guint keyval, gpointer user_data)
{
    GtkListBoxRow *row;
    GtkListBox *lb;
    int index;

    if (keyval != GDK_KEY_Return &&
        keyval != GDK_KEY_Tab &&
        keyval != GDK_KEY_Down &&
        keyval != GDK_KEY_Up)
    {
        return FALSE;
    }

    row = GTK_LIST_BOX_ROW(widget);
    lb = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(row)));
    index = gtk_list_box_row_get_index(row);
    if (keyval == GDK_KEY_Return ||
        keyval == GDK_KEY_Down ||
        keyval == GDK_KEY_Tab)
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
            ghb_chapter_row_grab_focus(GHB_CHAPTER_ROW(row));
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean
chapter_keypress_cb (GtkEventController * keycon, guint keyval, guint keycode,
                     GdkModifierType state, gpointer user_data)
{
    GtkWidget *widget;

    widget = gtk_event_controller_get_widget(keycon);
    return chapter_keypress(widget, keyval, user_data);
}

static void
ghb_clear_chapter_list_ui(signal_user_data_t * ud)
{
    GtkListBox    * lb;

    lb = GTK_LIST_BOX(ghb_builder_widget("chapters_list"));
    ghb_list_box_remove_all(lb);
}

static void
chapter_refresh_list_ui(signal_user_data_t *ud)
{
    GhbValue   * chapter_list;
    GtkListBox * lb;
    GtkWidget  * row;
    gint         ii, count;
    gint64       start = 0, duration;

    lb = GTK_LIST_BOX(ghb_builder_widget("chapters_list"));

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

static void
chapter_changed_cb (GhbChapterRow *row, GParamSpec *pspec, signal_user_data_t *ud)
{
    const char    * text;
    int             index;

    index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));
    text  = ghb_chapter_row_get_name(row);
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

static char *
pts_to_xml_time (gint64 pts)
{
    int hrs, mins;
    double fsecs;
    gchar *str;

    hrs = pts / (90000 * 3600);
    mins = (pts % (90000 * 3600)) / (90000 * 60);
    fsecs = (pts % (90000 * 60)) / 90000.0;
    str = g_strdup_printf("%02d:%02d:%06.03f000000", hrs, mins, fsecs);
    return str;
}

static char *
chapter_time_end (GhbValue *chapter, gint64 *time_pts)
{
    GhbValue *duration;
    gint64 pts;

    duration = ghb_dict_get(chapter, "Duration");
    pts = ghb_dict_get_int(duration, "Ticks");

    *time_pts += pts;
    return pts_to_xml_time(*time_pts);
}

static void
chapter_list_export_xml (const gchar *filename, GhbValue *chapter_dict)
{
    GhbValue *chapter;
    xmlDocPtr doc;
    xmlNodePtr node, node2, node3;
    int n_chapters;
    gint64 time_pts = 0;

    n_chapters = ghb_array_len(chapter_dict);
    doc = xmlNewDoc((const xmlChar *) "1.0");
    xmlCreateIntSubset(doc, (const xmlChar *) "Chapters", NULL, (const xmlChar *) "matroskachapters.dtd");
    node = xmlNewDocNode(doc, NULL, (const xmlChar *) "Chapters", NULL);
    xmlDocSetRootElement(doc, node);
    node = xmlNewChild(node, NULL, (const xmlChar *) "EditionEntry", NULL);
    for (int index = 0; index < n_chapters; index++)
    {
        chapter = ghb_array_get(chapter_dict, index);
        node2 = xmlNewChild(node, NULL, (const xmlChar *) "ChapterAtom", NULL);
        xmlNewChild(node2, NULL, (const xmlChar *) "ChapterTimeStart", (const xmlChar *) pts_to_xml_time(time_pts));
        xmlNewChild(node2, NULL, (const xmlChar *) "ChapterTimeEnd", (const xmlChar *) chapter_time_end(chapter, &time_pts));
        node3 = xmlNewChild(node2, NULL, (const xmlChar *) "ChapterDisplay", NULL);
        xmlNewChild(node3, NULL, (const xmlChar *) "ChapterString", (const xmlChar *) ghb_dict_get_string(chapter, "Name"));
    }
    xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
}

static void
chapter_list_export (GtkFileChooser *chooser,
                     GtkResponseType response, signal_user_data_t *ud)
{
    gchar             *filename, *dir;
    const gchar       *exportDir;
    GhbValue          *chapter_list;

    if (response == GTK_RESPONSE_ACCEPT)
    {

        chapter_list = ghb_get_job_chapter_list(ud->settings);

        if (chapter_list == NULL)
        {
            ghb_file_chooser_destroy(chooser);
            return;
        }

        filename = ghb_file_chooser_get_filename(chooser);

        chapter_list_export_xml(filename, chapter_list);
        exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
        dir = g_path_get_dirname(filename);
        if (strcmp(dir, exportDir) != 0)
        {
            ghb_dict_set_string(ud->prefs, "ExportDirectory", dir);
            ghb_pref_save(ud->prefs, "ExportDirectory");
        }
        g_free(dir);
        g_free(filename);
    }
    ghb_file_chooser_destroy(chooser);
}

static xmlNodePtr
xml_get_node (xmlNodePtr node, const char *child_name)
{
    node = node->xmlChildrenNode;
    while (node != NULL)
    {
        if ((!xmlStrcmp(node->name, (const xmlChar *) child_name)))
        {
            return node;
        }
        node = node->next;
    }
    g_debug("Could not find node %s", child_name);
    return NULL;
}

static char *
xml_get_data (xmlDocPtr doc, xmlNodePtr node, const char *name)
{
    char *str = NULL;

    if (node != NULL && (node = xml_get_node(node, name)))
    {
        str = (char *) xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
    }
    return str;
}

static gboolean
xml_get_bool (xmlDocPtr doc, xmlNodePtr node,
              const char *name, gboolean default_value)
{
    gboolean res = FALSE;
    char *str = xml_get_data(doc, node, name);

    if (str == NULL)
        res = default_value;
    else if (!g_ascii_strcasecmp(str, "true"))
        res = TRUE;
    else if (!g_ascii_strcasecmp(str, "yes"))
        res = TRUE;
    else if (atoi(str) > 0)
        res = TRUE;

    xmlFree(str);
    return res;
}

static xmlNodePtr
xml_get_child_node (xmlNodePtr node, const char *child_name)
{
    node = node->xmlChildrenNode;
    while (node != NULL)
    {
        if ((!xmlStrcmp(node->name, (const xmlChar *) child_name)))
        {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static xmlXPathObjectPtr
xml_get_all_paths (xmlDocPtr doc, xmlNodePtr node, const char *path)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (node)
        xmlXPathSetContextNode(node, context);

    result = xmlXPathEvalExpression((const xmlChar *) path, context);
    xmlXPathFreeContext(context);
    if (result == NULL)
    {
        g_debug("xml: Error in xmlXPathEvalExpression");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        g_debug("xml: Node set is empty");
        xmlXPathFreeObject(result);
        return NULL;
    }
    return result;
}

static gboolean
chapter_list_import_xml (const gchar *file, signal_user_data_t *ud)
{
    xmlDocPtr doc;
    xmlNodePtr node;
    xmlXPathObjectPtr chapter_list;
    xmlNodeSetPtr nodeset;
    char *name;
    GhbValue *chapter_array, *chapter, *dest;

    doc = xmlParseFile(file);
    if (!doc)
    {
        ghb_log("Could not open file %s", file);
        return FALSE;
    }
    node = xmlDocGetRootElement(doc);
    if (node == NULL || xmlStrcmp(node->name, (const xmlChar *) "Chapters"))
    {
        ghb_log("%s is not a valid Matroska XML file.", file);
        xmlFreeDoc(doc);
        return FALSE;
    }
    node = node->xmlChildrenNode;
    while (node != NULL)
    {
        // Get the EditionEntry with the default flag
        if (!xmlStrcmp(node->name, (const xmlChar *) "EditionEntry")
             && xml_get_bool(doc, node, "EditionFlagDefault", FALSE))
            break;
        node = node->next;
    }
    if (node == NULL)
    {
        // Get the first EditionEntry
        node = xmlDocGetRootElement(doc);
        node = xml_get_child_node(node, "EditionEntry");
    }
    chapter_list = xml_get_all_paths(doc, node, "./ChapterAtom");
    if (chapter_list == NULL)
    {
        ghb_log("%s has no chapters.", file);
        xmlFreeDoc(doc);
        return FALSE;
    }
    dest = ghb_get_job_dest_settings(ud->settings);
    chapter_array = ghb_dict_get(dest, "ChapterList");
    nodeset = chapter_list->nodesetval;
    for (int i=0; i < nodeset->nodeNr; i++)
    {
        if (!xml_get_bool(doc, nodeset->nodeTab[i], "ChapterFlagEnabled", TRUE))
            continue; // Skip disabled chapters

        chapter = ghb_array_get(chapter_array, i);
        if (chapter == NULL)
            break;

        node = xml_get_child_node(nodeset->nodeTab[i], "ChapterDisplay");
        name = xml_get_data(doc, node, "ChapterString");

        if (name != NULL)
        {
            ghb_dict_set_string(chapter, "Name", name);
            xmlFree(name);
        }
    }
    ghb_chapter_list_refresh_all(ud);
    xmlFree(chapter_list);
    xmlFreeDoc(doc);
    return TRUE;
}

static void
chapters_import_response_cb (GtkFileChooser *dialog,
                             GtkResponseType response,
                             signal_user_data_t *ud)
{
    char *filename;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        filename = ghb_file_chooser_get_filename(dialog);
        chapter_list_import_xml(filename, ud);
        g_free(filename);
    }
    ghb_file_chooser_destroy(dialog);
}

G_MODULE_EXPORT void
chapters_export_action_cb (GSimpleAction *action, GVariant *param,
                           signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkFileChooser  *dialog;
    const gchar     *exportDir;
    GtkFileFilter   *filter;

    hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));
    dialog = ghb_file_chooser_new("Export Chapters", hb_window,
                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                  _("_Save"),
                                  _("_Cancel"));

    ghb_add_file_filter(dialog, _("All Files"), "FilterAll");
    filter = ghb_add_file_filter(dialog, _("Chapters (*.xml)"), "FilterXML");
    gtk_file_chooser_set_filter(dialog, filter);
    gtk_file_chooser_set_current_name(dialog, "chapters.xml");

    exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
    if (exportDir && exportDir[0] != '\0')
        ghb_file_chooser_set_initial_file(dialog, exportDir);

    ghb_file_chooser_set_modal(dialog, TRUE);
    g_signal_connect(dialog, "response", G_CALLBACK(chapter_list_export), ud);
    ghb_file_chooser_show(dialog);
}

G_MODULE_EXPORT void
chapters_import_action_cb (GSimpleAction *action, GVariant *param,
                           signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkFileChooser  *dialog;
    GtkFileFilter   *filter;

    hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));
    dialog = ghb_file_chooser_new("Import Chapters", hb_window,
                                  GTK_FILE_CHOOSER_ACTION_OPEN,
                                  _("_Open"),
                                  _("_Cancel"));

    ghb_add_file_filter(dialog, _("All Files"), "FilterAll");
    filter = ghb_add_file_filter(dialog, _("Chapters (*.xml)"), "FilterXML");
    gtk_file_chooser_set_filter(dialog, filter);
    ghb_file_chooser_set_initial_file(dialog,
                                      ghb_dict_get_string(ud->prefs, "ExportDirectory"));

    ghb_file_chooser_set_modal(dialog, TRUE);
    g_signal_connect(dialog, "response", G_CALLBACK(chapters_import_response_cb), ud);
    ghb_file_chooser_show(dialog);
}
