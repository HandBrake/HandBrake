/* hb_json.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_JSON_H
#define HB_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

char       * hb_get_title_set_json(hb_handle_t * h);
char       * hb_title_to_json(const hb_title_t * title);
char       * hb_job_init_json(hb_handle_t *h, int title_index);
char       * hb_job_to_json(const hb_job_t * job);
hb_job_t   * hb_json_to_job(hb_handle_t * h, const char * json_job);
int          hb_add_json(hb_handle_t *h, const char * json_job);
char       * hb_set_anamorphic_size_json(const char * json_param);
char       * hb_get_state_json(hb_handle_t * h);
hb_image_t * hb_json_to_image(char *json_image);
char       * hb_get_preview_params_json(int title_idx, int preview_idx,
                            int deinterlace, hb_geometry_settings_t *settings);
char       * hb_get_preview_json(hb_handle_t * h, const char *json_param);
void         hb_json_job_scan( hb_handle_t * h, const char * json_job );

#ifdef __cplusplus
}
#endif

#endif // HB_JSON_H
