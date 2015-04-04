/* param.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#ifndef HB_PARAM_H
#define HB_PARAM_H

extern const char hb_filter_off[];

typedef struct hb_filter_param_s hb_filter_param_t;

struct hb_filter_param_s
{
    int         index;
    const char *name;
    const char *short_name;
    const char *settings;
};

char * hb_generate_filter_settings(int filter_id,
                                   const char *preset, const char *tune);
char * hb_generate_filter_settings_by_index(int filter_id, int preset,
                                            const char *custom);

int    hb_validate_filter_preset(int filter_id,
                                 const char *preset, const char *tune);
int    hb_validate_filter_settings(int filter_id, const char *filter_param);
int    hb_validate_param_string(const char *regex_pattern,
                                const char *param_string);

hb_filter_param_t * hb_filter_param_get_presets(int filter_id);
hb_filter_param_t * hb_filter_param_get_tunes(int filter_id);

#endif // HB_PARAM_H
