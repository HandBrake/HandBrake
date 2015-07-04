/* openclwrapper.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extras/cl.h"
#include "opencl.h"
#include "openclwrapper.h"
#include "openclkernels.h"

//#define USE_EXTERNAL_KERNEL
#ifdef SYS_MINGW
#include <windows.h>
#endif

#if defined(_MSC_VER)
#define strcasecmp strcmpi
#endif

#define MAX_KERNEL_STRING_LEN 64
#define MAX_CLFILE_NUM 50
#define MAX_CLKERNEL_NUM 200
#define MAX_CLFILE_PATH 255
#define MAX_KERNEL_NUM  50
#define MAX_KERNEL_NAME_LEN 64

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE NULL
#endif

//#define THREAD_PRIORITY_TIME_CRITICAL 15

enum VENDOR
{
    AMD = 0,
    Intel,
    NVIDIA,
    others
};
typedef struct _GPUEnv
{
    //share vb in all modules in hb library
    cl_platform_id platform;
    cl_device_type dType;
    cl_context context;
    cl_device_id * devices;
    cl_device_id dev;
    cl_command_queue command_queue;
    cl_kernel kernels[MAX_CLFILE_NUM];
    cl_program programs[MAX_CLFILE_NUM]; //one program object maps one kernel source file
    char kernelSrcFile[MAX_CLFILE_NUM][256]; //the max len of kernel file name is 256
    int file_count; // only one kernel file

    char kernel_names[MAX_CLKERNEL_NUM][MAX_KERNEL_STRING_LEN+1];
    cl_kernel_function kernel_functions[MAX_CLKERNEL_NUM];
    int kernel_count;
    int isUserCreated; // 1: created , 0:no create and needed to create by opencl wrapper
    enum VENDOR vendor;
}GPUEnv;

typedef struct
{
    char kernelName[MAX_KERNEL_NAME_LEN+1];
    char * kernelStr;
}hb_kernel_node;

static GPUEnv gpu_env;
static int isInited = 0;
static int useBuffers = 0;
static hb_kernel_node gKernels[MAX_KERNEL_NUM];

#define HB_OCL_ADD_KERNEL_CFG(idx, s, p)    \
{                                           \
    strcpy(gKernels[idx].kernelName, s);    \
    gKernels[idx].kernelStr = p;            \
    strcpy(gpu_env.kernel_names[idx], s);   \
    gpu_env.kernel_count++;                 \
}

/**
 * hb_regist_opencl_kernel
 */
int hb_regist_opencl_kernel()
{
    //if( !gpu_env.isUserCreated )
    //    memset( &gpu_env, 0, sizeof(gpu_env) );
    //Comment for posterity: When in doubt just zero out a structure full of pointers to allocated resources.

    gpu_env.file_count = 0; //argc;
    gpu_env.kernel_count = 0UL;

    HB_OCL_ADD_KERNEL_CFG(0, "frame_scale",  NULL);
    HB_OCL_ADD_KERNEL_CFG(1, "yadif_filter", NULL);

    return 0;
}

/**
 * hb_regist_opencl_kernel
 * @param filename -
 * @param source -
 * @param gpu_info -
 * @param int idx -
 */
int hb_convert_to_string( const char *filename, char **source, GPUEnv *gpu_info, int idx )
{
    int file_size;
    size_t result;
    FILE * file = NULL;
    file_size = 0;
    result = 0;
    file = fopen( filename, "rb+" );

    if( file!=NULL )
    {
        fseek( file, 0, SEEK_END );

        file_size = ftell( file );
        rewind( file );
        *source = (char*)malloc( sizeof(char) * file_size + 1 );
        if( *source == (char*)NULL )
        {
            return(0);
        }
        result = fread( *source, 1, file_size, file );
        if( result != file_size )
        {
            free( *source );
            return(0);
        }
        (*source)[file_size] = '\0';
        fclose( file );

        return(1);
    }
    return(0);
}

/**
 * hb_binary_generated
 * @param context -
 * @param cl_file_name -
 * @param fhandle -
 */
