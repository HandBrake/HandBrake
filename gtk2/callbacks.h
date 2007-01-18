#include <gtk/gtk.h>

extern GtkWidget *hb_win_main;
extern HBHandle  *hb_handle;
extern HBStatus  *hb_status;

gboolean on_win_main_delete_event( GtkWidget       *widget,
                                   GdkEvent        *event,
                                   gpointer         user_data);

gboolean on_win_open_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);


void on_button_browse_device                   (GtkButton       *button,
                                        gpointer         user_data);

void on_ok_button1_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void on_cancel_button1_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void on_entry_device_activate               (GtkEntry        *entry,
                                        gpointer         user_data);

void on_optionmenu_title_changed            (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data);

void on_button_browse_file_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void on_button_settings_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void on_button_start_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void on_entry_file_activate                 (GtkEntry        *entry,
                                        gpointer         user_data);

gboolean on_win_settings_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void on_button_ok_settings_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void on_button_settings_next_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void on_button_settings_previous_clicked    (GtkButton       *button,
                                        gpointer         user_data);

void on_checkbutton_deinterlace_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
void on_spinbutton_width_value_changed( GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void on_spinbutton_crop_top_value_changed   (GtkSpinButton   *spinbutton,
                                         gpointer         user_data);

void on_spinbutton_crop_bottom_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_spinbutton_crop_left_value_changed  (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_spinbutton_crop_right_value_changed (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

