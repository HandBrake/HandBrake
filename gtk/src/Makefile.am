## Process this file with automake to produce Makefile.in

icons =	\
	hb-icon.svg

icons_dep =	\
	hb-icon.svg \
	hb-start.svg \
	hb-pause.svg \
	hb-stop.svg \
	hb-add-queue.svg \
	hb-showqueue.svg \
	hb-picture.svg \
	hb-activity.svg \
	hb-source.svg \
	hb-remove.svg \
	hb-edit.svg \
	hb-complete.svg

hb_menu = ghb.desktop

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GHB_CFLAGS)

AM_CFLAGS = -Wall -g

bin_PROGRAMS = ghb

# Dummy file, not built.  Forces g++ linking
nodist_EXTRA_ghb_SOURCES = dummy.cpp

ghb_SOURCES = \
	callbacks.c \
	callbacks.h \
	queuehandler.c \
	queuehandler.h \
	videohandler.c \
	videohandler.h \
	audiohandler.c \
	audiohandler.h \
	subtitlehandler.c \
	subtitlehandler.h \
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
	data_res.c \
	data_res.h \
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
	ghbcellrenderertext.c \
	ghbcellrenderertext.h \
	ghbcompositor.c \
	ghbcompositor.h \
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

ghb_LDADD = $(HB_LIBS) $(GHB_LIBS)

ghb_DEPENDENCIES = $(HB_DIR)/libhb/libhandbrake.a

data_res.c : data_res.gresource.xml internal_defaults.json standard_presets.json
	cp $(srcdir)/internal_defaults.json $(srcdir)/standard_presets.json .
	glib-compile-resources --generate --target=$@ --c-name ghb_data --manual-register $<

data_res.h : data_res.gresource.xml internal_defaults.json standard_presets.json
	cp $(srcdir)/internal_defaults.json $(srcdir)/standard_presets.json .
	glib-compile-resources --generate --target=$@ --c-name ghb_data --manual-register $<

resources.o: resource_data.h data_res.h

resource_data.h: resources.json
	python2 $(srcdir)/quotestring.py resources.json resource_data.h

widget_reverse.deps: makedeps.py
widget.deps: makedeps.py
	python2 $(srcdir)/makedeps.py

resources.json: create_resources resources.list $(icons_dep) internal_defaults.json standard_presets.json ghb.ui ghb-3.14.ui widget.deps widget_reverse.deps
	./create_resources -I$(srcdir) $(srcdir)/resources.list resources.json

CREATE_RES.c = \
	create_resources.c \
	plist.c \
	values.c

CREATE_RES.c.o = $(patsubst %.c,%-native.o,$(CREATE_RES.c))

create_resources: $(CREATE_RES.c.o)
	$(CC_FOR_BUILD) -g -o $@ $^ $(GHB_TOOLS_LIBS)

$(CREATE_RES.c.o): %-native.o: %.c
	$(CC_FOR_BUILD) $(GHB_TOOLS_CFLAGS) -g -c -o $@ $<

ghbcellrenderertext.$(OBJEXT): marshalers.h

$(srcdir)/marshalers.h: marshalers.list
	glib-genmarshal --prefix=ghb_marshal $(srcdir)/marshalers.list --header > $(srcdir)/marshalers.h

$(srcdir)/marshalers.c: marshalers.list
	glib-genmarshal --prefix=ghb_marshal $(srcdir)/marshalers.list --body > $(srcdir)/marshalers.c

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
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
	mkdir -p $(DESTDIR)/$(datadir)/applications/; \
	$(INSTALL_DATA) $(srcdir)/$(hb_menu) $(DESTDIR)/$(datadir)/applications/$(hb_menu)
