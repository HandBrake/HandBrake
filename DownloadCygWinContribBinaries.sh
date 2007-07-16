#! /bin/sh

HOST=http://download.m0k.org
FILE=contribbin-cygwin-0006.tar.gz
URL=$HOST/handbrake/contrib/$FILE

# Get and install the package
echo "Getting contribs ($VERSION)..."
( cd contrib && rm -f contribbin-cygwin-0006.tar.gz && wget $URL && rm -Rf lib include && tar xzf contribbin-cygwin-0006.tar.gz && ranlib lib/*.a ) || exit 1

exit 0

