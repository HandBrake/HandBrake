#if !defined(_PLIST_H_)
#define _PLIST_H_

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

GValue* ghb_plist_parse(const gchar *buf, gssize len);
GValue* ghb_plist_parse_file(FILE *fd);
void ghb_plist_write(FILE *file, GValue *gval);
void ghb_plist_write_file(const gchar *filename, GValue *gval);

#endif // _PLIST_H_

