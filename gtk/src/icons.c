#include <gtk/gtk.h>
#include "hb-activity.32.h"
#include "hb-add-queue.32.h"
#include "hb-canceled.16.h"
#include "hb-complete.16.h"
#include "hb-drawer.32.h"
#include "hb-icon.128.h"
#include "hb-icon.64.h"
#include "hb-pause.32.h"
#include "hb-play.32.h"
#include "hb-pref.32.h"
#include "hb-queue.32.h"
#include "hb-queue-delete.16.h"
#include "hb-queue-job.16.h"
#include "hb-queue-pass1.16.h"
#include "hb-queue-pass2.16.h"
#include "hb-queue-subtitle.16.h"
#include "hb-remove.32.h"
#include "hb-source.32.h"
#include "hb-stop.32.h"
#include "hb-working0.16.h"
#include "hb-working1.16.h"
#include "hb-working2.16.h"
#include "hb-working3.16.h"
#include "hb-working4.16.h"
#include "hb-working5.16.h"

void
ghb_load_icons()
{
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new_from_inline(-1, hb_activity32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-activity", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_add_queue32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-add-queue", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_canceled16, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-canceled", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_complete16, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-complete", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_drawer32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-drawer", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_icon128, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-icon", 128, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_play32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-play", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_pref32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-pref", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue_delete16, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue-delete", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue_job16, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue-job", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue_pass1, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue-pass1", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue_pass2, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue-pass2", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_queue_subtitle16, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-queue-subtitle", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_remove32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-remove", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_source32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-source", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_stop32, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-stop", 32, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working0, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working0", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working1, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working1", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working2, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working2", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working3, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working3", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working4, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working4", 16, pb);

	pb = gdk_pixbuf_new_from_inline(-1, hb_working5, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon("hb-working5", 16, pb);

}
