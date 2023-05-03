/* param.h

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_PARAM_H
#define HANDBRAKE_PARAM_H

typedef struct hb_filter_param_s hb_filter_param_t;

struct hb_filter_param_s
{
    int         index;
    const char *name;
    const char *short_name;
    const char *settings;
};

hb_dict_t * hb_generate_filter_settings(int filter_id, const char *preset,
                                        const char *tune, const char *custom);
char * hb_generate_filter_settings_json(int filter_id, const char *preset,
                                        const char *tune, const char *custom);

int    hb_validate_param_string(const char *regex_pattern, const char *param_string);
int    hb_validate_filter_preset(int filter_id, const char *preset,
                                 const char *tune, const char *custom);
int    hb_validate_filter_settings(int filter_id, const hb_dict_t *settings);
int    hb_validate_filter_settings_json(int filter_id, const char * json);
int    hb_validate_filter_string(int filter_id, const char * filter_str);

hb_filter_param_t * hb_filter_param_get_presets(int filter_id);
hb_filter_param_t * hb_filter_param_get_tunes(int filter_id);

char ** hb_filter_get_keys(int filter_id);
char ** hb_filter_get_presets_short_name(int filter_id);
char ** hb_filter_get_presets_name(int filter_id);
char ** hb_filter_get_tunes_short_name(int filter_id);
char ** hb_filter_get_tunes_name(int filter_id);
char  * hb_filter_get_presets_json(int filter_id);
char  * hb_filter_get_tunes_json(int filter_id);

#endif // HANDBRAKE_PARAM_H
