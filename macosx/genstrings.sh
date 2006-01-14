#! /bin/sh
rm -f genstrings.tmp
cat Controller.mm | grep -v "^#define.*_(" | \
    sed 's/_( \(@"[^"]*"\) )/NSLocalizedString( \1, nil )/g' > \
    genstrings.tmp
genstrings genstrings.tmp
rm -f genstrings.tmp
