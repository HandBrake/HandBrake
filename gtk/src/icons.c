/*
 * icons.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * icons.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * icons.c is distributed in the hope that it will be useful,
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

#include "ghbcompat.h"
#include "icon_res.h"
#include "icons.h"

void
ghb_load_icons (void)
{
    ghb_icons_register_resource();
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(),
                                     "/fr/handbrake/ghb/icons");
}
