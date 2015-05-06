/* hb_preset.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#if !defined(HB_PRESET_H)
#define HB_PRESET_H

#include "common.h"
#include "hb_dict.h"

// Initialize the hb_value_array_t that holds HandBrake builtin presets
void         hb_presets_builtin_init(void);

// Load presets from GUI presets file if possible
int          hb_presets_gui_init(void);

// Free all libhb presets
void         hb_presets_free(void);

// Get list of HandBrake builtin presets as hb_value_array_t
hb_value_t * hb_presets_builtin_get(void);

// Get list of HandBrake builtin presets as json string
char       * hb_presets_builtin_get_json(void);

// Register new presets with libhb from
// hb_dict_t (single preset) or hb_value_array_t (list of presets)
int          hb_presets_add(hb_value_t *preset);

// Register new presets with libhb from json string
int          hb_presets_add_json(const char *json);

// Register new presets with libhb from json file
int          hb_presets_add_file(const char *filename);

// Register new presets with libhb from json file(s)
// path can be a directory, in which case all *.json files in the
// directory will be added
int          hb_presets_add_path(char * path);

// Get list of all presets registered with libhb as hb_value_array_t
hb_value_t * hb_presets_get(void);

// Get list of all presets registered with libhb as json string
char       * hb_presets_get_json(void);

// Initialize a job from the given title and preset
hb_dict_t  * hb_preset_job_init(hb_handle_t *h, int title_index,
                                hb_dict_t *preset);
int hb_preset_job_add_subtitles(hb_handle_t *h, int title_index,
                                hb_dict_t *preset, hb_dict_t *job_dict);
int hb_preset_job_add_audio(hb_handle_t *h, int title_index,
                            hb_dict_t *preset, hb_dict_t *job_dict);

// Lookup a preset in the preset list.  The "name" may contain '/'
// separators to explicitely specify a preset within the preset lists
// folder structure.
//
// If 'recurse' is specified, a recursive search for the first component
// in the name will be performed.
//
// I assume that the actual preset name does not include any '/'
hb_value_t * hb_preset_get(const char *name, int recurse);
char       * hb_preset_get_json(const char *name, int recurse);

// Recursively lookup the preset that is marked as 'Default'
hb_dict_t  * hb_presets_get_default(void);
char       * hb_presets_get_default_json(void);

// Set the preset that is marked as 'Default'
int          hb_presets_set_default(const char *name, int recurse);

// Package the provided preset (wrap in dict and add version etc)
// and write to json file
int          hb_preset_write_json(hb_value_t *preset, const char *path);
// Package the provided preset (wrap in dict and add version etc)
// and return as json string
char       * hb_preset_package_json(hb_value_t *preset);

#endif // HB_PRESET_H