int hb_binary_generated( cl_context context, const char * cl_file_name, FILE ** fhandle )
{
    int i = 0;
    cl_int status;
    cl_uint numDevices;
    cl_device_id *devices;
    char * str = NULL;
    FILE * fd = NULL;

    if (hb_ocl == NULL)
    {
        hb_error("hb_binary_generated: OpenCL support not available");
        return 0;
    }

    status = hb_ocl->clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES,
                                      sizeof(numDevices), &numDevices, NULL);
    if( status != CL_SUCCESS )
    {
        hb_log( "OpenCL: Get context info failed" );
        return 0;
    }

    devices = (cl_device_id*)malloc( sizeof(cl_device_id) * numDevices );
    if( devices == NULL )
    {
        hb_log( "OpenCL: No device found" );
        return 0;
    }

    /* grab the handles to all of the devices in the context. */
    status = hb_ocl->clGetContextInfo(context, CL_CONTEXT_DEVICES,
                                      sizeof(cl_device_id) * numDevices,
                                      devices, NULL);

    status = 0;
    /* dump out each binary into its own separate file. */
    for (i = 0; i < numDevices; i++)
    {
        char fileName[256] = { 0 };
        char  cl_name[128] = { 0 };
        if (devices[i])
        {
            char deviceName[1024];
            status = hb_ocl->clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                     sizeof(deviceName), deviceName, NULL);

            str = (char*)strstr(cl_file_name, ".cl");
            memcpy(cl_name, cl_file_name, str - cl_file_name);
            cl_name[str - cl_file_name] = '\0';
            sprintf(fileName, "./%s - %s.bin", cl_name, deviceName);
            fd = fopen(fileName, "rb");
            status = fd != NULL;
        }
    }

    if( devices != NULL )
    {
        free( devices );
        devices = NULL;
    }

    if( fd != NULL )
        *fhandle = fd;

    return status;
}

/**
 * hb_write_binary_to_file
 * @param fileName -
 * @param birary -
 * @param numBytes -
 */
int hb_write_binary_to_file( const char* fileName, const char* birary, size_t numBytes )
{
    FILE *output = NULL;
    output = fopen( fileName, "wb" );
    if( output == NULL )
        return 0;

    fwrite( birary, sizeof(char), numBytes, output );
    fclose( output );

    return 1;
}

/**
 * hb_generat_bin_from_kernel_source
 * @param program -
 * @param cl_file_name -
 */
int hb_generat_bin_from_kernel_source( cl_program program, const char * cl_file_name )
{
    int i = 0;
    cl_int status;
    cl_uint numDevices;
    size_t *binarySizes;
    cl_device_id *devices;
    char **binaries;
    char *str = NULL;

    if (hb_ocl == NULL)
    {
        hb_error("hb_generat_bin_from_kernel_source: OpenCL support not available");
        return 0;
    }

    status = hb_ocl->clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES,
                                      sizeof(numDevices), &numDevices, NULL);
    if( status != CL_SUCCESS )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: clGetProgramInfo for CL_PROGRAM_NUM_DEVICES failed");
        return 0;
    }

    devices = (cl_device_id*)malloc( sizeof(cl_device_id) * numDevices );
    if( devices == NULL )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: no device found");
        return 0;
    }

    /* grab the handles to all of the devices in the program. */
    status = hb_ocl->clGetProgramInfo(program, CL_PROGRAM_DEVICES,
                                      sizeof(cl_device_id) * numDevices,
                                      devices, NULL);
    if( status != CL_SUCCESS )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: clGetProgramInfo for CL_PROGRAM_DEVICES failed");
        return 0;
    }

    /* figure out the sizes of each of the binaries. */
    binarySizes = (size_t*)malloc( sizeof(size_t) * numDevices );

    status = hb_ocl->clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                      sizeof(size_t) * numDevices,
                                      binarySizes, NULL);
    if( status != CL_SUCCESS )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: clGetProgramInfo for CL_PROGRAM_BINARY_SIZES failed");
        return 0;
    }

    /* copy over all of the generated binaries. */
    binaries = (char**)malloc( sizeof(char *) * numDevices );
    if( binaries == NULL )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: malloc for binaries failed");
        return 0;
    }

    for( i = 0; i < numDevices; i++ )
    {
        if( binarySizes[i] != 0 )
        {
            binaries[i] = (char*)malloc( sizeof(char) * binarySizes[i] );
            if( binaries[i] == NULL )
            {
                hb_log("OpenCL: hb_generat_bin_from_kernel_source: malloc for binaries[%d] failed", i);
                return 0;
            }
        }
        else
        {
            binaries[i] = NULL;
        }
    }

    status = hb_ocl->clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                                      sizeof(char *) * numDevices,
                                      binaries, NULL);
    if( status != CL_SUCCESS )
    {
        hb_log("OpenCL: hb_generat_bin_from_kernel_source: clGetProgramInfo for CL_PROGRAM_BINARIES failed");
        return 0;
    }

    /* dump out each binary into its own separate file. */
    for (i = 0; i < numDevices; i++)
    {
        char fileName[256] = {0};
        char  cl_name[128] = {0};
        if (binarySizes[i])
        {
            char deviceName[1024];
            status = hb_ocl->clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                             sizeof(deviceName), deviceName,
                                             NULL);

            str = (char*)strstr( cl_file_name, (char*)".cl" );
            memcpy(cl_name, cl_file_name, str - cl_file_name);
            cl_name[str - cl_file_name] = '\0';
            sprintf(fileName, "./%s - %s.bin", cl_name, deviceName);

            if (!hb_write_binary_to_file(fileName, binaries[i], binarySizes[i]))
            {
                hb_log("OpenCL: hb_generat_bin_from_kernel_source: unable to write kernel, writing to temporary directory instead.");
                return 0;
            }
        }
    }

    // Release all resouces and memory
    for( i = 0; i < numDevices; i++ )
    {
        if( binaries[i] != NULL )
        {
            free( binaries[i] );
            binaries[i] = NULL;
        }
    }

    if( binaries != NULL )
    {
        free( binaries );
        binaries = NULL;
    }

    if( binarySizes != NULL )
    {
        free( binarySizes );
        binarySizes = NULL;
    }

    if( devices != NULL )
    {
        free( devices );
        devices = NULL;
    }
    return 1;
}


