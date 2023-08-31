/* rotate_vt.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <VideoToolbox/VideoToolbox.h>
#include "handbrake/handbrake.h"
#include "cv_utils.h"

struct hb_filter_private_s
{
    VTPixelRotationSessionRef session;
    CVPixelBufferPoolRef      pool;

    hb_filter_init_t          input;
    hb_filter_init_t          output;
};

static int rotate_vt_init(hb_filter_object_t *filter,
                          hb_filter_init_t   *init);

static int rotate_vt_work(hb_filter_object_t *filter,
                          hb_buffer_t **buf_in,
                          hb_buffer_t **buf_out);

static void rotate_vt_close(hb_filter_object_t *filter);

static const char rotate_vt_template[] =
    "angle=^(0|90|180|270)$:hflip=^"HB_BOOL_REG"$:disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_rotate_vt =
{
    .id                = HB_FILTER_ROTATE_VT,
    .enforce_order     = 1,
    .name              = "Rotate (VideoToolbox)",
    .settings          = NULL,
    .init              = rotate_vt_init,
    .work              = rotate_vt_work,
    .close             = rotate_vt_close,
    .settings_template = rotate_vt_template,
};

static int rotate_vt_init(hb_filter_object_t *filter,
                          hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("rotate_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;
    pv->input = *init;

    hb_dict_t    *settings = filter->settings;
    hb_rational_t par    = init->geometry.par;
    int           width  = init->geometry.width;
    int           height = init->geometry.height;
    int           angle  = 0, hflip = 0;

    if (__builtin_available (macOS 13, *))
    {
        CFStringRef   trans  = kVTRotation_0;

        hb_dict_extract_int(&angle, settings, "angle");
        hb_dict_extract_bool(&hflip, settings, "hflip");

        switch (angle)
        {
            case 0:
                break;
            case 90:
                trans   = kVTRotation_CW90;
                width   = init->geometry.height;
                height  = init->geometry.width;
                par.num = init->geometry.par.den;
                par.den = init->geometry.par.num;
                break;
            case 180:
                trans   = kVTRotation_180;
                break;
            case 270:
                trans   = kVTRotation_CCW90;
                width   = init->geometry.height;
                height  = init->geometry.width;
                par.num = init->geometry.par.den;
                par.den = init->geometry.par.num;
                break;
            default:
                break;
        }

        // Session init
        OSStatus err = noErr;
        err = VTPixelRotationSessionCreate(kCFAllocatorDefault, &pv->session);

        if (err != noErr)
        {
            hb_log("rotate_vt: err=%"PRId64"", (int64_t)err);
            return err;
        }

        err = VTSessionSetProperty(pv->session,
                                   kVTPixelRotationPropertyKey_Rotation,
                                   trans);

        if (err != noErr)
        {
            hb_log("rotate_vt: kVTPixelRotationPropertyKey_Rotation failed");
        }

        if (hflip)
        {
            err = VTSessionSetProperty(pv->session,
                                       kVTPixelRotationPropertyKey_FlipHorizontalOrientation,
                                       kCFBooleanTrue);

            if (err != noErr)
            {
                hb_log("rotate_vt: kVTPixelRotationPropertyKey_FlipHorizontalOrientation failed");
            }
        }
    }

    pv->pool = hb_cv_create_pixel_buffer_pool(width, height, init->pix_fmt, init->color_range);
    if (pv->pool == NULL)
    {
        hb_log("rotate_vt: CVPixelBufferPoolCreate failed");
        return -1;
    }

    init->geometry.width  = width;
    init->geometry.height = height;
    init->geometry.par    = par;
    pv->output = *init;

    return 0;
}

static void rotate_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    if (__builtin_available (macOS 13, *))
    {
        if (pv->session)
        {
            VTPixelRotationSessionInvalidate(pv->session);
            CFRelease(pv->session);
        }
    }

    if (pv->pool)
    {
        CVPixelBufferPoolRelease(pv->pool);
    }

    free(pv);
    filter->private_data = NULL;
}

static int rotate_vt_work(hb_filter_object_t *filter,
                          hb_buffer_t **buf_in,
                          hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    // Setup buffers
    OSStatus err = noErr;

    CVPixelBufferRef source_buf = hb_cv_get_pixel_buffer(in);
    if (source_buf == NULL)
    {
        hb_log("rotate_vt: extract_buf failed");
        return HB_FILTER_FAILED;
    }

    CVPixelBufferRef dest_buf = NULL;
    err = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->pool, &dest_buf);
    if (err != noErr)
    {
        hb_log("rotate_vt: CVPixelBufferPoolCreatePixelBuffer failed");
        return HB_FILTER_FAILED;
    }

    // Do work
    if (__builtin_available (macOS 13, *))
    {
        err = VTPixelRotationSessionRotateImage(pv->session, source_buf, dest_buf);
        if (err != noErr)
        {
            hb_log("rotate_vt: VTPixelRotationSessionRotateImage failed");
            return HB_FILTER_FAILED;
        }
    }

    out = hb_buffer_wrapper_init();
    out->storage_type      = COREMEDIA;
    out->storage           = dest_buf;
    out->f.width           = pv->output.geometry.width;
    out->f.height          = pv->output.geometry.height;
    out->f.fmt             = pv->output.pix_fmt;
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;
    hb_buffer_copy_props(out, in);

    *buf_out = out;

    return HB_FILTER_OK;
}
