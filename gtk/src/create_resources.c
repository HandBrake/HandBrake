#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "icon_tools.h"
#include "plist.h"
#include "values.h"

enum
{
	R_NONE = 0,
	R_RESOURCES,
	R_SECTION,
	R_ICON,
	R_PLIST,
};

typedef struct
{
	gchar *tag;
	gint id;
} tag_map_t;

static tag_map_t tag_map[] =
{
	{"resources", R_RESOURCES},
	{"section", R_SECTION},
	{"icon", R_ICON},
	{"plist", R_PLIST},
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
		case R_SECTION:
		{
			const gchar *sect;

			sect = lookup_attr_value("name", attr_names, attr_values);
			if (sect && strcmp(sect, "icons") == 0)
			{
				gval = ghb_dict_value_new();
				if (pd->key) g_free(pd->key);
				pd->key = g_strdup(sect);
				g_queue_push_head(pd->stack, gval);
			}
		} break;
		case R_ICON:
		{
			const gchar *filename, *icon;

			filename = lookup_attr_value("file", attr_names, attr_values);
			icon = lookup_attr_value("name", attr_names, attr_values);
			if (filename && icon)
			{
				ghb_rawdata_t *rd;
				guint size;

				rd = g_malloc(sizeof(ghb_rawdata_t));
				rd->data = icon_file_serialize(filename, &size);
				rd->size = size;
				gval = ghb_rawdata_value_new(rd);
				if (pd->key) g_free(pd->key);
				pd->key = g_strdup(icon);
			}
			else
			{
				g_warning("%s:missing a requried attribute", name);
			}
		} break;
		case R_PLIST:
		{
			const gchar *filename, *plist;

			filename = lookup_attr_value("file", attr_names, attr_values);
			plist = lookup_attr_value("name", attr_names, attr_values);
			if (filename && plist)
			{
				gval = ghb_plist_parse_file(filename);
				if (pd->key) g_free(pd->key);
				pd->key = g_strdup(plist);
			}
			else
			{
				g_warning("%s:missing a requried attribute", name);
			}
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
		case R_SECTION:
		{
			g_queue_pop_head(pd->stack);
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
	if (g_queue_is_empty(pd->tag_stack))
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
	g_warning("Resource parse error: %s", error->message);
}

// This is required or the parser crashes
static void 
destroy_notify(gpointer data)
{ // Do nothing
	//g_debug("destroy parser");
}

GValue*
ghb_resource_parse(const gchar *buf, gssize len)
{
	GMarkupParseContext *ctx;
	GMarkupParser parser;
	parse_data_t pd;
	GError *err = NULL;

	pd.stack = g_queue_new();
	pd.tag_stack = g_queue_new();
	pd.key = NULL;
	pd.value = NULL;
	pd.plist = ghb_dict_value_new();
	g_queue_push_head(pd.stack, pd.plist);
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
	g_queue_free(pd.stack);
	g_queue_free(pd.tag_stack);
	return pd.plist;
}

GValue*
ghb_resource_parse_file(FILE *fd)
{
	gchar *buffer;
	size_t size;
	GValue *gval;

	if (fd == NULL)
		return NULL;
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	buffer = g_malloc(size+1);
	size = fread(buffer, 1, size, fd);
	buffer[size] = 0;
	gval = ghb_resource_parse(buffer, (gssize)size);
	g_free(buffer);
	return gval;
}


gint
main(gint argc, gchar *argv[])
{
	FILE *file;
	GValue *gval;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: <in resource list> <out resource plist>\n");
		return 1;
	}
	g_type_init();
	file = g_fopen(argv[1], "r");
	if (file == NULL)
		return 1;

	gval = ghb_resource_parse_file(file);
	ghb_plist_write_file(argv[2], gval);
	return 0;
}

