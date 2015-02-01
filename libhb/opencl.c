/* opencl.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifdef _WIN32
#include <windows.h>
#define HB_OCL_DLOPEN  LoadLibraryW(L"OpenCL")
#define HB_OCL_DLSYM   GetProcAddress
#define HB_OCL_DLCLOSE FreeLibrary
#else
#include <dlfcn.h>
#ifdef __APPLE__
#define HB_OCL_DLOPEN  dlopen("/System/Library/Frameworks/OpenCL.framework/OpenCL", RTLD_NOW)
#else
#define HB_OCL_DLOPEN  dlopen("libOpenCL.so", RTLD_NOW)
#endif
#define HB_OCL_DLSYM   dlsym
#define HB_OCL_DLCLOSE dlclose
#endif

#include "common.h"
#include "opencl.h"

hb_opencl_library_t *hb_ocl = NULL;

int hb_ocl_init()
{
    if (hb_ocl == NULL)
    {
        if ((hb_ocl = hb_opencl_library_init()) == NULL)
        {
            return -1;
        }
    }
    return 0;
}

void hb_ocl_close()
{
    hb_opencl_library_close(&hb_ocl);
}

hb_opencl_library_t* hb_opencl_library_init()
{
    hb_opencl_library_t *opencl;
    if ((opencl = calloc(1, sizeof(hb_opencl_library_t))) == NULL)
    {
        hb_error("hb_opencl_library_init: memory allocation failure");
        goto fail;
    }

    opencl->library = HB_OCL_DLOPEN;
    if (opencl->library == NULL)
    {
        goto fail;
    }

#define HB_OCL_LOAD(func)                                                       \
{                                                                               \
    if ((opencl->func = (void*)HB_OCL_DLSYM(opencl->library, #func)) == NULL)   \
    {                                                                           \
        hb_log("hb_opencl_library_init: failed to load function '%s'", #func);  \
        goto fail;                                                              \
    }                                                                           \
}
    HB_OCL_LOAD(clBuildProgram);
    HB_OCL_LOAD(clCreateBuffer);
    HB_OCL_LOAD(clCreateCommandQueue);
    HB_OCL_LOAD(clCreateContextFromType);
    HB_OCL_LOAD(clCreateKernel);
    HB_OCL_LOAD(clCreateProgramWithBinary);
    HB_OCL_LOAD(clCreateProgramWithSource);
    HB_OCL_LOAD(clEnqueueCopyBuffer);
    HB_OCL_LOAD(clEnqueueMapBuffer);
    HB_OCL_LOAD(clEnqueueNDRangeKernel);
    HB_OCL_LOAD(clEnqueueReadBuffer);
    HB_OCL_LOAD(clEnqueueUnmapMemObject);
    HB_OCL_LOAD(clEnqueueWriteBuffer);
    HB_OCL_LOAD(clFlush);
    HB_OCL_LOAD(clGetCommandQueueInfo);
    HB_OCL_LOAD(clGetContextInfo);
    HB_OCL_LOAD(clGetDeviceIDs);
    HB_OCL_LOAD(clGetDeviceInfo);
    HB_OCL_LOAD(clGetPlatformIDs);
    HB_OCL_LOAD(clGetPlatformInfo);
    HB_OCL_LOAD(clGetProgramBuildInfo);
    HB_OCL_LOAD(clGetProgramInfo);
    HB_OCL_LOAD(clReleaseCommandQueue);
    HB_OCL_LOAD(clReleaseContext);
    HB_OCL_LOAD(clReleaseEvent);
    HB_OCL_LOAD(clReleaseKernel);
    HB_OCL_LOAD(clReleaseMemObject);
    HB_OCL_LOAD(clReleaseProgram);
    HB_OCL_LOAD(clSetKernelArg);
    HB_OCL_LOAD(clWaitForEvents);

    //success
    return opencl;

fail:
    hb_opencl_library_close(&opencl);
    return NULL;
}

void hb_opencl_library_close(hb_opencl_library_t **_opencl)
{
    if (_opencl == NULL)
    {
        return;
    }
    hb_opencl_library_t *opencl = *_opencl;

    if (opencl != NULL)
    {
        if (opencl->library != NULL)
        {
            HB_OCL_DLCLOSE(opencl->library);
        }
        free(opencl);
    }
    *_opencl = NULL;
}

static int hb_opencl_device_is_supported(hb_opencl_device_t* device)
{
    // we only support OpenCL on GPUs for now
    // Ivy Bridge supports OpenCL on GPU, but it's too slow to be usable
    // FIXME: disable on NVIDIA to to a bug
    if ((device != NULL) &&
        (device->type & CL_DEVICE_TYPE_GPU) &&
        (device->ocl_vendor != HB_OCL_VENDOR_NVIDIA) &&
        (device->ocl_vendor != HB_OCL_VENDOR_INTEL ||
         hb_get_cpu_platform() != HB_CPU_PLATFORM_INTEL_IVB))
    {
        int major, minor;
        // check OpenCL version:
        // OpenCL<space><major_version.minor_version><space><vendor-specific information>
        if (sscanf(device->version, "OpenCL %d.%d", &major, &minor) != 2)
        {
            return 0;
        }
        return (major > HB_OCL_MINVERSION_MAJOR) || (major == HB_OCL_MINVERSION_MAJOR &&
                                                     minor >= HB_OCL_MINVERSION_MINOR);
    }
    return 0;
}

static hb_opencl_device_t* hb_opencl_device_get(hb_opencl_library_t *opencl,
                                                cl_device_id device_id)
{
    if (opencl == NULL || opencl->clGetDeviceInfo == NULL)
    {
        hb_error("hb_opencl_device_get: OpenCL support not available");
        return NULL;
    }
    else if (device_id == NULL)
    {
        hb_error("hb_opencl_device_get: invalid device ID");
        return NULL;
    }

    hb_opencl_device_t *device = calloc(1, sizeof(hb_opencl_device_t));
    if (device == NULL)
    {
        hb_error("hb_opencl_device_get: memory allocation failure");
        return NULL;
    }

    cl_int status = CL_SUCCESS;
    device->id    = device_id;

    status |= opencl->clGetDeviceInfo(device->id, CL_DEVICE_VENDOR,   sizeof(device->vendor),
                                      device->vendor,    NULL);
    status |= opencl->clGetDeviceInfo(device->id, CL_DEVICE_NAME,     sizeof(device->name),
                                       device->name,      NULL);
    status |= opencl->clGetDeviceInfo(device->id, CL_DEVICE_VERSION,  sizeof(device->version),
                                      device->version,   NULL);
    status |= opencl->clGetDeviceInfo(device->id, CL_DEVICE_TYPE,     sizeof(device->type),
                                     &device->type,     NULL);
    status |= opencl->clGetDeviceInfo(device->id, CL_DEVICE_PLATFORM, sizeof(device->platform),
                                     &device->platform, NULL);
    status |= opencl->clGetDeviceInfo(device->id, CL_DRIVER_VERSION,  sizeof(device->driver),
                                      device->driver,    NULL);
    if (status != CL_SUCCESS)
    {
        free(device);
        return NULL;
    }

    if (!strcmp(device->vendor, "Advanced Micro Devices, Inc.") ||
        !strcmp(device->vendor, "AMD"))
    {
        device->ocl_vendor = HB_OCL_VENDOR_AMD;
    }
    else if (!strncmp(device->vendor, "NVIDIA", 6 /* strlen("NVIDIA") */))
    {
        device->ocl_vendor = HB_OCL_VENDOR_NVIDIA;
    }
    else if (!strncmp(device->vendor, "Intel", 5 /* strlen("Intel") */))
    {
        device->ocl_vendor = HB_OCL_VENDOR_INTEL;
    }
    else
    {
        device->ocl_vendor = HB_OCL_VENDOR_OTHER;
    }

    return device;
}

