/* opencl.c

   Copyright (c) 2003-2013 HandBrake Team
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

int hb_opencl_library_open(hb_opencl_library_t *opencl)
{
    if (opencl == NULL)
    {
        goto fail;
    }

    opencl->library = HB_OCL_DLOPEN;
    if (opencl->library == NULL)
    {
        goto fail;
    }

#define HB_OCL_LOAD(func)                                                      \
{                                                                              \
    if ((opencl->func = (void*)HB_OCL_DLSYM(opencl->library, #func)) == NULL)  \
    {                                                                          \
        hb_log("hb_opencl_library_open: failed to load function '%s'", #func); \
        goto fail;                                                             \
    }                                                                          \
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
    HB_OCL_LOAD(clReleaseProgram);
    HB_OCL_LOAD(clSetKernelArg);
    HB_OCL_LOAD(clWaitForEvents);
    return 0;

fail:
    hb_opencl_library_close(opencl);
    return -1;
}

void hb_opencl_library_close(hb_opencl_library_t *opencl)
{
    if (opencl != NULL)
    {
        if (opencl->library != NULL)
        {
            HB_OCL_DLCLOSE(opencl->library);
        }
        opencl->library = NULL;

#define HB_OCL_UNLOAD(func) { opencl->func = NULL; }
        HB_OCL_UNLOAD(clBuildProgram);
        HB_OCL_UNLOAD(clCreateBuffer);
        HB_OCL_UNLOAD(clCreateCommandQueue);
        HB_OCL_UNLOAD(clCreateContextFromType);
        HB_OCL_UNLOAD(clCreateKernel);
        HB_OCL_UNLOAD(clCreateProgramWithBinary);
        HB_OCL_UNLOAD(clCreateProgramWithSource);
        HB_OCL_UNLOAD(clEnqueueCopyBuffer);
        HB_OCL_UNLOAD(clEnqueueMapBuffer);
        HB_OCL_UNLOAD(clEnqueueNDRangeKernel);
        HB_OCL_UNLOAD(clEnqueueReadBuffer);
        HB_OCL_UNLOAD(clEnqueueUnmapMemObject);
        HB_OCL_UNLOAD(clEnqueueWriteBuffer);
        HB_OCL_UNLOAD(clFlush);
        HB_OCL_UNLOAD(clGetCommandQueueInfo);
        HB_OCL_UNLOAD(clGetContextInfo);
        HB_OCL_UNLOAD(clGetDeviceIDs);
        HB_OCL_UNLOAD(clGetDeviceInfo);
        HB_OCL_UNLOAD(clGetPlatformIDs);
        HB_OCL_UNLOAD(clGetPlatformInfo);
        HB_OCL_UNLOAD(clGetProgramBuildInfo);
        HB_OCL_UNLOAD(clGetProgramInfo);
        HB_OCL_UNLOAD(clReleaseCommandQueue);
        HB_OCL_UNLOAD(clReleaseContext);
        HB_OCL_UNLOAD(clReleaseEvent);
        HB_OCL_UNLOAD(clReleaseKernel);
        HB_OCL_UNLOAD(clReleaseProgram);
        HB_OCL_UNLOAD(clSetKernelArg);
        HB_OCL_UNLOAD(clWaitForEvents);
    }
}

static int hb_opencl_device_is_supported(cl_device_type type,
                                         const char *vendor,
                                         const char *version)
{
    int major, minor;

    // we only support OpenCL on GPUs
    // disable on NVIDIA to to a bug (FIXME)
    if (!(type & CL_DEVICE_TYPE_GPU) ||
        !(strncmp(vendor, "NVIDIA", 6 /* strlen("NVIDIA") */)))
    {
        return 0;
    }

    // check OpenCL version; format:
    // OpenCL<space><major_version.minor_version><space><vendor-specific information>
    if (sscanf(version, "OpenCL %d.%d", &major, &minor) != 2)
    {
        return 0;
    }

    return (major > HB_OCL_MINVERSION_MAJOR) || (major == HB_OCL_MINVERSION_MAJOR &&
                                                 minor >= HB_OCL_MINVERSION_MINOR);
}