/**
 * hb_init_opencl_attr
 * @param env -
 */
int hb_init_opencl_attr( OpenCLEnv * env )
{
    if( gpu_env.isUserCreated )
        return 1;

    gpu_env.context = env->context;
    gpu_env.platform = env->platform;
    gpu_env.dev = env->devices;
    gpu_env.command_queue = env->command_queue;

    gpu_env.isUserCreated = 1;

    return 0;
}

/**
 * hb_create_kernel
 * @param kernelname -
 * @param env -
 */
int hb_create_kernel( char * kernelname, KernelEnv * env )
{
    int status;

    if (hb_ocl == NULL)
    {
        hb_error("hb_create_kernel: OpenCL support not available");
        return 0;
    }

    env->kernel        = hb_ocl->clCreateKernel(gpu_env.programs[0], kernelname, &status);
    env->context       = gpu_env.context;
    env->command_queue = gpu_env.command_queue;
    return status != CL_SUCCESS ? 1 : 0;
}

/**
 * hb_release_kernel
 * @param env -
 */
int hb_release_kernel( KernelEnv * env )
{
    if (hb_ocl == NULL)
    {
        hb_error("hb_release_kernel: OpenCL support not available");
        return 0;
    }

    int status = hb_ocl->clReleaseKernel(env->kernel);
    return status != CL_SUCCESS ? 1 : 0;
}

/**
 * hb_init_opencl_env
 * @param gpu_info -
 */

