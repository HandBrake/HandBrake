/* encx264.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "x264.h"

static const char * const  hb_h264_level_names[] = { "1.0", "1b", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0", "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2",  NULL };
static const int    const hb_h264_level_values[] = {    10,    9,    11,    12,    13,    20,    21,    22,    30,    31,    32,    40,    41,    42,    50,    51,    52,     0 };

/* x264 preferred option names (left) and synonyms (right).
 * The "preferred" names match names used in x264's param2string function more
 * closely than their corresponding synonyms, or are just shorter. */
static const char * const hb_x264_encopt_synonyms[][2] =
{
    { "deterministic",  "n-deterministic", },
    { "level",          "level-idc",       },
    { "ref",            "frameref",        },
    { "keyint-min",     "min-keyint",      },
    { "no-deblock",     "nf",              },
    { "deblock",        "filter",          },
    { "cqm",            "cqmfile",         },
    { "analyse",        "partitions",      },
    { "weightb",        "weight-b",        },
    { "direct",         "direct-pred",     },
    { "merange",        "me-range",        },
    { "mvrange",        "mv-range",        },
    { "mvrange-thread", "mv-range-thread", },
    { "subme",          "subq",            },
    { "qp",             "qp_constant",     },
    { "qpmin",          "qp-min",          },
    { "qpmax",          "qp-max",          },
    { "qpstep",         "qp-step",         },
    { "ipratio",        "ip-factor",       },
    { "pbratio",        "pb-factor",       },
    { "cplxblur",       "cplx-blur",       },
    { NULL,             NULL,              },
};

/*
 * Check whether a valid h264_level is compatible with the given framerate,
 * resolution and interlaced compression/flags combination.
 *
 * width, height, fps_num and fps_den should be greater than zero.
 *
 * interlacing parameters can be set to zero when the information is
 * unavailable, as hb_apply_h264_level() will disable interlacing if necessary.
 *
 * Returns 0 if the level is valid and compatible, 1 otherwise.
 */
int hb_check_h264_level(const char *h264_level, int width, int height,
                        int fps_num, int fps_den, int interlaced,
                        int fake_interlaced);

/*
 * Applies the restrictions of the requested H.264 level to an x264_param_t.
 *
 * Returns -1 if an invalid level (or no level) is specified. GUIs should be
 * capable of always providing a valid level.
 *
 * Does not modify resolution/framerate but warns when they exceed level limits.
 *
 * Based on a x264_param_apply_level() draft and other x264 code.
 */
int hb_apply_h264_level(x264_param_t *param, const char *h264_level,
                        const char *x264_profile, int verbose);
