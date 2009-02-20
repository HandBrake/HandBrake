include Makefile.config

SYSTEM = $(shell uname -s)

# Special case for Mac OS X: everything is handled from the Xcode project

#
# Darwin
#
ifeq ($(SYSTEM),Darwin)

snapshot:   clean unstable-libhb/hbversion.h snapshot-app
official:   clean force-hbversion app

force-hbversion:
	rm -f libhb/hbversion.h

all:    clean app

all-chunky:    clean app-chunky

test:	clean cli

dev:	clean internal

ub-app:    libhb/hbversion.h
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -target libhb -target HandBrake -target HandBrakeCLI -configuration UB HB_BUILD="$(HB_BUILD)" HB_VERSION="$(HB_VERSION)" APPCAST_URL="http://handbrake.fr/appcast.xml" build | sed '/^$$/d'  )

app: contrib/.contrib libhb/hbversion.h
		( cd macosx ; xcodebuild -target libhb -target HandBrake -target HandBrakeCLI -configuration Deployment HB_BUILD="$(HB_BUILD)" HB_VERSION="$(HB_VERSION)" CURRENT_PROJECT_VERSION="$(HB_VERSION)" APPCAST_URL="http://handbrake.fr/appcast.xml" build | sed '/^$$/d' )

contrib/.contrib:
	@$(MAKE) --no-print-directory -C contrib all

snapshot-app: contrib/.contrib libhb/hbversion.h
	( cd macosx ; xcodebuild -target libhb -target HandBrake -target HandBrakeCLI -configuration Deployment HB_BUILD="$(SNAP_HB_BUILD)" HB_VERSION="$(SNAP_HB_VERSION)" CURRENT_PROJECT_VERSION="$(SNAP_HB_VERSION)" APPCAST_URL="http://handbrake.fr/appcast_unstable.xml" build | sed '/^$$/d' )

app-chunky: libhb/hbversion.h
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -alltargets -configuration UB HB_BUILD="$(HB_BUILD)" HB_VERSION="$(HB_VERSION)" APPCAST_URL="http://handbrake.fr/appcast.xml" build | sed '/^$$/d'  )

cli:    libhb/hbversion.h
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -target libhb -target HandBrakeCLI -configuration UB HB_BUILD="$(HB_BUILD)" HB_VERSION="$(HB_VERSION)" build | sed '/^$$/d' )

