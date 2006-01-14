#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "../core/HandBrake.h"
#include "status.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"


#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)


static int i_settings_preview_picture;
static int i_settings_width = 0;
static int i_settings_crop_top = 0;
static int i_settings_crop_bottom = 0;
static int i_settings_crop_left = 0;
static int i_settings_crop_right = 0;
static int b_settings_deinterlace = 0;

gboolean on_win_main_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    HBClose( &hb_handle );

    hb_handle = NULL;

    gtk_main_quit();
    return FALSE;
}


void on_button_browse_device              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *win_open;

    /* Create the open window */
    win_open = create_win_open();
    gtk_object_set_data( GTK_OBJECT(win_open), "entry", "entry_device" );
    gtk_widget_show( win_open );
    gdk_window_raise( win_open->window );
}

/***
 * Open file dialog box handle (you should have set entry to the right entry name of hb_win_main
 */
gboolean on_win_open_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GtkWidget *win_open = gtk_widget_get_toplevel( GTK_WIDGET (widget) );

    gtk_widget_destroy( win_open );
    return FALSE;
}

void on_ok_button1_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *win_open = gtk_widget_get_toplevel( GTK_WIDGET (button) );

    char       *psz_entry = gtk_object_get_data( GTK_OBJECT( win_open ), "entry" );
    const char *psz_file = gtk_file_selection_get_filename( GTK_FILE_SELECTION( win_open ) );

    GtkWidget *entry = lookup_widget( hb_win_main, psz_entry );

    gtk_entry_set_text( GTK_ENTRY( entry ), psz_file );
    if( !strcmp( psz_entry, "entry_device" ) && *psz_file )
    {
        /* Feed libhb with a device */
        HBScanDVD( hb_handle, psz_file, 0 );
    }
    gtk_widget_destroy( win_open );
}


void on_cancel_button1_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *win_open = gtk_widget_get_toplevel( GTK_WIDGET (button) );
    gtk_widget_destroy( win_open );
}

void on_entry_device_activate( GtkEntry        *entry,
                               gpointer         user_data)
{
    const char *psz_file = gtk_entry_get_text( GTK_ENTRY( entry ) );

    /* Feed libhb with a device */
    if( *psz_file )
    {
        HBScanDVD( hb_handle, psz_file, 0 );
    }
}

static void menu_language_new( GtkOptionMenu *optionmenu, char *name, HBTitle *title, int i_default )
{
    int i;

    /* build audio menu */
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_mnemonic( "None" );
    gtk_widget_show( item );
    gtk_menu_shell_append( GTK_MENU_SHELL(menu), item );

    for( i = 0; i < HBListCount( title->audioList ); i++ )
    {
        HBAudio *audio = (HBAudio*) HBListItemAt( title->audioList, i );

        item = gtk_menu_item_new_with_mnemonic( audio->language );
        gtk_widget_show( item );
        gtk_menu_shell_append( GTK_MENU_SHELL(menu), item );
    }

    gtk_option_menu_set_menu( GTK_OPTION_MENU(optionmenu), menu );
    GLADE_HOOKUP_OBJECT( hb_win_main, menu, name );

    gtk_option_menu_set_history( GTK_OPTION_MENU(optionmenu), i_default );
}

void on_optionmenu_title_changed( GtkOptionMenu   *optionmenu,
                                  gpointer         user_data)
{
    GtkWidget *optionmenu_language = lookup_widget( hb_win_main, "optionmenu_language" );
    GtkWidget *optionmenu_language2 = lookup_widget( hb_win_main, "optionmenu_language2" );

    HBTitle *title;

    int i = gtk_option_menu_get_history( optionmenu );

    title = (HBTitle*) HBListItemAt( hb_status->titleList, i );

    /* FIXME is it ok ?*/
    gtk_option_menu_remove_menu( GTK_OPTION_MENU(optionmenu_language) );
    gtk_option_menu_remove_menu( GTK_OPTION_MENU(optionmenu_language2) );

    if( title )
    {
        /* build audio menu */
        menu_language_new( GTK_OPTION_MENU(optionmenu_language), "menu_language", title, 1 );
        menu_language_new( GTK_OPTION_MENU(optionmenu_language2), "menu_language2", title, 0 );
    }
}







