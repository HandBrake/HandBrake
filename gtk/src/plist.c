#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "plist.h"
#include "values.h"

#define BUF_SZ	(128*1024)

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
#define TAG_MAP_SZ	(sizeof(tag_map)/sizeof(tag_map_t))

typedef struct
{
	gchar *key;
	gchar *value;
	GValue *plist;
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
	gint ii;

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
	GType gtype = 0;
	GValue *gval = NULL;
	GValue *current = g_queue_peek_head(pd->stack);
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
			gval = ghb_dict_value_new();
			g_queue_push_head(pd->stack, gval);
		} break;
		case P_ARRAY:
		{
			gval = ghb_array_value_new(128);
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
		gtype = G_VALUE_TYPE(current);
		if (gtype == ghb_array_get_type())
		{
			ghb_array_append(current, gval);
		}
		else if (gtype == ghb_dict_get_type())
		{
			if (pd->key == NULL)
			{
				g_warning("No key for dictionary item");
				ghb_value_free(gval);
			}
			else
			{
				ghb_dict_insert(current, g_strdup(pd->key), gval);
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
	gint ii;

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

	GValue *gval = NULL;
	GValue *current = g_queue_peek_head(pd->stack);
	GType gtype = 0;
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
			gval = ghb_int64_value_new(val);
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
		case P_DATE:
		{
			GDate date;
			GTimeVal time;
			g_time_val_from_iso8601(pd->value, &time);
			g_date_set_time_val(&date, &time);
			gval = ghb_date_value_new(&date);
		} break;
		case P_TRUE:
		{
			gval = ghb_boolean_value_new(TRUE);
		} break;
		case P_FALSE:
		{
			gval = ghb_boolean_value_new(FALSE);
		} break;
		case P_DATA:
		{
			ghb_rawdata_t *data;
			data = g_malloc(sizeof(ghb_rawdata_t));
			data->data = g_base64_decode(pd->value, &(data->size));
			gval = ghb_rawdata_value_new(data);
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
		gtype = G_VALUE_TYPE(current);
		if (gtype == ghb_array_get_type())
		{
			ghb_array_append(current, gval);
		}
		else if (gtype == ghb_dict_get_type())
		{
			if (pd->key == NULL)
			{
				g_warning("No key for dictionary item");
				ghb_value_free(gval);
			}
			else
			{
				ghb_dict_insert(current, g_strdup(pd->key), gval);
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

GValue*
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

GValue*
ghb_plist_parse_file(const gchar *filename)
{
	gchar *buffer;
	size_t size;
	GValue *gval;
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

// Used for sorting dictionaries.
static gint
key_cmp(gconstpointer a, gconstpointer b)
{
	gchar *stra = (gchar*)a;
	gchar *strb = (gchar*)b;

	return strcmp(stra, strb);
}

static void
gval_write(FILE *file, GValue *gval)
{
	static gint indent = 0;
	gint ii;
	GType gtype;

	if (gval == NULL) return;
	gtype = G_VALUE_TYPE(gval);
	if (gtype == ghb_array_get_type())
	{
		GValue *val;
		gint count;

		indent_fprintf(file, indent, "<array>\n");
		indent++;
		count = ghb_array_len(gval);
		for (ii = 0; ii < count; ii++)
		{
			val = ghb_array_get_nth(gval, ii);
			gval_write(file, val);
		}
		indent--;
		indent_fprintf(file, indent, "</array>\n");
	}
	else if (gtype == ghb_dict_get_type())
	{
		GValue *val;
		GHashTable *dict = g_value_get_boxed(gval);
		GList *link, *keys;
		keys = g_hash_table_get_keys(dict);
		// Sort the dictionary.  Not really necessray, but it makes
		// finding things easier
		keys = g_list_sort(keys, key_cmp);
		link = keys;
		indent_fprintf(file, indent, "<dict>\n");
		indent++;
		while (link)
		{
			gchar *key = (gchar*)link->data;
			val = g_hash_table_lookup(dict, key);
			indent_fprintf(file, indent, "<key>%s</key>\n", key);
			gval_write(file, val);
			link = link->next;
		}
		indent--;
		indent_fprintf(file, indent, "</dict>\n");
		g_list_free(keys);
	}
	else if (gtype == G_TYPE_BOOLEAN)
	{
		gchar *tag;
		if (g_value_get_boolean(gval))
		{
			tag = "true";
		}
		else
		{
			tag = "false";
		}
		indent_fprintf(file, indent, "<%s />\n", tag);
	}
	else if (gtype == g_date_get_type())
	{
		GDate *date;
		date = g_value_get_boxed(gval);
		indent_fprintf(file, indent, "<date>%d-%d-%d</date>\n", 
			g_date_get_year(date),
			g_date_get_month(date),
			g_date_get_day(date)
		);
	}
	else if (gtype == ghb_rawdata_get_type())
	{
		ghb_rawdata_t *data;
		gchar *base64;
		data = g_value_get_boxed(gval);
		base64 = g_base64_encode(data->data, data->size);
		indent_fprintf(file, indent, "<data>\n");
		indent_fprintf(file, 0, "%s\n", base64);
		indent_fprintf(file, indent, "</data>\n");
		g_free(base64);
	}
	else if (gtype == G_TYPE_DOUBLE)
	{
		gdouble val = g_value_get_double(gval);
		indent_fprintf(file, indent, "<real>%.17g</real>\n", val);
	}
	else if (gtype == G_TYPE_INT64)
	{
		gint val = g_value_get_int64(gval);
		indent_fprintf(file, indent, "<integer>%d</integer>\n", val);
	}
	else if (gtype == G_TYPE_INT)
	{
		gint val = g_value_get_int(gval);
		indent_fprintf(file, indent, "<integer>%d</integer>\n", val);
	}
	else if (gtype == G_TYPE_STRING)
	{
		const gchar *str = g_value_get_string(gval);
		gchar *esc = g_markup_escape_text(str, -1);
		indent_fprintf(file, indent, "<string>%s</string>\n", esc);
		g_free(esc);
	}
	else
	{
		// Try to make anything thats unrecognized into a string
		const gchar *str;
		GValue val = {0,};
		g_value_init(&val, G_TYPE_STRING);
		if (g_value_transform(gval, &val))
		{
			str = g_value_get_string(&val);
			gchar *esc = g_markup_escape_text(str, -1);
			indent_fprintf(file, indent, "<string>%s</string>\n", esc);
			g_free(esc);
		}
		else
		{
			g_message("failed to transform");
		}
		g_value_unset(&val);
	}
}

void
ghb_plist_write(FILE *file, GValue *gval)
{
	fprintf(file, "%s", preamble);
	gval_write(file, gval);
	fprintf(file, "%s", postfix);
}

void
ghb_plist_write_file(const gchar *filename, GValue *gval)
{
	FILE *file;

	file = fopen(filename, "w");
	if (file == NULL)
		return;

	fprintf(file, "%s", preamble);
	gval_write(file, gval);
	fprintf(file, "%s", postfix);
}


#if defined(PL_TEST)
gint
main(gint argc, gchar *argv[])
{
	GValue *gval;

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
