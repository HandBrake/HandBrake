/*
 * ghbcellrenderertext.h
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * ghbcellrenderertext.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * ghbcellrenderertext.h is distributed in the hope that it will be useful,
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

#ifndef __GHB_CELL_RENDERER_TEXT_H__
#define __GHB_CELL_RENDERER_TEXT_H__

#include <pango/pango.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS


#define GHB_TYPE_CELL_RENDERER_TEXT     (ghb_cell_renderer_text_get_type ())
#define GHB_CELL_RENDERER_TEXT(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHB_TYPE_CELL_RENDERER_TEXT, GhbCellRendererText))
#define GHB_CELL_RENDERER_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GHB_TYPE_CELL_RENDERER_TEXT, GhbCellRendererTextClass))
#define GHB_IS_CELL_RENDERER_TEXT(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHB_TYPE_CELL_RENDERER_TEXT))
#define GHB_IS_CELL_RENDERER_TEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GHB_TYPE_CELL_RENDERER_TEXT))
#define GHB_CELL_RENDERER_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GHB_TYPE_CELL_RENDERER_TEXT, GhbCellRendererTextClass))

typedef struct _GhbCellRendererText         GhbCellRendererText;
typedef struct _GhbCellRendererTextPrivate  GhbCellRendererTextPrivate;
typedef struct _GhbCellRendererTextClass    GhbCellRendererTextClass;

struct _GhbCellRendererText
{
  GtkCellRenderer parent;
};

struct _GhbCellRendererTextClass
{
  GtkCellRendererClass parent_class;

  void (* edited) (GhbCellRendererText *cell_renderer_text,
           const gchar         *path,
           const gchar         *new_text);

  gboolean (* keypress) (GhbCellRendererText *cell_renderer_text,
           GdkEventKey *event);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType            ghb_cell_renderer_text_get_type (void) G_GNUC_CONST;
GtkCellRenderer *ghb_cell_renderer_text_new      (void);

void             ghb_cell_renderer_text_set_fixed_height_from_font (GhbCellRendererText *renderer,
                                    gint                 number_of_rows);


G_END_DECLS


#endif /* __GHB_CELL_RENDERER_TEXT_H__ */
