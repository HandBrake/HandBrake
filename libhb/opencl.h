/* opencl.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_OPENCL_H
#define HB_OPENCL_H

#include "extras/cl.h"
#include "openclwrapper.h"

// we only support OpenCL 1.1 or later
#define HB_OCL_MINVERSION_MAJOR 1
#define HB_OCL_MINVERSION_MINOR 1

#define HB_OCL_FUNC_TYPE(name)      hb_opencl_##name##_func
#define HB_OCL_FUNC_DECL(name)      HB_OCL_FUNC_TYPE(name) name
#define HB_OCL_API(ret, attr, name) typedef ret (attr* HB_OCL_FUNC_TYPE(name))

#ifdef __APPLE__
#pragma mark -
#pragma mark OpenCL API
#endif // __APPLE__

/* Platform API */
HB_OCL_API(cl_int, CL_API_CALL, clGetPlatformIDs)
(cl_uint          /* num_entries */,
 cl_platform_id * /* platforms */,
 cl_uint *        /* num_platforms */);

HB_OCL_API(cl_int, CL_API_CALL, clGetPlatformInfo)
(cl_platform_id   /* platform */,
 cl_platform_info /* param_name */,
 size_t           /* param_value_size */,
 void *           /* param_value */,
 size_t *         /* param_value_size_ret */);

/* Device APIs */
HB_OCL_API(cl_int, CL_API_CALL, clGetDeviceIDs)
(cl_platform_id   /* platform */,
 cl_device_type   /* device_type */,
 cl_uint          /* num_entries */,
 cl_device_id *   /* devices */,
 cl_uint *        /* num_devices */);

HB_OCL_API(cl_int, CL_API_CALL, clGetDeviceInfo)
(cl_device_id    /* device */,
 cl_device_info  /* param_name */,
 size_t          /* param_value_size */,
 void *          /* param_value */,
 size_t *        /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clCreateSubDevices)
(cl_device_id                         /* in_device */,
 const cl_device_partition_property * /* properties */,
 cl_uint                              /* num_devices */,
 cl_device_id *                       /* out_devices */,
 cl_uint *                            /* num_devices_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainDevice)
(cl_device_id /* device */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseDevice)
(cl_device_id /* device */);

/* Context APIs  */
HB_OCL_API(cl_context, CL_API_CALL, clCreateContext)
(const cl_context_properties * /* properties */,
 cl_uint                 /* num_devices */,
 const cl_device_id *    /* devices */,
 void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
 void *                  /* user_data */,
 cl_int *                /* errcode_ret */);

HB_OCL_API(cl_context, CL_API_CALL, clCreateContextFromType)
(const cl_context_properties * /* properties */,
 cl_device_type          /* device_type */,
 void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
 void *                  /* user_data */,
 cl_int *                /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainContext)
(cl_context /* context */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseContext)
(cl_context /* context */);

HB_OCL_API(cl_int, CL_API_CALL, clGetContextInfo)
(cl_context         /* context */,
 cl_context_info    /* param_name */,
 size_t             /* param_value_size */,
 void *             /* param_value */,
 size_t *           /* param_value_size_ret */);

/* Command Queue APIs */
HB_OCL_API(cl_command_queue, CL_API_CALL, clCreateCommandQueue)
(cl_context                     /* context */,
 cl_device_id                   /* device */,
 cl_command_queue_properties    /* properties */,
 cl_int *                       /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainCommandQueue)
(cl_command_queue /* command_queue */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseCommandQueue)
(cl_command_queue /* command_queue */);

HB_OCL_API(cl_int, CL_API_CALL, clGetCommandQueueInfo)
(cl_command_queue      /* command_queue */,
 cl_command_queue_info /* param_name */,
 size_t                /* param_value_size */,
 void *                /* param_value */,
 size_t *              /* param_value_size_ret */);

/* Memory Object APIs */
HB_OCL_API(cl_mem, CL_API_CALL, clCreateBuffer)
(cl_context   /* context */,
 cl_mem_flags /* flags */,
 size_t       /* size */,
 void *       /* host_ptr */,
 cl_int *     /* errcode_ret */);

HB_OCL_API(cl_mem, CL_API_CALL, clCreateSubBuffer)
(cl_mem                   /* buffer */,
 cl_mem_flags             /* flags */,
 cl_buffer_create_type    /* buffer_create_type */,
 const void *             /* buffer_create_info */,
 cl_int *                 /* errcode_ret */);

