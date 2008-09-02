#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include "values.h"
#include "plist.h"

gboolean
string_is_true(const gchar *str)
{
	return (strcmp(str, "enable") == 0);
}

gboolean
string_is_bool(const gchar *str)
{
	return (strcmp(str, "enable") == 0) || (strcmp(str, "disable") == 0);
}

GType
guess_type(const gchar *str)
{
	gchar *end;
	gdouble dval;

	if (*str == 0)
		return G_TYPE_STRING;
	if (string_is_bool(str))
		return G_TYPE_BOOLEAN;
	dval = g_strtod(str, &end);
	if (*end == 0)
	{
		if (strchr(str, '.') == NULL)
			return G_TYPE_INT64;
		else
			return G_TYPE_DOUBLE;
	}

	return G_TYPE_STRING;
}

void
set_value(GValue *gval, const gchar *str, GType gtype)
{
	if (gtype == G_TYPE_STRING)
	{
		g_value_set_string(gval, str);
	}
	else if (gtype == G_TYPE_INT64)
	{
		gint64 val = g_strtod(str, NULL);
		g_value_set_int64(gval, val);
	}
	else if (gtype == G_TYPE_DOUBLE)
	{
		gdouble val = g_strtod(str, NULL);
		g_value_set_double(gval, val);
	}
	else if (gtype == G_TYPE_BOOLEAN)
	{
		if (string_is_true(str))
			g_value_set_boolean(gval, TRUE);
		else
			g_value_set_boolean(gval, FALSE);
	}
}

int
main(gint argc, gchar *argv[])
{
	GKeyFile *kf;
	gchar **groups;
	gchar **keys;
	gint ii, jj;
	GValue *top;
	GValue *dict;

	g_type_init();
	top = ghb_dict_value_new();
	kf = g_key_file_new();
	g_key_file_load_from_file(kf, argv[1], 0, NULL);
	groups = g_key_file_get_groups(kf, NULL);
	for (ii = 0; groups[ii]; ii++)
	{
		dict = ghb_dict_value_new();
		ghb_dict_insert(top, , g_strdup(groups[ii]), dict);
		keys = g_key_file_get_keys(kf, groups[ii], NULL, NULL);
		for (jj = 0; keys[jj]; jj++)
		{
			gchar *str;
			GValue *gval;
			GType gtype;

			str = g_key_file_get_string(kf, groups[ii], keys[jj], NULL);
			gtype = guess_type(str);
			gval = g_malloc0(sizeof(GValue));
			g_value_init(gval, gtype);
			set_value(gval, str, gtype);
			ghb_dict_insert(dict, g_strdup(keys[jj]), gval);
		}
	}
	ghb_plist_write_file("a_p_list", top);
}

