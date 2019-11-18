## Process this file with automake to produce Makefile.in

icons =	\
	hb-icon.svg fr.handbrake.ghb.svg

icons_dep =	\
	icons/24x24/apps/hb-remove.png \
	icons/32x32/apps/hb-source.png \
	icons/32x32/apps/hb-picture.png \
	icons/32x32/apps/hb-add-queue.png \
	icons/32x32/apps/hb-showqueue.png \
	icons/32x32/apps/hb-activity.png \
	icons/32x32/apps/hb-start.png \
	icons/32x32/apps/hb-stop.png \
	icons/32x32/apps/hb-pause.png \
	icons/48x48/apps/hb-remove.png \
	icons/64x64/apps/hb-source.png \
	icons/64x64/apps/hb-picture.png \
	icons/64x64/apps/hb-add-queue.png \
	icons/64x64/apps/hb-showqueue.png \
	icons/64x64/apps/hb-activity.png \
	icons/64x64/apps/hb-start.png \
	icons/64x64/apps/hb-stop.png \
	icons/64x64/apps/hb-pause.png \
	icons/scalable/apps/hb-source.svg \
	icons/scalable/apps/hb-picture.svg \
	icons/scalable/apps/hb-add-queue.svg \
	icons/scalable/apps/hb-showqueue.svg \
	icons/scalable/apps/hb-activity.svg \
	icons/scalable/apps/hb-start.svg \
	icons/scalable/apps/hb-stop.svg \
	icons/scalable/apps/hb-pause.svg \
	icons/scalable/apps/hb-remove.svg \
	icons/scalable/apps/hb-complete.svg \
	icons/scalable/apps/hb-edit.svg \
	icons/scalable/apps/hb-icon.svg \
	icons/scalable/apps/hb-advanced.svg \
	icons/scalable/apps/fr.handbrake.ghb.svg

hb_menu = fr.handbrake.ghb.desktop

appdatadir = $(datarootdir)/metainfo
appdata_in_files = fr.handbrake.ghb.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
dist_appdata_DATA = $(appdata_files)

EXTRA_DIST = \
	$(appdata_in_files)

DISTCLEANFILES = \
	$(appdata_files)

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(HB_CPPFLAGS)

AM_CFLAGS = -Wall -g

bin_PROGRAMS = ghb

# Dummy file, not built.  Forces g++ linking
nodist_EXTRA_ghb_SOURCES = dummy.cpp

ghb_SOURCES = \
	ghbcompat.c \
	ghbcompat.h \
	callbacks.c \
	callbacks.h \
	chapters.c \
	chapters.h \
	queuehandler.c \
	queuehandler.h \
	videohandler.c \
	videohandler.h \
	audiohandler.c \
	audiohandler.h \
	subtitlehandler.c \
	subtitlehandler.h \
	main.c \
	jobdict.c \
	jobdict.h \
	titledict.c \
	titledict.h \
	settings.c \
	settings.h \
	resources.c \
	resources.h \
	presets.c \
	presets.h \
	preview.c \
	data_res.c \
	data_res.h \
	ui_res.c \
	ui_res.h \
	icon_res.c \
	icon_res.h \
	icons.c \
	icons.h \
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
	ghb-dvd.c \
	ghb-dvd.h \
	marshalers.c \
	marshalers.h

if MINGW
ghb_LDFLAGS = \
	-mwindows -Wl,--export-dynamic -Wl,--exclude-libs,ALL
else
ghb_LDFLAGS = \
	-Wl,--export-dynamic -Wl,--exclude-libs,ALL
endif

ghb_LDADD = $(HB_LIBS)

ghb_DEPENDENCIES = $(HB_DIR)/libhb/libhandbrake.a

icon_res.c : icon_res.gresource.xml $(icons_dep)
	glib-compile-resources --generate --target=$@ --c-name ghb_icons --manual-register --sourcedir=$(srcdir) $<

icon_res.h : icon_res.gresource.xml $(icons_dep)
	glib-compile-resources --generate --target=$@ --c-name ghb_icons --manual-register --sourcedir=$(srcdir) $<

icon_res.o: icon_res.c icon_res.h
icons.o: icon_res.h

data_res.c : data_res.gresource.xml $(srcdir)/internal_defaults.json widget.deps widget_reverse.deps
	cp $(srcdir)/internal_defaults.json .
	glib-compile-resources --generate --target=$@ --c-name ghb_data --manual-register $<

data_res.h : data_res.gresource.xml $(srcdir)/internal_defaults.json widget.deps widget_reverse.deps
	cp $(srcdir)/internal_defaults.json .
	glib-compile-resources --generate --target=$@ --c-name ghb_data --manual-register $<

if GHB_GTK_4_00
UI=400
UI_FILE=ghb4.ui
else
if GHB_GTK_3_16
UI=316
UI_FILE=ghb3.ui
else
UI=300
UI_FILE=ghb3.ui
endif
endif

ui_res.c : ui_res.gresource.xml ui_res.h $(srcdir)/$(UI_FILE)
	glib-compile-resources --generate --target=$@ --c-name ghb_ui --manual-register $<

ui_res.h : ui_res.gresource.xml $(srcdir)/$(UI_FILE)
	cp $(srcdir)/$(UI_FILE) ghb.ui
	glib-compile-resources --generate --target=$@ --c-name ghb_ui --manual-register $<

ui_res.o: ui_res.h ui_res.c

main.o : ui_res.o

resources.o: data_res.h

widget_reverse.deps: makedeps.py
widget.deps: makedeps.py
	$(HB_PYTHON) $(srcdir)/makedeps.py

fr.handbrake.ghb.appdata.xml: fr.handbrake.ghb.appdata.xml.in
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

fr.handbrake.ghb.appdata.xml.in: fr.handbrake.ghb.appdata.template.xml
	sed -e 's^RELEASE_TAG^<release version="$(HB.version)" date="$(word 1,$(HB.repo.date))" />^' $< > $@

$(srcdir)/marshalers.h: marshalers.list
	glib-genmarshal --prefix=ghb_marshal $(srcdir)/marshalers.list --header > $(srcdir)/marshalers.h

$(srcdir)/marshalers.c: marshalers.list
	glib-genmarshal --prefix=ghb_marshal $(srcdir)/marshalers.list --body > $(srcdir)/marshalers.c

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(DESTDIR)/$(datadir)/icons/hicolor

uninstall-local:
	for icon in $(icons); do \
		rm -f $(DESTDIR)/$(datadir)/icons/hicolor/scalable/apps/$$icon; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	rm -f $(DESTDIR)/$(datadir)/applications/$(hb_menu)

install-data-local:
	for icon in $(icons); do \
		mkdir -p $(DESTDIR)/$(datadir)/icons/hicolor/scalable/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)/$(datadir)/icons/hicolor/scalable/apps/$$icon; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	mkdir -p $(DESTDIR)/$(datadir)/applications/; \
	$(INSTALL_DATA) $(srcdir)/$(hb_menu) $(DESTDIR)/$(datadir)/applications/$(hb_menu)