HB_OCL_API(cl_mem, CL_API_CALL, clCreateImage)
(cl_context              /* context */,
 cl_mem_flags            /* flags */,
 const cl_image_format * /* image_format */,
 const cl_image_desc *   /* image_desc */,
 void *                  /* host_ptr */,
 cl_int *                /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainMemObject)
(cl_mem /* memobj */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseMemObject)
(cl_mem /* memobj */);

HB_OCL_API(cl_int, CL_API_CALL, clGetSupportedImageFormats)
(cl_context           /* context */,
 cl_mem_flags         /* flags */,
 cl_mem_object_type   /* image_type */,
 cl_uint              /* num_entries */,
 cl_image_format *    /* image_formats */,
 cl_uint *            /* num_image_formats */);

HB_OCL_API(cl_int, CL_API_CALL, clGetMemObjectInfo)
(cl_mem           /* memobj */,
 cl_mem_info      /* param_name */,
 size_t           /* param_value_size */,
 void *           /* param_value */,
 size_t *         /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clGetImageInfo)
(cl_mem           /* image */,
 cl_image_info    /* param_name */,
 size_t           /* param_value_size */,
 void *           /* param_value */,
 size_t *         /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clSetMemObjectDestructorCallback)
(cl_mem /* memobj */,
 void (CL_CALLBACK * /*pfn_notify*/)( cl_mem /* memobj */, void* /*user_data*/),
 void * /*user_data */ );

/* Sampler APIs */
HB_OCL_API(cl_sampler, CL_API_CALL, clCreateSampler)
(cl_context          /* context */,
 cl_bool             /* normalized_coords */,
 cl_addressing_mode  /* addressing_mode */,
 cl_filter_mode      /* filter_mode */,
 cl_int *            /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainSampler)
(cl_sampler /* sampler */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseSampler)
(cl_sampler /* sampler */);

HB_OCL_API(cl_int, CL_API_CALL, clGetSamplerInfo)
(cl_sampler         /* sampler */,
 cl_sampler_info    /* param_name */,
 size_t             /* param_value_size */,
 void *             /* param_value */,
 size_t *           /* param_value_size_ret */);

/* Program Object APIs  */
HB_OCL_API(cl_program, CL_API_CALL, clCreateProgramWithSource)
(cl_context        /* context */,
 cl_uint           /* count */,
 const char **     /* strings */,
 const size_t *    /* lengths */,
 cl_int *          /* errcode_ret */);

HB_OCL_API(cl_program, CL_API_CALL, clCreateProgramWithBinary)
(cl_context                     /* context */,
 cl_uint                        /* num_devices */,
 const cl_device_id *           /* device_list */,
 const size_t *                 /* lengths */,
 const unsigned char **         /* binaries */,
 cl_int *                       /* binary_status */,
 cl_int *                       /* errcode_ret */);

HB_OCL_API(cl_program, CL_API_CALL, clCreateProgramWithBuiltInKernels)
(cl_context            /* context */,
 cl_uint               /* num_devices */,
 const cl_device_id *  /* device_list */,
 const char *          /* kernel_names */,
 cl_int *              /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainProgram)
(cl_program /* program */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseProgram)
(cl_program /* program */);

HB_OCL_API(cl_int, CL_API_CALL, clBuildProgram)
(cl_program           /* program */,
 cl_uint              /* num_devices */,
 const cl_device_id * /* device_list */,
 const char *         /* options */,
 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
 void *               /* user_data */);

HB_OCL_API(cl_int, CL_API_CALL, clCompileProgram)
(cl_program           /* program */,
 cl_uint              /* num_devices */,
 const cl_device_id * /* device_list */,
 const char *         /* options */,
 cl_uint              /* num_input_headers */,
 const cl_program *   /* input_headers */,
 const char **        /* header_include_names */,
 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
 void *               /* user_data */);

HB_OCL_API(cl_program, CL_API_CALL, clLinkProgram)
(cl_context           /* context */,
 cl_uint              /* num_devices */,
 const cl_device_id * /* device_list */,
 const char *         /* options */,
 cl_uint              /* num_input_programs */,
 const cl_program *   /* input_programs */,
 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
 void *               /* user_data */,
 cl_int *             /* errcode_ret */ );


