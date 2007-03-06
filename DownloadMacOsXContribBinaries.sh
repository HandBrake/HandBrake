#! /bin/sh

# Incremented every time a new contrib package is available
VERSION=`cat MacOsXContribBinariesVersion.txt`

if [ -f contrib/DarwinContribVersion.txt ]; then
  if [ "`cat contrib/DarwinContribVersion.txt`" = $VERSION ]; then
    echo "Contribs are up to date."
    exit 0
  fi
fi

HOST=download.m0k.org
#HOST=download.mediafork.dynalias.com
#HOST=sr55.ashosted.com
FILE=contribbin-darwin-$VERSION.tar.gz
#URL=http://$HOST/contrib/$FILE
URL=http://$HOST/handbrake/contrib/$FILE
# Check for internet connectivity
if ! host $HOST > /dev/null 2>&1; then
  echo "Please connect to the Internet (could not resolve $HOST)."
  exit 1
fi

# Look for something that can handle an HTTP download
  WGET="curl -L -O"

# Get and install the package
echo "Getting contribs ($VERSION)..."
( cd contrib && rm -f $FILE && $WGET $URL && rm -Rf lib include && \
  tar xzf $FILE && ranlib lib/*.a ) || exit 1

exit 0

