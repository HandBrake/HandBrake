#!/bin/sh
# This script localize the built application
 cd macosx/i18n/
export LNG=`ls *strings | sed 's/.strings//g' | sed 's/Localizable//g'`
cd ../../$1/Contents/Resources
for l in $LNG 
do
	cp -r English.lproj $l.lproj
	cp ../../../macosx/i18n/$l.strings $l.lproj/Localizable.strings
done

echo Generating Info.plist with correct version information
cd ..
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
        <key>CFBundleDevelopmentRegion</key>
        <string>English</string>
        <key>CFBundleDisplayName</key>
        <string>MediaFork</string>
        <key>CFBundleExecutable</key>
        <string>MediaFork</string>
        <key>CFBundleGetInfoString</key>
        <string>0.8.0b1</string>
        <key>CFBundleIconFile</key>
        <string>MediaFork</string>
        <key>CFBundleIdentifier</key>
        <string>org.mediafork.dynalias</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundleName</key>
        <string>MediaFork</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleShortVersionString</key>
        <string>$2</string>
        <key>CFBundleSignature</key>
        <string>HB##</string>
        <key>CFBundleVersion</key>
        <string>$3</string>
        <key>NSHumanReadableCopyright</key>
        <string>MediaFork Devs</string>
        <key>NSMainNibFile</key>
        <string>MainMenu</string>
        <key>NSPrincipalClass</key>
        <string>NSApplication</string>
</dict>
</plist>" > Info.plist

