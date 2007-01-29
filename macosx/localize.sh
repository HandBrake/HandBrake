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