int hb_opencl_available()
{
    static int opencl_available = -1;
    if (opencl_available >= 0)
    {
        return opencl_available;
    }
    opencl_available = 0;

    cl_device_type type;
    char vendor[100], version[100];
    cl_device_id *device_ids = NULL;
    cl_platform_id *platform_ids = NULL;
    hb_opencl_library_t lib, *opencl = &lib;
    cl_uint i, j, num_platforms, num_devices;

    /*
     * Check whether we can load the OpenCL library, then check devices and make
     * sure we support running OpenCL code on at least one of them.
     */
    if (hb_opencl_library_open(opencl) == 0)
    {
        if (opencl->clGetPlatformIDs(0, NULL, &num_platforms) != CL_SUCCESS || !num_platforms)
        {
            goto end;
        }
        if ((platform_ids = malloc(sizeof(cl_platform_id) * num_platforms)) == NULL)
        {
            goto end;
        }
        if (opencl->clGetPlatformIDs(num_platforms, platform_ids, NULL) != CL_SUCCESS)
        {
            goto end;
        }
        for (i = 0; i < num_platforms; i++)
        {
            if (opencl->clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices) != CL_SUCCESS || !num_devices)
            {
                goto end;
            }
            if ((device_ids = malloc(sizeof(cl_device_id) * num_devices)) == NULL)
            {
                goto end;
            }
            if (opencl->clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, num_devices, device_ids, NULL) != CL_SUCCESS)
            {
                goto end;
            }
            for (j = 0; j < num_devices; j++)
            {
                if (device_ids[j] != NULL)
                {
                    opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_VENDOR,  sizeof(vendor),
                                            vendor,  NULL);
                    opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_VERSION, sizeof(version),
                                            version, NULL);
                    opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_TYPE,    sizeof(type),
                                            &type,  NULL);

                    if (hb_opencl_device_is_supported(type,
                                                      (const char*)vendor,
                                                      (const char*)version))
                    {
                        opencl_available = 1;
                        goto end;
                    }
                }
            }
            free(device_ids);
            device_ids = NULL;
        }
    }

end:
    free(device_ids);
    free(platform_ids);
    hb_opencl_library_close(opencl);
    return opencl_available;
}

void hb_opencl_info_print()
{
    /*
     * Note: this function should not log any warnings or errors.
     * Its only purpose is to list OpenCL-capable devices, so let's initialize
     * only what we absolutely need here, rather than calling library_open().
     */
    hb_opencl_library_t lib, *opencl = &lib;
    if ((opencl->library          = (void*)HB_OCL_DLOPEN)                                     == NULL ||
        (opencl->clGetDeviceIDs   = (void*)HB_OCL_DLSYM(opencl->library, "clGetDeviceIDs"  )) == NULL ||
        (opencl->clGetDeviceInfo  = (void*)HB_OCL_DLSYM(opencl->library, "clGetDeviceInfo" )) == NULL ||
        (opencl->clGetPlatformIDs = (void*)HB_OCL_DLSYM(opencl->library, "clGetPlatformIDs")) == NULL)
    {
        // zero or insufficient OpenCL support
        hb_log("OpenCL: library not available");
        goto end;
    }

    cl_device_type type;
    cl_device_id *device_ids;
    cl_platform_id *platform_ids;
    cl_uint i, j, k, num_platforms, num_devices;
    char vendor[100], name[1024], version[100], driver[1024];

    if (opencl->clGetPlatformIDs(0, NULL, &num_platforms) != CL_SUCCESS || !num_platforms)
    {
        goto end;
    }
    if ((platform_ids = malloc(sizeof(cl_platform_id) * num_platforms)) == NULL)
    {
        goto end;
    }
    if (opencl->clGetPlatformIDs(num_platforms, platform_ids, NULL) != CL_SUCCESS)
    {
        goto end;
    }
    for (i = 0, k = 1; i < num_platforms; i++)
    {
        if (opencl->clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices) != CL_SUCCESS || !num_devices)
        {
            goto end;
        }
        if ((device_ids = malloc(sizeof(cl_device_id) * num_devices)) == NULL)
        {
            goto end;
        }
        if (opencl->clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, num_devices, device_ids, NULL) != CL_SUCCESS)
        {
            goto end;
        }
        for (j = 0; j < num_devices; j++)
        {
            if (device_ids[j] != NULL)
            {
                opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_VENDOR,  sizeof(vendor),
                                        vendor,  NULL);
                opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_NAME,    sizeof(name),
                                        name,    NULL);
                opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_VERSION, sizeof(version),
                                        version, NULL);
                opencl->clGetDeviceInfo(device_ids[j], CL_DRIVER_VERSION, sizeof(driver),
                                        driver, NULL);
                opencl->clGetDeviceInfo(device_ids[j], CL_DEVICE_TYPE,    sizeof(type),
                                        &type,  NULL);

                // don't list unsupported devices
                if (type & CL_DEVICE_TYPE_CPU)
                {
                    continue;
                }
                hb_log("OpenCL device #%d: %s %s", k++, vendor, name);
                hb_log(" - OpenCL version: %s", version + 7 /* strlen("OpenCL ") */);
                hb_log(" - driver version: %s", driver);
                hb_log(" - device type: %s%s",
                       type & CL_DEVICE_TYPE_CPU         ? "CPU"         :
                       type & CL_DEVICE_TYPE_GPU         ? "GPU"         :
                       type & CL_DEVICE_TYPE_CUSTOM      ? "Custom"      :
                       type & CL_DEVICE_TYPE_ACCELERATOR ? "Accelerator" : "Unknown",
                       type & CL_DEVICE_TYPE_DEFAULT     ? " (default)"  : "");
                hb_log(" - supported: %s",
                       hb_opencl_device_is_supported(type,
                                                     (const char*)vendor,
                                                     (const char*)version) ? "yes" : "no");
            }
        }
        free(device_ids);
    }

end:
    hb_opencl_library_close(opencl);
}
