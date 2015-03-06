#if !defined(_PLIST_H_)
#define _PLIST_H_

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "values.h"

GhbValue* ghb_plist_parse(const gchar *buf, gssize len);
GhbValue* ghb_plist_parse_file(const gchar *filename);
void ghb_plist_write(FILE *file, GhbValue *gval);
void ghb_plist_write_file(const gchar *filename, GhbValue *gval);

#endif // _PLIST_H_

