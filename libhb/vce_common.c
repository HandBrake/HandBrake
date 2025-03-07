/* vce_common.c
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/project.h"
#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

static int is_vcn_available = -1;
static int is_vcn_hevc_available = -1;
static int is_vcn_av1_available = -1;
static int is_vcn_decoder_available = -1;

#if HB_PROJECT_FEATURE_VCE
#include "AMF/core/Factory.h"
#include "AMF/components/VideoEncoderVCE.h"
#include "AMF/components/VideoEncoderHEVC.h"
#include "AMF/components/VideoEncoderAV1.h"
#include "AMF/components/VideoDecoderUVD.h"

AMF_RESULT check_component_available(const wchar_t *componentID)
{
    amf_handle          libHandle = NULL;
    AMFInit_Fn          initFun;
    AMFFactory         *factory = NULL;
    AMFContext         *context = NULL;
    AMFContext1        *context1 = NULL;
    AMFComponent       *component = NULL;
    AMFCaps            *componentCaps = NULL;
    AMF_RESULT          result = AMF_FAIL;

    libHandle = hb_dlopen(AMF_DLL_NAMEA);

    if(!libHandle)
    {
        result =  AMF_FAIL;
        goto clean;
    }

    initFun = (AMFInit_Fn)(hb_dlsym(libHandle, AMF_INIT_FUNCTION_NAME));
    if(!initFun)
    {
        result = AMF_FAIL;
        hb_error("VCE: Load Library Failed");
        goto clean;
    }

    result = initFun(AMF_FULL_VERSION, &factory);
    if(result != AMF_OK)
    {
        hb_error("VCE: Init Failed");
        goto clean;
    }

    result = factory->pVtbl->CreateContext(factory, &context);
    if(result != AMF_OK)
    {
        hb_error("VCE: Context Failed");
        goto clean;
    }

    result = context->pVtbl->InitDX11(context, NULL, AMF_DX11_1);
    if (result != AMF_OK) {
        result = context->pVtbl->InitDX9(context, NULL);
        if (result != AMF_OK) {
            AMFGuid guid = IID_AMFContext1();
            result = context->pVtbl->QueryInterface(context, &guid, (void**)&context1);
            if (result != AMF_OK) {
                hb_error("VCE: CreateContext1() failed");
                goto clean;
            }

            result = context1->pVtbl->InitVulkan(context1, NULL);
            if (result != AMF_OK) {
                if (result == AMF_NOT_SUPPORTED)
                    hb_error("VCE: AMF via Vulkan is not supported on the given device.\n");
                else
                    hb_error("VCE: AMF failed to initialise on the given Vulkan device.\n");
                goto clean;
            }
        }
    }

    result = factory->pVtbl->CreateComponent(factory, context, componentID, &component);

    if(result != AMF_OK)
    {
        goto clean;
    }

    result = component->pVtbl->GetCaps(component, &componentCaps);

clean:
    if (componentCaps)
    {
        componentCaps->pVtbl->Clear(componentCaps);
        componentCaps->pVtbl->Release(componentCaps);
        componentCaps = NULL;
    }
    if (component)
    {
        component->pVtbl->Terminate(component);
        component->pVtbl->Release(component);
        component = NULL;
    }
    if (context)
    {
        context->pVtbl->Terminate(context);
        context->pVtbl->Release(context);
        context = NULL;
    }
    if (context1)
    {
        context1->pVtbl->Terminate(context1);
        context1->pVtbl->Release(context1);
        context1 = NULL;
    }
    if(libHandle)
    {
        hb_dlclose(libHandle);
    }

    return result;
}

int hb_vce_h264_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }
    
    if (is_vcn_available != -1)
    {
        return is_vcn_available;
    }

    is_vcn_available = (check_component_available(AMFVideoEncoderVCE_AVC) == AMF_OK) ? 1 : 0;
    if (is_vcn_available == 1)
    {
        hb_log("vcn: is available");
    } 
    else 
    {
        hb_log("vcn: not available on this system");
    }
    
    return is_vcn_available;
}

int hb_vce_h265_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }
    
    if (is_vcn_hevc_available != -1)
    {
        return is_vcn_hevc_available;
    }

    is_vcn_hevc_available = (check_component_available(AMFVideoEncoder_HEVC) == AMF_OK) ? 1 : 0;
    return is_vcn_hevc_available;
}

int hb_vce_av1_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    if (is_vcn_av1_available != -1)
    {
        return is_vcn_av1_available;
    }

    is_vcn_av1_available = (check_component_available(AMFVideoEncoder_AV1) == AMF_OK) ? 1 : 0;
    return is_vcn_av1_available;
}

int hb_check_amfdec_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    if (is_vcn_decoder_available != -1)
    {
        return is_vcn_decoder_available;
    }

    is_vcn_decoder_available = (check_component_available(AMFVideoDecoderUVD_H264_AVC) == AMF_OK) ? 1 : 0;
    if (is_vcn_decoder_available == 1)
    {
        hb_log("vcn decoder: is available");
    }
    else
    {
        hb_log("vcn decoder: not available on this system");
    }

    return is_vcn_decoder_available;
}

int hb_vce_are_filters_supported(hb_list_t *filters)
{
    return  1;
}
int hb_vce_dec_is_enabled(hb_job_t *job)
{
    if (job && (job->hw_decode & HB_DECODE_SUPPORT_AMFDEC))
    {
        return 1;
    }
    return 0;
}
int hb_vce_sanitize_filter_list(hb_job_t *job)
{
    if (job && (job->hw_decode & HB_DECODE_SUPPORT_AMFDEC))
    {
        int i = 0;
        int num_sw_filters = 0;
        int num_hw_filters = 0;
        int converter_needed = 1;
        int mode;
        if (job->list_filter != NULL && hb_list_count(job->list_filter) > 0)
        {
            for (i = 0; i < hb_list_count(job->list_filter); i++)
            {
                hb_filter_object_t *filter = hb_list_item(job->list_filter, i);

                switch (filter->id)
                {
                    // cropping and scaling always done via VPP filter
                    case HB_FILTER_CROP_SCALE:
                        if(num_sw_filters == 0) // if corp/scale is a filter before SW filters, no need for filter in decoder
                        {
                            converter_needed = 0;
                        }
                        num_hw_filters++;
                        break;
                    case HB_FILTER_VFR:
                    {
                        mode = hb_dict_get_int(filter->settings, "mode");
                        if(mode != 0)
                        {
                            num_sw_filters++;
                        }

                        break;
                    }
                    default:
                        // count only filters with access to frame data
                        num_sw_filters++;
                        break;
                }
            }
        }
        if(num_sw_filters == 0)
        {
            converter_needed = 0;
        }

        job->amf.num_sw_filters = num_sw_filters;
        job->amf.num_hw_filters = num_hw_filters;
        job->amf.converter_needed = converter_needed;
        job->amf.converter_inserted = 0;
    }
    return 0;
}

#else // !HB_PROJECT_FEATURE_VCE

int hb_vce_h264_available()
{
    if (is_vcn_available != -1)
    {
        return is_vcn_available;
    }
    
    is_vcn_available = -2;
    hb_log("vcn: not compiled into this build.");
    
    return -1;
}

int hb_vce_h265_available()
{
    if (is_vcn_hevc_available != -1)
    {
        return is_vcn_hevc_available;
    }
    
    is_vcn_hevc_available = -2;
    
    return -1; 
}

int hb_vce_av1_available()
{
    if (is_vcn_av1_available != -1)
    {
        return is_vcn_av1_available;
    }

    is_vcn_av1_available = -2;

    return -1;
}

int hb_check_amfdec_available()
{
    #if HB_PROJECT_FEATURE_AMFDEC
        return 1;
    #else
        return 0;
    #endif
}

int hb_vce_are_filters_supported(hb_list_t *filters)
{
    return 0;
}
int hb_vce_dec_is_enabled(hb_job_t *job)
{
    return 0;
}
int hb_vce_sanitize_filter_list(hb_job_t *job)
{
    return 0;
}

#endif // HB_PROJECT_FEATURE_VCE
const char* hb_vce_decode_get_codec_name(enum AVCodecID codec_id)
{
    switch (codec_id)
    {
        case AV_CODEC_ID_H264:
            return "h264_amf";

        case AV_CODEC_ID_HEVC:
            return "hevc_amf";

        case AV_CODEC_ID_AV1:
            return "av1_amf";

        default:
            return NULL;
    }

}
int hb_vce_hw_filters_via_video_memory_are_enabled(hb_job_t *job)
{
    return job->hw_pix_fmt == AV_PIX_FMT_AMF_SURFACE;
}
hb_buffer_t *  hb_vce_copy_avframe_to_video_buffer(hb_job_t *job, AVFrame *frame, AVRational time_base)
{
    hb_buffer_t *out = hb_buffer_wrapper_init();
    AVFrame *frame_copy = NULL;

    if (out == NULL)
    {
        return NULL;
    }

    if(job->amf.num_sw_filters == 0)
    {
        // keep frame in video memory
        frame_copy = av_frame_clone(frame);
        if (frame_copy == NULL)
        {
            goto fail;
        }
    }
    else
    {
    // Alloc new frame
        frame_copy = av_frame_alloc();
        if (frame_copy == NULL)
        {
            goto fail;
        }
        
        AVHWFramesContext *hwfc = (AVHWFramesContext *)frame->hw_frames_ctx->data;
        frame_copy->format = hwfc->sw_format;

        av_hwframe_transfer_data(frame_copy, frame, 0);
        av_frame_copy_props(frame_copy, frame);

        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(frame_copy->format);
        for (int ii = 0; ii < desc->nb_components; ii++)
        {
            int pp = desc->comp[ii].plane;
            if (pp > out->f.max_plane)
            {
                out->f.max_plane = pp;
            }
        }

        for (int pp = 0; pp <= out->f.max_plane; pp++)
        {
            out->plane[pp].data          = frame_copy->data[pp];
            out->plane[pp].width         = hb_image_width(out->f.fmt, out->f.width, pp);
            out->plane[pp].height        = hb_image_height(out->f.fmt, out->f.height, pp);
            out->plane[pp].stride        = frame_copy->linesize[pp];
            out->plane[pp].size          = out->plane[pp].stride * out->plane[pp].height;

            out->size += out->plane[pp].size;
        }

    }

    out->storage = frame_copy;

    out->storage_type = AVFRAME;

    out->s.type = FRAME_BUF;
    out->f.width  = frame_copy->width;
    out->f.height = frame_copy->height;
    hb_avframe_set_video_buffer_flags(out, frame_copy, time_base);

    out->side_data = (void **)frame_copy->side_data;
    out->nb_side_data = frame_copy->nb_side_data;

    return out;

fail:
    hb_buffer_close(&out);
    av_frame_unref(frame_copy);
    return NULL;

}

int hb_amf_decode_is_codec_supported(int video_codec_param)
{
#if HB_PROJECT_FEATURE_AMFDEC
    switch (video_codec_param)
    {
        case AV_CODEC_ID_H264:
            return hb_vce_h264_available();
        case AV_CODEC_ID_HEVC:
            return hb_vce_h265_available();
        case AV_CODEC_ID_AV1:
            return hb_vce_av1_available();
        default:
            return 0;
    }
    return 0;
#else
    return 0;
#endif
}

int hb_vce_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    int vce_available = 0;
    vce_available = (hb_vce_h264_available() |
                    hb_vce_h265_available() |
                    hb_vce_av1_available());
    
    return vce_available;
}

int hb_vce_setup_job(hb_job_t *job)
{
    int result = 0;

    if (!job)
    {
        return result;
    }

    if (hb_check_amfdec_available())
    {
        job->hw_decode = HB_DECODE_SUPPORT_AMFDEC;
        result = 1;
    }

    return result;
}
