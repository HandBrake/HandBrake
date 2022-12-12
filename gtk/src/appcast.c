/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * appcast.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * appcast.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * appcast.c is distributed in the hope that it will be useful,
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

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "plist.h"
#include "values.h"

enum
{
    A_NONE = 0,
    A_DESCRIPTION,
    A_ENCLOSURE,
    A_ITEM,
};

typedef struct
{
    gchar *tag;
    gint id;
} tag_map_t;

static tag_map_t tag_map[] =
{
    {"sparkle:releaseNotesLink", A_DESCRIPTION},
    {"enclosure", A_ENCLOSURE},
    {"item", A_ITEM},
};
#define TAG_MAP_SZ  (sizeof(tag_map)/sizeof(tag_map_t))

typedef struct
{
    gchar *key;
    gchar *value;
    GQueue *stack;
    GQueue *tag_stack;
    GString *description;
    gchar *build;
    gchar *version;
    gboolean item;
} parse_data_t;

static const gchar*
lookup_attr_value(
    const gchar *name,
    const gchar **attr_names,
    const gchar **attr_values)
{
    gint ii;

    if (attr_names == NULL) return NULL;
    for (ii = 0; attr_names[ii] != NULL; ii++)
    {
        if (strcmp(name, attr_names[ii]) == 0)
            return attr_values[ii];
    }
    return NULL;
}

static void
start_element(
    GMarkupParseContext *ctx,
    const gchar *tag,
    const gchar **attr_names,
    const gchar **attr_values,
    gpointer ud,
    GError **error)
{
    parse_data_t *pd = (parse_data_t*)ud;
    union
    {
        gint id;
        gpointer pid;
    } id;
    gint ii;

    for (ii = 0; ii < TAG_MAP_SZ; ii++)
    {
        if (strcmp(tag, tag_map[ii].tag) == 0)
        {
            id.id = tag_map[ii].id;
            break;
        }
    }
    if (ii == TAG_MAP_SZ)
    {
        g_debug("Unrecognized start tag (%s)", tag);
        id.id = A_NONE;
    }
    g_queue_push_head(pd->tag_stack, id.pid);
    switch (id.id)
    {
        case A_ITEM:
        {
            pd->item = TRUE;
        } break;
        case A_ENCLOSURE:
        {
            const gchar *build, *version;
            build = lookup_attr_value(
                        "sparkle:version", attr_names, attr_values);
            version = lookup_attr_value(
                        "sparkle:shortVersionString", attr_names, attr_values);
            if (build)
                pd->build = g_strdup(build);
            if (version)
                pd->version = g_strdup(version);
        } break;
    }
}

static void
end_element(
    GMarkupParseContext *ctx,
    const gchar *tag,
    gpointer ud,
    GError **error)
{
    parse_data_t *pd = (parse_data_t*)ud;
    gint id;
    union
    {
        gint id;
        gpointer pid;
    } start_id;
    gint ii;

    for (ii = 0; ii < TAG_MAP_SZ; ii++)
    {
        if (strcmp(tag, tag_map[ii].tag) == 0)
        {
            id = tag_map[ii].id;
            break;
        }
    }
    if (ii == TAG_MAP_SZ)
    {
        g_debug("Unrecognized end tag (%s)", tag);
        id = A_NONE;
    }
    start_id.pid = g_queue_pop_head(pd->tag_stack);
    if (start_id.id != id)
        g_warning("start tag != end tag: (%s %d) %d", tag, start_id.id, id);
    switch (id)
    {
        case A_ITEM:
        {
            pd->item = FALSE;
        } break;
        default:
        {
        } break;
    }

}

static void
text_data(
    GMarkupParseContext *ctx,
    const gchar *text,
    gsize len,
    gpointer ud,
    GError **error)
{
    parse_data_t *pd = (parse_data_t*)ud;
    union
    {
        gint id;
        gpointer pid;
    } start_id;

    start_id.pid = g_queue_peek_head(pd->tag_stack);
    switch (start_id.id)
    {
        case A_DESCRIPTION:
        {
            if (pd->item)
            {
                g_string_append(pd->description, text);
            }
        } break;
        default:
        {
            if (pd->value) g_free(pd->value);
            pd->value = g_strdup(text);
        } break;
    }
}

static void
passthrough(
    GMarkupParseContext *ctx,
    const gchar *text,
    gsize len,
    gpointer ud,
    GError **error)
{
    //parse_data_t *pd = (parse_data_t*)ud;

    //g_debug("passthrough %s", text);
}

static void
parse_error(GMarkupParseContext *ctx, GError *error, gpointer ud)
{
    g_warning("Resource parse error: %s", error->message);
}

// This is required or the parser crashes
static void
destroy_notify(gpointer data)
{ // Do nothing
    //g_debug("destroy parser");
}

void
ghb_appcast_parse(gchar *buf, gchar **desc, gchar **build, gchar **version)
{
    GMarkupParseContext *ctx;
    GMarkupParser parser;
    parse_data_t pd;
    GError *err = NULL;
    gint len;
    gchar *start;
    //gchar tmp[4096]

    // Skip junk at beginning of buffer
    start = strstr(buf, "<?xml ");
    pd.description = g_string_new("");
    pd.item = FALSE;
    pd.build = NULL;
    pd.version = NULL;
    len = strlen(start);
    pd.tag_stack = g_queue_new();
    pd.key = NULL;
    pd.value = NULL;

    parser.start_element = start_element;
    parser.end_element = end_element;
    parser.text = text_data;
    parser.passthrough = passthrough;
    parser.error = parse_error;
    ctx = g_markup_parse_context_new(
            &parser, G_MARKUP_TREAT_CDATA_AS_TEXT, &pd, destroy_notify);

    g_markup_parse_context_parse(ctx, start, len, &err);
    g_markup_parse_context_end_parse(ctx, &err);
    g_markup_parse_context_free(ctx);
    g_queue_free(pd.tag_stack);
    *desc = g_string_free(pd.description, FALSE);
    // work around a bug to leaves the CDATA closing brackets on the string
    gchar *glitch;
    glitch = g_strrstr(*desc, "]]>");
    if (glitch)
        *glitch = 0;
    *build = pd.build;
    *version = pd.version;
}