void on_button_browse_file_clicked( GtkButton       *button,
                                    gpointer         user_data)
{
    GtkWidget *win_open;

    /* Create the open window */
    win_open = create_win_open();
    gtk_object_set_data( GTK_OBJECT(win_open), "entry", "entry_file" );
    gtk_widget_show( win_open );
    gdk_window_raise( win_open->window );
}


static void on_settings_darea_expose_event_callback( GtkWidget *widget, GdkEventExpose *event, gpointer data )
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(widget) );
    char label[1024];

    int i;
    uint8_t *rgba;

    GtkWidget *optionmenu_title = lookup_widget( hb_win_main, "optionmenu_title" );
    HBTitle   *title;

    title = HBListItemAt( hb_status->titleList, gtk_option_menu_get_history( GTK_OPTION_MENU(optionmenu_title) ) );
    title->deinterlace = b_settings_deinterlace;
    title->outWidth = i_settings_width;

    title->topCrop = i_settings_crop_top;
    title->bottomCrop = i_settings_crop_bottom;
    title->leftCrop = i_settings_crop_left;
    title->rightCrop = i_settings_crop_right;

    rgba = HBGetPreview( hb_handle, title, i_settings_preview_picture );

    if( title->outWidth != i_settings_width ||
        title->topCrop != i_settings_crop_top ||
        title->bottomCrop != i_settings_crop_bottom ||
        title->leftCrop != i_settings_crop_left ||
        title->rightCrop != i_settings_crop_right )
    {
        i_settings_width = title->outWidth;

        i_settings_crop_top = title->topCrop;
        i_settings_crop_bottom = title->bottomCrop;
        i_settings_crop_left = title->leftCrop;
        i_settings_crop_right = title->rightCrop;

        gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_width")), i_settings_width );
        gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_top")), i_settings_crop_top );
        gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_bottom")), i_settings_crop_bottom );
        gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_left")), i_settings_crop_left );
        gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_right")), i_settings_crop_right );
    }

    sprintf( label, "Final size: %dx%d", title->outWidth, title->outHeight );
    gtk_label_set_text( GTK_LABEL(lookup_widget(win, "label_settings")), label );

    /* FIXME do that only under x86 */
    for( i = 0; i < (title->outWidthMax + 2)*(title->outHeightMax + 2); i++ )
    {
        const uint8_t r = rgba[4*i+2];
        const uint8_t g = rgba[4*i+1];
        const uint8_t b = rgba[4*i+0];
        const uint8_t a = rgba[4*i+3];

        rgba[4*i+0] = r;
        rgba[4*i+1] = g;
        rgba[4*i+2] = b;
        rgba[4*i+3] = a;
    }
    gdk_draw_rgb_32_image( widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                           0, 0, title->outWidthMax + 2, title->outHeightMax + 2,
                           GDK_RGB_DITHER_MAX,
                           rgba,
                           (title->outWidthMax+2) * 4 );

    free( rgba );
}

void on_spinbutton_width_value_changed( GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(spinbutton) );

    g_print( "on_spinbutton_width_value_changed\n" );

    i_settings_width = gtk_spin_button_get_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_width")) );

    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );

}
/*
 * FIXME use only one callback
 */
void
on_spinbutton_crop_top_value_changed   (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(spinbutton) );

    i_settings_crop_top = gtk_spin_button_get_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_top")) );

    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
}


void
on_spinbutton_crop_bottom_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(spinbutton) );

    i_settings_crop_bottom = gtk_spin_button_get_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_bottom")) );

    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
}


void
on_spinbutton_crop_left_value_changed  (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(spinbutton) );

    i_settings_crop_left = gtk_spin_button_get_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_left")) );

    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
}


void
on_spinbutton_crop_right_value_changed (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(spinbutton) );

    i_settings_crop_right = gtk_spin_button_get_value( GTK_SPIN_BUTTON(lookup_widget(win, "spinbutton_crop_right")) );

    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
}


void on_button_settings_clicked( GtkButton       *button,
                                 gpointer         user_data)
{
    GtkWidget *optionmenu_title = lookup_widget( hb_win_main, "optionmenu_title" );
    HBTitle   *title;

    GtkWidget *settings = create_win_settings();
    GtkWidget *darea = lookup_widget( settings, "drawingarea" );

    title = HBListItemAt( hb_status->titleList, gtk_option_menu_get_history( GTK_OPTION_MENU(optionmenu_title) ) );

    i_settings_preview_picture = 0;

    gtk_widget_set_size_request( darea, title->outWidthMax + 2, title->outHeightMax + 2 );

    g_signal_connect (G_OBJECT(darea), "expose_event",
                    G_CALLBACK (on_settings_darea_expose_event_callback), NULL);

    gtk_spin_button_set_value( GTK_SPIN_BUTTON(lookup_widget(settings, "spinbutton_width")), title->outWidth );

    gtk_widget_show( settings );
}