static int init_once = 0;
int hb_init_opencl_env( GPUEnv *gpu_info )
{
    size_t length;
    cl_int status;
    cl_uint numPlatforms, numDevices;
    cl_platform_id *platforms;
    cl_context_properties cps[3];
    char platformName[100];
    unsigned int i;
    void *handle = INVALID_HANDLE_VALUE;

    if (init_once != 0)
        return 0;
    else
        init_once = 1;

    if (hb_ocl == NULL)
    {
        hb_error("hb_init_opencl_env: OpenCL support not available");
        return 1;
    }

    /*
     * Have a look at the available platforms.
     */
    if( !gpu_info->isUserCreated )
    {
        status = hb_ocl->clGetPlatformIDs(0, NULL, &numPlatforms);
        if( status != CL_SUCCESS )
        {
            hb_log( "OpenCL: OpenCL device platform not found." );
            return(1);
        }

        gpu_info->platform = NULL;
        if( 0 < numPlatforms )
        {
            platforms = (cl_platform_id*)malloc(
                numPlatforms * sizeof(cl_platform_id));
            if( platforms == (cl_platform_id*)NULL )
            {
                return(1);
            }
            status = hb_ocl->clGetPlatformIDs(numPlatforms, platforms, NULL);

            if( status != CL_SUCCESS )
            {
                hb_log( "OpenCL: Specific opencl platform not found." );
                return(1);
            }

            for( i = 0; i < numPlatforms; i++ )
            {
                status = hb_ocl->clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
                                                   sizeof(platformName), platformName, NULL);

                if( status != CL_SUCCESS )
                {
                    continue;
                }
                gpu_info->platform = platforms[i];

                if (!strcmp(platformName, "Advanced Micro Devices, Inc.") ||
                    !strcmp(platformName, "AMD"))
                    gpu_info->vendor = AMD;
                else 
                    gpu_info->vendor = others;
                
                gpu_info->platform = platforms[i];

                status = hb_ocl->clGetDeviceIDs(gpu_info->platform /* platform */,
                                                CL_DEVICE_TYPE_GPU /* device_type */,
                                                0 /* num_entries */,
                                                NULL /* devices */, &numDevices);

                if( status != CL_SUCCESS )
                {
                    continue;
                }

                if( numDevices )
                        break;
                
            }
            free( platforms );
        }

        if( NULL == gpu_info->platform )
        {
            hb_log( "OpenCL: No OpenCL-compatible GPU found." );
            return(1);
        }

        if( status != CL_SUCCESS )
        {
            hb_log( "OpenCL: No OpenCL-compatible GPU found." );
            return(1);
        }

        /*
         * Use available platform.
         */
        cps[0] = CL_CONTEXT_PLATFORM;
        cps[1] = (cl_context_properties)gpu_info->platform;
        cps[2] = 0;
        /* Check for GPU. */
        gpu_info->dType = CL_DEVICE_TYPE_GPU;
        gpu_info->context = hb_ocl->clCreateContextFromType(cps, gpu_info->dType,
                                                            NULL, NULL, &status);

        if( (gpu_info->context == (cl_context)NULL) || (status != CL_SUCCESS) )
        {
            gpu_info->dType = CL_DEVICE_TYPE_CPU;
            gpu_info->context = hb_ocl->clCreateContextFromType(cps, gpu_info->dType,
                                                                NULL, NULL, &status);
        }

        if( (gpu_info->context == (cl_context)NULL) || (status != CL_SUCCESS) )
        {
            gpu_info->dType = CL_DEVICE_TYPE_DEFAULT;
            gpu_info->context = hb_ocl->clCreateContextFromType(cps, gpu_info->dType,
                                                                NULL, NULL, &status);
        }

        if( (gpu_info->context == (cl_context)NULL) || (status != CL_SUCCESS) )
        {
            hb_log( "OpenCL: Unable to create opencl context." );
            return(1);
        }

        /* Detect OpenCL devices. */
        /* First, get the size of device list data */
        status = hb_ocl->clGetContextInfo(gpu_info->context, CL_CONTEXT_DEVICES,
                                          0, NULL, &length);
        if((status != CL_SUCCESS) || (length == 0))
        {
            hb_log( "OpenCL: Unable to get the list of devices in context." );
            return(1);
        }

        /* Now allocate memory for device list based on the size we got earlier */
        gpu_info->devices = (cl_device_id*)malloc( length );
        if( gpu_info->devices == (cl_device_id*)NULL )
        {
            return(1);
        }

        /* Now, get the device list data */
        status = hb_ocl->clGetContextInfo(gpu_info->context, CL_CONTEXT_DEVICES,
                                          length, gpu_info->devices, NULL);
        if( status != CL_SUCCESS )
        {
            hb_log( "OpenCL: Unable to get the device list data in context." );
            return(1);
        }

        /* Create OpenCL command queue. */
        gpu_info->command_queue = hb_ocl->clCreateCommandQueue(gpu_info->context,
                                                               gpu_info->devices[0],
                                                               0, &status);
        if( status != CL_SUCCESS )
        {
            hb_log( "OpenCL: Unable to create opencl command queue." );
            return(1);
        }
    }

    if ((CL_SUCCESS == hb_ocl->clGetCommandQueueInfo(gpu_info->command_queue,
                                                     CL_QUEUE_THREAD_HANDLE_AMD,
                                                     sizeof(handle), &handle, NULL)) &&
        (INVALID_HANDLE_VALUE != handle))
    {
#ifdef SYS_MINGW 
        SetThreadPriority( handle, THREAD_PRIORITY_TIME_CRITICAL );
#endif
    }

    return 0;
}


