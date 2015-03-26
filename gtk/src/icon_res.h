#ifndef __RESOURCE_ghb_icons_H__
#define __RESOURCE_ghb_icons_H__

#include <gio/gio.h>

extern GResource *ghb_icons_get_resource (void);

extern void ghb_icons_register_resource (void);
extern void ghb_icons_unregister_resource (void);

#endif
