include Makefile.config

SYSTEM = $(shell uname -s)

# Special case for Mac OS X: everything is handled from the Xcode project

#
# Darwin
#
ifeq ($(SYSTEM),Darwin)

all:    clean app

snapshot:   clean snapshot-app

all-chunky:    clean app-chunky

test:	clean cli

dev:	clean internal

app:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -target libhb -target HandBrake -target HandBrakeCLI -configuration UB  OTHER_CFLAGS_QUOTED_1="-DHB_VERSION=\\\"$(HB_VERSION)\\\" -DHB_BUILD=$(HB_BUILD) " build | sed '/^$$/d'  )

snapshot-app:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; defaults write "$(FULL_PATH)"/macosx/HandBrake CFBundleGetInfoString '$(SNAP_HB_VERSION)' ; defaults write "$(FULL_PATH)"/macosx/HandBrake CFBundleShortVersionString '$(SNAP_HB_VERSION)' ;  defaults write "$(FULL_PATH)"/macosx/HandBrake CFBundleVersion '"$(SNAP_HB_BUILD)"' ; plutil -convert xml1 "$(FULL_PATH)"/macosx/HandBrake.plist ; xcodebuild -target libhb -target HandBrake -target HandBrakeCLI -configuration UB  OTHER_CFLAGS_QUOTED_1="-g -HB_BUILD="$(SNAP_HB_BUILD)" -HB_VERSION=\\\"$(SNAP_HB_VERSION)\\\" -DHB_BUILD="$(SNAP_HB_BUILD)" -DHB_VERSION=\\\"$(SNAP_HB_VERSION)\\\" -CURRENT_PROJECT_VERSION=\\\"$(SNAP_HB_VERSION)\\\" " build | sed '/^$$/d'  )

app-chunky:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -alltargets -configuration UB  OTHER_CFLAGS_QUOTED_1="-DHB_VERSION=\\\"$(HB_VERSION)\\\" -DHB_BUILD=$(HB_BUILD) " build | sed '/^$$/d'  )

cli:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -target libhb -target HandBrakeCLI -configuration UB  OTHER_CFLAGS_QUOTED_1="-DHB_VERSION=\\\"$(HB_VERSION)\\\" -DHB_BUILD=$(HB_BUILD) " build | sed '/^$$/d' )

clean:
	(cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

mrproper:
	(rm -rf contrib/*tar.gz contrib/include contrib/lib contrib/DarwinContribVersion.txt ; cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

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

all:	contrib/.contrib libhb/libhb.a HandBrakeCLI
	(rm -rf HandBrake HandBrake*.tar.gz ; mkdir -p HandBrake/api HandBrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS HandBrake/doc ;  cp -rp libhb/libhb.so HandBrake/api ; cp -rp libhb/hb.h libhb/common.h libhb/ports.h HandBrake/api ; cp -rp HandBrakeCLI HandBrake ; tar zcvf HandBrake-$(HB_VERSION)_i386.tar.gz HandBrake ; rm -rf HandBrake )

contrib/.contrib:
	@$(MAKE) --no-print-directory -C contrib all

libhb/libhb.a:
	@$(MAKE) --no-print-directory -C libhb all

HandBrakeCLI:
	@$(MAKE) --no-print-directory -C test all

clean:
	@$(MAKE) --no-print-directory -C libhb clean
	@$(MAKE) --no-print-directory -C test clean

mrproper: clean
	@$(MAKE) --no-print-directory -C contrib mrproper

endif

#
# Cygwin
#
ifeq ($(findstring CYGWIN_NT,$(SYSTEM)),CYGWIN_NT)

all:    contrib/.contrib libhb/libhb.a HandBrakeCLI

app:	contribPack libhb/libhb.a HandBrakeCLI

contribPack:
	(./DownloadCygWinContribBinaries.sh)
	
contrib/.contrib:
	@$(MAKE) --no-print-directory -C contrib all

libhb/libhb.a:
	@$(MAKE) --no-print-directory -C libhb all

HandBrakeCLI: 
	@$(MAKE) --no-print-directory -C test all
	
clean:
	@$(MAKE) --no-print-directory -C libhb clean
	@$(MAKE) --no-print-directory -C test clean

mrproper: clean
	@$(MAKE) --no-print-directory -C contrib mrproper

endif
