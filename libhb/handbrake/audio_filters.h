/* audio_filters.h

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_AUDIO_FILTERS_H
#define HANDBRAKE_AUDIO_FILTERS_H

#include "handbrake/hbtypes.h"

// Audio filter identifiers
// Order defines the recommended signal chain:
//   restore -> denoise -> dynamics -> enhance -> spatial -> upmix -> limit -> normalize
enum
{
    HB_AUDIO_FILTER_ADECLICK = 0,   // Restoration
    HB_AUDIO_FILTER_ADECLIP,
    HB_AUDIO_FILTER_AFFTDN,         // Denoising
    HB_AUDIO_FILTER_ANLMDN,
    HB_AUDIO_FILTER_ACOMPRESSOR,    // Dynamics
    HB_AUDIO_FILTER_AGATE,
    HB_AUDIO_FILTER_DIALOGUENHANCE, // Enhancement
    HB_AUDIO_FILTER_CROSSFEED,      // Spatial
    HB_AUDIO_FILTER_STEREOWIDEN,
    HB_AUDIO_FILTER_SURROUND,       // Upmix
    HB_AUDIO_FILTER_ALIMITER,       // Limiting
    HB_AUDIO_FILTER_LOUDNORM,       // Normalization (always last)
    HB_AUDIO_FILTER_COUNT,
};

typedef struct
{
    int          id;             // HB_AUDIO_FILTER_* enum value
    const char * name;           // Display name
    const char * short_name;     // CLI option name (e.g. "acompressor")
    const char * ffmpeg_name;    // FFmpeg filter name
    const char * default_str;    // Full default FFmpeg filter string
    const char * prefix;         // Prefix to prepend (e.g. "aformat=...,")
                                 // NULL if no prefix needed
    const char * description;    // Short description for help text
} hb_audio_filter_info_t;

// Get the number of registered audio filters
int hb_audio_filter_get_count(void);

// Get audio filter info by id
const hb_audio_filter_info_t * hb_audio_filter_get_info(int filter_id);

// Get audio filter info by short_name (CLI name)
const hb_audio_filter_info_t * hb_audio_filter_get_info_by_name(const char * name);

// Build an FFmpeg filter string from user input for a given audio filter.
// If value is "default", returns the default string.
// If value already starts with the FFmpeg filter name, returns it as-is.
// Otherwise, prepends "ffmpeg_name=" to value.
// Caller must free() the returned string.
char * hb_audio_filter_build_string(int filter_id, const char * value);

// Audio filter chain entry (one per filter in the chain)
typedef struct
{
    int    id;       // HB_AUDIO_FILTER_* enum value
    char * settings; // User-provided settings (e.g. "nr=12:nf=-50" or "default")
} hb_audio_filter_entry_t;

// Create/close individual filter entries
hb_audio_filter_entry_t * hb_audio_filter_entry_init(int id, const char * settings);
void                      hb_audio_filter_entry_close(hb_audio_filter_entry_t ** entry);

// Audio filter list management (list of hb_audio_filter_entry_t *)
hb_list_t * hb_audio_filter_list_copy(const hb_list_t * src);
void        hb_audio_filter_list_close(hb_list_t ** list);

// Build combined avfilter string from a filter list.
// Caller must free() the returned string.
char * hb_audio_filter_list_build_string(const hb_list_t * list);

#endif // HANDBRAKE_AUDIO_FILTERS_H
