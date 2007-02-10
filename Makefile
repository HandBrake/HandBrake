SYSTEM = $(shell uname -s)

# Special case for Mac OS X: everything is handled from the Xcode project
ifeq ($(SYSTEM),Darwin)

all:    clean app release

app:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -alltargets -configuration UB build | sed '/^$$/d' ; cd .. ; ./macosx/localize.sh MediaFork.app)

clean:
	(cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

mrproper:
	(rm -rf contrib/*tar.gz contrib/include contrib/lib contrib/DarwinContribVersion.txt ; cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

release:
	(rm -rf MediaFork MediaFork.dmg ; mkdir -p MediaFork/api MediaFork/doc; cp test/BUILDSHARED AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS MediaFork/doc ; cp -rp *MediaFork.app MediaFork ; cp -rp libmediafork/libmediafork.dylib MediaFork/api ; cp -rp libmediafork/mediafork.h libmediafork/common.h libmediafork/ports.h MediaFork/api ; cp -rp MediaForkCLI MediaFork ; hdiutil create -srcfolder MediaFork  -format UDBZ MediaFork.dmg ; rm -rf MediaFork )
   
endif

ifeq ($(SYSTEM),Linux)

all:	contrib/lib libmediafork/libmediafork.a MediaForkCLI

contrib/lib:
	(./configure ; cd contrib ; cp -f ../config.jam . ; jam ; cd ..)

libmediafork/libmediafork.a:
	@$(MAKE) --no-print-directory -C libmediafork all

MediaForkCLI:
	@$(MAKE) --no-print-directory -C test all

clean:
	@$(MAKE) --no-print-directory -C libmediafork clean
	@$(MAKE) --no-print-directory -C test clean

mrproper: clean
	(rm -rf contrib/lib ; rm -rf contrib/include/* )

endif

ifeq ($(SYSTEM),CYGWIN_NT-5.1)

all:    contrib/lib libmediafork/libmediafork.a MediaForkCLI

contrib/lib:
	(./configure ; cd contrib ; cp -f ../config.jam . ; jam.exe ; cd ..)

libmediafork/libmediafork.a:
	@$(MAKE) --no-print-directory -C libmediafork all

MediaForkCLI:
	@$(MAKE) --no-print-directory -C test all

clean:
	@$(MAKE) --no-print-directory -C libmediafork clean
	@$(MAKE) --no-print-directory -C test clean

mrproper: clean
	(rm -rf contrib/lib ; rm -rf contrib/include/* )

endif