HB_OCL_API(cl_int, CL_API_CALL, clUnloadPlatformCompiler)
(cl_platform_id /* platform */);

HB_OCL_API(cl_int, CL_API_CALL, clGetProgramInfo)
(cl_program         /* program */,
 cl_program_info    /* param_name */,
 size_t             /* param_value_size */,
 void *             /* param_value */,
 size_t *           /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clGetProgramBuildInfo)
(cl_program            /* program */,
 cl_device_id          /* device */,
 cl_program_build_info /* param_name */,
 size_t                /* param_value_size */,
 void *                /* param_value */,
 size_t *              /* param_value_size_ret */);

/* Kernel Object APIs */
HB_OCL_API(cl_kernel, CL_API_CALL, clCreateKernel)
(cl_program      /* program */,
 const char *    /* kernel_name */,
 cl_int *        /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clCreateKernelsInProgram)
(cl_program     /* program */,
 cl_uint        /* num_kernels */,
 cl_kernel *    /* kernels */,
 cl_uint *      /* num_kernels_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainKernel)
(cl_kernel    /* kernel */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseKernel)
(cl_kernel   /* kernel */);

HB_OCL_API(cl_int, CL_API_CALL, clSetKernelArg)
(cl_kernel    /* kernel */,
 cl_uint      /* arg_index */,
 size_t       /* arg_size */,
 const void * /* arg_value */);

HB_OCL_API(cl_int, CL_API_CALL, clGetKernelInfo)
(cl_kernel       /* kernel */,
 cl_kernel_info  /* param_name */,
 size_t          /* param_value_size */,
 void *          /* param_value */,
 size_t *        /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clGetKernelArgInfo)
(cl_kernel       /* kernel */,
 cl_uint         /* arg_indx */,
 cl_kernel_arg_info  /* param_name */,
 size_t          /* param_value_size */,
 void *          /* param_value */,
 size_t *        /* param_value_size_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clGetKernelWorkGroupInfo)
(cl_kernel                  /* kernel */,
 cl_device_id               /* device */,
 cl_kernel_work_group_info  /* param_name */,
 size_t                     /* param_value_size */,
 void *                     /* param_value */,
 size_t *                   /* param_value_size_ret */);

/* Event Object APIs */
HB_OCL_API(cl_int, CL_API_CALL, clWaitForEvents)
(cl_uint             /* num_events */,
 const cl_event *    /* event_list */);

HB_OCL_API(cl_int, CL_API_CALL, clGetEventInfo)
(cl_event         /* event */,
 cl_event_info    /* param_name */,
 size_t           /* param_value_size */,
 void *           /* param_value */,
 size_t *         /* param_value_size_ret */);

HB_OCL_API(cl_event, CL_API_CALL, clCreateUserEvent)
(cl_context    /* context */,
 cl_int *      /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clRetainEvent)
(cl_event /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clReleaseEvent)
(cl_event /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clSetUserEventStatus)
(cl_event   /* event */,
 cl_int     /* execution_status */);

HB_OCL_API(cl_int, CL_API_CALL, clSetEventCallback)
(cl_event    /* event */,
 cl_int      /* command_exec_callback_type */,
 void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
 void *      /* user_data */);

/* Profiling APIs */
HB_OCL_API(cl_int, CL_API_CALL, clGetEventProfilingInfo)
(cl_event            /* event */,
 cl_profiling_info   /* param_name */,
 size_t              /* param_value_size */,
 void *              /* param_value */,
 size_t *            /* param_value_size_ret */);

/* Flush and Finish APIs */
HB_OCL_API(cl_int, CL_API_CALL, clFlush)
(cl_command_queue /* command_queue */);

HB_OCL_API(cl_int, CL_API_CALL, clFinish)
(cl_command_queue /* command_queue */);

/* Enqueued Commands APIs */
HB_OCL_API(cl_int, CL_API_CALL, clEnqueueReadBuffer)
(cl_command_queue    /* command_queue */,
 cl_mem              /* buffer */,
 cl_bool             /* blocking_read */,
 size_t              /* offset */,
 size_t              /* size */,
 void *              /* ptr */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueReadBufferRect)
