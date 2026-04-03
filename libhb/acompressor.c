/* acompressor.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

const char acompressor_template[] =
    "level-in=^"HB_FLOAT_REG"$:mode=^"HB_INT_REG"$:threshold=^"HB_FLOAT_REG"$:"
    "ratio=^"HB_FLOAT_REG"$:attack=^"HB_FLOAT_REG"$:release=^"HB_FLOAT_REG"$:"
    "makeup=^"HB_FLOAT_REG"$:knee=^"HB_FLOAT_REG"$:link=^"HB_INT_REG"$"
    "detection=^"HB_INT_REG"$:level-sc=^"HB_FLOAT_REG"$:mix=^"HB_FLOAT_REG"$";

static int acompressor_init(hb_filter_object_t *filter, hb_filter_init_t *init)
{
    hb_filter_private_t *pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }

    hb_filter_init_copy(&pv->input, init);

    hb_dict_t *settings = filter->settings;

    double level_in = 1, threshold = 0.125, ratio = 2, attack = 20;
    double release = 250, makeup = 1, knee = 2.82843, level_sc = 1, mix = 1;
    int    mode = 0, link = 0, detection = 1;

    hb_dict_extract_double(&level_in, settings, "level-in");
    hb_dict_extract_int(&mode, settings, "mode");
    hb_dict_extract_double(&threshold, settings, "threshold");
    hb_dict_extract_double(&ratio, settings, "ratio");
    hb_dict_extract_double(&attack, settings, "attack");
    hb_dict_extract_double(&release, settings, "release");
    hb_dict_extract_double(&makeup, settings, "makeup");
    hb_dict_extract_double(&knee, settings, "knee");
    hb_dict_extract_int(&link, settings, "link");
    hb_dict_extract_int(&detection, settings, "detection");
    hb_dict_extract_double(&level_sc, settings, "level-sc");
    hb_dict_extract_double(&mix, settings, "mix");

    hb_dict_t *avfilter = hb_dict_init();
    hb_dict_t *avsettings = hb_dict_init();

    hb_dict_set(avsettings, "level_in",  hb_value_double(level_in));
    hb_dict_set(avsettings, "mode",      hb_value_int(mode));
    hb_dict_set(avsettings, "threshold", hb_value_double(threshold));
    hb_dict_set(avsettings, "ratio",     hb_value_double(ratio));
    hb_dict_set(avsettings, "attack",    hb_value_double(attack));
    hb_dict_set(avsettings, "release",   hb_value_double(release));
    hb_dict_set(avsettings, "makeup",    hb_value_double(makeup));
    hb_dict_set(avsettings, "knee",      hb_value_double(knee));
    hb_dict_set(avsettings, "link",      hb_value_int(link));
    hb_dict_set(avsettings, "detection", hb_value_int(detection));
    hb_dict_set(avsettings, "level_sc",  hb_value_double(level_sc));
    hb_dict_set(avsettings, "mix",       hb_value_double(mix));

    hb_dict_set(avfilter, "acompressor", avsettings);
    pv->avfilters = avfilter;

    hb_filter_init_copy(&pv->output, init);

    return 0;
}

hb_filter_object_t hb_filter_acompressor =
{
    .id                = HB_AUDIO_FILTER_ACOMPRESSOR,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Compressor",
    .settings          = NULL,
    .init              = acompressor_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = acompressor_template,
};
