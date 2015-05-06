#if !defined(_HB_PLIST_H_)
#define _HB_PLIST_H_

#include <stdio.h>
#include "hb_dict.h"

hb_value_t * hb_plist_parse(const char *buf, size_t len);
hb_value_t * hb_plist_parse_file(const char *filename);
void         hb_plist_write(FILE *file, hb_value_t  *val);
void         hb_plist_write_file(const char *filename, hb_value_t  *val);

#endif // _HB_PLIST_H_

