/* vidstab.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/avfilter_priv.h"

const char vidstabdetect_template[] =
    "result=^"HB_ALL_REG"$:"
    "shakiness=^"HB_INT_REG"$:accuracy=^"HB_INT_REG"$:stepsize=^"HB_INT_REG"$:"
    "mincontrast=^"HB_FLOAT_REG"$:"
    "tripod=^"HB_INT_REG"$:"
    "show=^([012])$:"
    "fileformat=^"HB_ALL_REG"$";

const char vidstabtransform_template[] =
    "input=^"HB_ALL_REG"$:"
    "smoothing=^"HB_INT_REG"$:"
    "optalgo=^"HB_ALL_REG"$:"
    "maxshift=^"HB_NEG_INT_REG"$:maxangle=^"HB_NEG_INT_REG"$:"
    "crop=^"HB_ALL_REG"$:"
    "invert=^([01])$:"
    "relative=^([01])$:"
    "zoom=^"HB_NEG_INT_REG"$:"
    "optzoom=^([012])$:"
    "zoomspeed=^"HB_FLOAT_REG"$:"
    "interpol=^"HB_ALL_REG"$:"
    "tripod=^([01])$:debug=^([01])$";

static int vidstabdetect_init(hb_filter_object_t *filter, hb_filter_init_t *init);
static int vidstabtransform_init(hb_filter_object_t *filter, hb_filter_init_t *init);

hb_filter_object_t hb_filter_vidstabdetect =
{
    .id                = HB_FILTER_VIDSTABDETECT,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Libvidstab (Pass 1)",
    .settings          = NULL,
    .init              = vidstabdetect_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = vidstabdetect_template
};

hb_filter_object_t hb_filter_vidstabtransform =
{
    .id                = HB_FILTER_VIDSTABTRANSFORM,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Libvidstab (Pass 2)",
    .settings          = NULL,
    .init              = vidstabtransform_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = vidstabtransform_template
};

static int vidstabdetect_init(hb_filter_object_t * filter, hb_filter_init_t *init)
{
    hb_filter_private_t *pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t *settings = filter->settings;

    char * result = NULL;
    int shakiness = 5, accuracy = 15, stepsize = 6;
    double mincontrast = 0.3;
    int tripod = 0, show = 0;
    char * fileformat = NULL;

    hb_dict_extract_int(&shakiness, settings, "shakiness");
    hb_dict_extract_int(&accuracy, settings, "accuracy");
    hb_dict_extract_int(&stepsize, settings, "stepsize");

    hb_dict_extract_double(&mincontrast, settings, "mincontrast");

    hb_dict_extract_int(&tripod, settings, "tripod");
    hb_dict_extract_int(&show, settings, "show");

    hb_dict_extract_string(&result, settings, "result");
    hb_dict_extract_string(&fileformat, settings, "fileformat");

    hb_dict_t *avsettings = hb_dict_init();
    hb_dict_t *avfilter = hb_dict_init();

    hb_dict_set_int(avsettings, "shakiness", shakiness);
    hb_dict_set_int(avsettings, "accuracy", accuracy);
    hb_dict_set_int(avsettings, "stepsize", stepsize);

    hb_dict_set_double(avsettings, "mincontrast", mincontrast);

    hb_dict_set_int(avsettings, "tripod", tripod);
    hb_dict_set_int(avsettings, "show", show);

    if (result != NULL)
    {
        hb_dict_set_string(avsettings, "result", result);
        free(result);
    }
    if (fileformat != NULL)
    {
        hb_dict_set_string(avsettings, "fileformat", fileformat);
        free(fileformat);
    }

    hb_dict_set(avfilter, "vidstabdetect", avsettings);

    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}

static int vidstabtransform_init(hb_filter_object_t * filter, hb_filter_init_t *init)
{
    hb_filter_private_t *pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t *settings = filter->settings;

    char * input = NULL;
    int smoothing = 10;
    char * optalgo = NULL;
    int maxshift = -1, maxangle = -1;
    char * crop = NULL;
    int invert = 0, relative = 0;
    int zoom = 0;
    int optzoom = 1;
    double zoomspeed = 0.25;
    char * interpol = NULL;
    int tripod = 0, debug = 0;

    hb_dict_extract_string(&input, settings, "input");

    hb_dict_extract_int(&smoothing, settings, "smoothing");

    hb_dict_extract_string(&optalgo, settings, "optalgo");

    hb_dict_extract_int(&maxshift, settings, "maxshift");
    hb_dict_extract_int(&maxangle, settings, "maxangle");

    hb_dict_extract_string(&crop, settings, "crop");

    hb_dict_extract_int(&invert, settings, "invert");
    hb_dict_extract_int(&relative, settings, "relative");
    hb_dict_extract_int(&zoom, settings, "zoom");
    hb_dict_extract_int(&zoom, settings, "optzoom");

    hb_dict_extract_double(&zoomspeed, settings, "zoomspeed");

    hb_dict_extract_string(&interpol, settings, "interpol");

    hb_dict_extract_int(&tripod, settings, "tripod");
    hb_dict_extract_int(&debug, settings, "debug");

    hb_dict_t *avsettings = hb_dict_init();
    hb_dict_t *avfilter = hb_dict_init();

    hb_dict_set_int(avsettings, "smoothing", smoothing);
    hb_dict_set_int(avsettings, "maxshift", maxshift);
    hb_dict_set_int(avsettings, "maxangle", maxangle);
    hb_dict_set_int(avsettings, "invert", invert);
    hb_dict_set_int(avsettings, "relative", relative);
    hb_dict_set_int(avsettings, "zoom", zoom);
    hb_dict_set_int(avsettings, "optzoom", optzoom);
    hb_dict_set_int(avsettings, "tripod", tripod);
    hb_dict_set_int(avsettings, "debug", debug);

    hb_dict_set_double(avsettings, "zoomspeed", zoomspeed);

    if (input != NULL)
    {
        hb_dict_set_string(avsettings, "input", input);
        free(input);
    }
    if (optalgo != NULL)
    {
        hb_dict_set_string(avsettings, "optalgo", optalgo);
        free(optalgo);
    }
    if (crop != NULL)
    {
        hb_dict_set_string(avsettings, "crop", crop);
        free(crop);
    }
    if (interpol != NULL)
    {
        hb_dict_set_string(avsettings, "interpol", interpol);
        free(interpol);
    }

    hb_dict_set(avfilter, "vidstabtransform", avsettings);

    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
