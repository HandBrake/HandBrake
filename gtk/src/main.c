/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * main.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "renderer_button.h"
#include "hb-backend.h"
#include "ghb-dvd.h"


/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif



#include "callbacks.h"
#include "settings.h"

#define BUILDER_NAME "ghb"

static gchar*
find_builder_file(gchar *builder_name)
{
	const gchar *dir;
	const gchar * const *dirs;
	gchar *file;
	gint ii;

	file = g_strdup_printf("./%s.ui", builder_name);
	if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
	{
		return file;
	}
	g_free(file);
	// The following resolves to /$HOME/.local/share currently
	dir = g_get_user_data_dir();
	file = g_strdup_printf("%s/%s/builder/%s.ui", dir, builder_name, builder_name);
	if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
	{
		return file;
	}
	g_free(file);
	dirs = g_get_system_data_dirs();
	if (dirs != NULL)
	{
		for (ii = 0; dirs[ii] != NULL; ii++)
		{
			file = g_strdup_printf("%s/%s/builder/%s.ui", dirs[ii], builder_name, builder_name);
			if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
			{
				return file;
			}
			g_free(file);
		}
	}
	return NULL;
}

GtkBuilder*
create_builder_or_die(const gchar * name,
                    const gchar * path)
{
	guint res;
    const gchar *markup =
        N_("<b><big>Unable to create %s.</big></b>\n"
        "\n"
        "Could not open Gtk Builder file (%s). Please check your "
        "installation.\n");
	g_debug("create_builder_or_die ()\n");
	GtkBuilder *xml = gtk_builder_new();
	if (xml != NULL)
		res = gtk_builder_add_from_file(xml, path, NULL);
    if (!xml || !res) 
	{
        GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _(markup),
            name, path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(EXIT_FAILURE);
    }
    return xml;
}

static GCallback
self_symbol_lookup(const gchar * symbol_name)
{
    static GModule *module = NULL;
    gpointer symbol = NULL;

    if (!module)
        module = g_module_open(NULL, 0);

    g_module_symbol(module, symbol_name, &symbol);
    return (GCallback) symbol;
}


static void
MyConnect(
	GtkBuilder *builder,
	GObject *object,
	const gchar *signal_name,
	const gchar *handler_name,
	GObject *connect_object,
	GConnectFlags flags,
	gpointer user_data)
{
	GCallback callback;

    g_return_if_fail(object != NULL);
    g_return_if_fail(handler_name != NULL);
    g_return_if_fail(signal_name != NULL);

	//const gchar *name = gtk_widget_get_name((GtkWidget*)object);
	//g_message("\n\nname %s", name);
	g_debug("handler_name %s", handler_name);
	g_debug("signal_name %s", signal_name);
    callback = self_symbol_lookup(handler_name);
    if (!callback) 
	{
        g_message("Signal handler (%s) not found", handler_name);
        return;
    }
    if (connect_object) 
	{
        g_signal_connect_object(object, signal_name, callback, connect_object, flags);
    }
    else 
	{
        if (flags & G_CONNECT_AFTER)
		{
            g_signal_connect_after( object, signal_name, callback, user_data);
		}
        else
		{
            g_signal_connect(object, signal_name, callback, user_data);
		}
    }
}

#if 0
// If you should ever need to change the font for the running application..
// Ugly but effective.
void
change_font(GtkWidget *widget, gpointer data)
{
    PangoFontDescription *font_desc;
    gchar *font = (gchar*)data;
    const gchar *name;

    font_desc = pango_font_description_from_string(font);
    if (font_desc == NULL) exit(1);
    gtk_widget_modify_font(widget, font_desc);
    name = gtk_widget_get_name(widget);
    g_debug("changing font for widget %s\n", name);
    if (GTK_IS_CONTAINER(widget))
    {
        gtk_container_foreach((GtkContainer*)widget, change_font, data);
    }
}
    //gtk_container_foreach((GtkContainer*)window, change_font, "sans 20");
#endif

extern void chapter_list_selection_changed_cb(void);
extern void chapter_edited_cb(void);

// Create and bind the tree model to the tree view for the chapter list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_chapter_tree_model (signal_user_data_t *ud)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkListStore *treestore;
	GtkTreeView  *treeview;
	GtkTreeSelection *selection;

	g_debug("bind_chapter_tree_model ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "chapters_list"));
	selection = gtk_tree_view_get_selection (treeview);
	treestore = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Chapter No."), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
					_("Chapter Title"), cell, "text", 1, "editable", 2, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
	g_signal_connect(cell, "edited", chapter_edited_cb, ud);

	g_signal_connect(selection, "changed", chapter_list_selection_changed_cb, ud);
	g_debug("Done\n");
}

extern void queue_list_selection_changed_cb(void);
extern void queue_remove_clicked_cb(void);
extern void queue_list_size_allocate_cb(void);

