/* vce_common.c
 *
 * Copyright (c) 2003-2018 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifdef USE_VCE
#include "AMF/core/Factory.h"
#include "AMF/components/VideoDecoderUVD.h"
#include "hb.h"

void amf_components_clear(amf_handle          library,
                         AMFFactory*         factory,
                         AMFContext*         context,
                         AMFComponent*       encoder,
                         AMFCaps*            encoderCaps)
{
    if (factory)
    {
        factory = NULL;
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
    if (encoderCaps)
    {
        encoderCaps->pVtbl->Clear(encoderCaps);
        encoderCaps->pVtbl->Release(encoderCaps);
        encoderCaps = NULL;
    }
    if(library)
    {
        hb_dlclose(library);
    }
}

AMF_RESULT check_component_available(const wchar_t *componentID)
{
    amf_handle          library = NULL;
    AMFInit_Fn          init_fun;
    AMF_RESULT          res;
    AMFFactory*         factory = NULL;
    AMFContext*         context = NULL;
    AMFComponent*       encoder = NULL;
    AMFCaps*            encoderCaps = NULL;

    library = hb_dlopen(AMF_DLL_NAMEA);
    if(!library)
    {
        return AMF_FAIL;
    }

    init_fun = (AMFInit_Fn)(hb_dlsym(library, AMF_INIT_FUNCTION_NAME));
    if(!init_fun)
    {
        amf_components_clear(library, factory, context, encoder, encoderCaps);
        return AMF_FAIL;
    }

    res = init_fun(AMF_FULL_VERSION, &factory);
    if(res != AMF_OK)
    {
        amf_components_clear(library, factory, context, encoder, encoderCaps);
        return res;
    }

    res = factory->pVtbl->CreateContext(factory, &context);
    if(res != AMF_OK)
    {
        amf_components_clear(library, factory, context, encoder, encoderCaps);
        return res;
    }

    res = context->pVtbl->InitDX11(context, NULL, AMF_DX11_1);
    if (res != AMF_OK) {
        res = context->pVtbl->InitDX9(context, NULL);
        if (res != AMF_OK) {
            amf_components_clear(library, factory, context, encoder, encoderCaps);
            return res;
        }
    }

    factory->pVtbl->CreateComponent(factory, context, componentID, &encoder);
    if(!encoder)
    {
        amf_components_clear(library, factory, context, encoder, encoderCaps);
        return AMF_FAIL;
    }

    res = encoder->pVtbl->GetCaps(encoder, &encoderCaps);
    if (res != AMF_OK)
    {
        return res;
    }
    amf_components_clear(library, factory, context, encoder, encoderCaps);
    return AMF_OK;
}

int hb_vce_h264_available()
{
    return (check_component_available(AMFVideoDecoderUVD_H264_AVC) == AMF_OK) ? 1 : 0;
}

int hb_vce_h265_available()
{
    return (check_component_available(AMFVideoDecoderHW_H265_HEVC) == AMF_OK) ? 1 : 0;
}

#else

int hb_vce_h264_available()
{
    return 0;
}

int hb_vce_h265_available()
{
    return 0;
}

#endif // USE_QSV
