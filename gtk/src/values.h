/* values.h
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

#pragma once

#include "common.h"
#include "handbrake/hb_dict.h"

G_BEGIN_DECLS

#define GHB_DICT    HB_VALUE_TYPE_DICT
#define GHB_ARRAY   HB_VALUE_TYPE_ARRAY
#define GHB_STRING  HB_VALUE_TYPE_STRING
#define GHB_INT     HB_VALUE_TYPE_INT
#define GHB_DOUBLE  HB_VALUE_TYPE_DOUBLE
#define GHB_NULL    HB_VALUE_TYPE_NULL
#define GHB_BOOL    HB_VALUE_TYPE_BOOL

typedef hb_value_t      GhbValue;
typedef hb_value_type_t GhbType;
typedef hb_dict_iter_t  GhbDictIter;

#define ghb_dict_new                hb_dict_init
#define ghb_dict_get                hb_dict_get
#define ghb_dict_set                hb_dict_set
#define ghb_dict_remove             hb_dict_remove
#define ghb_dict_iter_init          hb_dict_iter_init
#define ghb_dict_iter_next          hb_dict_iter_next_ex

#define ghb_value_incref            hb_value_incref
#define ghb_value_decref            hb_value_decref
#define ghb_value_free              hb_value_free
#define ghb_value_type              hb_value_type
#define ghb_value_dup               hb_value_dup

#define ghb_array_new               hb_value_array_init
#define ghb_array_get               hb_value_array_get
#define ghb_array_insert            hb_value_array_insert
#define ghb_array_append            hb_value_array_append
#define ghb_array_remove            hb_value_array_remove
#define ghb_array_replace           hb_value_array_set
#define ghb_array_len               hb_value_array_len
#define ghb_array_copy              hb_value_array_copy
#define ghb_array_reset             hb_value_array_clear

#define ghb_value_get_int           hb_value_get_int
#define ghb_value_get_double        hb_value_get_double
#define ghb_value_get_bool          hb_value_get_bool
#define ghb_value_get_string        hb_value_get_string
#define ghb_value_get_string_xform  hb_value_get_string_xform

#define ghb_value_xform             hb_value_xform

#define ghb_string_value_new        hb_value_string
#define ghb_int_value_new           hb_value_int
#define ghb_double_value_new        hb_value_double
#define ghb_bool_value_new          hb_value_bool

#define ghb_json_write(file,val)        hb_value_write_file_json(val,file)
#define ghb_json_write_file(path,val)   hb_value_write_json(val,path)
#define ghb_json_parse                  hb_value_json
#define ghb_json_parse_file             hb_value_read_json

gint ghb_value_cmp(const GhbValue *vala, const GhbValue *valb);
void ghb_string_value_set(GhbValue *gval, const gchar *str);

GhbValue* ghb_string_value(const gchar *str);
GhbValue* ghb_int_value(gint64 ival);
GhbValue* ghb_double_value(gdouble dval);
GhbValue* ghb_boolean_value(gboolean bval);

void debug_show_value(GhbValue *gval);
void debug_show_type(GhbType tp);

void ghb_dict_copy(GhbValue *dst, const GhbValue *src);

void ghb_dict_set_string(GhbValue *dict, const gchar *key, const gchar *sval);
void ghb_dict_set_double(GhbValue *dict, const gchar *key, gdouble dval);
void ghb_dict_set_int(GhbValue *dict, const gchar *key, gint64 ival);
void ghb_dict_set_bool(GhbValue *dict, const gchar *key, gboolean bval);

GhbValue* ghb_dict_get_value(const GhbValue *dict, const gchar *key);
gboolean ghb_dict_get_bool(const GhbValue *dict, const gchar *key);
gint64 ghb_dict_get_int(const GhbValue *dict, const gchar *key);
gdouble ghb_dict_get_double(const GhbValue *dict, const gchar *key);
gchar* ghb_dict_get_string_xform(const GhbValue *dict, const gchar *key);
const gchar* ghb_dict_get_string(const GhbValue *dict, const gchar *key);

G_END_DECLS
