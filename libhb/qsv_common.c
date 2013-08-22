/* qsv_common.c
 *
 * Copyright (c) 2003-2013 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "ports.h"
#include "common.h"
#include "hb_dict.h"
#include "qsv_common.h"
#include "h264_common.h"

// for x264_vidformat_names etc.
#include "x264.h"

// avoids a warning
#include "libavutil/cpu.h"
extern void ff_cpu_cpuid(int index, int *eax, int *ebx, int *ecx, int *edx);

// make the Intel QSV information available to the UIs
hb_qsv_info_t *hb_qsv_info = NULL;

// availability and versions
static mfxVersion qsv_hardware_version;
static mfxVersion qsv_software_version;
static mfxVersion qsv_minimum_version;
static int qsv_hardware_available = 0;
static int qsv_software_available = 0;

// check available Intel Media SDK version against a minimum
#define HB_CHECK_MFX_VERSION(MFX_VERSION, MAJOR, MINOR) \
    (MFX_VERSION.Major == MAJOR  && MFX_VERSION.Minor >= MINOR)

int hb_qsv_available()
{
    return hb_qsv_info != NULL && (qsv_hardware_available ||
                                   qsv_software_available);
}

int hb_qsv_info_init()
{
    static int init_done = 0;
    if (init_done)
        return (hb_qsv_info == NULL);
    init_done = 1;

    hb_qsv_info = calloc(1, sizeof(*hb_qsv_info));
    if (hb_qsv_info == NULL)
    {
        hb_error("hb_qsv_info_init: alloc failure");
        return -1;
    }

    mfxSession session;
    qsv_minimum_version.Major = HB_QSV_MINVERSION_MAJOR;
    qsv_minimum_version.Minor = HB_QSV_MINVERSION_MINOR;

    // check for software fallback
    if (MFXInit(MFX_IMPL_SOFTWARE,
                &qsv_minimum_version, &session) == MFX_ERR_NONE)
    {
        qsv_software_available = 1;
        // our minimum is supported, but query the actual version
        MFXQueryVersion(session, &qsv_software_version);
        MFXClose(session);
    }

    // check for actual hardware support
    if (MFXInit(MFX_IMPL_HARDWARE_ANY|MFX_IMPL_VIA_ANY,
                &qsv_minimum_version, &session) == MFX_ERR_NONE)
    {
        qsv_hardware_available = 1;
        // our minimum is supported, but query the actual version
        MFXQueryVersion(session, &qsv_hardware_version);
        MFXClose(session);
    }

    // check for version-specific or hardware-specific capabilities
    // we only use software as a fallback, so check hardware first
    if (qsv_hardware_available)
    {
        if (HB_CHECK_MFX_VERSION(qsv_hardware_version, 1, 6))
        {
            hb_qsv_info->capabilities |= HB_QSV_CAP_OPTION2_BRC;
            hb_qsv_info->capabilities |= HB_QSV_CAP_MSDK_API_1_6;
        }
        if (hb_get_cpu_platform() == HB_CPU_PLATFORM_INTEL_HSW)
        {
            if (HB_CHECK_MFX_VERSION(qsv_hardware_version, 1, 7))
            {
                hb_qsv_info->capabilities |= HB_QSV_CAP_OPTION2_TRELLIS;
                hb_qsv_info->capabilities |= HB_QSV_CAP_OPTION2_LOOKAHEAD;
            }
            hb_qsv_info->capabilities |= HB_QSV_CAP_H264_BPYRAMID;
        }
    }
    else if (qsv_software_available)
    {
        if (HB_CHECK_MFX_VERSION(qsv_software_version, 1, 6))
        {
            hb_qsv_info->capabilities |= HB_QSV_CAP_MSDK_API_1_6;
            hb_qsv_info->capabilities |= HB_QSV_CAP_H264_BPYRAMID;
        }
    }

    // note: we pass a pointer to MFXInit but it never gets modified
    //       let's make sure of it just to be safe though
    if (qsv_minimum_version.Major != HB_QSV_MINVERSION_MAJOR ||
        qsv_minimum_version.Minor != HB_QSV_MINVERSION_MINOR)
    {
        hb_error("hb_qsv_info_init: minimum version (%d.%d) was modified",
                 qsv_minimum_version.Major,
                 qsv_minimum_version.Minor);
    }

    // success
    return 0;
}

// we don't need it beyond this point
#undef HB_CHECK_MFX_VERSION

void hb_qsv_info_print()
{
    if (hb_qsv_info == NULL)
        return;

    // is QSV available?
    hb_log("Intel Quick Sync Video support: %s",
           hb_qsv_available() ? "yes": "no");

    // if we have Quick Sync Video support, also print the details
    if (hb_qsv_available())
    {
        if (qsv_hardware_available)
        {
            hb_log(" - Intel Media SDK hardware: API %d.%d (minimum: %d.%d)",
                   qsv_hardware_version.Major,
                   qsv_hardware_version.Minor,
                   qsv_minimum_version.Major,
                   qsv_minimum_version.Minor);
        }
        if (qsv_software_available)
        {
            hb_log(" - Intel Media SDK software: API %d.%d (minimum: %d.%d)",
                   qsv_software_version.Major,
                   qsv_software_version.Minor,
                   qsv_minimum_version.Major,
                   qsv_minimum_version.Minor);
        }
    }
}

const char* hb_qsv_decode_get_codec_name(enum AVCodecID codec_id)
{
    switch (codec_id)
    {
        case AV_CODEC_ID_H264:
            return "h264_qsv";

        default:
            return NULL;
    }
}

int hb_qsv_decode_is_enabled(hb_job_t *job)
{
    return ((job != NULL && job->title->qsv_decode_support && job->qsv_decode) &&
            (job->vcodec & HB_VCODEC_QSV_MASK));
}

int hb_qsv_decode_is_supported(enum AVCodecID codec_id,
                               enum AVPixelFormat pix_fmt)
{
    switch (codec_id)
    {
        case AV_CODEC_ID_H264:
            return (pix_fmt == AV_PIX_FMT_YUV420P ||
                    pix_fmt == AV_PIX_FMT_YUVJ420P);

        default:
            return 0;
    }
}

int hb_qsv_codingoption_xlat(int val)
{
    switch (HB_QSV_CLIP3(-1, 2, val))
    {
        case 0:
            return MFX_CODINGOPTION_OFF;
        case 1:
        case 2: // MFX_CODINGOPTION_ADAPTIVE, reserved
            return MFX_CODINGOPTION_ON;
        case -1:
        default:
            return MFX_CODINGOPTION_UNKNOWN;
    }
}

int hb_qsv_trellisvalue_xlat(int val)
{
    switch (HB_QSV_CLIP3(-1, 3, val))
    {
        case 0:
            return MFX_TRELLIS_OFF;
        case 1: // I-frames only
            return MFX_TRELLIS_I;
        case 2: // I- and P-frames
            return MFX_TRELLIS_I|MFX_TRELLIS_P;
        case 3: // all frames
            return MFX_TRELLIS_I|MFX_TRELLIS_P|MFX_TRELLIS_B;
        case -1:
        default:
            return MFX_TRELLIS_UNKNOWN;
    }
}

int hb_qsv_atoindex(const char* const *arr, const char *str, int *err)
{
    int i;
    for (i = 0; arr[i] != NULL; i++)
    {
        if (!strcasecmp(arr[i], str))
        {
            break;
        }
    }
    *err = (arr[i] == NULL);
    return i;
}

// adapted from libx264
int hb_qsv_atobool(const char *str, int *err)
{
    if (!strcasecmp(str,    "1") ||
        !strcasecmp(str,  "yes") ||
        !strcasecmp(str, "true"))
    {
        return 1;
    }
    if (!strcasecmp(str,     "0") ||
        !strcasecmp(str,    "no") ||
        !strcasecmp(str, "false"))
    {
        return 0;
    }
    *err = 1;
    return 0;
}

// adapted from libx264
int hb_qsv_atoi(const char *str, int *err)
{
    char *end;
    int v = strtol(str, &end, 0);
    if (end == str || end[0] != '\0')
    {
        *err = 1;
    }
    return v;
}

// adapted from libx264
float hb_qsv_atof(const char *str, int *err)
{
    char *end;
    float v = strtod(str, &end);
    if (end == str || end[0] != '\0')
    {
        *err = 1;
    }
    return v;
}

int hb_qsv_param_parse(hb_qsv_param_t *param,
                       const char *key, const char *value, int vcodec)
{
    float fvalue;
    int ivalue, error = 0;
    if (param == NULL)
    {
        return HB_QSV_PARAM_ERROR;
    }
    if (value == NULL || value[0] == '\0')
    {
        value = "true";
    }
    else if (value[0] == '=')
    {
        value++;
    }
    if (key == NULL || key[0] == '\0')
    {
        return HB_QSV_PARAM_BAD_NAME;
    }
    else if (!strncasecmp(key, "no-", 3))
    {
        key  += 3;
        value = hb_qsv_atobool(value, &error) ? "false" : "true";
        if (error)
        {
            return HB_QSV_PARAM_BAD_VALUE;
        }
    }
    if (!strcasecmp(key, "target-usage") ||
        !strcasecmp(key, "tu"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->videoParam->mfx.TargetUsage = HB_QSV_CLIP3(MFX_TARGETUSAGE_1,
                                                              MFX_TARGETUSAGE_7,
                                                              ivalue);
        }
    }
    else if (!strcasecmp(key, "num-ref-frame") ||
             !strcasecmp(key, "ref"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->videoParam->mfx.NumRefFrame = HB_QSV_CLIP3(0, 16, ivalue);
        }
    }
    else if (!strcasecmp(key, "gop-ref-dist"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->videoParam->mfx.GopRefDist = HB_QSV_CLIP3(0, 32, ivalue);
        }
    }
    else if (!strcasecmp(key, "gop-pic-size") ||
             !strcasecmp(key, "keyint"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->gop.gop_pic_size = HB_QSV_CLIP3(-1, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "scenecut"))
    {
        ivalue = hb_qsv_atobool(value, &error);
        if (!error)
        {
            if (!ivalue)
            {
                param->videoParam->mfx.GopOptFlag |= MFX_GOP_STRICT;
            }
            else
            {
                param->videoParam->mfx.GopOptFlag &= ~MFX_GOP_STRICT;
            }
        }
    }
    else if (!strcasecmp(key, "cqp-offset-i"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[0] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cqp-offset-p"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[1] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cqp-offset-b"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[2] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "vbv-init"))
    {
        fvalue = hb_qsv_atof(value, &error);
        if (!error)
        {
            param->rc.vbv_buffer_init = HB_QSV_CLIP3(0, UINT16_MAX, fvalue);
        }
    }
    else if (!strcasecmp(key, "vbv-bufsize"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.vbv_buffer_size = HB_QSV_CLIP3(0, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "vbv-maxrate"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.vbv_max_bitrate = HB_QSV_CLIP3(0, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cabac"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = !hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->codingOption.CAVLC = hb_qsv_codingoption_xlat(ivalue);
        }
    }
    else if (!strcasecmp(key, "rate-distorsion-opt") ||
             !strcasecmp(key, "rdo"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->codingOption.RateDistortionOpt = hb_qsv_codingoption_xlat(ivalue);
        }
    }
    else if (!strcasecmp(key, "videoformat"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoindex(x264_vidformat_names, value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.VideoFormat = ivalue;
        }
    }
    else if (!strcasecmp(key, "fullrange"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoindex(x264_fullrange_names, value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.VideoFullRange = ivalue;
        }
    }
    else if (!strcasecmp(key, "colorprim"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoindex(x264_colorprim_names, value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.ColourPrimaries = ivalue;
        }
    }
    else if (!strcasecmp(key, "transfer"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoindex(x264_transfer_names, value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.TransferCharacteristics = ivalue;
        }
    }
    else if (!strcasecmp(key, "colormatrix"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoindex(x264_colmatrix_names, value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.MatrixCoefficients = ivalue;
        }
    }
    else if (!strcasecmp(key, "tff") ||
             !strcasecmp(key, "interlaced"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoParam->mfx.FrameInfo.PicStruct = (ivalue                  ?
                                                          MFX_PICSTRUCT_FIELD_TFF :
                                                          MFX_PICSTRUCT_PROGRESSIVE);
        }
    }
    else if (!strcasecmp(key, "bff"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoParam->mfx.FrameInfo.PicStruct = (ivalue                  ?
                                                          MFX_PICSTRUCT_FIELD_BFF :
                                                          MFX_PICSTRUCT_PROGRESSIVE);
        }
    }
    else if (!strcasecmp(key, "mbbrc"))
    {
        if (hb_qsv_info->capabilities & HB_QSV_CAP_OPTION2_BRC)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->codingOption2.MBBRC = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "extbrc"))
    {
        if (hb_qsv_info->capabilities & HB_QSV_CAP_OPTION2_BRC)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->codingOption2.ExtBRC = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lookahead") ||
             !strcasecmp(key, "la"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (hb_qsv_info->capabilities & HB_QSV_CAP_OPTION2_LOOKAHEAD)
        {
            if (!error)
            {
                param->rc.lookahead = ivalue;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lookahead-depth") ||
             !strcasecmp(key, "la-depth"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoi(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (hb_qsv_info->capabilities & HB_QSV_CAP_OPTION2_LOOKAHEAD)
        {
            if (!error)
            {
                param->codingOption2.LookAheadDepth = HB_QSV_CLIP3(10, 100,
                                                                   ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "trellis"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_QSV_H264:
                ivalue = hb_qsv_atoi(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (hb_qsv_info->capabilities & HB_QSV_CAP_OPTION2_TRELLIS)
        {
            if (!error)
            {
                param->codingOption2.Trellis = hb_qsv_trellisvalue_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else
    {
        /*
         * TODO:
         * - slice count control
         * - open-gop
         * - fake-interlaced (mfxExtCodingOption.FramePicture???)
         * - intra-refresh
         */
        return HB_QSV_PARAM_BAD_NAME;
    }
    return error ? HB_QSV_PARAM_BAD_VALUE : HB_QSV_PARAM_OK;
}

