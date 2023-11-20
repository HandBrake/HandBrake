/* plist.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include "libxml/parser.h"

#include "handbrake/common.h"
#include "handbrake/hb_dict.h"
#include "handbrake/plist.h"

#define BUF_SZ  (128*1024)

static char *preamble =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
    "<plist version=\"1.0\">\n";
static char *postfix =
    "</plist>\n";

typedef struct queue_item_s queue_item_t;
struct queue_item_s
{
    void         *value;
    queue_item_t *next;
};

typedef struct
{
    queue_item_t *head;
} queue_t;

queue_t * queue_new()
{
    return calloc(1, sizeof(queue_t));
}

void queue_free(queue_t **_q)
{
    queue_t *q = *_q;
    if (q == NULL)
        return;

    queue_item_t *n, *i = q->head;
    while (i != NULL)
    {
        n = i->next;
        free(i);
        i = n;
    }
    free(q);
    *_q = NULL;
}

void queue_push_head(queue_t *q, void *v)
{
    queue_item_t *i = calloc(1, sizeof(queue_item_t));
    i->value = v;
    i->next = q->head;
    q->head = i;
}

void * queue_peek_head(queue_t *q)
{
    if (q->head != NULL)
        return q->head->value;
    return NULL;
}

void * queue_pop_head(queue_t *q)
{
    void *result;
    queue_item_t *i;

    if (q->head == NULL)
        return NULL;

    i = q->head;
    result = i->value;
    q->head = i->next;
    free(i);

    return result;
}

int queue_is_empty(queue_t *q)
{
    return q->head == NULL;
}

char * markup_escape_text(const char *str)
{
    int ii, jj;
    int len = strlen(str);
    int step = 40;
    int alloc = len + step;
    char *markup = malloc(alloc);

    for (ii = 0, jj = 0; ii < len; ii++)
    {
        if (jj > alloc - 8)
        {
            alloc += step;
            char *tmp = realloc(markup, alloc);
            if (tmp == NULL)
            {
                markup[jj] = 0;
                return markup;
            }
            markup = tmp;
        }
        switch (str[ii])
        {
            case '<':
                markup[jj++] = '&';
                markup[jj++] = 'l';
                markup[jj++] = 't';
                markup[jj++] = ';';
                break;
            case '>':
                markup[jj++] = '&';
                markup[jj++] = 'g';
                markup[jj++] = 't';
                markup[jj++] = ';';
                break;
            case '\'':
                markup[jj++] = '&';
                markup[jj++] = 'a';
                markup[jj++] = 'p';
                markup[jj++] = 'o';
                markup[jj++] = 's';
                markup[jj++] = ';';
                break;
            case '"':
                markup[jj++] = '&';
                markup[jj++] = 'q';
                markup[jj++] = 'u';
                markup[jj++] = 'o';
                markup[jj++] = 't';
                markup[jj++] = ';';
                break;
            case '&':
                markup[jj++] = '&';
                markup[jj++] = 'a';
                markup[jj++] = 'm';
                markup[jj++] = 'p';
                markup[jj++] = ';';
                break;
            default:
                markup[jj++] = str[ii];
                break;
        }
        markup[jj] = 0;
    }
    return markup;
}

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
    char *tag;
    int id;
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
    char *key;
    char *value;
    hb_value_t *plist;
    queue_t *stack;
    queue_t *tag_stack;
    int closed_top;
} parse_data_t;

static void
start_element(
    void *ud,
    const xmlChar *xname,
    const xmlChar **attr_names)
{
    char *name = (char*)xname;
    parse_data_t *pd = (parse_data_t*)ud;
    union
    {
        int id;
        void * pid;
    } id;
    int ii;

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
        hb_error("Unrecognized start tag (%s)", name);
        return;
    }
    if (pd->value)
    {
        free(pd->value);
        pd->value = NULL;
    }
    queue_push_head(pd->tag_stack, id.pid);
    hb_value_type_t gtype = 0;
    hb_value_t *gval = NULL;
    hb_value_t *current = queue_peek_head(pd->stack);
    switch (id.id)
    {
        case P_PLIST:
        { // Ignore
        } break;
        case P_KEY:
        {
            if (pd->key) free(pd->key);
            pd->key = NULL;
        } break;
        case P_DICT:
        {
            gval = hb_dict_init();
            queue_push_head(pd->stack, gval);
        } break;
        case P_ARRAY:
        {
            gval = hb_value_array_init();
            queue_push_head(pd->stack, gval);
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
        gtype = hb_value_type(current);
        if (gtype == HB_VALUE_TYPE_ARRAY)
        {
            hb_value_array_append(current, gval);
        }
        else if (gtype == HB_VALUE_TYPE_DICT)
        {
            if (pd->key == NULL)
            {
                hb_error("No key for dictionary item");
                hb_value_free(&gval);
            }
            else
            {
                hb_dict_set(current, pd->key, gval);
            }
        }
        else
        {
            hb_error("Invalid container type. This shouldn't happen");
        }
    }
}

static void
end_element(
    void *ud,
    const xmlChar *xname)
{
    char *name = (char*)xname;
    parse_data_t *pd = (parse_data_t*)ud;
    int id;
    union
    {
        int id;
        void * pid;
    } start_id;
    int ii;

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
        hb_error("Unrecognized start tag (%s)", name);
        return;
    }
    start_id.pid = queue_pop_head(pd->tag_stack);
    if (start_id.id != id)
        hb_error("start tag != end tag: (%s %d) %d", name, id, id);

    hb_value_t *gval = NULL;
    hb_value_t *current = queue_peek_head(pd->stack);
    hb_value_type_t gtype = 0;
    const char *value;
    if (pd->value != NULL)
        value = pd->value;
    else
        value = "";
    switch (id)
    {
        case P_PLIST:
        { // Ignore
        } break;
        case P_KEY:
        {
            if (pd->key) free(pd->key);
            pd->key = strdup(value);
            return;
        }
        case P_DICT:
        {
            queue_pop_head(pd->stack);
        } break;
        case P_ARRAY:
        {
            queue_pop_head(pd->stack);
        } break;
        case P_INTEGER:
        {
            uint64_t val = strtoll(value, NULL, 0);
            gval = hb_value_int(val);
        } break;
        case P_REAL:
        {
            double val = strtod(value, NULL);
            gval = hb_value_double(val);
        } break;
        case P_STRING:
        {
            gval = hb_value_string(value);
        } break;
        case P_TRUE:
        {
            gval = hb_value_bool(1);
        } break;
        case P_FALSE:
        {
            gval = hb_value_bool(0);
        } break;
        default:
        {
            hb_error("Unhandled plist type %d", id);
        } break;
    }
    if (gval)
    {
        // Get the top of the data structure stack and if it's an array
        // or dict, add the current element
        if (current == NULL)
        {
            pd->plist = gval;
            pd->closed_top = 1;
            return;
        }
        gtype = hb_value_type(current);
        if (gtype == HB_VALUE_TYPE_ARRAY)
        {
            hb_value_array_append(current, gval);
        }
        else if (gtype == HB_VALUE_TYPE_DICT)
        {
            if (pd->key == NULL)
            {
                hb_error("No key for dictionary item");
                hb_value_free(&gval);
            }
            else
            {
                hb_dict_set(current, pd->key, gval);
            }
        }
        else
        {
            hb_error("Invalid container type. This shouldn't happen");
        }
    }
    if (queue_is_empty(pd->stack))
        pd->closed_top = 1;
}

static void
text_data(
    void *ud,
    const xmlChar *xtext,
    int len)
{
    char *text = (char*)xtext;
    parse_data_t *pd = (parse_data_t*)ud;

    int pos = 0;
    if (pd->value != NULL)
    {
        pos = strlen(pd->value);
    }
    char *tmp = realloc(pd->value, pos + len + 1);
    if (tmp == NULL)
        return;
    pd->value = tmp;
    strncpy(pd->value + pos, text, len);
    pd->value[pos + len] = 0;
}

static void
parse_warning(void *ud, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    hb_valog(0, "Plist parse warning: ", msg, args);
    va_end(args);
}

static void
parse_error(void *ud, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    hb_valog(0, "Plist parse error: ", msg, args);
    va_end(args);
}

hb_value_t*
hb_plist_parse(const char *buf, size_t len)
{
    xmlSAXHandler parser;
    parse_data_t pd;

    pd.stack = queue_new();
    pd.tag_stack = queue_new();
    pd.key = NULL;
    pd.value = NULL;
    pd.plist = NULL;
    pd.closed_top = 0;

    memset(&parser, 0, sizeof(parser));
    parser.initialized = XML_SAX2_MAGIC;
    parser.startElement = start_element;
    parser.endElement = end_element;
    parser.characters = text_data;
    parser.warning = parse_warning;
    parser.error = parse_error;
    int result = xmlSAXUserParseMemory(&parser, &pd, buf, len);
    if (result != 0)
    {
        hb_error("Plist parse failed");
        return NULL;
    }
    xmlCleanupParser();

    if (pd.key) free(pd.key);
    if (pd.value) free(pd.value);
    queue_free(&pd.stack);
    queue_free(&pd.tag_stack);

    return pd.plist;
}

hb_value_t*
hb_plist_parse_file(const char *filename)
{
    char *buffer;
    size_t size;
    hb_value_t *gval;
    FILE *fd;

    fd = fopen(filename, "r");
    if (fd == NULL)
    {
        // File doesn't exist
        return NULL;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buffer = malloc(size+1);
    size = fread(buffer, 1, size, fd);
    buffer[size] = 0;
    gval = hb_plist_parse(buffer, size);
    free(buffer);
    fclose(fd);
    return gval;
}

static void
indent_fprintf(FILE *file, int indent, const char *fmt, ...)
{
    va_list ap;

    for (; indent; indent--)
        putc('\t', file);
    va_start(ap, fmt);
    vfprintf(file, fmt, ap);
    va_end(ap);
}

static void
gval_write(FILE *file, hb_value_t *gval)
{
    static int indent = 0;
    int ii;
    hb_value_type_t gtype;

    if (gval == NULL) return;
    gtype = hb_value_type(gval);
    if (gtype == HB_VALUE_TYPE_ARRAY)
    {
        hb_value_t *val;
        int count;

        indent_fprintf(file, indent, "<array>\n");
        indent++;
        count = hb_value_array_len(gval);
        for (ii = 0; ii < count; ii++)
        {
            val = hb_value_array_get(gval, ii);
            gval_write(file, val);
        }
        indent--;
        indent_fprintf(file, indent, "</array>\n");
    }
    else if (gtype == HB_VALUE_TYPE_DICT)
    {
        const char *key;
        hb_value_t *val;
        hb_dict_iter_t iter;

        indent_fprintf(file, indent, "<dict>\n");
        indent++;

        for (iter = hb_dict_iter_init(gval);
             iter != HB_DICT_ITER_DONE;
             iter = hb_dict_iter_next(gval, iter))
        {
            key = hb_dict_iter_key(iter);
            val = hb_dict_iter_value(iter);
            indent_fprintf(file, indent, "<key>%s</key>\n", key);
            gval_write(file, val);
        }

        indent--;
        indent_fprintf(file, indent, "</dict>\n");
    }
    else if (gtype == HB_VALUE_TYPE_BOOL)
    {
        char *tag;
        if (hb_value_get_bool(gval))
        {
            tag = "true";
        }
        else
        {
            tag = "false";
        }
        indent_fprintf(file, indent, "<%s />\n", tag);
    }
    else if (gtype == HB_VALUE_TYPE_DOUBLE)
    {
        double val = hb_value_get_double(gval);
        indent_fprintf(file, indent, "<real>%.17g</real>\n", val);
    }
    else if (gtype == HB_VALUE_TYPE_INT)
    {
        int64_t val = hb_value_get_int(gval);
        indent_fprintf(file, indent, "<integer>%"PRId64"</integer>\n", val);
    }
    else if (gtype == HB_VALUE_TYPE_STRING)
    {
        const char *str = hb_value_get_string(gval);
        char *esc = markup_escape_text(str);
        indent_fprintf(file, indent, "<string>%s</string>\n", esc);
        free(esc);
    }
    else
    {
        // Try to make anything that's unrecognized into a string
        hb_error("Unhandled data type %d", gtype);
    }
}

void
hb_plist_write(FILE *file, hb_value_t *gval)
{
    fprintf(file, "%s", preamble);
    gval_write(file, gval);
    fprintf(file, "%s", postfix);
}

void
hb_plist_write_file(const char *filename, hb_value_t *gval)
{
    FILE *file;

    file = fopen(filename, "w");
    if (file == NULL)
        return;

    hb_plist_write(file, gval);
    fclose(file);
}


#if defined(PL_TEST)
int
main(int argc, char *argv[])
{
    hb_value_t *gval;

    file = fopen(argv[1], "r");
    gval = hb_plist_parse_file(file);
    if (argc > 2)
        hb_plist_write_file(argv[2], gval);
    else
        hb_plist_write(stdout, gval);
    if (file) fclose (file);
    return 0;
}
#endif