static void DisplayError( char *title, char *msg )
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                    "<b>%s</b>\n\n%s", title, msg );
    gtk_label_set_use_markup( GTK_LABEL( GTK_MESSAGE_DIALOG(dialog)->label), TRUE );
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK );
    gtk_window_set_modal( GTK_WINDOW(dialog), TRUE );
    gtk_dialog_run( GTK_DIALOG(dialog) );
    gtk_widget_destroy( dialog );
}

void on_button_start_clicked( GtkButton       *button,
                              gpointer         user_data)
{
    int        i;

    if( hb_status->i_state != HB_STATE_ENCODING )
    {
        GtkWidget *optionmenu_title = lookup_widget( hb_win_main, "optionmenu_title" );
        GtkWidget *widget;
        HBTitle   *title = HBListItemAt( hb_status->titleList, gtk_option_menu_get_history( GTK_OPTION_MENU(optionmenu_title) ) );

        int i_ab;

        widget = lookup_widget( hb_win_main, "entry_file" );
        title->file = strdup( gtk_entry_get_text( GTK_ENTRY(widget) ) );

        if( title->file == NULL || *title->file == '\0' )
        {
            DisplayError( "Missing parameters", "You need to specify a file" );
            return;
        }

        widget = lookup_widget( hb_win_main, "checkbutton_2pass" );
        title->twoPass = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget) );

        title->deinterlace = b_settings_deinterlace;

        title->outWidth = i_settings_width;

        title->topCrop = i_settings_crop_top;
        title->bottomCrop = i_settings_crop_bottom;
        title->leftCrop = i_settings_crop_left;
        title->rightCrop = i_settings_crop_right;

        widget = lookup_widget( hb_win_main, "radiobutton_cbitrate" );
        if( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON( widget ) ) )
        {
            widget = lookup_widget( hb_win_main, "spinbutton_bitrate" );
            title->bitrate = gtk_spin_button_get_value( GTK_SPIN_BUTTON( widget ) );
        }
        else
        {
            int i_size;
            widget = lookup_widget( hb_win_main, "spinbutton_size" );
            i_size = gtk_spin_button_get_value( GTK_SPIN_BUTTON( widget ) );

            /* FIXME FIXME target size FIXME FIXME */
            title->bitrate = 1024;
            /* FIXME */
        }

        widget = lookup_widget( hb_win_main, "optionmenu_codec" );
        switch( gtk_option_menu_get_history( GTK_OPTION_MENU(widget) ) )
        {
            case 0:
                title->codec = HB_CODEC_FFMPEG;
                break;
            case 1:
                title->codec = HB_CODEC_XVID;
                break;
            case 2:
                title->codec = HB_CODEC_X264;
                break;
        }

        widget = lookup_widget( hb_win_main, "optionmenu_format" );
        switch( gtk_option_menu_get_history( GTK_OPTION_MENU(widget) ) )
        {
            case 0:
            {
                /* Auto detect */
                char *p = strrchr( title->file, '.' );
                if( p && !strcasecmp( p, ".avi" ) )
                {
                    title->mux = HB_MUX_AVI;
                }
                else if( p && !strcasecmp( p, ".mp4" ) )
                {
                    title->mux = HB_MUX_MP4;
                }
                else if( p && !strcasecmp( p, ".ogm" ) )
                {
                    title->mux = HB_MUX_OGM;
                }
                else
                {
                    title->mux = HB_MUX_AVI;
                }
                break;
            }
            case 1:
                title->mux = HB_MUX_AVI;
                break;
            case 2:
                title->mux = HB_MUX_MP4;
                break;
            case 3:
                title->mux = HB_MUX_OGM;
                break;
        }

        /* audio */
        widget = lookup_widget( hb_win_main, "optionmenu_bitrate" );
        switch( gtk_option_menu_get_history( GTK_OPTION_MENU(widget) ) )
        {
            case 0:
                i_ab = 64;
                break;
            case 1:
                i_ab = 92;
                break;
            case 2:
                i_ab = 128;
                break;
            case 3:
                i_ab = 160;
                break;
            case 4:
                i_ab = 192;
                break;
            case 5:
                i_ab = 256;
                break;
            case 6:
            default:
                i_ab = 320;
                break;

        }

        HBListInit( &title->ripAudioList );
        widget = lookup_widget( hb_win_main, "optionmenu_language" );
        if( ( i = gtk_option_menu_get_history( GTK_OPTION_MENU(widget) ) ) > 0 )
        {
            HBAudio   *audio = HBListItemAt( title->audioList, i - 1 );

            audio->outBitrate = i_ab;
            if( title->mux == HB_MUX_AVI )
            {
                audio->codec = HB_CODEC_MP3;
            }
            else if( title->mux == HB_MUX_MP4 )
            {
                audio->codec = HB_CODEC_AAC;
            }
            else if( title->mux == HB_MUX_OGM )
            {
                audio->codec = HB_CODEC_VORBIS;
            }
            HBListAdd( title->ripAudioList, audio );
        }

        widget = lookup_widget( hb_win_main, "optionmenu_language2" );
        if( ( i = gtk_option_menu_get_history( GTK_OPTION_MENU(widget) ) ) > 0 )
        {
            HBAudio   *audio = HBListItemAt( title->audioList, i - 1 );

            audio->outBitrate = i_ab;
            if( title->mux == HB_MUX_AVI )
            {
                audio->codec = HB_CODEC_MP3;
            }
            else if( title->mux == HB_MUX_MP4 )
            {
                audio->codec = HB_CODEC_AAC;
            }
            else if( title->mux == HB_MUX_OGM )
            {
                audio->codec = HB_CODEC_VORBIS;
            }
            HBListAdd( title->ripAudioList, audio );
        }
        HBStartRip( hb_handle, title );
    }
    else if( hb_status->i_state == HB_STATE_ENCODING )
    {
        HBStopRip( hb_handle );
    }
}