/**
 * hb_release_opencl_env
 * @param gpu_info -
 */
int hb_release_opencl_env( GPUEnv *gpu_info )
{
    if( !isInited )
        return 1;
    int i;

    if (hb_ocl == NULL)
    {
        hb_error("hb_release_opencl_env: OpenCL support not available");
        return 0;
    }

    for( i = 0; i<gpu_env.file_count; i++ )
    {
        if( gpu_env.programs[i] )
        {
            hb_ocl->clReleaseProgram(gpu_env.programs[i]);
            gpu_env.programs[i] = NULL;
        }
    }

    if( gpu_env.command_queue )
    {
        hb_ocl->clReleaseCommandQueue(gpu_env.command_queue);
        gpu_env.command_queue = NULL;
    }

    if( gpu_env.context )
    {
        hb_ocl->clReleaseContext(gpu_env.context);
        gpu_env.context = NULL;
    }

    isInited = 0;
    useBuffers = 0;
    gpu_info->isUserCreated = 0;

    return 1;
}


/**
 * hb_register_kernel_wrapper
 * @param kernel_name -
 * @param function -
 */
int hb_register_kernel_wrapper( const char *kernel_name, cl_kernel_function function )
{
    int i;
    for( i = 0; i < gpu_env.kernel_count; i++ )
    {
        if( strcasecmp( kernel_name, gpu_env.kernel_names[i] ) == 0 )
        {
            gpu_env.kernel_functions[i] = function;
            return(1);
        }
    }
    return(0);
}

/**
 * hb_cached_of_kerner_prg
 * @param gpu_env -
 * @param cl_file_name -
 */
int hb_cached_of_kerner_prg( const GPUEnv *gpu_env, const char * cl_file_name )
{
    int i;
    for( i = 0; i < gpu_env->file_count; i++ )
    {
        if( strcasecmp( gpu_env->kernelSrcFile[i], cl_file_name ) == 0 )
        {
            if( gpu_env->programs[i] != NULL )
                return(1);
        }
    }

    return(0);
}

/**
 * hb_compile_kernel_file
 * @param filename -
 * @param gpu_info -
 * @param indx -
 * @param build_option -
 */
