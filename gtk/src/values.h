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
#include "jansson.h"

#define GHB_DICT    JSON_OBJECT
#define GHB_ARRAY   JSON_ARRAY
#define GHB_STRING  JSON_STRING
#define GHB_INT     JSON_INTEGER
#define GHB_DOUBLE  JSON_REAL
#define GHB_NULL    JSON_NULL
#define GHB_BOOL    0xff

typedef json_t GhbValue;
typedef int    GhbType;
typedef void*  GhbDictIter;

void ghb_value_incref(GhbValue *gval);
void ghb_value_decref(GhbValue *gval);
GhbType ghb_value_type(const GhbValue *val);
GhbType ghb_array_get_type(void);
GhbType ghb_dict_get_type(void);
GhbValue* ghb_array_get_nth(const GhbValue *array, gint ii);
void ghb_array_insert(GhbValue *gval, guint ii, GhbValue *val);
void ghb_array_replace(GhbValue *gval, guint ii, GhbValue *val);
void ghb_array_append(GhbValue *gval, GhbValue *val);
void ghb_array_remove(GhbValue *gval, guint ii);
gint ghb_array_len(const GhbValue *gval);
void ghb_array_copy(GhbValue *arr1, GhbValue *arr2, gint count);

GhbValue* ghb_value_xform(const GhbValue *val, GhbType type);
void ghb_value_free(GhbValue *gval);
GhbValue* ghb_value_dup(const GhbValue *val);
gint ghb_value_int(const GhbValue *val);
gint64 ghb_value_int64(const GhbValue *val);
gdouble ghb_value_double(const GhbValue *val);
gchar* ghb_value_string(const GhbValue *val);
const gchar* ghb_value_const_string(const GhbValue *val);
gboolean ghb_value_boolean(const GhbValue *val);

GhbValue* ghb_string_value(const gchar *str);
void ghb_string_value_set(GhbValue *gval, const gchar *str);
GhbValue* ghb_int64_value(gint64 ival);
GhbValue* ghb_int_value(gint ival);
GhbValue* ghb_double_value(gdouble dval);
GhbValue* ghb_boolean_value(gboolean bval);

gint ghb_value_cmp(const GhbValue *vala, const GhbValue *valb);
GhbValue* ghb_string_value_new(const gchar *str);
GhbValue* ghb_int64_value_new(gint64 ival);
GhbValue* ghb_int_value_new(gint ival);
GhbValue* ghb_double_value_new(gdouble dval);
GhbValue* ghb_boolean_value_new(gboolean bval);
GhbValue* ghb_dict_value_new(void);
GhbValue* ghb_array_value_new();
void ghb_array_value_reset(GhbValue *array);

void ghb_dict_insert(GhbValue *gval, const gchar *key, GhbValue *val);
void ghb_dict_iter_init(GhbValue *gval, GhbDictIter *iter);
int  ghb_dict_iter_next(GhbValue *dict, GhbDictIter *iter,
                        const char **key, GhbValue **val);
GhbValue* ghb_dict_lookup(const GhbValue *gval, const gchar *key);
gboolean ghb_dict_remove(GhbValue *gval, const gchar *key);

void debug_show_value(GhbValue *gval);
void debug_show_type(GhbType tp);

void ghb_json_write(FILE *file, GhbValue *gval);
void ghb_json_write_file(const char *path, GhbValue *gval);
GhbValue* ghb_json_parse(const char *json, size_t size);
GhbValue* ghb_json_parse_file(const char *path);

#endif // _GHB_VALUES_H_