static void hb_opencl_devices_list_close(hb_list_t **_list)
{
    if (_list != NULL)
    {
        hb_list_t *list = *_list;
        hb_opencl_device_t *device;
        while (list != NULL && hb_list_count(list) > 0)
        {
            if ((device = hb_list_item(list, 0)) != NULL)
            {
                hb_list_rem(list, device);
                free(device);
            }
        }
    }
    hb_list_close(_list);
}

static hb_list_t* hb_opencl_devices_list_get(hb_opencl_library_t *opencl,
                                             cl_device_type device_type)
{
    if (opencl                   == NULL ||
        opencl->library          == NULL ||
        opencl->clGetDeviceIDs   == NULL ||
        opencl->clGetDeviceInfo  == NULL ||
        opencl->clGetPlatformIDs == NULL)
    {
        hb_error("hb_opencl_devices_list_get: OpenCL support not available");
        return NULL;
    }

    hb_list_t *list = hb_list_init();
    if (list == NULL)
    {
        hb_error("hb_opencl_devices_list_get: memory allocation failure");
        return NULL;
    }

    cl_device_id *device_ids = NULL;
    hb_opencl_device_t *device = NULL;
    cl_platform_id *platform_ids = NULL;
    cl_uint i, j, num_platforms, num_devices;

    if (opencl->clGetPlatformIDs(0, NULL, &num_platforms) != CL_SUCCESS || !num_platforms)
    {
        goto fail;
    }
    if ((platform_ids = malloc(sizeof(cl_platform_id) * num_platforms)) == NULL)
    {
        hb_error("hb_opencl_devices_list_get: memory allocation failure");
        goto fail;
    }
    if (opencl->clGetPlatformIDs(num_platforms, platform_ids, NULL) != CL_SUCCESS)
    {
        goto fail;
    }
    for (i = 0; i < num_platforms; i++)
    {
        if (opencl->clGetDeviceIDs(platform_ids[i], device_type, 0, NULL, &num_devices) != CL_SUCCESS || !num_devices)
        {
            // non-fatal
            continue;
        }
        if ((device_ids = malloc(sizeof(cl_device_id) * num_devices)) == NULL)
        {
            hb_error("hb_opencl_devices_list_get: memory allocation failure");
            goto fail;
        }
        if (opencl->clGetDeviceIDs(platform_ids[i], device_type, num_devices, device_ids, NULL) != CL_SUCCESS)
        {
            // non-fatal
            continue;
        }
        for (j = 0; j < num_devices; j++)
        {
            if ((device = hb_opencl_device_get(opencl, device_ids[j])) != NULL)
            {
                hb_list_add(list, device);
            }
        }
    }

    goto end;

fail:
    hb_opencl_devices_list_close(&list);

end:
    free(platform_ids);
    free(device_ids);
    return list;
}

