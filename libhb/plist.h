/* plist.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#if !defined(_HB_PLIST_H_)
#define _HB_PLIST_H_

#include <stdio.h>
#include "hb_dict.h"

hb_value_t * hb_plist_parse(const char *buf, size_t len);
hb_value_t * hb_plist_parse_file(const char *filename);
void         hb_plist_write(FILE *file, hb_value_t  *val);
void         hb_plist_write_file(const char *filename, hb_value_t  *val);

#endif // _HB_PLIST_H_

