/*
 * appcast.h
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * appcast.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * appcast.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#if !defined(_GHB_APPCAST_H_)
#define _GHB_APPCAST_H_

void ghb_appcast_parse(
    gchar *buf, gchar **desc, gchar **build, gchar **version);

#endif // _GHB_APPCAST_H_
