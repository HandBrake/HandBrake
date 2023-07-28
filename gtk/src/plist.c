/*
 * plist.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * plist.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * plist.c is distributed in the hope that it will be useful,
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "values.h"
#include "plist.h"

#define BUF_SZ  (128*1024)

static gchar *preamble =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
    "<plist version=\"1.0\">\n";
static gchar *postfix =
    "</plist>\n";

enum
{
    P_NONE = 0,
    P_PLIST,
    P_KEY,
    P_ARRAY,
    P_DICT,
    P_INTEGER,
    P_REAL,
    P_STRING,
    P_DATE,
    P_TRUE,
    P_FALSE,
    P_DATA,
};

typedef struct
{
    gchar *tag;
    gint id;
} tag_map_t;

static tag_map_t tag_map[] =
{
    {"plist", P_PLIST},
    {"key", P_KEY},
    {"array", P_ARRAY},
    {"dict", P_DICT},
    {"integer", P_INTEGER},
    {"real", P_REAL},
    {"string", P_STRING},
    {"date", P_DATE},
    {"true", P_TRUE},
    {"false", P_FALSE},
    {"data", P_DATA},
};
#define TAG_MAP_SZ  (sizeof(tag_map)/sizeof(tag_map_t))

typedef struct
{
    gchar *key;
    gchar *value;
    GhbValue *plist;
    GQueue *stack;
    GQueue *tag_stack;
    gboolean closed_top;
} parse_data_t;

static void
start_element(
    GMarkupParseContext *ctx,
    const gchar *name,
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
    guint ii;

    // Check to see if the first element found has been closed
    // If so, ignore any junk following it.
    if (pd->closed_top)
        return;

    for (ii = 0; ii < TAG_MAP_SZ; ii++)
    {
        if (strcmp(name, tag_map[ii].tag) == 0)
        {
            id.id = tag_map[ii].id;
            break;
        }
    }
    if (ii == TAG_MAP_SZ)
    {
        g_warning("Unrecognized start tag (%s)", name);
        return;
    }
    g_queue_push_head(pd->tag_stack, id.pid);
    GhbType gtype = 0;
    GhbValue *gval = NULL;
    GhbValue *current = g_queue_peek_head(pd->stack);
    switch (id.id)
    {
        case P_PLIST:
        { // Ignore
        } break;
        case P_KEY:
        {
            if (pd->key) g_free(pd->key);
            pd->key = NULL;
        } break;
        case P_DICT:
        {
            gval = ghb_dict_new();
            g_queue_push_head(pd->stack, gval);
        } break;
        case P_ARRAY:
        {
            gval = ghb_array_new();
            g_queue_push_head(pd->stack, gval);
        } break;
        case P_INTEGER:
        {
        } break;
        case P_REAL:
        {
        } break;
        case P_STRING:
        {
        } break;
        case P_DATE:
        {
        } break;
        case P_TRUE:
        {
        } break;
        case P_FALSE:
        {
        } break;
        case P_DATA:
        {
        } break;
    }
    // Add the element to the current container
    if (gval)
    { // There's an element to add
        if (current == NULL)
        {
            pd->plist = gval;
            return;
        }
        gtype = ghb_value_type(current);
        if (gtype == GHB_ARRAY)
        {
            ghb_array_append(current, gval);
        }
        else if (gtype == GHB_DICT)
        {
            if (pd->key == NULL)
            {
                g_warning("No key for dictionary item");
                ghb_value_free(&gval);
            }
            else
            {
                ghb_dict_set(current, pd->key, gval);
            }
        }
        else
        {
            g_error("Invalid container type. This shouldn't happen");
        }
    }
}

static void
end_element(
    GMarkupParseContext *ctx,
    const gchar *name,
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
    guint ii;

    // Check to see if the first element found has been closed
    // If so, ignore any junk following it.
    if (pd->closed_top)
        return;

    for (ii = 0; ii < TAG_MAP_SZ; ii++)
    {
        if (strcmp(name, tag_map[ii].tag) == 0)
        {
            id = tag_map[ii].id;
            break;
        }
    }
    if (ii == TAG_MAP_SZ)
    {
        g_warning("Unrecognized start tag (%s)", name);
        return;
    }
    start_id.pid = g_queue_pop_head(pd->tag_stack);
    if (start_id.id != id)
        g_warning("start tag != end tag: (%s %d) %d", name, id, id);

    GhbValue *gval = NULL;
    GhbValue *current = g_queue_peek_head(pd->stack);
    GhbType gtype = 0;
    switch (id)
    {
        case P_PLIST:
        { // Ignore
        } break;
        case P_KEY:
        {
            if (pd->key) g_free(pd->key);
            pd->key = g_strdup(pd->value);
            return;
        } break;
        case P_DICT:
        {
            g_queue_pop_head(pd->stack);
        } break;
        case P_ARRAY:
        {
            g_queue_pop_head(pd->stack);
        } break;
        case P_INTEGER:
        {
            gint64 val = g_strtod(pd->value, NULL);
            gval = ghb_int_value_new(val);
        } break;
        case P_REAL:
        {
            gdouble val = g_strtod(pd->value, NULL);
            gval = ghb_double_value_new(val);
        } break;
        case P_STRING:
        {
            gval = ghb_string_value_new(pd->value);
        } break;
        case P_TRUE:
        {
            gval = ghb_bool_value_new(TRUE);
        } break;
        case P_FALSE:
        {
            gval = ghb_bool_value_new(FALSE);
        } break;
        default:
        {
            g_message("Unhandled plist type %d", id);
        } break;
    }
    if (gval)
    {
        // Get the top of the data structure stack and if it's an array
        // or dict, add the current element
        if (current == NULL)
        {
            pd->plist = gval;
            pd->closed_top = TRUE;
            return;
        }
        gtype = ghb_value_type(current);
        if (gtype == GHB_ARRAY)
        {
            ghb_array_append(current, gval);
        }
        else if (gtype == GHB_DICT)
        {
            if (pd->key == NULL)
            {
                g_warning("No key for dictionary item");
                ghb_value_free(&gval);
            }
            else
            {
                ghb_dict_set(current, pd->key, gval);
            }
        }
        else
        {
            g_error("Invalid container type. This shouldn't happen");
        }
    }
    if (g_queue_is_empty(pd->stack))
        pd->closed_top = TRUE;
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
    if (pd->value) g_free(pd->value);
    pd->value = g_strdup(text);
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
    g_warning("Plist parse error: %s", error->message);
}

// This is required or the parser crashes
static void
destroy_notify(gpointer data)
{ // Do nothing
    //g_debug("destroy parser");
}

GhbValue*
ghb_plist_parse(const gchar *buf, gssize len)
{
    GMarkupParseContext *ctx;
    GMarkupParser parser;
    parse_data_t pd;
    GError *err = NULL;

    pd.stack = g_queue_new();
    pd.tag_stack = g_queue_new();
    pd.key = NULL;
    pd.value = NULL;
    pd.plist = NULL;
    pd.closed_top = FALSE;

    parser.start_element = start_element;
    parser.end_element = end_element;
    parser.text = text_data;
    parser.passthrough = passthrough;
    parser.error = parse_error;
    ctx = g_markup_parse_context_new(&parser, 0, &pd, destroy_notify);

    g_markup_parse_context_parse(ctx, buf, len, &err);
    g_markup_parse_context_end_parse(ctx, &err);
    g_markup_parse_context_free(ctx);
    if (pd.key) g_free(pd.key);
    if (pd.value) g_free(pd.value);
    g_queue_free(pd.stack);
    g_queue_free(pd.tag_stack);
    return pd.plist;
}

GhbValue*
ghb_plist_parse_file(const gchar *filename)
{
    gchar *buffer;
    size_t size;
    GhbValue *gval;
    FILE *fd;

    fd = g_fopen(filename, "r");
    if (fd == NULL)
    {
        g_warning("Plist parse: failed to open %s", filename);
        return NULL;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buffer = g_malloc(size+1);
    size = fread(buffer, 1, size, fd);
    buffer[size] = 0;
    gval = ghb_plist_parse(buffer, (gssize)size);
    g_free(buffer);
    fclose(fd);
    return gval;
}

static void
indent_fprintf(FILE *file, gint indent, const gchar *fmt, ...)
{
    va_list ap;

    for (; indent; indent--)
        putc('\t', file);
    va_start(ap, fmt);
    vfprintf(file, fmt, ap);
    va_end(ap);
}

static void
gval_write(FILE *file, GhbValue *gval)
{
    static gint indent = 0;
    gint ii;
    GhbType gtype;

    if (gval == NULL) return;
    gtype = ghb_value_type(gval);
    if (gtype == GHB_ARRAY)
    {
        GhbValue *val;
        gint count;

        indent_fprintf(file, indent, "<array>\n");
        indent++;
        count = ghb_array_len(gval);
        for (ii = 0; ii < count; ii++)
        {
            val = ghb_array_get(gval, ii);
            gval_write(file, val);
        }
        indent--;
        indent_fprintf(file, indent, "</array>\n");
    }
    else if (gtype == GHB_DICT)
    {
        const char *key;
        GhbValue *val;
        GhbDictIter iter;

        indent_fprintf(file, indent, "<dict>\n");
        indent++;

        iter = ghb_dict_iter_init(gval);
        while (ghb_dict_iter_next(gval, &iter, &key, &val))
        {
            indent_fprintf(file, indent, "<key>%s</key>\n", key);
            gval_write(file, val);
        }

        indent--;
        indent_fprintf(file, indent, "</dict>\n");
    }
    else if (gtype == GHB_BOOL)
    {
        gchar *tag;
        if (ghb_value_get_bool(gval))
        {
            tag = "true";
        }
        else
        {
            tag = "false";
        }
        indent_fprintf(file, indent, "<%s />\n", tag);
    }
    else if (gtype == GHB_DOUBLE)
    {
        gdouble val = ghb_value_get_double(gval);
        indent_fprintf(file, indent, "<real>%.17g</real>\n", val);
    }
    else if (gtype == GHB_INT)
    {
        gint64 val = ghb_value_get_int(gval);
        indent_fprintf(file, indent, "<integer>%"PRId64"</integer>\n", val);
    }
    else if (gtype == GHB_STRING)
    {
        const gchar *str = ghb_value_get_string(gval);
        gchar *esc = g_markup_escape_text(str, -1);
        indent_fprintf(file, indent, "<string>%s</string>\n", esc);
        g_free(esc);
    }
    else
    {
        // Try to make anything that's unrecognized into a string
        g_warning("Unhandled data type %d", gtype);
    }
}

void
ghb_plist_write(FILE *file, GhbValue *gval)
{
    fprintf(file, "%s", preamble);
    gval_write(file, gval);
    fprintf(file, "%s", postfix);
}

void
ghb_plist_write_file(const gchar *filename, GhbValue *gval)
{
    FILE *file;

    file = g_fopen(filename, "w");
    if (file == NULL)
        return;

    ghb_plist_write(file, gval);
    fclose(file);
}


#if defined(PL_TEST)
gint
main(gint argc, gchar *argv[])
{
    GhbValue *gval;

    g_type_init();

    file = g_fopen(argv[1], "r");
    gval = ghb_plist_parse_file(file);
    if (argc > 2)
        ghb_plist_write_file(argv[2], gval);
    else
        ghb_plist_write(stdout, gval);
    if (file) fclose (file);
    return 0;
}
#endif
