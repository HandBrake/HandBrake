/* decavsub.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_DECAVSUB_H
#define HANDBRAKE_DECAVSUB_H

#include "handbrake/handbrake.h"

typedef struct hb_decavsub_context_s hb_decavsub_context_t;

hb_decavsub_context_t * decavsubInit( hb_work_object_t * w, hb_job_t * job );
int                     decavsubWork( hb_decavsub_context_t * ctx,
                                   hb_buffer_t ** in, hb_buffer_t ** out );
void                    decavsubClose( hb_decavsub_context_t * ctx );

#endif // HANDBRAKE_DECAVSUB_H
