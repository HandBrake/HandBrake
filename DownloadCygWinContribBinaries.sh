#! /bin/sh


HOST=http://download.m0k.org
FILE=contribbin-cygwin-0001.tar.gz
URL=$HOST/handbrake/contrib/$FILE
# Check for internet connectivity
#if ! host $HOST > /dev/null 2>&1; then
#  echo "Please connect to the Internet (could not resolve $HOST)."
#  exit 1
#fi

# Get and install the package
echo "Getting contribs ($VERSION)..."
( cd contrib && rm -f contribbin-cygwin-0001.tar.gz && wget $URL && rm -Rf lib include && tar xzf contribbin-cygwin-0001.tar.gz && ranlib lib/*.a ) || exit 1

exit 0