clean:
	(cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' ; rm -f libhb/hbversion.h ; rm -f contrib/config.cache )

mrproper:
	(rm -rf libhb/hbversion.h contrib/*tar.gz contrib/include contrib/lib contrib/DarwinContribVersion.txt ; cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

release:
	(rm -rf HandBrake HandBrake*dmg ; mkdir -p HandBrake/api HandBrake/doc HandBrake/doc/pdf; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ; cp -rp pdf/ HandBrake/doc/pdf/ ; cp -rp HandBrake.app HandBrake ; cp -rp libhb/libhb.dylib HandBrake/api ; cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; hdiutil create -srcfolder HandBrake  -format UDZO HandBrake-$(HB_VERSION)-MacOS_UB.dmg ; rm -rf HandBrake )

gui-release:
	(rm -rf HandBrake HandBrake*GUI_UB.dmg ; mkdir -p HandBrake/docs ; cp AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/docs ; cp -rp HandBrake.app HandBrake  ; hdiutil create -srcfolder HandBrake  -format UDBZ HandBrake-$(HB_VERSION)-MacOSX.4_GUI_UB.dmg ; rm -rf HandBrake )

cli-release:
	(rm -rf HandBrake HandBrake*CLI_UB.dmg ; mkdir -p HandBrake/docs ; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/docs ; cp -rp HandBrakeCLI HandBrake ; hdiutil create -srcfolder HandBrake  -format UDZO HandBrake-$(HB_VERSION)-MacOSX.3_CLI_UB.dmg ; rm -rf HandBrake )

gui-snapshot-release:
	(rm -rf HandBrake HandBrake*GUI_UB.dmg ; mkdir -p HandBrake/docs ; cp AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/docs ; cp -rp HandBrake.app HandBrake  ; hdiutil create -srcfolder HandBrake  -format UDBZ HandBrake-$(SNAP_HB_VERSION)-MacOSX.5_GUI_UB.dmg ; rm -rf HandBrake )

cli-snapshot-release:
	(rm -rf HandBrake HandBrake*CLI_UB.dmg ; mkdir -p HandBrake/docs ; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/docs ; cp -rp HandBrakeCLI HandBrake ; hdiutil create -srcfolder HandBrake  -format UDZO HandBrake-$(SNAP_HB_VERSION)-MacOSX.5_CLI_UB.dmg ; rm -rf HandBrake )

endif

#
# Linux
#
ifeq ($(SYSTEM),Linux)

snapshot: unstable-libhb/hbversion.h all
	(rm -rf HandBrake HandBrake*.tar.gz ; mkdir -p HandBrake/api HandBrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ;  cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; tar zcvf HandBrake-$(SNAP_HB_VERSION)_i386.tar.gz HandBrake ; rm -rf HandBrake )

official: force-hbversion all
	(rm -rf HandBrake HandBrake*.tar.gz ; mkdir -p HandBrake/api HandBrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ;  cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; tar zcvf HandBrake-$(HB_VERSION)_i386.tar.gz HandBrake ; rm -rf HandBrake )

force-hbversion:
	rm -f libhb/hbversion.h

all:	libhb/hbversion.h
	@$(MAKE) --no-print-directory -C contrib all
	@$(MAKE) --no-print-directory -C libhb all
	@$(MAKE) --no-print-directory -C test all

clean:
	@$(MAKE) --no-print-directory -C libhb clean
	@$(MAKE) --no-print-directory -C test clean
	@rm libhb/hbversion.h
	@rm -f contrib/config.cache

mrproper: clean
	@$(MAKE) --no-print-directory -C contrib mrproper

endif

#
# Cygwin
#
ifeq ($(findstring CYGWIN_NT,$(SYSTEM)),CYGWIN_NT)

snapshot: clean unstable-libhb/hbversion.h all
official: clean force-hbversion all

snapshot-release: snapshot
	(rm -rf HandBrake HandBrake*.zip ; mkdir -p HandBrake/api HandBrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ;  cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; cp /bin/cygwin1.dll HandBrake ; zip -r HandBrake-$(SNAP_HB_VERSION)-Win_CLI.zip HandBrake ; rm -rf HandBrake )
    
official-release: official
	(rm -rf HandBrake HandBrake*.zip ; mkdir -p HandBrake/api HandBrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ;  cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; cp /bin/cygwin1.dll HandBrake ;  zip -r HandBrake-$(HB_VERSION)-Win_GUI.zip HandBrake ; rm -rf HandBrake )

force-hbversion:
	rm -f libhb/hbversion.h
    
all: contrib/.contrib HandBrakeCLI

contrib/.contrib:
	@$(MAKE) --no-print-directory -C contrib all

libhb/libhb.a: unstable-libhb/hbversion.h
	@$(MAKE) --no-print-directory -C libhb all

HandBrakeCLI: libhb/libhb.a
	@$(MAKE) --no-print-directory -C test all
	
clean:
	@$(MAKE) --no-print-directory -C libhb clean
	@$(MAKE) --no-print-directory -C test clean
	@rm -f libhb/hbversion.h
	@rm -f contrib/config.cache
	@rm -f HandBrake HandBrake*.zip

mrproper: clean
	@$(MAKE) --no-print-directory -C contrib mrproper

endif

#
# Version Data
#

libhb/hbversion.h:
	echo "#ifndef HB_BUILD" > libhb/hbversion.h
	echo "#define HB_BUILD $(HB_BUILD)" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h
	echo "#ifndef HB_VERSION" >> libhb/hbversion.h
	echo "#define HB_VERSION \"$(HB_VERSION)\"" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h
	echo "#ifndef HB_APPCAST_URL" >> libhb/hbversion.h
	echo "#define APPCAST_URL \"http://handbrake.fr/appcast.xml\"" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h

unstable-libhb/hbversion.h:
	echo "#ifndef HB_BUILD" > libhb/hbversion.h
	echo "#define HB_BUILD $(SNAP_HB_BUILD)" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h
	echo "#ifndef HB_VERSION" >> libhb/hbversion.h
	echo "#define HB_VERSION \"$(SNAP_HB_VERSION)\"" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h
	echo "#ifndef HB_APPCAST_URL" >> libhb/hbversion.h
	echo "#define APPCAST_URL \"http://handbrake.fr/appcast_unstable.xml\"" >> libhb/hbversion.h
	echo "#endif" >> libhb/hbversion.h