int hb_compile_kernel_file( const char *filename, GPUEnv *gpu_info,
                            int indx, const char *build_option )
{
    cl_int status;
    size_t length;
    char *source_str;
    const char *source;
    size_t source_size[1];
    char *buildLog = NULL;
    int b_error, binary_status, binaryExisted;
    char * binary;
    cl_uint numDevices;
    cl_device_id *devices;
    FILE * fd;
    FILE * fd1;
    int idx;

    if( hb_cached_of_kerner_prg( gpu_info, filename ) == 1 )
        return (1);

    idx = gpu_info->file_count;

#ifdef USE_EXTERNAL_KERNEL
    status = hb_convert_to_string( filename, &source_str, gpu_info, idx );
    if( status == 0 )
        return(0);
#else
    int kernel_src_size = strlen(kernel_src_scale) + strlen(kernel_src_yadif_filter);

//    char *scale_src;
//    status = hb_convert_to_string("./scale_kernels.cl", &scale_src, gpu_info, idx);
//    if (status != 0)
//        kernel_src_size += strlen(scale_src);

    source_str = (char*)malloc( kernel_src_size + 2 );
    strcpy( source_str, kernel_src_scale );
//    strcat( source_str, scale_src );    //
    strcat( source_str, kernel_src_yadif_filter );
#endif

    source = source_str;
    source_size[0] = strlen( source );

    if (hb_ocl == NULL)
    {
        hb_error("hb_compile_kernel_file: OpenCL support not available");
        return 0;
    }

    if ((binaryExisted = hb_binary_generated(gpu_info->context, filename, &fd)) == 1)
    {
        status = hb_ocl->clGetContextInfo(gpu_info->context, CL_CONTEXT_NUM_DEVICES,
                                          sizeof(numDevices), &numDevices, NULL);
        if (status != CL_SUCCESS)
        {
            hb_log("OpenCL: Unable to get the number of devices in context.");
            return 0;
        }

        devices = (cl_device_id*)malloc(sizeof(cl_device_id) * numDevices);
        if (devices == NULL)
            return 0;

        length   = 0;
        b_error  = 0;
        b_error |= fseek(fd, 0, SEEK_END) <  0;
        b_error |= (length = ftell(fd))   <= 0;
        b_error |= fseek(fd, 0, SEEK_SET) <  0;
        if (b_error)
            return 0;

        binary = (char*)calloc(length + 2, sizeof(char));
        if (binary == NULL)
            return 0;

        b_error |= fread(binary, 1, length, fd) != length;
#if 0   // this doesn't work under OS X and/or with some non-AMD GPUs
        if (binary[length-1] != '\n')
            binary[length++]  = '\n';
#endif

        if (b_error)
            return 0;

        /* grab the handles to all of the devices in the context. */
        status = hb_ocl->clGetContextInfo(gpu_info->context, CL_CONTEXT_DEVICES,
                                          sizeof(cl_device_id) * numDevices,
                                          devices, NULL);

        gpu_info->programs[idx] = hb_ocl->clCreateProgramWithBinary(gpu_info->context,
                                                                    numDevices,
                                                                    devices,
                                                                    &length,
                                                                    (const unsigned char**)&binary,
                                                                    &binary_status,
                                                                    &status);

        fclose(fd);
        free(devices);
        fd      = NULL;
        devices = NULL;
    }
    else
    {
        /* create a CL program using the kernel source */
        gpu_info->programs[idx] = hb_ocl->clCreateProgramWithSource(gpu_info->context, 1,
                                                                    &source, source_size,
                                                                    &status);
    }

    if((gpu_info->programs[idx] == (cl_program)NULL) || (status != CL_SUCCESS)){
        hb_log( "OpenCL: Unable to get list of devices in context." );
        return(0);
    }

    /* create a cl program executable for all the devices specified */
    if( !gpu_info->isUserCreated ) 
    {
        status = hb_ocl->clBuildProgram(gpu_info->programs[idx], 1, gpu_info->devices,
                                        build_option, NULL, NULL);
    }
    else
    {
        status = hb_ocl->clBuildProgram(gpu_info->programs[idx], 1, &(gpu_info->dev),
                                        build_option, NULL, NULL);
    }

    if( status != CL_SUCCESS )
    {
        if( !gpu_info->isUserCreated ) 
        {
            status = hb_ocl->clGetProgramBuildInfo(gpu_info->programs[idx],
                                                   gpu_info->devices[0],
                                                   CL_PROGRAM_BUILD_LOG,
                                                   0, NULL, &length);
        }
        else
        {
            status = hb_ocl->clGetProgramBuildInfo(gpu_info->programs[idx],
                                                   gpu_info->dev,
                                                   CL_PROGRAM_BUILD_LOG,
                                                   0, NULL, &length);
        }

        if( status != CL_SUCCESS )
        {
            hb_log( "OpenCL: Unable to get GPU build information." );
            return(0);
        }

        buildLog = (char*)malloc( length );
        if( buildLog == (char*)NULL )
        {
            return(0);
        }

        if( !gpu_info->isUserCreated )
        {
            status = hb_ocl->clGetProgramBuildInfo(gpu_info->programs[idx],
                                                   gpu_info->devices[0],
                                                   CL_PROGRAM_BUILD_LOG,
                                                   length, buildLog, &length);
        }
        else
        {
            status = hb_ocl->clGetProgramBuildInfo(gpu_info->programs[idx],
                                                   gpu_info->dev,
                                                   CL_PROGRAM_BUILD_LOG,
                                                   length, buildLog, &length);
        }

        fd1 = fopen( "kernel-build.log", "w+" );
        if( fd1 != NULL ) {
            fwrite( buildLog, sizeof(char), length, fd1 );
            fclose( fd1 );
        }

        free( buildLog );
        return(0);
    }

    strcpy( gpu_env.kernelSrcFile[idx], filename );

    if (binaryExisted != 1)
    {
        //hb_generat_bin_from_kernel_source(gpu_env.programs[idx], filename);
    }

    gpu_info->file_count += 1;

    return(1);
}


/**
 * hb_get_kernel_env_and_func
 * @param kernel_name -
 * @param env -
 * @param function -
 */
