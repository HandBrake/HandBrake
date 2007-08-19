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
        <string>HandBrake</string>
        <key>CFBundleExecutable</key>
        <string>HandBrake</string>
        <key>CFBundleGetInfoString</key>
        <string>0.9.0</string>
        <key>CFBundleIconFile</key>
        <string>HandBrake</string>
        <key>CFBundleIdentifier</key>
        <string>org.m0k.handbrake</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundleName</key>
        <string>HandBrake</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleShortVersionString</key>
        <string>$2</string>
        <key>CFBundleSignature</key>
        <string>HB##</string>
        <key>CFBundleVersion</key>
        <string>$3</string>
        <key>NSHumanReadableCopyright</key>
        <string>HandBrake Devs</string>
        <key>NSMainNibFile</key>
        <string>MainMenu</string>
        <key>NSPrincipalClass</key>
        <string>NSApplication</string>
</dict>
</plist>" > Info.plist

if [ $4 == "DEV" ]; then
echo Installing libquicktime Plugins in the $1 Bundle
cd Resources
mkdir plugins
cd plugins
cp ../../../../contrib/lib/libquicktime/* .
fi

