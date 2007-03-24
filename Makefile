include Makefile.config

SYSTEM = $(shell uname -s)

# Special case for Mac OS X: everything is handled from the Xcode project
ifeq ($(SYSTEM),Darwin)

all:    clean app

dev:	clean internal

app:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -alltargets -configuration UB  OTHER_CFLAGS_QUOTED_1="-DHB_VERSION=\\\"$(HB_VERSION)\\\" -DHB_BUILD=$(HB_BUILD) " build | sed '/^$$/d' ; cd .. ; ./macosx/localize.sh HandBrake.app $(HB_VERSION) $(HB_BUILD) UB )

clean:
	(cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

mrproper:
	(rm -rf contrib/*tar.gz contrib/include contrib/lib contrib/DarwinContribVersion.txt ; cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

release:
	(rm -rf Handbrake Handbrake.dmg ; mkdir -p Handbrake/api Handbrake/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS Handbrake/doc ; cp -rp HandBrake.app Handbrake ; cp -rp libhb/libhb.dylib Handbrake/api ; cp -rp libhb/hb.h libhb/common.h libhb/ports.h Handbrake/api ; cp -rp HandbrakeCLI Handbrake ; hdiutil create -srcfolder Handbrake  -format UDZO Handbrake.dmg ; rm -rf Handbrake )
ifeq ($(SNAP), 1)
	( mv Handbrake.dmg Handbrake-$(HB_VERSION)-MacOS_UB.dmg )
endif

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

ifeq ($(SYSTEM),CYGWIN_NT-5.1)

all:    contrib/.contrib libhb/libhb.a HandbrakeCLI

contrib/.contrib:
	@$(MAKE) --no-print-directory -C contrib all

libhb/libhb.a:
	@$(MAKE) --no-print-directory -C libhb all

HandbrakeCLI:
	@$(MAKE) --no-print-directory -C test all

clean:
	@$(MAKE) --no-print-directory -C libhb clean
	@$(MAKE) --no-print-directory -C test clean

mrproper: clean
	@$(MAKE) --no-print-directory -C contrib mrproper

endif
