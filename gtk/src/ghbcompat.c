/*
 * ghbcompat.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * ghbcompat.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * ghbcompat.h is distributed in the hope that it will be useful,
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

#include "ghbcompat.h"

G_MODULE_EXPORT gboolean
ghb_widget_hide_on_close(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    gpointer *ud)
{
    gtk_widget_set_visible(widget, FALSE);
    return TRUE;
}
