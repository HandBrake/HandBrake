/* encavsub.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_ENCAVSUB_H
#define HANDBRAKE_ENCAVSUB_H

#include "handbrake/handbrake.h"

typedef struct hb_encavsub_context_s hb_encavsub_context_t;

hb_encavsub_context_t * encavsubInit( hb_work_object_t * w, hb_job_t * job );
int                     encavsubWork( hb_encavsub_context_t * ctx,
                                      hb_buffer_t ** in, hb_buffer_t ** out );
void                    encavsubClose( hb_encavsub_context_t * ctx );

#endif // HANDBRAKE_ENCAVSUB_H
