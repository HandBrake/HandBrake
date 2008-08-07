## Process this file with automake to produce Makefile.in

## Created by Anjuta

HB_LIBS=$(HB_DIR)/libhb/libhb.a -la52 -lmkv -lavformat -lavcodec -lavutil -ldca -ldvdread -lfaac -lmp3lame -lmpeg2 -lvorbis -lvorbisenc -logg -lsamplerate -lx264 -lxvidcore -lmp4v2 -lswscale -ltheora -lfaad -lz -lbz2 -lpthread

icons =	\
	hb-activity.32.png \
	hb-add-queue.32.png \
	hb-canceled.16.png \
	hb-complete.16.png \
	hb-drawer.32.png \
	hb-icon.128.png \
	hb-icon.64.png \
	hb-pause.32.png \
	hb-play.32.png \
	hb-queue.32.png \
	hb-queue-delete.16.png \
	hb-queue-job.16.png \
	hb-queue-pass1.16.png \
	hb-queue-pass2.16.png \
	hb-queue-subtitle.16.png \
	hb-remove.32.png \
	hb-source.32.png \
	hb-stop.32.png \
	hb-working0.16.png \
	hb-working1.16.png \
	hb-working2.16.png \
	hb-working3.16.png \
	hb-working4.16.png \
	hb-working5.16.png 

hb_data = standard_presets
hb_menu = ghb.desktop

builderdir = $(datadir)/ghb/builder
builder_DATA = ghb.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GHB_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = ghb 
BUILT_SOURCES = HandBrakeCLI

HandBrakeCLI: $(HB_DIR)/HandBrakeCLI
	/bin/cp $< $@

# Dummy file, not built.  Forces g++ linking
nodist_EXTRA_ghb_SOURCES = dummy.cpp

ghb_SOURCES = \
	callbacks.c \
	callbacks.h \
	main.c \
	settings.c \
	settings.h \
	hb-backend.c \
	hb-backend.h \
	renderer_button.h \
	renderer_button.c \
	ghb-dvd.c \
	ghb-dvd.h 

ghb_LDFLAGS = \
	-Wl,--export-dynamic

ghb_LDADD = $(GHB_LIBS) $(HB_LIBS)

ghb_DEPENDENCIES = $(HB_DIR)/libhb/libhb.a

EXTRA_DIST = $(builder_DATA) $(icons) HandBrakeCLI

uninstall-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		rm -f $(DESTDIR)/$(datadir)/icons/hicolor/$$SIZE/apps/$$FILE; \
	done
	for file in $(hb_data); do \
		rm -f $(DESTDIR)/$(datadir)/ghb/$$file; \
	done
	rm -f $(DESTDIR)/$(datadir)/applications/$(hb_menu)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(DESTDIR)/$(datadir)/icons/hicolor

install-exec-local:
	$(INSTALL_PROGRAM) $(srcdir)/HandBrakeCLI $(DESTDIR)/$(bindir)/HandBrakeCLI

install-data-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		mkdir -p $(DESTDIR)/$(datadir)/icons/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)/$(datadir)/icons/hicolor/$$SIZE/apps/$$FILE; \
	done
	for file in $(hb_data); do \
		mkdir -p $(DESTDIR)/$(datadir)/ghb/; \
		$(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)/$(datadir)/ghb/$$file; \
	done
	@-if test -n "$(DESTDIR)/$(datadir)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	mkdir -p $(DESTDIR)/$(datadir)/applications/; \
	$(INSTALL_DATA) $(srcdir)/$(hb_menu) $(DESTDIR)/$(datadir)/applications/$(hb_menu)
