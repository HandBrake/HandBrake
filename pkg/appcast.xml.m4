dnl
dnl This file is a template used to generate various appcast.xml files.
dnl
changequote(<<, >>)dnl
include(<<handbrake.m4>>)dnl
changequote([, ])dnl
dnl
dnl
dnl
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
    <channel>
        <title>__HB_name __BUILD_arch Appcast</title>
        <link>__HB_url_appcast</link>
        <description></description>
        <language>en</language>
        <pubDate>__BUILD_date</pubDate>
        <lastBuildDate>__BUILD_date</lastBuildDate>
        <item>
            <title>__HB_name __HB_version Released</title>
            <cli>__HB_build "__HB_version __BUILD_arch"</cli>
            <sparkle:releaseNotesLink>__HB_url_appnote</sparkle:releaseNotesLink>
            <pubDate>__BUILD_date</pubDate>
            <enclosure
                sparkle:version="__HB_build"
                sparkle:shortVersionString="__HB_version __BUILD_arch"
                url="http://handbrake.fr/rotation.php?file=__APPCAST_dmg" 
                length="__APPCAST_dmg_size"
                type="application/octet-stream"/>
            <sparkle:minimumSystemVersion>10.6.0</sparkle:minimumSystemVersion>
        </item>
    </channel>
</rss>
