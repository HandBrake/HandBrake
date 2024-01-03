/*
 * render_button.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * render_button.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * render_button.c is distributed in the hope that it will be useful,
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
#include "marshalers.h"
#include "renderer_button.h"

#define MyGdkRectangle const GdkRectangle

/* Some boring function declarations: GObject type system stuff */
static void     custom_cell_renderer_button_init       (CustomCellRendererButton      *cellprogress,
                                                        gpointer                       class_data);
static void     custom_cell_renderer_button_class_init (CustomCellRendererButtonClass *klass,
                                                        gpointer                       class_data);
static void     custom_cell_renderer_button_get_property  (GObject                    *object,
                                                             guint                       param_id,
                                                             GValue                     *value,
                                                             GParamSpec                 *pspec);
static void     custom_cell_renderer_button_set_property  (GObject                    *object,
                                                             guint                       param_id,
                                                             const GValue               *value,
                                                             GParamSpec                 *pspec);
static void     custom_cell_renderer_button_finalize (GObject *gobject);

// My customized part that adds "clicked" signal
static gboolean
custom_cell_renderer_button_activate (
                    GtkCellRenderer      *cell,
                    GdkEvent             *event,
                    GtkWidget            *widget,
                    const gchar          *path,
                    MyGdkRectangle       *background_area,
                    MyGdkRectangle       *cell_area,
                    GtkCellRendererState flags);

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint button_cell_signals[LAST_SIGNAL] = { 0 };

static   gpointer parent_class;

/***************************************************************************
 *
 *  custom_cell_renderer_button_get_type: here we register our type with
 *                                        the GObject type system if we
 *                                        haven't done so yet. Everything
 *                                        else is done in the callbacks.
 *
 ***************************************************************************/
GType
custom_cell_renderer_button_get_type (void)
{
    static GType cell_button_type = 0;

    if (cell_button_type == 0)
    {
        static const GTypeInfo cell_button_info =
        {
            sizeof (CustomCellRendererButtonClass),
            NULL,                                                     /* base_init */
            NULL,                                                     /* base_finalize */
            (GClassInitFunc) custom_cell_renderer_button_class_init,
            NULL,                                                     /* class_finalize */
            NULL,                                                     /* class_data */
            sizeof (CustomCellRendererButton),
            0,                                                        /* n_preallocs */
            (GInstanceInitFunc) custom_cell_renderer_button_init,
        };

        /* Derive from GtkCellRendererPixbuf */
        cell_button_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_PIXBUF,
                                                     "CustomCellRendererButton",
                                                      &cell_button_info,
                                                      0);
    }

    return cell_button_type;
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_init: set some default properties of the
 *                                    parent (GtkCellRendererPixbuf).
 *
 ***************************************************************************/
static void
custom_cell_renderer_button_init (CustomCellRendererButton *cellbutton,
                                  gpointer class_data)
{
    g_object_set(cellbutton, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
    g_object_set(cellbutton, "xpad", 2, NULL);
    g_object_set(cellbutton, "ypad", 2, NULL);
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_class_init:
 *
 *  set up our own get_property and set_property functions, and
 *  override the parent's functions that we need to implement.
 *  If you want cells that can be activated on their own (ie. not
 *  just the whole row selected) or cells that are editable, you
 *  will need to override 'activate' and 'start_editing' as well.
 *
 ***************************************************************************/
static void
custom_cell_renderer_button_class_init (CustomCellRendererButtonClass *klass,
                                        gpointer class_data)
{
    GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
    GObjectClass         *object_class = G_OBJECT_CLASS(klass);

    parent_class           = g_type_class_peek_parent (klass);
    object_class->finalize = custom_cell_renderer_button_finalize;

    /* Hook up functions to set and get our
     *   custom cell renderer properties */
    object_class->get_property = custom_cell_renderer_button_get_property;
    object_class->set_property = custom_cell_renderer_button_set_property;

    // Override activate
    cell_class->activate = custom_cell_renderer_button_activate;

    button_cell_signals[CLICKED] =
        g_signal_new (g_intern_static_string ("clicked"),
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_LAST,
              G_STRUCT_OFFSET (CustomCellRendererButtonClass, clicked),
              NULL, NULL,
              ghb_marshal_VOID__STRING,
              G_TYPE_NONE, 1,
              G_TYPE_STRING);
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_finalize: free any resources here
 *
 ***************************************************************************/
static void
custom_cell_renderer_button_finalize (GObject *object)
{
    /*
    If you need to do anything with the renderer button ...
    CustomCellRendererProgress *cellrendererbutton = CUSTOM_CELL_RENDERER_BUTTON(object);
    */

    /* Free any dynamically allocated resources here */

    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_get_property: as it says
 *
 ***************************************************************************/
static void
custom_cell_renderer_button_get_property (GObject    *object,
                                          guint       param_id,
                                          GValue     *value,
                                          GParamSpec *psec)
{
    //CustomCellRendererButton  *cellbutton = CUSTOM_CELL_RENDERER_BUTTON(object);

    switch (param_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
            break;
    }
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_set_property: as it says
 *
 ***************************************************************************/
static void
custom_cell_renderer_button_set_property (GObject      *object,
                                          guint         param_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
    //CustomCellRendererButton *cellbutton = CUSTOM_CELL_RENDERER_BUTTON(object);

    switch (param_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
            break;
    }
}

/***************************************************************************
 *
 *  custom_cell_renderer_button_new: return a new cell renderer instance
 *
 ***************************************************************************/
GtkCellRenderer *
custom_cell_renderer_button_new (void)
{
    return g_object_new(CUSTOM_TYPE_CELL_RENDERER_BUTTON, NULL);
}

static gboolean
custom_cell_renderer_button_activate (
        GtkCellRenderer         *cell,
        GdkEvent                *event,
        GtkWidget               *widget,
        const gchar             *path,
        MyGdkRectangle          *background_area,
        MyGdkRectangle          *cell_area,
        GtkCellRendererState    flags)
{
    ghb_log_func();
    g_signal_emit (cell, button_cell_signals[CLICKED], 0, path);
    return TRUE;
}
