/* preset.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#if !defined(HANDBRAKE_PRESET_H)
#define HANDBRAKE_PRESET_H

#include "handbrake/common.h"
#include "handbrake/hb_dict.h"

#define HB_MAX_PRESET_FOLDER_DEPTH  8

#define HB_PRESET_TYPE_OFFICIAL 0
#define HB_PRESET_TYPE_CUSTOM   1
#define HB_PRESET_TYPE_ALL      2

typedef struct hb_preset_index_s hb_preset_index_t;

// A preset index is a list of indexes that specifies a path to a
// specific preset in a preset list.  Since a preset list can have
// folders that contain sub-lists, multiple index values are needed
// to create a complete path.
struct hb_preset_index_s
{
    int depth;
    int index[HB_MAX_PRESET_FOLDER_DEPTH];
};

#ifdef __LIBHB__
// Preset APIs reserved for libhb

// Initialize the hb_value_array_t that holds HandBrake builtin presets
// These presets come from a json string embedded in libhb and can be
// retrieved with hb_presets_builtin_get() after initialization.
void         hb_presets_builtin_init(void);

// Free all libhb presets. This should only be called when closing
// an hb_handle_t.
void         hb_presets_free(void);

#endif // __LIBHB__

// Add the default CLI preset(s) to the currently loaded presets
int          hb_presets_cli_default_init(void);

// Get the currently supported preset format version
void hb_presets_current_version(int *major, int* minor, int *micro);

// Get the format version of a preset dict
int hb_presets_version(hb_value_t *preset, int *major, int *minor, int *micro);

// Initialize a new preset index.  "index" may be NULL
hb_preset_index_t * hb_preset_index_init(const int *index, int depth);

// Duplicate a preset index
hb_preset_index_t * hb_preset_index_dup(const hb_preset_index_t *path);

// Append one index to another
void                hb_preset_index_append(hb_preset_index_t *dst,
                                           const hb_preset_index_t *src);

// Load presets list from GUI presets file if it exists. This should probably
// only be used by the CLI.
int          hb_presets_gui_init(void);

// Get HandBrake builtin presets list as hb_value_array_t
hb_value_t * hb_presets_builtin_get(void);

// Get HandBrake builtin presets list as json string
char       * hb_presets_builtin_get_json(void);

// Load default builtin presets list over the top of any builtins in the
// current preset list. This should be used by a frontend when it recognizes
// that it's preset file is from an older version of HandBrake.
void         hb_presets_builtin_update(void);

// Clean presets.  Removes unknown keys and normalizes values.
// This should be applied before exporting a preset and is applied
// for you by hb_presets_write_json() and hb_preset_package_json().
void         hb_presets_clean(hb_value_t *preset);

// Clean json presets.  Removes unknown keys and normalizes values.
// This should be applied before exporting a preset and is applied
// for you by hb_presets_write_json() and hb_preset_package_json().
char       * hb_presets_clean_json(const char *json);

// Import a preset.  Sanitizes and converts old key/value pairs
// to new key/value pairs.  This is applied for you by hb_presets_add(),
// hb_presets_add_json(), hb_presets_add_file(), and hb_presets_add_path()
int          hb_presets_import(const hb_value_t *in, hb_value_t **out);

// Import a json preset.  Sanitizes and converts old key/value pairs
// to new key/value pairs.
int          hb_presets_import_json(const char *in, char **out);

// Register new presets with libhb from json string
int          hb_presets_add_json(const char *json);

// Read a preset file.  Does not add to internal preset list.
hb_value_t * hb_presets_read_file(const char *filename);

// Read a preset file.  Does not add to internal preset list.
// Returns a json string.
char       * hb_presets_read_file_json(const char *filename);

// Register new presets with libhb from a preset dict
int          hb_presets_add(hb_value_t *preset);

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

// Apply preset Muxer settings to a job
int hb_preset_apply_mux(const hb_dict_t *preset, hb_dict_t *job_dict);

// Apply preset Video settings to a job
int hb_preset_apply_video(const hb_dict_t *preset, hb_dict_t *job_dict);

// Apply preset Filter settings to a job
//
// Note that this does not apply scale filter settings.  A title is
// required to set the default scale filter settings, so this filter
// is applied in hb_preset_apply_title()
int hb_preset_apply_filters(const hb_dict_t *preset, hb_dict_t *job_dict);

// Apply preset settings that require a title to a job
int hb_preset_apply_title(hb_handle_t *h, int title_index,
                          const hb_dict_t *preset, hb_dict_t *job_dict);

// Initialize a job from the given title and preset
hb_dict_t  * hb_preset_job_init(hb_handle_t *h, int title_index,
                                const hb_dict_t *preset);

// Reinitialize subtitles from preset defaults.
int hb_preset_job_add_subtitles(hb_handle_t *h, int title_index,
                                const hb_dict_t *preset, hb_dict_t *job_dict);

// Reinitialize audio from preset defaults.
int hb_preset_job_add_audio(hb_handle_t *h, int title_index,
                            const hb_dict_t *preset, hb_dict_t *job_dict);
void hb_sanitize_audio_settings(const hb_title_t * title,
                                hb_value_t * audio_settings);

// Lookup a preset in the preset list.  The "name" may contain '/'
// separators to explicitly specify a preset within the preset lists
// folder structure.
//
// If 'recurse' is specified, a recursive search for the first component
// in the name will be performed.
//
// I assume that the actual preset name does not include any '/'
hb_preset_index_t * hb_preset_search_index(const char *name,
                                           int recurse, int type);
hb_value_t        * hb_preset_search(const char *name, int recurse, int type);
char              * hb_preset_search_json(const char *name,
                                          int recurs, int type);

hb_value_t * hb_presets_get_folder_children(const hb_preset_index_t *path);
hb_value_t * hb_preset_get(const hb_preset_index_t *path);
int          hb_preset_delete(const hb_preset_index_t *path);
int          hb_preset_set(const hb_preset_index_t *path,
                           const hb_value_t *dict);
int          hb_preset_insert(const hb_preset_index_t *path,
                              const hb_value_t *dict);
int          hb_preset_append(const hb_preset_index_t *path,
                              const hb_value_t *dict);
int          hb_preset_move(const hb_preset_index_t *src_path,
                            const hb_preset_index_t *dst_path);

// Recursively lookup the preset that is marked as 'Default'
hb_dict_t         * hb_presets_get_default(void);
char              * hb_presets_get_default_json(void);
hb_preset_index_t * hb_presets_get_default_index(void);
void                hb_presets_clear_default(void);

// Package the provided preset (wrap in dict and add version etc)
// and write to json file
int          hb_presets_write_json(const hb_value_t *preset, const char *path);

// Package the provided preset (wrap in dict and add version etc)
// and return as json string
char       * hb_presets_package_json(const hb_value_t *presets);

// Package the provided json presets list (wrap in dict and add version etc)
// and return as json string
char       * hb_presets_json_package(const char *json_presets);

#endif // HANDBRAKE_PRESET_H