(cl_command_queue    /* command_queue */,
 cl_mem              /* buffer */,
 cl_bool             /* blocking_read */,
 const size_t *      /* buffer_offset */,
 const size_t *      /* host_offset */,
 const size_t *      /* region */,
 size_t              /* buffer_row_pitch */,
 size_t              /* buffer_slice_pitch */,
 size_t              /* host_row_pitch */,
 size_t              /* host_slice_pitch */,
 void *              /* ptr */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueWriteBuffer)
(cl_command_queue   /* command_queue */,
 cl_mem             /* buffer */,
 cl_bool            /* blocking_write */,
 size_t             /* offset */,
 size_t             /* size */,
 const void *       /* ptr */,
 cl_uint            /* num_events_in_wait_list */,
 const cl_event *   /* event_wait_list */,
 cl_event *         /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueWriteBufferRect)
(cl_command_queue    /* command_queue */,
 cl_mem              /* buffer */,
 cl_bool             /* blocking_write */,
 const size_t *      /* buffer_offset */,
 const size_t *      /* host_offset */,
 const size_t *      /* region */,
 size_t              /* buffer_row_pitch */,
 size_t              /* buffer_slice_pitch */,
 size_t              /* host_row_pitch */,
 size_t              /* host_slice_pitch */,
 const void *        /* ptr */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueFillBuffer)
(cl_command_queue   /* command_queue */,
 cl_mem             /* buffer */,
 const void *       /* pattern */,
 size_t             /* pattern_size */,
 size_t             /* offset */,
 size_t             /* size */,
 cl_uint            /* num_events_in_wait_list */,
 const cl_event *   /* event_wait_list */,
 cl_event *         /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueCopyBuffer)
(cl_command_queue    /* command_queue */,
 cl_mem              /* src_buffer */,
 cl_mem              /* dst_buffer */,
 size_t              /* src_offset */,
 size_t              /* dst_offset */,
 size_t              /* size */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueCopyBufferRect)
(cl_command_queue    /* command_queue */,
 cl_mem              /* src_buffer */,
 cl_mem              /* dst_buffer */,
 const size_t *      /* src_origin */,
 const size_t *      /* dst_origin */,
 const size_t *      /* region */,
 size_t              /* src_row_pitch */,
 size_t              /* src_slice_pitch */,
 size_t              /* dst_row_pitch */,
 size_t              /* dst_slice_pitch */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueReadImage)
(cl_command_queue     /* command_queue */,
 cl_mem               /* image */,
 cl_bool              /* blocking_read */,
 const size_t *       /* origin[3] */,
 const size_t *       /* region[3] */,
 size_t               /* row_pitch */,
 size_t               /* slice_pitch */,
 void *               /* ptr */,
 cl_uint              /* num_events_in_wait_list */,
 const cl_event *     /* event_wait_list */,
 cl_event *           /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueWriteImage)
(cl_command_queue    /* command_queue */,
 cl_mem              /* image */,
 cl_bool             /* blocking_write */,
 const size_t *      /* origin[3] */,
 const size_t *      /* region[3] */,
 size_t              /* input_row_pitch */,
 size_t              /* input_slice_pitch */,
 const void *        /* ptr */,
 cl_uint             /* num_events_in_wait_list */,
 const cl_event *    /* event_wait_list */,
 cl_event *          /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueFillImage)
(cl_command_queue   /* command_queue */,
 cl_mem             /* image */,
 const void *       /* fill_color */,
 const size_t *     /* origin[3] */,
 const size_t *     /* region[3] */,
 cl_uint            /* num_events_in_wait_list */,
 const cl_event *   /* event_wait_list */,
 cl_event *         /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueCopyImage)
(cl_command_queue     /* command_queue */,
 cl_mem               /* src_image */,
 cl_mem               /* dst_image */,
 const size_t *       /* src_origin[3] */,
 const size_t *       /* dst_origin[3] */,
 const size_t *       /* region[3] */,
 cl_uint              /* num_events_in_wait_list */,
 const cl_event *     /* event_wait_list */,
 cl_event *           /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueCopyImageToBuffer)
(cl_command_queue /* command_queue */,
 cl_mem           /* src_image */,
 cl_mem           /* dst_buffer */,
 const size_t *   /* src_origin[3] */,
 const size_t *   /* region[3] */,
 size_t           /* dst_offset */,
 cl_uint          /* num_events_in_wait_list */,
 const cl_event * /* event_wait_list */,
 cl_event *       /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueCopyBufferToImage)
