/*
 * plist.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * plist.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * plist.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#if !defined(_PLIST_H_)
#define _PLIST_H_

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "values.h"

GhbValue* ghb_plist_parse(const gchar *buf, gssize len);
GhbValue* ghb_plist_parse_file(const gchar *filename);
void ghb_plist_write(FILE *file, GhbValue *gval);
void ghb_plist_write_file(const gchar *filename, GhbValue *gval);

#endif // _PLIST_H_

