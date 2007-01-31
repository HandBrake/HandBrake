SYSTEM = $(shell uname -s)

# Special case for Mac OS X: everything is handled from the Xcode project
ifeq ($(SYSTEM),Darwin)

all:
	(./DownloadMacOsXContribBinaries.sh ; cd macosx ; xcodebuild -alltargets -configuration UB build | sed '/^$$/d' ; cd .. ; ./macosx/localize.sh MediaFork.app)

clean:
	(cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

mrproper:
	(rm -rf contrib/*tar.gz contrib/include contrib/lib contrib/DarwinContribVersion.txt ; cd macosx ; xcodebuild -alltargets -configuration UB clean | sed '/^$$/d' )

release:
	(rm -rf MediaFork MediaFork.dmg ; mkdir -p MediaFork/api MediaFork/doc; cp AUTHORS BUILD COPYING CREDITS NEWS THANKS TRANSLATIONS MediaFork/doc ; cp -rp *MediaFork.app MediaFork ; cp -rp libmediafork/libmediafork.dylib MediaFork/api ; cp -rp libmediafork/mediafork.h MediaFork/api ; cp -rp MediaForkCLI MediaFork ; hdiutil create -srcfolder MediaFork  -format UDBZ MediaFork.dmg ; rm -rf MediaFork )
   
endif
