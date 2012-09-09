/* encx264.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "x264.h"

static const char * const  hb_h264_level_names[] = { "1.0", "1b", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0", "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2",  0 };
static const int    const hb_h264_level_values[] = {    10,    9,    11,    12,    13,    20,    21,    22,    30,    31,    32,    40,    41,    42,    50,    51,    52,  0 };

/* x264 preferred option names (left) and synonyms (right).
 * The "preferred" names match names used in x264's param2string function more
 * closely than their corresponding synonyms, or are just shorter. */
static const char * const hb_x264_encopt_synonyms[] =
{
    "deterministic",  "n-deterministic",
    "level",          "level-idc",
    "ref",            "frameref",
    "keyint-min",     "min-keyint",
    "deblock",        "filter",
    "analyse",        "partitions",
    "weightb",        "weight-b",
    "direct",         "direct-pred",
    "merange",        "me-range",
    "mvrange",        "mv-range",
    "mvrange-thread", "mv-range-thread",
    "subme",          "subq",
    "qp",             "qp_constant",
    "qpmin",          "qp-min",
    "qpmax",          "qp-max",
    "qpstep",         "qp-step",
    "ipratio",        "ip-factor",
    "pbratio",        "pb-factor",
    "cplxblur",       "cplx-blur",
    "cqm",            "cqmfile",
    0
};

int hb_apply_h264_level(x264_param_t *param,
                        int width, int height,
                        const char *h264_level,
                        const char *x264_profile);