int hb_get_kernel_env_and_func( const char *kernel_name,
                                KernelEnv *env,
                                cl_kernel_function *function )
{
    int i;
    for( i = 0; i < gpu_env.kernel_count; i++ )
    {
        if( strcasecmp( kernel_name, gpu_env.kernel_names[i] ) == 0 )
        {
            env->context = gpu_env.context;
            env->command_queue = gpu_env.command_queue;
            env->program = gpu_env.programs[0];
            env->kernel = gpu_env.kernels[i];
            env->isAMD = ( gpu_env.vendor == AMD ) ? 1 : 0;
            *function = gpu_env.kernel_functions[i];
            return(1);
        }
    }
    return(0);
}

/**
 * hb_get_kernel_env_and_func
 * @param kernel_name -
 * @param userdata -
 */
int hb_run_kernel( const char *kernel_name, void **userdata )
{
    KernelEnv env;
    cl_kernel_function function;
    int status;
    memset( &env, 0, sizeof(KernelEnv));
    status = hb_get_kernel_env_and_func( kernel_name, &env, &function );
    strcpy( env.kernel_name, kernel_name );
    if( status == 1 ) 
    {
        return(function( userdata, &env ));
    }

    return(0);
}

/**
 * hb_init_opencl_run_env
 * @param argc -
 * @param argv -
 * @param build_option -
 */
int hb_init_opencl_run_env( int argc, char **argv, const char *build_option )
{
    int status = 0;
    if( MAX_CLKERNEL_NUM <= 0 )
    {
        return 1;
    }

    if((argc > MAX_CLFILE_NUM) || (argc<0))
    {
        return 1;
    }

    if( !isInited )
    {
        hb_regist_opencl_kernel();

        /*initialize devices, context, comand_queue*/
        status = hb_init_opencl_env( &gpu_env );
        if( status )
            return(1);

        /*initialize program, kernel_name, kernel_count*/
        status = hb_compile_kernel_file("hb-opencl-kernels.cl",
                                        &gpu_env, 0, build_option);

        if( status == 0 || gpu_env.kernel_count == 0 )
        {
            return(1);

        }

        useBuffers = 1;
        isInited = 1;
    }

    return(0);
}

/**
 * hb_release_opencl_run_env
 */
int hb_release_opencl_run_env()
{
    return hb_release_opencl_env( &gpu_env );
}

/**
 * hb_opencl_stats
 */
int hb_opencl_stats()
{
    return isInited;
}

/**
 * hb_get_opencl_env
 */
int hb_get_opencl_env()
{
    /* initialize devices, context, command_queue */
    return hb_init_opencl_env(&gpu_env);
}

/**
 * hb_create_buffer
 * @param cl_inBuf -
 * @param flags -
 * @param size -
 */
int hb_create_buffer( cl_mem *cl_Buf, int flags, int size )
{
    int status;

    if (hb_ocl == NULL)
    {
        hb_error("hb_create_buffer: OpenCL support not available");
        return 0;
    }

    *cl_Buf = hb_ocl->clCreateBuffer(gpu_env.context, flags, size, NULL, &status);
    
    if( status != CL_SUCCESS )
    { 
        hb_log( "OpenCL: clCreateBuffer error '%d'", status );
        return 0; 
    }

    return 1;
}


/**
 * hb_read_opencl_buffer
 * @param cl_inBuf -
 * @param outbuf -
 * @param size -
 */
int hb_read_opencl_buffer( cl_mem cl_inBuf, unsigned char *outbuf, int size )
{
    int status;

    if (hb_ocl == NULL)
    {
        hb_error("hb_read_opencl_suffer: OpenCL support not available");
        return 0;
    }

    status = hb_ocl->clEnqueueReadBuffer(gpu_env.command_queue, cl_inBuf,
                                         CL_TRUE, 0, size, outbuf, 0, 0, 0);
    if( status != CL_SUCCESS )
    { 
        hb_log( "OpenCL: av_read_opencl_buffer error '%d'", status );
        return 0; 
    }

    return 1;
}

int hb_cl_create_mapped_buffer(cl_mem *mem, unsigned char **addr, int size)
{
    int status;
    int flags = CL_MEM_ALLOC_HOST_PTR;

    if (hb_ocl == NULL)
    {
        hb_error("hb_cl_create_mapped_buffer: OpenCL support not available");
        return 0;
    }

    //cl_event event;
    *mem  = hb_ocl->clCreateBuffer(gpu_env.context, flags, size, NULL, &status);
    *addr = hb_ocl->clEnqueueMapBuffer(gpu_env.command_queue, *mem, CL_TRUE,
                                       CL_MAP_READ|CL_MAP_WRITE, 0, size, 0,
                                       NULL, NULL/*&event*/, &status);

    //hb_log("\t **** context: %.8x   cmdqueue: %.8x   cl_mem: %.8x   mapaddr: %.8x   size: %d    status: %d", gpu_env.context, gpu_env.command_queue, mem, addr, size, status);

    return (status == CL_SUCCESS) ? 1 : 0;
}

