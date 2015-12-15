dnl
dnl
dnl
changequote(<<, >>)dnl
include(<<handbrake.m4>>)dnl
dnl
dnl
dnl
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
    <key>CFBundleDisplayName</key>
    <string>__HB_name</string>
 	<key>CFBundleDocumentTypes</key>
 	<array>
 		<dict>
 			<key>CFBundleTypeExtensions</key>
 			<array>
 				<string>*</string>
 			</array>
 			<key>CFBundleTypeName</key>
 			<string>All files</string>
 			<key>CFBundleTypeRole</key>
 			<string>Viewer</string>
 		</dict>
 		<dict>
			<key>LSItemContentTypes</key>
			<array>
				<string>public.movie</string>
			</array>
			<key>CFBundleTypeRole</key>
 			<string>Viewer</string>
		</dict>
 	</array>
	<key>CFBundleExecutable</key>
	<string>${EXECUTABLE_NAME}</string>
    <key>CFBundleGetInfoString</key>
    <string>__HB_build</string>
	<key>CFBundleIconFile</key>
	<string>${EXECUTABLE_NAME}</string>
	<key>CFBundleIdentifier</key>
	<string>fr.handbrake.${PRODUCT_NAME:rfc1034identifier}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>${PRODUCT_NAME}</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>__HB_version __BUILD_arch</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleVersion</key>
	<string>__HB_build</string>
	<key>LSMinimumSystemVersion</key>
	<string>${MACOSX_DEPLOYMENT_TARGET}</string>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright © 2003-2015 __HB_name Developers.
All rights reserved.</string>
	<key>NSMainNibFile</key>
	<string>MainMenu</string>
	<key>NSPrincipalClass</key>
	<string>HBApplication</string>
    <key>SUFeedURL</key>
    <string>__HB_url_appcast</string>
    <key>NSAppTransportSecurity</key>
	<dict>
		<key>NSExceptionDomains</key>
		<dict>
			<key>handbrake.fr</key>
			<dict>
				<key>NSIncludesSubdomains</key>
				<true/>
				<key>NSTemporaryExceptionAllowsInsecureHTTPLoads</key>
				<true/>
			</dict>
		</dict>
	</dict>
</dict>
</plist>
