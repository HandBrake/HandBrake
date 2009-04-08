## Process this file with automake to produce Makefile.in

HB_LIBS=\
	-lhb \
	-la52 \
	-lmkv \
	-lavformat \
	-lavcodec \
	-lavutil \
	-ldca \
	-ldvdread \
	-lfaac \
	-lmp3lame \
	-lmpeg2 \
	-lvorbis \
	-lvorbisenc \
	-logg \
	-lsamplerate \
	-lx264 \
	-lxvidcore \
	-lmp4v2 \
	-lswscale \
	-ltheora \
	-lfaad \
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
	hb-status.32.png \
	hb-status-empty.32.png \
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
noinst_PROGRAMS = makewidgetdeps quotestring create_resources preset_xlat \
	resource_data.h resources.plist widget.deps widget_reverse.deps

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
    ghbcompositor.c \
    ghbcompositor.h \
	ghb-dvd.c \
	ghb-dvd.h \
	marshalers.c \
	marshalers.h

ghb_LDFLAGS = \
	-Wl,--export-dynamic -Wl,--exclude-libs,ALL

ghb_LDADD = $(HB_LIBS) $(GHB_LIBS)

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

resources.o: resource_data.h

resource_data.h: quotestring resources.plist
	./quotestring resources.plist resource_data.h

widget_reverse.deps: makewidgetdeps
widget.deps: makewidgetdeps
	./makewidgetdeps

resources.plist: create_resources resources.list $(icons_dep) internal_defaults.xml standard_presets.xml ghb.ui widget.deps widget_reverse.deps
	./create_resources -I$(srcdir) $(srcdir)/resources.list resources.plist

ghbcellrenderertext.c: marshalers.h

marshalers.h: marshalers.list
	glib-genmarshal --prefix=ghb_marshal marshalers.list --header > marshalers.h

marshalers.c: marshalers.list
	glib-genmarshal --prefix=ghb_marshal marshalers.list --body > marshalers.c

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