// Create and bind the tree model to the tree view for the queue list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_queue_tree_model (signal_user_data_t *ud)
{
	GtkCellRenderer *cell, *textcell;
	GtkTreeViewColumn *column;
	GtkTreeStore *treestore;
	GtkTreeView  *treeview;
	GtkTreeSelection *selection;

	g_debug("bind_queue_tree_model ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "queue_list"));
	selection = gtk_tree_view_get_selection (treeview);
	treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, _("Job Information"));
	cell = gtk_cell_renderer_pixbuf_new();
	g_object_set(cell, "yalign", 0.0, NULL);
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_add_attribute (column, cell, "icon-name", 0);
	textcell = gtk_cell_renderer_text_new();
	g_object_set(textcell, "wrap-mode", PANGO_WRAP_CHAR, NULL);
	g_object_set(textcell, "wrap-width", 500, NULL);
	gtk_tree_view_column_pack_start (column, textcell, TRUE);
	gtk_tree_view_column_add_attribute (column, textcell, "markup", 1);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_max_width (column, 550);

	cell = custom_cell_renderer_button_new();
	g_object_set(cell, "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(
									_(""), cell, "icon-name", 2, NULL);
	gtk_tree_view_column_set_min_width (column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	g_signal_connect(selection, "changed", queue_list_selection_changed_cb, ud);
	g_signal_connect(cell, "clicked", queue_remove_clicked_cb, ud);
	g_signal_connect(treeview, "size-allocate", queue_list_size_allocate_cb, textcell);
}

extern void audio_list_selection_changed_cb(void);

// Create and bind the tree model to the tree view for the audio track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_audio_tree_model (signal_user_data_t *ud)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkListStore *treestore;
	GtkTreeView  *treeview;
	GtkTreeSelection *selection;
	GtkWidget *widget;

	g_debug("bind_audio_tree_model ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	// 11 columns in model.  6 are visible, the other 5 are for storing
	// values that I need
	treestore = gtk_list_store_new(11, G_TYPE_STRING, G_TYPE_STRING, 
								   G_TYPE_STRING, G_TYPE_STRING, 
								   G_TYPE_STRING, G_TYPE_STRING,
								   G_TYPE_STRING, G_TYPE_STRING, 
								   G_TYPE_STRING, G_TYPE_STRING,
								   G_TYPE_STRING);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Track"), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Codec"), cell, "text", 1, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Bitrate"), cell, "text", 2, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Sample Rate"), cell, "text", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("Mix"), cell, "text", 4, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
									_("DRC"), cell, "text", 5, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

	g_signal_connect(selection, "changed", audio_list_selection_changed_cb, ud);
	// Need to disable remove and update buttons since there are initially
	// no selections
	widget = GHB_WIDGET (ud->builder, "audio_remove");
	gtk_widget_set_sensitive(widget, FALSE);
	g_debug("Done\n");
}

extern void presets_list_selection_changed_cb(void);

// Create and bind the tree model to the tree view for the preset list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_presets_tree_model (signal_user_data_t *ud)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkListStore *treestore;
	GtkTreeView  *treeview;
	GtkTreeSelection *selection;
	GtkWidget *widget;

	g_debug("bind_presets_tree_model ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	treestore = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, 
								   G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Preset Name"), cell, 
					"text", 0, "weight", 1, "style", 2, "foreground", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
	gtk_tree_view_set_tooltip_column (treeview, 4);
	g_signal_connect(selection, "changed", presets_list_selection_changed_cb, ud);
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	gtk_widget_set_sensitive(widget, FALSE);
	g_debug("Done\n");
}

static void
IoRedirect(signal_user_data_t *ud)
{
	GIOChannel *channel;
	gint pfd[2];

	// I'm opening a pipe and attaching the writer end to stderr
	// The reader end will be polled by main event loop and I'll get
	// a callback when there is data available.
	if (pipe( pfd ) < 0)
	{
		g_warning("Failed to redirect IO. Logging impaired\n");
		return;
	}
	// Open activity log.
	// TODO: Put this in the same directory as the encode destination
	ud->activity_log = g_io_channel_new_file ("Activity.log", "w", NULL);
	// Set encoding to raw.
	g_io_channel_set_encoding (ud->activity_log, NULL, NULL);
	stderr->_fileno = pfd[1];
	stdin->_fileno = pfd[0];
	channel = g_io_channel_unix_new (pfd[0]);
	// I was getting an this error:
	// "Invalid byte sequence in conversion input"
	// Set disable encoding on the channel.
	g_io_channel_set_encoding (channel, NULL, NULL);
	g_io_add_watch (channel, G_IO_IN, ghb_log_cb, (gpointer)ud );
}

typedef struct
{
	gchar *filename;
	gchar *iconname;
} icon_map_t;

static gchar *dvd_device = NULL;
static gchar *arg_preset = NULL;

static GOptionEntry entries[] = 
{
	{ "device", 'd', 0, G_OPTION_ARG_FILENAME, &dvd_device, "The device or file to encode", NULL },
	{ "preset", 'p', 0, G_OPTION_ARG_STRING, &arg_preset, "The preset values to use for encoding", NULL },
	{ NULL }
};