int hb_opencl_available()
{
    static int opencl_available = -1;
    if (opencl_available >= 0)
    {
        return opencl_available;
    }
    opencl_available = 0;

    /*
     * Check whether we can load the OpenCL library, then check devices and make
     * sure we support running OpenCL code on at least one of them.
     */
    hb_opencl_library_t *opencl;
    if ((opencl = hb_opencl_library_init()) != NULL)
    {
        int i;
        hb_list_t *device_list;
        hb_opencl_device_t *device;
        if ((device_list = hb_opencl_devices_list_get(opencl, CL_DEVICE_TYPE_ALL)) != NULL)
        {
            for (i = 0; i < hb_list_count(device_list); i++)
            {
                if ((device = hb_list_item(device_list, i)) != NULL &&
                    (hb_opencl_device_is_supported(device)))
                {
                    opencl_available = 1;
                    break;
                }
            }
            hb_opencl_devices_list_close(&device_list);
        }
        hb_opencl_library_close(&opencl);
    }
    return opencl_available;
}

void hb_opencl_info_print()
{
    /*
     * Note: this function should not log any warnings or errors.
     * Its only purpose is to list OpenCL-capable devices, so let's initialize
     * only what we absolutely need here, rather than calling library_open().
     */
    hb_opencl_library_t ocl, *opencl = &ocl;
    if ((opencl->library          = (void*)HB_OCL_DLOPEN)                                     == NULL ||
        (opencl->clGetDeviceIDs   = (void*)HB_OCL_DLSYM(opencl->library, "clGetDeviceIDs"  )) == NULL ||
        (opencl->clGetDeviceInfo  = (void*)HB_OCL_DLSYM(opencl->library, "clGetDeviceInfo" )) == NULL ||
        (opencl->clGetPlatformIDs = (void*)HB_OCL_DLSYM(opencl->library, "clGetPlatformIDs")) == NULL)
    {
        // zero or insufficient OpenCL support
        hb_log("OpenCL: library not available");
        goto end;
    }

    int i, idx;
    hb_list_t *device_list;
    hb_opencl_device_t *device;
    if ((device_list = hb_opencl_devices_list_get(opencl, CL_DEVICE_TYPE_ALL)) != NULL)
    {
        for (i = 0, idx = 1; i < hb_list_count(device_list); i++)
        {
            if ((device = hb_list_item(device_list, i)) != NULL)
            {
                // don't list CPU devices (always unsupported)
                if (!(device->type & CL_DEVICE_TYPE_CPU))
                {
                    hb_log("OpenCL device #%d: %s %s", idx++, device->vendor, device->name);
                    hb_log(" - OpenCL version: %s", device->version + 7 /* strlen("OpenCL ") */);
                    hb_log(" - driver version: %s", device->driver);
                    hb_log(" - device type:    %s%s",
                           device->type & CL_DEVICE_TYPE_CPU         ? "CPU"         :
                           device->type & CL_DEVICE_TYPE_GPU         ? "GPU"         :
                           device->type & CL_DEVICE_TYPE_CUSTOM      ? "Custom"      :
                           device->type & CL_DEVICE_TYPE_ACCELERATOR ? "Accelerator" : "Unknown",
                           device->type & CL_DEVICE_TYPE_DEFAULT     ? " (default)"  : "");
                    hb_log(" - supported:      %s",
                           hb_opencl_device_is_supported(device) ? "YES" : "no");
                }
            }
        }
        hb_opencl_devices_list_close(&device_list);
    }

end:
    /*
     * Close only the initialized part
     */
    if (opencl->library != NULL)
    {
        HB_OCL_DLCLOSE(opencl->library);
    }
}
