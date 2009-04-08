#include <gtk/gtk.h>
#include "ghbcompositor.h"

// GhbCompositor example
int
main(gint argc, gchar *argv[])
{
    GtkWidget *window;
    GtkWidget *blender;
    GtkWidget *eb_bottom;
    GtkWidget *eb_top1;
    GtkWidget *eb_top2;
    GtkWidget *eb_top3;
    GtkWidget *bottom;
    GtkWidget *top1;
    GtkWidget *top2;
    GtkWidget *top3;
    GtkWidget *table;
    GtkWidget *image;

    gtk_init(&argc, &argv);

    // Make the top level window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    // Only widgets that have a GdkDrawing area can be composited
    // These would be GtkEventBox or GtkDrawingArea
    // Create the widgets that will be composited
    eb_bottom = gtk_event_box_new();
    eb_top1 = gtk_event_box_new();
    eb_top2 = gtk_event_box_new();
    eb_top3 = gtk_event_box_new();

    // Create the compositor
    blender = ghb_compositor_new();

    // Create an button to put on the bottom layer
    bottom = gtk_button_new_with_label("Bottom");
    image = gtk_image_new_from_stock("gtk-help", 6);
    gtk_button_set_image(GTK_BUTTON(bottom), image);

    // The button must be placed inside an event box since
    // buttons do not have their own window.
    gtk_container_add(GTK_CONTAINER(eb_bottom), bottom);

    // Create the buttons that will be visible on the top layer
    top1 = gtk_button_new_with_label("Top 1");
    top2 = gtk_button_new_with_label("Top 2");
    top3 = gtk_button_new_with_label("Top 3");

    // The buttons must be placed inside an event box since
    // buttons do not have their own window.
    gtk_container_add(GTK_CONTAINER(eb_top1), top1);
    gtk_container_add(GTK_CONTAINER(eb_top2), top2);
    gtk_container_add(GTK_CONTAINER(eb_top3), top3);

    // Create the table that will be the top layer
    // Using a layout widget gives flexibility in the layout of the layer
    table = gtk_table_new(3, 3, TRUE);
    gtk_table_attach(GTK_TABLE(table), eb_top1, 0, 1, 0, 1, 0, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eb_top2, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), eb_top3, 2, 3, 2, 3, 0, 0, 0, 0);

    // Add the blender to the main window.
    gtk_container_add(GTK_CONTAINER(window), blender);

    // Set the blenders zlist, with opacity values
    // Bottom layer is opaque, top layer 60%
    ghb_compositor_zlist_insert(GHB_COMPOSITOR(blender), eb_bottom, 1, 1);
    ghb_compositor_zlist_insert(GHB_COMPOSITOR(blender), table, 2, 0.6);

    // Start the show
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}

