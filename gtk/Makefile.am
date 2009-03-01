## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

ghbdocdir = ${prefix}/share/doc/ghb
ghbdoc_DATA = \
	ChangeLog

EXTRA_DIST = $(ghbdoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

