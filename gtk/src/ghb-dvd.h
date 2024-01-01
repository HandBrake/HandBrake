/*
 * ghb-dvd.h
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * ghb-dvd.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * ghb-dvd.h is distributed in the hope that it will be useful,
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

#if !defined(_GHB_DVD_H_)
#define _GHB_DVD_H_

#include "settings.h"

void ghb_dvd_set_current(const gchar *name, signal_user_data_t *ud);
gchar* ghb_resolve_symlink(const gchar *name);

#endif // _GHB_DVD_H_