(cl_command_queue /* command_queue */,
 cl_mem           /* src_buffer */,
 cl_mem           /* dst_image */,
 size_t           /* src_offset */,
 const size_t *   /* dst_origin[3] */,
 const size_t *   /* region[3] */,
 cl_uint          /* num_events_in_wait_list */,
 const cl_event * /* event_wait_list */,
 cl_event *       /* event */);

HB_OCL_API(void *, CL_API_CALL, clEnqueueMapBuffer)
(cl_command_queue /* command_queue */,
 cl_mem           /* buffer */,
 cl_bool          /* blocking_map */,
 cl_map_flags     /* map_flags */,
 size_t           /* offset */,
 size_t           /* size */,
 cl_uint          /* num_events_in_wait_list */,
 const cl_event * /* event_wait_list */,
 cl_event *       /* event */,
 cl_int *         /* errcode_ret */);

HB_OCL_API(void *, CL_API_CALL, clEnqueueMapImage)
(cl_command_queue  /* command_queue */,
 cl_mem            /* image */,
 cl_bool           /* blocking_map */,
 cl_map_flags      /* map_flags */,
 const size_t *    /* origin[3] */,
 const size_t *    /* region[3] */,
 size_t *          /* image_row_pitch */,
 size_t *          /* image_slice_pitch */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */,
 cl_int *          /* errcode_ret */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueUnmapMemObject)
(cl_command_queue  /* command_queue */,
 cl_mem            /* memobj */,
 void *            /* mapped_ptr */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueMigrateMemObjects)
(cl_command_queue       /* command_queue */,
 cl_uint                /* num_mem_objects */,
 const cl_mem *         /* mem_objects */,
 cl_mem_migration_flags /* flags */,
 cl_uint                /* num_events_in_wait_list */,
 const cl_event *       /* event_wait_list */,
 cl_event *             /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueNDRangeKernel)
(cl_command_queue /* command_queue */,
 cl_kernel        /* kernel */,
 cl_uint          /* work_dim */,
 const size_t *   /* global_work_offset */,
 const size_t *   /* global_work_size */,
 const size_t *   /* local_work_size */,
 cl_uint          /* num_events_in_wait_list */,
 const cl_event * /* event_wait_list */,
 cl_event *       /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueTask)
(cl_command_queue  /* command_queue */,
 cl_kernel         /* kernel */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueNativeKernel)
(cl_command_queue  /* command_queue */,
 void (CL_CALLBACK * /*user_func*/)(void *),
 void *            /* args */,
 size_t            /* cb_args */,
 cl_uint           /* num_mem_objects */,
 const cl_mem *    /* mem_list */,
 const void **     /* args_mem_loc */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueMarkerWithWaitList)
(cl_command_queue  /* command_queue */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */);

HB_OCL_API(cl_int, CL_API_CALL, clEnqueueBarrierWithWaitList)
(cl_command_queue  /* command_queue */,
 cl_uint           /* num_events_in_wait_list */,
 const cl_event *  /* event_wait_list */,
 cl_event *        /* event */);


/* Extension function access
 *
 * Returns the extension function address for the given function name,
 * or NULL if a valid function can not be found.  The client must
 * check to make sure the address is not NULL, before using or
 * calling the returned function address.
 */
HB_OCL_API(void *, CL_API_CALL, clGetExtensionFunctionAddressForPlatform)
(cl_platform_id /* platform */,
 const char *   /* func_name */);

#ifdef __APPLE__
#pragma mark -
#endif // __APPLE__

