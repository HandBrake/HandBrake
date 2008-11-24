## Process this file with automake to produce Makefile.in

## Created by Anjuta

#HB_LIBS=$(HB_DIR)/libhb/libhb.a -la52 -lmkv -lavformat -lavcodec -lavutil -ldca -ldvdread -lfaac -lmp3lame -lmpeg2 -lvorbis -lvorbisenc -logg -lsamplerate -lx264 -lxvidcore -lmp4v2 -lswscale -ltheora -lfaad -lz -lbz2 -lpthread

HB_LIBS=\
	$(HB_DIR)/libhb/libhb.a \
	$(HB_DIR)/contrib/lib/liba52.a \
	$(HB_DIR)/contrib/lib/libmkv.a \
	$(HB_DIR)/contrib/lib/libavformat.a \
	$(HB_DIR)/contrib/lib/libavcodec.a \
	$(HB_DIR)/contrib/lib/libavutil.a \
	$(HB_DIR)/contrib/lib/libdca.a \
	$(HB_DIR)/contrib/lib/libdvdread.a \
	$(HB_DIR)/contrib/lib/libfaac.a \
	$(HB_DIR)/contrib/lib/libmp3lame.a \
	$(HB_DIR)/contrib/lib/libmpeg2.a \
	$(HB_DIR)/contrib/lib/libvorbis.a \
	$(HB_DIR)/contrib/lib/libvorbisenc.a \
	$(HB_DIR)/contrib/lib/libogg.a \
	$(HB_DIR)/contrib/lib/libsamplerate.a \
	$(HB_DIR)/contrib/lib/libx264.a \
	$(HB_DIR)/contrib/lib/libxvidcore.a \
	$(HB_DIR)/contrib/lib/libmp4v2.a \
	$(HB_DIR)/contrib/lib/libswscale.a \
	$(HB_DIR)/contrib/lib/libtheora.a \
	$(HB_DIR)/contrib/lib/libfaad.a \
	-lz \
	-lbz2 \
	-lpthread

icons =	\
	hb-icon.128.png 

icons_dep =	\
	hb-activity.32.png \
	hb-add-queue.32.png \
	hb-canceled.16.png \
	hb-complete.16.png \
	hb-drawer.32.png \
	hb-icon.128.png \
	hb-icon.64.png \
	hb-pause.32.png \
	hb-play.32.png \
	hb-pref.32.png \
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

hb_menu = ghb.desktop

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GHB_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = ghb 
noinst_PROGRAMS = makewidgetdeps quotestring create_resources preset_xlat

BUILT_SOURCES = HandBrakeCLI

HandBrakeCLI: $(HB_DIR)/HandBrakeCLI
	/bin/cp $< $@

# Dummy file, not built.  Forces g++ linking
nodist_EXTRA_ghb_SOURCES = dummy.cpp

ghb_SOURCES = \
	callbacks.c \
	callbacks.h \
	queuehandler.c \
	queuehandler.h \
	audiohandler.c \
	audiohandler.h \
	x264handler.c \
	x264handler.h \
	main.c \
	settings.c \
	settings.h \
	resources.c \
	resources.h \
	presets.c \
	presets.h \
	preview.c \
	icons.c \
	icons.h \
	icon_tools.c \
	icon_tools.h \
	values.c \
	values.h \
	appcast.c \
	appcast.h \
	plist.c \
	plist.h \
	hb-backend.c \
	hb-backend.h \
	renderer_button.h \
	renderer_button.c \
	ghbcellrenderertext.c \
	ghbcellrenderertext.h \
	ghb-dvd.c \
	ghb-dvd.h \
	marshalers.c \
	marshalers.h

ghb_LDFLAGS = \
	-Wl,--export-dynamic

ghb_LDADD = $(GHB_LIBS) $(HB_LIBS)

ghb_DEPENDENCIES = $(HB_DIR)/libhb/libhb.a

makewidgetdeps_SOURCES = \
	plist.c \
	plist.h \
	values.c \
	values.h \
	makedeps.c 

makewidgetdeps_LDADD = $(GHBTOOLS_LIBS)

create_resources_SOURCES = \
	create_resources.c \
	plist.c \
	plist.h \
	values.c \
	values.h \
	icon_tools.c \
	icon_tools.h

create_resources_LDADD = $(GHBTOOLS_LIBS)

preset_xlat_SOURCES = \
	preset_xlat.c \
	plist.c \
	plist.h \
	values.c \
	values.h

preset_xlat_LDADD = $(GHBTOOLS_LIBS)

quotestring_SOURCES = quotestring.c

dumbell: preset_xlat

resources.c: resource_data.h

resource_data.h: quotestring resources.plist
	./quotestring resources.plist resource_data.h

widget_deps: makewidgetdeps
	./makewidgetdeps

resources.plist: create_resources resources.list $(icons_dep) internal_defaults.xml standard_presets.xml ghb.ui widget_deps widget_reverse_deps
	./create_resources resources.list resources.plist

ghbcellrenderertext.c: marshalers.h

marshalers.h: marshalers.list
	glib-genmarshal --prefix=ghb_marshal marshalers.list --header > marshalers.h

marshalers.c: marshalers.list
	glib-genmarshal --prefix=ghb_marshal marshalers.list --body > marshalers.c

EXTRA_DIST = HandBrakeCLI

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(DESTDIR)/$(datadir)/icons/hicolor

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
	@-if test -n "$(DESTDIR)/$(datadir)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	rm -f $(DESTDIR)/$(datadir)/applications/$(hb_menu)

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
	@-if test -n "$(DESTDIR)/$(datadir)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	mkdir -p $(DESTDIR)/$(datadir)/applications/; \
	$(INSTALL_DATA) $(srcdir)/$(hb_menu) $(DESTDIR)/$(datadir)/applications/$(hb_menu)
