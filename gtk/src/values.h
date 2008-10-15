/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#if !defined(_GHB_VALUES_H_)
#define _GHB_VALUES_H_

#include <glib.h>
#include <glib-object.h>

typedef struct
{
	guchar *data;
	gsize size;
} ghb_rawdata_t;

GType ghb_rawdata_get_type(void);

GType ghb_array_get_type(void);
GType ghb_dict_get_type(void);
GValue* ghb_array_get_nth(const GValue *array, gint ii);
void ghb_array_insert(GValue *gval, guint ii, GValue *val);
void ghb_array_replace(GValue *gval, guint ii, GValue *val);
void ghb_array_append(GValue *gval, GValue *val);
void ghb_array_remove(GValue *gval, guint ii);
gint ghb_array_len(const GValue *gval);
void ghb_array_copy(GValue *arr1, GValue *arr2, gint count);

void ghb_value_free(GValue *gval);
GValue* ghb_value_new(GType gtype);
GValue* ghb_value_dup(const GValue *val);
gint ghb_value_int(const GValue *val);
gint64 ghb_value_int64(const GValue *val);
gdouble ghb_value_double(const GValue *val);
gchar* ghb_value_string(const GValue *val);
gboolean ghb_value_boolean(const GValue *val);

GValue* ghb_string_value(const gchar *str);
GValue* ghb_int64_value(gint64 ival);
GValue* ghb_int_value(gint ival);
GValue* ghb_double_value(gdouble dval);
GValue* ghb_boolean_value(gboolean bval);

gint ghb_value_cmp(const GValue *vala, const GValue *valb);
GValue* ghb_string_value_new(const gchar *str);
GValue* ghb_int64_value_new(gint64 ival);
GValue* ghb_int_value_new(gint ival);
GValue* ghb_double_value_new(gdouble dval);
GValue* ghb_boolean_value_new(gboolean bval);
GValue* ghb_dict_value_new(void);
GValue* ghb_array_value_new(guint size);
void ghb_array_value_reset(GValue *gval, guint size);

GValue* ghb_date_value_new(GDate *date);
GValue* ghb_rawdata_value_new(ghb_rawdata_t *data);

void ghb_dict_insert(GValue *gval, gchar *key, GValue *val);
void ghb_dict_iter_init(GHashTableIter *iter, GValue *gval);
GValue* ghb_dict_lookup(GValue *gval, const gchar *key);
gboolean ghb_dict_remove(GValue *gval, const gchar *key);
void ghb_register_transforms(void);


#endif // _GHB_VALUES_H_
