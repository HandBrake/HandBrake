#ifndef _RENDERER_BUTTON_H_
#define _RENDERER_BUTTON_H_

#include <gtk/gtk.h>

/* Some boilerplate GObject type check and type cast macros.
*  'klass' is used here instead of 'class', because 'class'
*  is a c++ keyword */
#define CUSTOM_TYPE_CELL_RENDERER_BUTTON             (custom_cell_renderer_button_get_type())
#define CUSTOM_CELL_RENDERER_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_BUTTON, CustomCellRendererButton))
#define CUSTOM_CELL_RENDERER_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_BUTTON, CustomCellRendererButtonClass))
#define CUSTOM_IS_CELL_BUTTON_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_BUTTON))
#define CUSTOM_IS_CELL_BUTTON_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_BUTTON))
#define CUSTOM_CELL_RENDERER_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_BUTTON, CustomCellRendererButtonClass))

typedef struct _CustomCellRendererButton CustomCellRendererButton;
typedef struct _CustomCellRendererButtonClass CustomCellRendererButtonClass;

/* CustomCellRendererProgress: Our custom cell renderer
*   structure. Extend according to need */
struct _CustomCellRendererButton
{
	GtkCellRendererPixbuf   parent;
};

struct _CustomCellRendererButtonClass
{
	GtkCellRendererPixbufClass  parent_class;

	void (* clicked) (CustomCellRendererButton *cell_renderer_button,
						const gchar *path);
};

GType                custom_cell_renderer_button_get_type (void);
GtkCellRenderer     *custom_cell_renderer_button_new (void);


#endif // _RENDERER_BUTTON_H_

