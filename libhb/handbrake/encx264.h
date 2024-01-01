/* encx264.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_ENCX264_H
#define HANDBRAKE_ENCX264_H

#include "x264.h"
#include "handbrake/h264_common.h"

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

typedef struct x264_api_s
{
    int     bit_depth;
    void    (*param_default)(x264_param_t*);
    int     (*param_default_preset)(x264_param_t*, const char*, const char*);
    int     (*param_apply_profile)(x264_param_t*, const char*);
    void    (*param_apply_fastfirstpass)(x264_param_t*);
    int     (*param_parse)(x264_param_t*, const char*, const char*);
    x264_t* (*encoder_open)(x264_param_t*);
    int     (*encoder_headers)(x264_t*, x264_nal_t**, int*);
    int     (*encoder_encode)(x264_t*, x264_nal_t**, int*,
                              x264_picture_t*, x264_picture_t*);
    int     (*encoder_delayed_frames)(x264_t*);
    void    (*encoder_close)(x264_t*);
    void    (*picture_init)(x264_picture_t*);
} x264_api_t;

void               hb_x264_global_init(void);
const x264_api_t * hb_x264_api_get(int bit_depth);

#endif // HANDBRAKE_ENCX264_H