typedef struct hb_opencl_library_s
{
    void *library;

    /* Pointers to select OpenCL API functions */
    HB_OCL_FUNC_DECL(clBuildProgram);
    HB_OCL_FUNC_DECL(clCreateBuffer);
    HB_OCL_FUNC_DECL(clCreateCommandQueue);
    HB_OCL_FUNC_DECL(clCreateContextFromType);
    HB_OCL_FUNC_DECL(clCreateKernel);
    HB_OCL_FUNC_DECL(clCreateProgramWithBinary);
    HB_OCL_FUNC_DECL(clCreateProgramWithSource);
    HB_OCL_FUNC_DECL(clEnqueueCopyBuffer);
    HB_OCL_FUNC_DECL(clEnqueueMapBuffer);
    HB_OCL_FUNC_DECL(clEnqueueNDRangeKernel);
    HB_OCL_FUNC_DECL(clEnqueueReadBuffer);
    HB_OCL_FUNC_DECL(clEnqueueUnmapMemObject);
    HB_OCL_FUNC_DECL(clEnqueueWriteBuffer);
    HB_OCL_FUNC_DECL(clFlush);
    HB_OCL_FUNC_DECL(clGetCommandQueueInfo);
    HB_OCL_FUNC_DECL(clGetContextInfo);
    HB_OCL_FUNC_DECL(clGetDeviceIDs);
    HB_OCL_FUNC_DECL(clGetDeviceInfo);
    HB_OCL_FUNC_DECL(clGetPlatformIDs);
    HB_OCL_FUNC_DECL(clGetPlatformInfo);
    HB_OCL_FUNC_DECL(clGetProgramBuildInfo);
    HB_OCL_FUNC_DECL(clGetProgramInfo);
    HB_OCL_FUNC_DECL(clReleaseCommandQueue);
    HB_OCL_FUNC_DECL(clReleaseContext);
    HB_OCL_FUNC_DECL(clReleaseEvent);
    HB_OCL_FUNC_DECL(clReleaseKernel);
    HB_OCL_FUNC_DECL(clReleaseMemObject);
    HB_OCL_FUNC_DECL(clReleaseProgram);
    HB_OCL_FUNC_DECL(clSetKernelArg);
    HB_OCL_FUNC_DECL(clWaitForEvents);
} hb_opencl_library_t;

hb_opencl_library_t* hb_opencl_library_init();
void                 hb_opencl_library_close(hb_opencl_library_t **_opencl);

/*
 * Convenience pointer to a single shared OpenCL library wrapper.
 *
 * It can be initialized and closed via hb_ocl_init/close().
 */
extern hb_opencl_library_t *hb_ocl;
int    hb_ocl_init();
void   hb_ocl_close();

typedef struct hb_opencl_device_s
{
    cl_platform_id platform;
    cl_device_type type;
    cl_device_id   id;
    char version[128];
    char  driver[128];
    char  vendor[128];
    char    name[128];
    enum
    {
        HB_OCL_VENDOR_AMD,
        HB_OCL_VENDOR_NVIDIA,
        HB_OCL_VENDOR_INTEL,
        HB_OCL_VENDOR_OTHER,
    } ocl_vendor;
} hb_opencl_device_t;

int  hb_opencl_available();
void hb_opencl_info_print();

/* OpenCL scaling */
typedef struct hb_oclscale_s
{
    int initialized;
    // bicubic scale weights
    cl_mem bicubic_x_weights;
    cl_mem bicubic_y_weights;
    cl_float xscale;
    cl_float yscale;
    int width;
    int height;
    // horizontal scaling and vertical scaling kernel handle
    cl_kernel m_kernel;
    int use_ocl_mem; // 0 use host memory. 1 use gpu oclmem
} hb_oclscale_t;

int hb_ocl_scale(hb_buffer_t *in, hb_buffer_t *out, int *crop,
                 hb_oclscale_t *os);

/* Utilities */
#define HB_OCL_BUF_CREATE(ocl_lib, out, flags, size)                            \
{                                                                               \
    out = ocl_lib->clCreateBuffer(kenv->context, flags, size, NULL, &status);   \
    if (CL_SUCCESS != status)                                                   \
    {                                                                           \
        return -1;                                                              \
    }                                                                           \
}

#define HB_OCL_BUF_FREE(ocl_lib, buf)                                           \
{                                                                               \
    if (buf != NULL)                                                            \
    {                                                                           \
        ocl_lib->clReleaseMemObject(buf);                                       \
        buf = NULL;                                                             \
    }                                                                           \
}

#define HB_OCL_CHECK(method, ...)                                               \
{                                                                               \
    status = method(__VA_ARGS__);                                               \
    if (status != CL_SUCCESS)                                                   \
    {                                                                           \
        hb_error("%s:%d (%s) error: %d\n",__FUNCTION__,__LINE__,#method,status);\
        return status;                                                          \
    }                                                                           \
}

#endif//HB_OPENCL_H