int hb_cl_free_mapped_buffer(cl_mem mem, unsigned char *addr)
{
    cl_event event;

    if (hb_ocl == NULL)
    {
        hb_error("hb_cl_free_mapped_buffer: OpenCL support not available");
        return 0;
    }

    int status = hb_ocl->clEnqueueUnmapMemObject(gpu_env.command_queue, mem,
                                                 addr, 0, NULL, &event);
    if (status == CL_SUCCESS)
        hb_ocl->clWaitForEvents(1, &event);
    else
        hb_log("hb_free_mapped_buffer: error %d", status);
    return (status == CL_SUCCESS) ? 1 : 0;
}

void hb_opencl_init()
{
    hb_get_opencl_env();
}

int hb_use_buffers()
{
    return useBuffers;
}

int hb_copy_buffer(cl_mem src_buffer,cl_mem dst_buffer,size_t src_offset,size_t dst_offset,size_t cb)
{
    if (hb_ocl == NULL)
    {
        hb_error("hb_copy_buffer: OpenCL support not available");
        return 0;
    }

    int status = hb_ocl->clEnqueueCopyBuffer(gpu_env.command_queue,
                                             src_buffer, dst_buffer,
                                             src_offset, dst_offset,
                                             cb, 0, 0, 0);
    if( status != CL_SUCCESS )
    { 
        av_log(NULL,AV_LOG_ERROR, "hb_read_opencl_buffer error '%d'\n", status ); 
        return 0; 
    }
    return 1;
}

int hb_read_opencl_frame_buffer(cl_mem cl_inBuf,unsigned char *Ybuf,unsigned char *Ubuf,unsigned char *Vbuf,int linesize0,int linesize1,int linesize2,int height)
{

    int chrH = -(-height >> 1);
        unsigned char *temp = (unsigned char *)av_malloc(sizeof(uint8_t) * (linesize0 * height + linesize1 * chrH * 2));
    if(hb_read_opencl_buffer(cl_inBuf,temp,sizeof(uint8_t)*(linesize0 + linesize1)*height))
    {
        memcpy(Ybuf,temp,linesize0 * height);
        memcpy(Ubuf,temp + linesize0 * height,linesize1 *chrH);
        memcpy(Vbuf,temp + linesize0 * height + linesize1 * chrH,linesize2 * chrH);
        
    }
    av_free(temp);

    return 1;
}

int hb_write_opencl_frame_buffer(cl_mem cl_inBuf,unsigned char *Ybuf,unsigned char *Ubuf,unsigned char *Vbuf,int linesize0,int linesize1,int linesize2,int height,int offset)
{
    if (hb_ocl == NULL)
    {
        hb_error("hb_write_opencl_frame_buffer: OpenCL support not available");
        return 0;
    }

    void *mapped = hb_ocl->clEnqueueMapBuffer(gpu_env.command_queue, cl_inBuf,
                                              CL_TRUE,CL_MAP_WRITE, 0,
                                              sizeof(uint8_t) * (linesize0  + linesize1) * height + offset,
                                              0, NULL, NULL, NULL);
    uint8_t *temp = (uint8_t *)mapped;
    temp += offset;
    memcpy(temp,Ybuf,sizeof(uint8_t) * linesize0 * height);
    memcpy(temp + sizeof(uint8_t) * linesize0 * height,Ubuf,sizeof(uint8_t) * linesize1 * height/2);
    memcpy(temp + sizeof(uint8_t) * (linesize0 * height + linesize1 * height/2),Vbuf,sizeof(uint8_t) * linesize2 * height/2);
    hb_ocl->clEnqueueUnmapMemObject(gpu_env.command_queue, cl_inBuf, mapped, 0, NULL, NULL);
    return 1;
}

cl_command_queue hb_get_command_queue()
{
    return gpu_env.command_queue;
}

cl_context hb_get_context()
{
    return gpu_env.context;
}
