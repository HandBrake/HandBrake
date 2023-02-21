/*
 * resources.h
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * resources.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * resources.h is distributed in the hope that it will be useful,
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

#if !defined(_RESOURCES_H_)
#define _RESOURCES_H_

#include "values.h"

void ghb_resource_init(void);
void ghb_resource_free(void);
GhbValue* ghb_resource_get(const gchar *name);

#endif // _RESOURCES_H_