#if defined(__linux__)
void drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, signal_user_data_t *ud);
//void drive_disconnected_cb(GnomeVFSVolumeMonitor *gvm, GnomeVFSDrive *gd, signal_user_data_t *ud);

void
watch_volumes(signal_user_data_t *ud)
{
	GVolumeMonitor *gvm;
	gvm = g_volume_monitor_get ();

	g_signal_connect(gvm, "drive-changed", (GCallback)drive_changed_cb, ud);
	//g_signal_connect(gvm, "drive-connected", (GCallback)drive_connected_cb, ud);
}
#else
void
watch_volumes(signal_user_data_t *ud)
{
}
#endif

// Hack to avoid a segfault in libavcodec
extern int mm_flags;
int mm_support();

void x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud);

int
main (int argc, char *argv[])
{
 	GtkWidget *window;
	signal_user_data_t *ud;
	const gchar *preset;
	gchar *builder_file;
	GError *error = NULL;
	GOptionContext *context;

	mm_flags = mm_support();
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	context = g_option_context_new ("- Rip and encode DVD or MPEG file");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, &error);
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);

#if defined(__linux__)
	ghb_hal_init();
#endif

	// Add ./icons so that we can find the icons if we haven't installed
	GtkIconTheme *theme;
	theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_append_search_path (theme, "./icons");
	
	ud = g_malloc(sizeof(signal_user_data_t));
	ud->debug = FALSE;
	g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, ud);
	ud->settings = ghb_settings_new();
	ud->audio_settings = NULL;
	ud->chapter_list = NULL;
	ud->queue = NULL;
	ud->current_dvd_device = NULL;
	// Redirect stderr to the activity window
	IoRedirect(ud);
	// Enable events that alert us to media change events
	watch_volumes (ud);
	builder_file = find_builder_file(BUILDER_NAME);
	if (builder_file != NULL)
	{
		ud->builder = create_builder_or_die (BUILDER_NAME, builder_file);
		g_free(builder_file);
	}

	// Need to connect x264_options textview buffer to the changed signal
	// since it can't be done automatically
	GtkTextView *textview;
	GtkTextBuffer *buffer;
	textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "x264_options"));
	buffer = gtk_text_view_get_buffer (textview);
	g_signal_connect(buffer, "changed", (GCallback)x264_entry_changed_cb, ud);

	ghb_file_menu_add_dvd(ud);
	ghb_backend_init(ud->builder, 1, 0);

	g_debug("ud %p\n", ud);
	g_debug("ud->builder %p\n", ud->builder);
	bind_audio_tree_model(ud);
	bind_presets_tree_model(ud);
	bind_queue_tree_model(ud);
	bind_chapter_tree_model(ud);
	// Connect up the signals to their callbacks
	// I wrote my own connector so that I could pass user data
	// to the callbacks.  Builder's standard autoconnect doesn't all this.
	gtk_builder_connect_signals_full (ud->builder, MyConnect, ud);

	ghb_prefs_load(ud);
	if (ghb_settings_get_bool(ud->settings, "hbfd"))
	{
		ghb_hbfd(ud, TRUE);
	}
	const gchar *source = ghb_settings_get_string(ud->settings, "default_source");
	gboolean tweaks = ghb_settings_get_bool(ud->settings, "allow_tweaks");
	GtkWidget *widget;
	widget = GHB_WIDGET(ud->builder, "deinterlace");
	tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);
	widget = GHB_WIDGET(ud->builder, "tweak_deinterlace");
	!tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);

	widget = GHB_WIDGET(ud->builder, "denoise");
	tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);
	widget = GHB_WIDGET(ud->builder, "tweak_denoise");
	!tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);

	ghb_dvd_set_current(source, ud);
	// Start the show.
	window = GHB_WIDGET (ud->builder, "hb_window");
	gtk_widget_show (window);

	// Load the presets files
	ghb_presets_load(ud);
	// Populate the presets tree view
	ghb_presets_list_update(ud);
	// Get the first preset name
	if (arg_preset != NULL)
	{
		preset = arg_preset;
	}
	else
	{
		preset = ghb_settings_get_string (ud->settings, "default_preset");
		if (preset == NULL)
			preset = ghb_presets_get_name(0);
	}
	// Parsing x264 options "" initializes x264 widgets to proper defaults
	ghb_x264_parse_options(ud, "");
	ghb_select_preset(ud->builder, preset);
	ghb_prefs_to_ui(ud);
	// Grey out widgets that are dependent on a disabled feature
	ghb_check_all_depencencies (ud);

	if (dvd_device != NULL)
	{
		// Source overridden from command line option
		ghb_settings_set_string(ud->settings, "source", dvd_device);
	}
	// Start timer for monitoring libhb status, 500ms
	g_timeout_add (500, ghb_timer_cb, (gpointer)ud);
	// Everything should be go-to-go.  Lets rock!
	gtk_main ();
	g_free(ud);
	return 0;
}

