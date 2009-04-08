/* "Borrowed" from: */
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */
#ifndef __GHB_COMPOSITOR_H__
#define __GHB_COMPOSITOR_H__


#include <gtk/gtkbin.h>


G_BEGIN_DECLS

#define GHB_TYPE_COMPOSITOR              (ghb_compositor_get_type ())
#define GHB_COMPOSITOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHB_TYPE_COMPOSITOR, GhbCompositor))
#define GHB_COMPOSITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GHB_TYPE_COMPOSITOR, GhbCompositorClass))
#define GHB_IS_COMPOSITOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHB_TYPE_COMPOSITOR))
#define GHB_IS_COMPOSITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GHB_TYPE_COMPOSITOR))
#define GHB_COMPOSITOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GHB_TYPE_COMPOSITOR, GhbCompositorClass))

typedef struct _GhbCompositor       GhbCompositor;
typedef struct _GhbCompositorClass  GhbCompositorClass;
typedef struct _GhbCompositorChild  GhbCompositorChild;

struct _GhbCompositor
{
    GtkContainer  container;
    GList        *children;
};

struct _GhbCompositorClass
{
    GtkContainerClass parent_class;
};

struct _GhbCompositorChild
{
    GtkWidget *widget;
    GList     *drawables;
    guint      z_pos;
    gdouble    opacity;
};

GType      ghb_compositor_get_type           (void) G_GNUC_CONST;
GtkWidget* ghb_compositor_new                (void);
void       ghb_compositor_zlist_insert       (GhbCompositor *compositor,
                                              GtkWidget *child, 
                                              gint z_pos, gdouble opacity);

G_END_DECLS

#endif /* __GHB_COMPOSITOR_H__ */