void on_entry_file_activate( GtkEntry        *entry,
                             gpointer         user_data)
{
#if 0
    GtkWidget *widget = lookup_widget( hb_win_main, "button_start" );
    char *psz_file = gtk_entry_get_text( entry );

    if( psz_file && *psz_file )
    {
        gtk_widget_set_sensitive( widget, TRUE );
    }
    else
    {
        gtk_widget_set_sensitive( widget, FALSE );
    }
#endif
}


static void SettingsUpdate( GtkWidget *win )
{
    b_settings_deinterlace = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(lookup_widget(win, "checkbutton_deinterlace")) );
}

gboolean on_win_settings_delete_event( GtkWidget       *widget,
                                       GdkEvent        *event,
                                       gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET (widget) );

    SettingsUpdate( win );
    gtk_widget_destroy( win );

    return FALSE;
}


void on_button_ok_settings_clicked( GtkButton       *button,
                                    gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(button) );

    SettingsUpdate( win );

    gtk_widget_destroy( win );
}


void on_button_settings_next_clicked( GtkButton       *button,
                                      gpointer         user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(button) );

    if( i_settings_preview_picture < 9 )
    {
        i_settings_preview_picture++;
        gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
    }

    if( i_settings_preview_picture == 9 )
    {
        gtk_widget_set_sensitive( lookup_widget(win, "button_settings_next"), FALSE );
    }
    if( i_settings_preview_picture > 0 )
    {
        gtk_widget_set_sensitive( lookup_widget(win, "button_settings_previous"), TRUE );
    }
}


void on_button_settings_previous_clicked( GtkButton *button,
                                          gpointer   user_data)
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(button) );

    if( i_settings_preview_picture > 0 )
    {
        i_settings_preview_picture--;
        gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
    }

    if( i_settings_preview_picture < 9 )
    {
        gtk_widget_set_sensitive( lookup_widget(win, "button_settings_next"), TRUE );
    }
    if( i_settings_preview_picture == 0 )
    {
        gtk_widget_set_sensitive( lookup_widget(win, "button_settings_previous"), FALSE );
    }
}


void on_checkbutton_deinterlace_toggled( GtkToggleButton *togglebutton,
                                         gpointer         user_data )
{
    GtkWidget *win = gtk_widget_get_toplevel( GTK_WIDGET(togglebutton) );
    b_settings_deinterlace = gtk_toggle_button_get_active( togglebutton );
    gtk_widget_queue_draw( lookup_widget(win, "drawingarea" ) );
}