int hb_qsv_param_default(hb_qsv_param_t *param, mfxVideoParam *videoParam)
{
    if (param != NULL && videoParam != NULL)
    {
        // introduced in API 1.0
        memset(&param->codingOption, 0, sizeof(mfxExtCodingOption));
        param->codingOption.Header.BufferId      = MFX_EXTBUFF_CODING_OPTION;
        param->codingOption.Header.BufferSz      = sizeof(mfxExtCodingOption);
        param->codingOption.MECostType           = 0; // reserved, must be 0
        param->codingOption.MESearchType         = 0; // reserved, must be 0
        param->codingOption.MVSearchWindow.x     = 0; // reserved, must be 0
        param->codingOption.MVSearchWindow.y     = 0; // reserved, must be 0
        param->codingOption.RefPicListReordering = 0; // reserved, must be 0
        param->codingOption.IntraPredBlockSize   = 0; // reserved, must be 0
        param->codingOption.InterPredBlockSize   = 0; // reserved, must be 0
        param->codingOption.MVPrecision          = 0; // reserved, must be 0
        param->codingOption.EndOfSequence        = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.RateDistortionOpt    = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.CAVLC                = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.ResetRefList         = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.MaxDecFrameBuffering = 0; // unspecified
        param->codingOption.AUDelimiter          = MFX_CODINGOPTION_OFF;
        param->codingOption.SingleSeiNalUnit     = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.PicTimingSEI         = MFX_CODINGOPTION_OFF;
        param->codingOption.VuiNalHrdParameters  = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.FramePicture         = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.3
        param->codingOption.RefPicMarkRep        = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.FieldOutput          = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.NalHrdConformance    = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.SingleSeiNalUnit     = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.VuiVclHrdParameters  = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.4
        param->codingOption.ViewOutput           = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.6
        param->codingOption.RecoveryPointSEI     = MFX_CODINGOPTION_UNKNOWN;

        // introduced in API 1.3
        memset(&param->videoSignalInfo, 0, sizeof(mfxExtVideoSignalInfo));
        param->videoSignalInfo.Header.BufferId          = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
        param->videoSignalInfo.Header.BufferSz          = sizeof(mfxExtVideoSignalInfo);
        param->videoSignalInfo.VideoFormat              = 5; // undefined
        param->videoSignalInfo.VideoFullRange           = 0; // TV range
        param->videoSignalInfo.ColourDescriptionPresent = 0; // don't write to bitstream
        param->videoSignalInfo.ColourPrimaries          = 2; // undefined
        param->videoSignalInfo.TransferCharacteristics  = 2; // undefined
        param->videoSignalInfo.MatrixCoefficients       = 2; // undefined

        // introduced in API 1.6
        memset(&param->codingOption2, 0, sizeof(mfxExtCodingOption2));
        param->codingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        param->codingOption2.Header.BufferSz = sizeof(mfxExtCodingOption2);
        param->codingOption2.IntRefType      = 0;
        param->codingOption2.IntRefCycleSize = 2;
        param->codingOption2.IntRefQPDelta   = 0;
        param->codingOption2.MaxFrameSize    = 0;
        param->codingOption2.BitrateLimit    = MFX_CODINGOPTION_ON;
        param->codingOption2.ExtBRC          = MFX_CODINGOPTION_OFF;
        param->codingOption2.MBBRC           = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.7
        param->codingOption2.LookAheadDepth  = 40;
        param->codingOption2.Trellis         = MFX_TRELLIS_UNKNOWN;

        // GOP & rate control
        param->gop.gop_pic_size       = -1; // set automatically
        param->gop.int_ref_cycle_size = -1; // set automatically
        param->rc.lookahead           = -1; // set automatically
        param->rc.cqp_offsets[0]      =  0;
        param->rc.cqp_offsets[1]      =  2;
        param->rc.cqp_offsets[2]      =  4;
        param->rc.vbv_max_bitrate     =  0;
        param->rc.vbv_buffer_size     =  0;
        param->rc.vbv_buffer_init     = .5;

        // introduced in API 1.0
        memset(videoParam, 0, sizeof(mfxVideoParam));
        param->videoParam                   = videoParam;
        param->videoParam->Protected        = 0; // reserved, must be 0
        param->videoParam->NumExtParam      = 0;
        param->videoParam->IOPattern        = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        param->videoParam->mfx.TargetUsage  = MFX_TARGETUSAGE_2;
        param->videoParam->mfx.GopOptFlag   = MFX_GOP_CLOSED;
        param->videoParam->mfx.NumThread    = 0; // deprecated, must be 0
        param->videoParam->mfx.EncodedOrder = 0; // input is in display order
        param->videoParam->mfx.IdrInterval  = 0; // all I-frames are IDR
        param->videoParam->mfx.NumSlice     = 0; // use Media SDK default
        param->videoParam->mfx.NumRefFrame  = 0; // use Media SDK default
        param->videoParam->mfx.GopPicSize   = 0; // use Media SDK default
        param->videoParam->mfx.GopRefDist   = 4; // power of 2, >= 4: B-pyramid
        // introduced in API 1.1
        param->videoParam->AsyncDepth = AV_QSV_ASYNC_DEPTH_DEFAULT;
        // introduced in API 1.3
        param->videoParam->mfx.BRCParamMultiplier = 0; // no multiplier

        // FrameInfo: set by video encoder, except PicStruct
        param->videoParam->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        // attach supported mfxExtBuffer structures to the mfxVideoParam
        param->videoParam->NumExtParam                                = 0;
        param->videoParam->ExtParam                                   = param->ExtParamArray;
        param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->codingOption;
        param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->videoSignalInfo;
        if (hb_qsv_info->capabilities & HB_QSV_CAP_MSDK_API_1_6)
        {
            param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->codingOption2;
        }
    }
    else
    {
        hb_error("hb_qsv_param_default: invalid pointer(s)");
        return -1;
    }
    return 0;
}
