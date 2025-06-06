﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBFunctions.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Contains p-invoke function declarations to libhb arm64 edition.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    internal static class HbFunctionsArm
    {
        [DllImport("hb_a64", EntryPoint = "hb_register_logger", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_register_logger(LoggingCallback callback);

        [DllImport("hb_a64", EntryPoint = "hb_register_error_handler", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_register_error_handler(LoggingCallback callback);

        [DllImport("hb_a64", EntryPoint = "hb_global_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_global_init();

        [DllImport("hb_a64", EntryPoint = "hb_global_init_no_hardware", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_global_init_no_hardware();

        [DllImport("hb_a64", EntryPoint = "hb_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_init(int verbose, int update_check);

        [DllImport("hb_a64", EntryPoint = "hb_init_dl", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_init_dl(int verbose, int update_check);

        [DllImport("hb_a64", EntryPoint = "hb_get_version", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_version(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_get_build", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_get_build(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_dvd_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_dvd_name(IntPtr path);

        [DllImport("hb_a64", EntryPoint = "hb_dvd_set_dvdnav", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_dvd_set_dvdnav(int enable);

        [DllImport("hb_a64", EntryPoint = "hb_scan", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_scan(IntPtr hbHandle, IntPtr path, int title_index, int preview_count, int store_previews, ulong min_duration, ulong max_duration, int crop_auto_switch_threshold, int crop_median_threshold, IntPtr exclude_extensions, int hw_decode, int keep_duplicate_titles);

        [DllImport("hb_a64", EntryPoint = "hb_scan_stop", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_scan_stop(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_get_titles", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_titles(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_set_anamorphic_size2", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_anamorphic_size2(ref hb_geometry_s sourceGeometry, ref hb_geometry_settings_s uiGeometry, ref hb_geometry_s result);

        [DllImport("hb_a64", EntryPoint = "hb_rotate_geometry", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_rotate_geometry(ref hb_geometry_crop_s geo, ref hb_geometry_crop_s result, int angle, int hflip);

        [DllImport("hb_a64", EntryPoint = "hb_count", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_count(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_job", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_job(IntPtr hbHandle, int jobIndex);

        [DllImport("hb_a64", EntryPoint = "hb_rem", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_rem(IntPtr hbHandle, IntPtr job);

        [DllImport("hb_a64", EntryPoint = "hb_start", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_start(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_pause", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_pause(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_resume", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_resume(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_stop", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_stop(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_close(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_global_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_global_close();

        [DllImport("hb_a64", EntryPoint = "hb_list_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_list_init();

        [DllImport("hb_a64", EntryPoint = "hb_list_count", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_list_count(IntPtr listPtr);

        [DllImport("hb_a64", EntryPoint = "hb_list_add", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_list_add(IntPtr listPtr, IntPtr item);

        [DllImport("hb_a64", EntryPoint = "hb_list_insert", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_list_insert(IntPtr listPtr, int pos, IntPtr item);

        [DllImport("hb_a64", EntryPoint = "hb_list_rem", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_list_rem(IntPtr listPtr, IntPtr item);

        [DllImport("hb_a64", EntryPoint = "hb_list_item", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_list_item(IntPtr listPtr, int itemIndex);

        [DllImport("hb_a64", EntryPoint = "hb_list_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_list_close(IntPtr listPtrPtr);

        [DllImport("hb_a64", EntryPoint = "hb_subtitle_can_force", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_subtitle_can_force(int source);

        [DllImport("hb_a64", EntryPoint = "hb_subtitle_can_burn", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_subtitle_can_burn(int source);

        [DllImport("hb_a64", EntryPoint = "hb_subtitle_can_pass", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_subtitle_can_pass(int source, int mux);

        [DllImport("hb_a64", EntryPoint = "hb_video_framerate_get_from_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_video_framerate_get_from_name(IntPtr name);

        [DllImport("hb_a64", EntryPoint = "hb_video_framerate_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_framerate_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_audio_samplerate_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_audio_samplerate_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_audio_samplerate_find_closest", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_samplerate_find_closest(int samplerate, uint codec);

        [DllImport("hb_a64", EntryPoint = "hb_audio_bitrate_get_best", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_bitrate_get_best(uint codec, int bitrate, int samplerate, int mixdown);

        [DllImport("hb_a64", EntryPoint = "hb_audio_bitrate_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_bitrate_get_default(uint codec, int samplerate, int mixdown);

        [DllImport("hb_a64", EntryPoint = "hb_audio_bitrate_get_limits", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_bitrate_get_limits(uint codec, int samplerate, int mixdown, ref int low, ref int high);

        [DllImport("hb_a64", EntryPoint = "hb_audio_bitrate_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_audio_bitrate_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_audio_autonaming_behavior_get_from_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_autonaming_behavior_get_from_name(IntPtr name);

        [DllImport("hb_a64", EntryPoint = "hb_audio_name_generate", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_audio_name_generate(IntPtr name, ulong layout, int mixdown, int keep_name, int behaviour);

        [DllImport("hb_a64", EntryPoint = "hb_video_quality_get_limits", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_video_quality_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction);

        [DllImport("hb_a64", EntryPoint = "hb_video_quality_get_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_quality_get_name(uint codec);

        [DllImport("hb_a64", EntryPoint = "hb_video_multipass_is_supported", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_video_multipass_is_supported(uint codec, int constant_quality);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_is_supported", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_video_encoder_is_supported(int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_audio_quality_get_limits", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_audio_quality_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction);

        [DllImport("hb_a64", EntryPoint = "hb_audio_quality_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern float hb_audio_quality_get_default(uint codec);

        [DllImport("hb_a64", EntryPoint = "hb_audio_compression_get_limits", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_audio_compression_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction);

        [DllImport("hb_a64", EntryPoint = "hb_audio_compression_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern float hb_audio_compression_get_default(uint codec);

        [DllImport("hb_a64", EntryPoint = "hb_audio_can_apply_drc", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_can_apply_drc(uint codec, uint codec_param, int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_autopassthru_get_encoder", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_autopassthru_get_encoder(int in_codec, int copy_mask, int fallback, int muxer);

        [DllImport("hb_a64", EntryPoint = "hb_audio_encoder_get_fallback_for_passthru", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_encoder_get_fallback_for_passthru(int passthru);
        
        [DllImport("hb_a64", EntryPoint = "hb_mixdown_is_supported", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_mixdown_is_supported(int mixdown, uint codec, ulong layout);

        [DllImport("hb_a64", EntryPoint = "hb_mixdown_has_codec_support", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_mixdown_has_codec_support(int mixdown, uint codec);

        [DllImport("hb_a64", EntryPoint = "hb_mixdown_has_remix_support", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_mixdown_has_remix_support(int mixdown, ulong layout);

        [DllImport("hb_a64", EntryPoint = "hb_mixdown_get_best", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_mixdown_get_best(uint codec, ulong layout, int mixdown);

        [DllImport("hb_a64", EntryPoint = "hb_mixdown_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_mixdown_get_default(uint codec, ulong layout);

        [DllImport("hb_a64", EntryPoint = "hb_mixdown_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_mixdown_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_encoder_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_video_encoder_get_default(int muxer);

        [DllImport("hb_a64", EntryPoint = "hb_audio_encoder_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_audio_encoder_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_audio_encoder_get_default", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_audio_encoder_get_default(int muxer);

        [DllImport("hb_a64", EntryPoint = "hb_container_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_container_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_presets", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_encoder_get_presets(int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_tunes", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_encoder_get_tunes(int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_profiles", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_encoder_get_profiles(int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_video_encoder_get_levels", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_encoder_get_levels(int encoder);

        [DllImport("hb_a64", EntryPoint = "lang_get_next", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr lang_get_next(IntPtr last);

        [DllImport("hb_a64", EntryPoint = "lang_for_code2", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr lang_for_code2([In] [MarshalAs(UnmanagedType.LPStr)] string code2);

        [DllImport("hb_a64", EntryPoint = "hb_job_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_job_close(IntPtr job);

        [DllImport("hb_a64", EntryPoint = "hb_filter_get_keys", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_filter_get_keys(int filter_id);

        [DllImport("hb_a64", EntryPoint = "hb_x264_encopt_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_x264_encopt_name(IntPtr name);

        [DllImport("hb_a64", EntryPoint = "hb_check_h264_level", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_check_h264_level([In] [MarshalAs(UnmanagedType.LPStr)] string level, int width, int height, int fps_num, int fps_den, int interlaced, int fake_interlaced);

        [DllImport("hb_a64", EntryPoint = "hb_x264_param_unparse", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_x264_param_unparse(
            int bit_depth,
            [In] [MarshalAs(UnmanagedType.LPStr)] string x264_preset,
            [In] [MarshalAs(UnmanagedType.LPStr)] string x264_tune,
            [In] [MarshalAs(UnmanagedType.LPStr)] string x264_encopts,
            [In] [MarshalAs(UnmanagedType.LPStr)] string x264_profile,
            [In] [MarshalAs(UnmanagedType.LPStr)] string h264_level,
            int width,
            int height);

        [DllImport("hb_a64", EntryPoint = "hb_directx_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_directx_available();

        [DllImport("hb_a64", EntryPoint = "hb_qsv_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_qsv_available();

        [DllImport("hb_a64", EntryPoint = "hb_vce_h264_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_vce_h264_available();

        [DllImport("hb_a64", EntryPoint = "hb_vce_h265_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_vce_h265_available();

        [DllImport("hb_a64", EntryPoint = "hb_nvenc_h264_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_nvenc_h264_available();

        [DllImport("hb_a64", EntryPoint = "hb_nvenc_h265_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_nvenc_h265_available();

        [DllImport("hb_a64", EntryPoint = "hb_check_nvdec_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_check_nvdec_available();

        [DllImport("hb_a64", EntryPoint = "hb_image_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_image_close(IntPtr image);

        [DllImport("hb_a64", EntryPoint = "hb_video_quality_is_supported", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_quality_is_supported(int encoder);

        [DllImport("hb_a64", EntryPoint = "hb_video_bitrate_is_supported", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_video_bitrate_is_supported(int encoder);

        /* JSON API */

        [DllImport("hb_a64", EntryPoint = "hb_get_title_set_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_title_set_json(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_job_init_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_job_init_json(IntPtr hbHandle, int title_index);

        [DllImport("hb_a64", EntryPoint = "hb_json_to_job", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_json_to_job(IntPtr hbHandle, IntPtr json_job);

        [DllImport("hb_a64", EntryPoint = "hb_add_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_add_json(IntPtr hbHandle, IntPtr json_job);

        [DllImport("hb_a64", EntryPoint = "hb_set_anamorphic_size_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_set_anamorphic_size_json(IntPtr json_param);

        [DllImport("hb_a64", EntryPoint = "hb_get_state_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_state_json(IntPtr hbHandle);

        [DllImport("hb_a64", EntryPoint = "hb_get_preview_params_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_preview_params_json(int title_idx, int preview_idx, int deinterlace, ref hb_geometry_settings_s settings);
        
        [DllImport("hb_a64", EntryPoint = "hb_get_title_coverarts", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_title_coverarts(IntPtr hbHandle, int title_index);

        [DllImport("hb_a64", EntryPoint = "hb_presets_builtin_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_presets_builtin_init();

        [DllImport("hb_a64", EntryPoint = "hb_presets_builtin_get_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_presets_builtin_get_json();

        [DllImport("hb_a64", EntryPoint = "hb_presets_read_file_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_presets_read_file_json(IntPtr filename);

        [DllImport("hb_a64", EntryPoint = "hb_presets_clean_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_presets_clean_json(IntPtr json);

        [DllImport("hb_a64", EntryPoint = "hb_presets_import_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_presets_import_json(IntPtr jsonIn, ref IntPtr jsonOutput);
        
        [DllImport("hb_a64", EntryPoint = "hb_presets_current_version", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_presets_current_version(IntPtr major, IntPtr minor, IntPtr micro);

        [DllImport("hb_a64", EntryPoint = "hb_generate_filter_settings_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_generate_filter_settings_json(
            int filter_id,
            [In] [MarshalAs(UnmanagedType.LPStr)] string preset,
            [In] [MarshalAs(UnmanagedType.LPStr)] string tune,
            [In] [MarshalAs(UnmanagedType.LPStr)] string custom);

        [DllImport("hb_a64", EntryPoint = "hb_filter_get_presets_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_filter_get_presets_json(int filter_id);

        [DllImport("hb_a64", EntryPoint = "hb_filter_get_tunes_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_filter_get_tunes_json(int filter_id);

        [DllImport("hb_a64", EntryPoint = "hb_get_cpu_platform", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_get_cpu_platform();

        [DllImport("hb_a64", EntryPoint = "hb_qsv_get_platform", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_qsv_get_platform(int adapter_index);

        [DllImport("hb_a64", EntryPoint = "hb_qsv_hyper_encode_available", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_qsv_hyper_encode_available(int adapter_index);

        [DllImport("hb_a64", EntryPoint = "hb_qsv_hardware_generation", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_qsv_hardware_generation(int cpu_platform);

        [DllImport("hb_a64", EntryPoint = "hb_qsv_get_adapter_index", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_qsv_get_adapter_index();

        [DllImport("hb_a64", EntryPoint = "hb_qsv_adapters_list", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_qsv_adapters_list();
        
        // hb_get_preview3(hb_handle_t* h, int picture, const char * job_dict)
        [DllImport("hb_a64", EntryPoint = "hb_get_preview3_json", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_preview3_json(IntPtr hbHandle, int preview_idx, [In][MarshalAs(UnmanagedType.LPStr)] string job_dict);
    }
}
