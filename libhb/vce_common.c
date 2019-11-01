/* vce_common.c
 *
 * Copyright (c) 2003-2019 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_VCE
#include "AMF/core/Factory.h"
#include "AMF/components/VideoEncoderVCE.h"
#include "AMF/components/VideoEncoderHEVC.h"
#include "handbrake/handbrake.h"

AMF_RESULT check_component_available(const wchar_t *componentID)
{
    amf_handle          libHandle = NULL;
    AMFInit_Fn          initFun;
    AMFFactory         *factory = NULL;
    AMFContext         *context = NULL;
    AMFContext1        *context1 = NULL;
    AMFComponent       *encoder = NULL;
    AMFCaps            *encoderCaps = NULL;
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

    result = factory->pVtbl->CreateComponent(factory, context, componentID, &encoder);

    if(result != AMF_OK)
    {
        goto clean;
    }

    result = encoder->pVtbl->GetCaps(encoder, &encoderCaps);

clean:
    if (encoderCaps)
    {
        encoderCaps->pVtbl->Clear(encoderCaps);
        encoderCaps->pVtbl->Release(encoderCaps);
        encoderCaps = NULL;
    }
    if (encoder)
    {
        encoder->pVtbl->Terminate(encoder);
        encoder->pVtbl->Release(encoder);
        encoder = NULL;
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

    return (check_component_available(AMFVideoEncoderVCE_AVC) == AMF_OK) ? 1 : 0;
}

int hb_vce_h265_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    return (check_component_available(AMFVideoEncoder_HEVC) == AMF_OK) ? 1 : 0;
}

#else // !HB_PROJECT_FEATURE_VCE

int hb_vce_h264_available()
{
    return 0;
}

int hb_vce_h265_available()
{
    return 0;
}

#endif // HB_PROJECT_FEATURE_VCE
