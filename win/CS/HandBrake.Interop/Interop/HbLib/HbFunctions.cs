// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBFunctions64.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HBFunctions type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Contains p-invoke function declarations to hblib.
    /// </summary>
    internal static class HBFunctions
    {
        private static bool IsArmDevice;

        static HBFunctions()
        {
            IsArmDevice = RuntimeInformation.ProcessArchitecture == Architecture.Arm64;
        }

        public static void hb_register_logger(LoggingCallback callback)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_register_logger(callback);
            else
                HBFunctions64.hb_register_logger(callback);
        }

        public static void hb_register_error_handler(LoggingCallback callback)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_register_error_handler(callback);
            else
                HBFunctions64.hb_register_error_handler(callback);
        }

        public static int hb_global_init()
        {
            return IsArmDevice ? HbFunctionsArm.hb_global_init() : HBFunctions64.hb_global_init();
        }

        public static int hb_global_init_no_hardware()
        {
            return IsArmDevice ? HbFunctionsArm.hb_global_init_no_hardware() : HBFunctions64.hb_global_init_no_hardware();
        }

        public static IntPtr hb_init(int verbose, int update_check)
        {
            return IsArmDevice ? HbFunctionsArm.hb_init(verbose, update_check) : HBFunctions64.hb_init(verbose, update_check);
        }

        public static IntPtr hb_init_dl(int verbose, int update_check)
        {
            return IsArmDevice ? HbFunctionsArm.hb_init_dl(verbose, update_check) : HBFunctions64.hb_init_dl(verbose, update_check);
        }

        public static IntPtr hb_get_version(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_version(hbHandle) : HBFunctions64.hb_get_version(hbHandle);
        }

        public static int hb_get_build(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_build(hbHandle) : HBFunctions64.hb_get_build(hbHandle);
        }

        public static IntPtr hb_dvd_name(IntPtr path)
        {
            return IsArmDevice ? HbFunctionsArm.hb_dvd_name(path) : HBFunctions64.hb_dvd_name(path);
        }

        public static void hb_dvd_set_dvdnav(int enable)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_dvd_set_dvdnav(enable);
            else
                HBFunctions64.hb_dvd_set_dvdnav(enable);
        }

        public static void hb_scan(IntPtr hbHandle, IntPtr path, int title_index, int preview_count, int store_previews, ulong min_duration, ulong max_duration, int crop_auto_switch_threshold, int crop_median_threshold, IntPtr exclude_extensions, int hw_decode, int keep_duplicate_titles)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_scan(hbHandle, path, title_index, preview_count, store_previews, min_duration, max_duration, crop_auto_switch_threshold, crop_median_threshold, exclude_extensions, hw_decode, keep_duplicate_titles);
            else
                HBFunctions64.hb_scan(hbHandle, path, title_index, preview_count, store_previews, min_duration, max_duration, crop_auto_switch_threshold, crop_median_threshold, exclude_extensions, hw_decode, keep_duplicate_titles);
        }

        public static void hb_scan_stop(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_scan_stop(hbHandle);
            else
                HBFunctions64.hb_scan_stop(hbHandle);
        }

        public static IntPtr hb_get_titles(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_titles(hbHandle) : HBFunctions64.hb_get_titles(hbHandle);
        }

        public static void hb_set_anamorphic_size2(ref hb_geometry_s sourceGeometry, ref hb_geometry_settings_s uiGeometry, ref hb_geometry_s result)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref result);
            else
                HBFunctions64.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref result);
        }

        public static IntPtr hb_rotate_geometry(ref hb_geometry_crop_s geo, ref hb_geometry_crop_s result, int angle, int hflip)
        {
            return IsArmDevice ? HbFunctionsArm.hb_rotate_geometry(ref geo, ref result, angle, hflip) : HBFunctions64.hb_rotate_geometry(ref geo, ref result, angle, hflip);
        }

        public static int hb_count(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_count(hbHandle) : HBFunctions64.hb_count(hbHandle);
        }

        public static IntPtr hb_job(IntPtr hbHandle, int jobIndex)
        {
            return IsArmDevice ? HbFunctionsArm.hb_job(hbHandle, jobIndex) : HBFunctions64.hb_job(hbHandle, jobIndex);
        }

        public static void hb_rem(IntPtr hbHandle, IntPtr job)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_rem(hbHandle, job);
            else
                HBFunctions64.hb_rem(hbHandle, job);
        }

        public static void hb_start(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_start(hbHandle);
            else
                HBFunctions64.hb_start(hbHandle);
        }

        public static void hb_pause(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_pause(hbHandle);
            else
                HBFunctions64.hb_pause(hbHandle);
        }

        public static void hb_resume(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_resume(hbHandle);
            else
                HBFunctions64.hb_resume(hbHandle);
        }

        public static void hb_stop(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_stop(hbHandle);
            else
                HBFunctions64.hb_stop(hbHandle);
        }

        public static void hb_close(IntPtr hbHandle)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_close(hbHandle);
            else
                HBFunctions64.hb_close(hbHandle);
        }

        public static void hb_global_close()
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_global_close();
            else
                HBFunctions64.hb_global_close();
        }

        public static IntPtr hb_list_init()
        {
            return IsArmDevice ? HbFunctionsArm.hb_list_init() : HBFunctions64.hb_list_init();
        }

        public static int hb_list_count(IntPtr listPtr)
        {
            return IsArmDevice ? HbFunctionsArm.hb_list_count(listPtr) : HBFunctions64.hb_list_count(listPtr);
        }

        public static void hb_list_add(IntPtr listPtr, IntPtr item)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_list_add(listPtr, item);
            else
                HBFunctions64.hb_list_add(listPtr, item);
        }

        public static void hb_list_insert(IntPtr listPtr, int pos, IntPtr item)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_list_insert(listPtr, pos, item);
            else
                HBFunctions64.hb_list_insert(listPtr, pos, item);
        }

        public static void hb_list_rem(IntPtr listPtr, IntPtr item)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_list_rem(listPtr, item);
            else
                HBFunctions64.hb_list_rem(listPtr, item);
        }

        public static IntPtr hb_list_item(IntPtr listPtr, int itemIndex)
        {
            return IsArmDevice ? HbFunctionsArm.hb_list_item(listPtr, itemIndex) : HBFunctions64.hb_list_item(listPtr, itemIndex);
        }

        public static void hb_list_close(IntPtr listPtrPtr)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_list_close(listPtrPtr);
            else
                HBFunctions64.hb_list_close(listPtrPtr);
        }

        public static int hb_subtitle_can_force(int source)
        {
            return IsArmDevice ? HbFunctionsArm.hb_subtitle_can_force(source) : HBFunctions64.hb_subtitle_can_force(source);
        }

        public static int hb_subtitle_can_burn(int source)
        {
            return IsArmDevice ? HbFunctionsArm.hb_subtitle_can_burn(source) : HBFunctions64.hb_subtitle_can_burn(source);
        }

        public static int hb_subtitle_can_pass(int source, int mux)
        {
            return IsArmDevice ? HbFunctionsArm.hb_subtitle_can_pass(source, mux) : HBFunctions64.hb_subtitle_can_pass(source, mux);
        }

        public static int hb_video_framerate_get_from_name(IntPtr name)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_framerate_get_from_name(name) : HBFunctions64.hb_video_framerate_get_from_name(name);
        }

        public static IntPtr hb_video_framerate_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_framerate_get_next(last) : HBFunctions64.hb_video_framerate_get_next(last);
        }

        public static IntPtr hb_audio_samplerate_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_samplerate_get_next(last) : HBFunctions64.hb_audio_samplerate_get_next(last);
        }

        public static int hb_audio_samplerate_find_closest(int samplerate, uint codec)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_samplerate_find_closest(samplerate, codec) : HBFunctions64.hb_audio_samplerate_find_closest(samplerate, codec);
        }

        public static int hb_audio_bitrate_get_best(uint codec, int bitrate, int samplerate, int mixdown)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_bitrate_get_best(codec, bitrate, samplerate, mixdown) : HBFunctions64.hb_audio_bitrate_get_best(codec, bitrate, samplerate, mixdown);
        }

        public static int hb_audio_bitrate_get_default(uint codec, int samplerate, int mixdown)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_bitrate_get_default(codec, samplerate, mixdown) : HBFunctions64.hb_audio_bitrate_get_default(codec, samplerate, mixdown);
        }

        public static int hb_audio_bitrate_get_limits(uint codec, int samplerate, int mixdown, ref int low, ref int high)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_bitrate_get_limits(codec, samplerate, mixdown, ref low, ref high) : HBFunctions64.hb_audio_bitrate_get_limits(codec, samplerate, mixdown, ref low, ref high);
        }

        public static IntPtr hb_audio_bitrate_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_bitrate_get_next(last) : HBFunctions64.hb_audio_bitrate_get_next(last);
        }

        public static void hb_video_quality_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_video_quality_get_limits(codec, ref low, ref high, ref granularity, ref direction);
            else
                HBFunctions64.hb_video_quality_get_limits(codec, ref low, ref high, ref granularity, ref direction);
        }

        public static IntPtr hb_video_quality_get_name(uint codec)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_quality_get_name(codec) : HBFunctions64.hb_video_quality_get_name(codec);
        }

        public static int hb_video_multipass_is_supported(uint codec, int constant_quality)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_multipass_is_supported(codec, constant_quality) : HBFunctions64.hb_video_multipass_is_supported(codec, constant_quality);
        }

        public static int hb_video_encoder_is_supported(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_is_supported(encoder) : HBFunctions64.hb_video_encoder_is_supported(encoder);
        }

        public static void hb_audio_quality_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_audio_quality_get_limits(codec, ref low, ref high, ref granularity, ref direction);
            else
                HBFunctions64.hb_audio_quality_get_limits(codec, ref low, ref high, ref granularity, ref direction);
        }

        public static float hb_audio_quality_get_default(uint codec)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_quality_get_default(codec) : HBFunctions64.hb_audio_quality_get_default(codec);
        }

        public static void hb_audio_compression_get_limits(uint codec, ref float low, ref float high, ref float granularity, ref int direction)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_audio_compression_get_limits(codec, ref low, ref high, ref granularity, ref direction);
            else
                HBFunctions64.hb_audio_compression_get_limits(codec, ref low, ref high, ref granularity, ref direction);
        }

        public static float hb_audio_compression_get_default(uint codec)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_compression_get_default(codec) : HBFunctions64.hb_audio_compression_get_default(codec);
        }

        public static int hb_audio_can_apply_drc(uint codec, uint codec_param, int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_can_apply_drc(codec, codec_param, encoder) : HBFunctions64.hb_audio_can_apply_drc(codec, codec_param, encoder);
        }

        public static int hb_autopassthru_get_encoder(int in_codec, int copy_mask, int fallback, int muxer)
        {
            return IsArmDevice ? HbFunctionsArm.hb_autopassthru_get_encoder(in_codec, copy_mask, fallback, muxer) : HBFunctions64.hb_autopassthru_get_encoder(in_codec, copy_mask, fallback, muxer);
        }

        public static int hb_audio_encoder_get_fallback_for_passthru(int passthru)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_encoder_get_fallback_for_passthru(passthru) : HBFunctions64.hb_audio_encoder_get_fallback_for_passthru(passthru);
        }

        public static int hb_mixdown_is_supported(int mixdown, uint codec, ulong layout)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_is_supported(mixdown, codec, layout) : HBFunctions64.hb_mixdown_is_supported(mixdown, codec, layout);
        }

        public static int hb_mixdown_has_codec_support(int mixdown, uint codec)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_has_codec_support(mixdown, codec) : HBFunctions64.hb_mixdown_has_codec_support(mixdown, codec);
        }

        public static int hb_mixdown_has_remix_support(int mixdown, ulong layout)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_has_remix_support(mixdown, layout) : HBFunctions64.hb_mixdown_has_remix_support(mixdown, layout);
        }

        public static int hb_mixdown_get_best(uint codec, ulong layout, int mixdown)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_get_best(codec, layout, mixdown) : HBFunctions64.hb_mixdown_get_best(codec, layout, mixdown);
        }

        public static int hb_mixdown_get_default(uint codec, ulong layout)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_get_default(codec, layout) : HBFunctions64.hb_mixdown_get_default(codec, layout);
        }

        public static IntPtr hb_mixdown_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_mixdown_get_next(last) : HBFunctions64.hb_mixdown_get_next(last);
        }

        public static IntPtr hb_video_encoder_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_next(last) : HBFunctions64.hb_video_encoder_get_next(last);
        }

        public static int hb_video_encoder_get_default(int muxer)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_default(muxer) : HBFunctions64.hb_video_encoder_get_default(muxer);
        }

        public static IntPtr hb_audio_encoder_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_encoder_get_next(last) : HBFunctions64.hb_audio_encoder_get_next(last);
        }

        public static int hb_audio_encoder_get_default(int muxer)
        {
            return IsArmDevice ? HbFunctionsArm.hb_audio_encoder_get_default(muxer) : HBFunctions64.hb_audio_encoder_get_default(muxer);
        }

        public static IntPtr hb_container_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.hb_container_get_next(last) : HBFunctions64.hb_container_get_next(last);
        }




        public static IntPtr hb_video_encoder_get_presets(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_presets(encoder) : HBFunctions64.hb_video_encoder_get_presets(encoder);
        }

        public static IntPtr hb_video_encoder_get_tunes(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_tunes(encoder) : HBFunctions64.hb_video_encoder_get_tunes(encoder);
        }

        public static IntPtr hb_video_encoder_get_profiles(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_profiles(encoder) : HBFunctions64.hb_video_encoder_get_profiles(encoder);
        }

        public static IntPtr hb_video_encoder_get_levels(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_encoder_get_levels(encoder) : HBFunctions64.hb_video_encoder_get_levels(encoder);
        }

        public static IntPtr lang_get_next(IntPtr last)
        {
            return IsArmDevice ? HbFunctionsArm.lang_get_next(last) : HBFunctions64.lang_get_next(last);
        }

        public static IntPtr lang_for_code2(string code2)
        {
            return IsArmDevice ? HbFunctionsArm.lang_for_code2(code2) : HBFunctions64.lang_for_code2(code2);
        }

        public static void hb_job_close(IntPtr job)
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_job_close(job);
            else
                HBFunctions64.hb_job_close(job);
        }

        public static IntPtr hb_filter_get_keys(int filter_id)
        {
            return IsArmDevice ? HbFunctionsArm.hb_filter_get_keys(filter_id) : HBFunctions64.hb_filter_get_keys(filter_id);
        }

        public static IntPtr hb_x264_encopt_name(IntPtr name)
        {
            return IsArmDevice ? HbFunctionsArm.hb_x264_encopt_name(name) : HBFunctions64.hb_x264_encopt_name(name);
        }

        public static int hb_check_h264_level(string level, int width, int height, int fps_num, int fps_den, int interlaced, int fake_interlaced)
        {
            return IsArmDevice ? HbFunctionsArm.hb_check_h264_level(level, width, height, fps_num, fps_den, interlaced, fake_interlaced)
                               : HBFunctions64.hb_check_h264_level(level, width, height, fps_num, fps_den, interlaced, fake_interlaced);
        }

        public static IntPtr hb_x264_param_unparse(
            int bit_depth,
            string x264_preset,
            string x264_tune,
            string x264_encopts,
            string x264_profile,
            string h264_level,
            int width,
            int height)
        {
            return IsArmDevice ? HbFunctionsArm.hb_x264_param_unparse(bit_depth, x264_preset, x264_tune, x264_encopts, x264_profile, h264_level, width, height)
                               : HBFunctions64.hb_x264_param_unparse(bit_depth, x264_preset, x264_tune, x264_encopts, x264_profile, h264_level, width, height);
        }

        public static int hb_directx_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_directx_available() : HBFunctions64.hb_directx_available();
        }

        public static int hb_qsv_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_available() : HBFunctions64.hb_qsv_available();
        }

        public static int hb_vce_h264_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_vce_h264_available() : HBFunctions64.hb_vce_h264_available();
        }

        public static int hb_vce_h265_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_vce_h265_available() : HBFunctions64.hb_vce_h265_available();
        }

        public static int hb_nvenc_h264_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_nvenc_h264_available() : HBFunctions64.hb_nvenc_h264_available();
        }

        public static int hb_nvenc_h265_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_nvenc_h265_available() : HBFunctions64.hb_nvenc_h265_available();
        }

        public static int hb_check_nvdec_available()
        {
            return IsArmDevice ? HbFunctionsArm.hb_check_nvdec_available() : HBFunctions64.hb_check_nvdec_available();
        }

        public static IntPtr hb_image_close(IntPtr image)
        {
            return IsArmDevice ? HbFunctionsArm.hb_image_close(image) : HBFunctions64.hb_image_close(image);
        }

        public static IntPtr hb_video_quality_is_supported(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_quality_is_supported(encoder) : HBFunctions64.hb_video_quality_is_supported(encoder);
        }

        public static IntPtr hb_video_bitrate_is_supported(int encoder)
        {
            return IsArmDevice ? HbFunctionsArm.hb_video_bitrate_is_supported(encoder) : HBFunctions64.hb_video_bitrate_is_supported(encoder);
        }

        // JSON API methods

        public static IntPtr hb_get_title_set_json(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_title_set_json(hbHandle) : HBFunctions64.hb_get_title_set_json(hbHandle);
        }

        public static IntPtr hb_job_init_json(IntPtr hbHandle, int title_index)
        {
            return IsArmDevice ? HbFunctionsArm.hb_job_init_json(hbHandle, title_index) : HBFunctions64.hb_job_init_json(hbHandle, title_index);
        }

        public static IntPtr hb_json_to_job(IntPtr hbHandle, IntPtr json_job)
        {
            return IsArmDevice ? HbFunctionsArm.hb_json_to_job(hbHandle, json_job) : HBFunctions64.hb_json_to_job(hbHandle, json_job);
        }

        public static int hb_add_json(IntPtr hbHandle, IntPtr json_job)
        {
            return IsArmDevice ? HbFunctionsArm.hb_add_json(hbHandle, json_job) : HBFunctions64.hb_add_json(hbHandle, json_job);
        }

        public static IntPtr hb_set_anamorphic_size_json(IntPtr json_param)
        {
            return IsArmDevice ? HbFunctionsArm.hb_set_anamorphic_size_json(json_param) : HBFunctions64.hb_set_anamorphic_size_json(json_param);
        }

        public static IntPtr hb_get_state_json(IntPtr hbHandle)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_state_json(hbHandle) : HBFunctions64.hb_get_state_json(hbHandle);
        }

        public static IntPtr hb_get_preview_params_json(int title_idx, int preview_idx, int deinterlace, ref hb_geometry_settings_s settings)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_preview_params_json(title_idx, preview_idx, deinterlace, ref settings)
                               : HBFunctions64.hb_get_preview_params_json(title_idx, preview_idx, deinterlace, ref settings);
        }

        public static IntPtr hb_get_title_coverarts(IntPtr hbHandle, int title_index)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_title_coverarts(hbHandle, title_index) : HBFunctions64.hb_get_title_coverarts(hbHandle, title_index);
        }

        public static void hb_presets_builtin_init()
        {
            if (IsArmDevice)
                HbFunctionsArm.hb_presets_builtin_init();
            else
                HBFunctions64.hb_presets_builtin_init();
        }

        public static IntPtr hb_presets_builtin_get_json()
        {
            return IsArmDevice ? HbFunctionsArm.hb_presets_builtin_get_json() : HBFunctions64.hb_presets_builtin_get_json();
        }

        public static IntPtr hb_presets_read_file_json(IntPtr filename)
        {
            return IsArmDevice ? HbFunctionsArm.hb_presets_read_file_json(filename) : HBFunctions64.hb_presets_read_file_json(filename);
        }

        public static IntPtr hb_presets_clean_json(IntPtr json)
        {
            return IsArmDevice ? HbFunctionsArm.hb_presets_clean_json(json) : HBFunctions64.hb_presets_clean_json(json);
        }

        public static int hb_presets_import_json(IntPtr jsonIn, ref IntPtr jsonOutput)
        {
            return IsArmDevice ? HbFunctionsArm.hb_presets_import_json(jsonIn, ref jsonOutput) : HBFunctions64.hb_presets_import_json(jsonIn, ref jsonOutput);
        }

        public static IntPtr hb_presets_current_version(IntPtr major, IntPtr minor, IntPtr micro)
        {
            return IsArmDevice ? HbFunctionsArm.hb_presets_current_version(major, minor, micro) : HBFunctions64.hb_presets_current_version(major, minor, micro);
        }

        public static IntPtr hb_generate_filter_settings_json(int filter_id, string preset, string tune, string custom)
        {
            return IsArmDevice ? HbFunctionsArm.hb_generate_filter_settings_json(filter_id, preset, tune, custom)
                               : HBFunctions64.hb_generate_filter_settings_json(filter_id, preset, tune, custom);
        }

        public static IntPtr hb_filter_get_presets_json(int filter_id)
        {
            return IsArmDevice ? HbFunctionsArm.hb_filter_get_presets_json(filter_id) : HBFunctions64.hb_filter_get_presets_json(filter_id);
        }

        public static IntPtr hb_filter_get_tunes_json(int filter_id)
        {
            return IsArmDevice ? HbFunctionsArm.hb_filter_get_tunes_json(filter_id) : HBFunctions64.hb_filter_get_tunes_json(filter_id);
        }

        public static int hb_get_cpu_platform()
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_cpu_platform() : HBFunctions64.hb_get_cpu_platform();
        }

        public static int hb_qsv_get_platform(int adapter_index)
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_get_platform(adapter_index) : HBFunctions64.hb_qsv_get_platform(adapter_index);
        }

        public static int hb_qsv_hyper_encode_available(int adapter_index)
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_hyper_encode_available(adapter_index) : HBFunctions64.hb_qsv_hyper_encode_available(adapter_index);
        }

        public static int hb_qsv_hardware_generation(int cpu_platform)
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_hardware_generation(cpu_platform) : HBFunctions64.hb_qsv_hardware_generation(cpu_platform);
        }

        public static int hb_qsv_get_adapter_index()
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_get_adapter_index() : HBFunctions64.hb_qsv_get_adapter_index();
        }

        public static IntPtr hb_qsv_adapters_list()
        {
            return IsArmDevice ? HbFunctionsArm.hb_qsv_adapters_list() : HBFunctions64.hb_qsv_adapters_list();
        }

        public static IntPtr hb_get_preview3_json(IntPtr hbHandle, int preview_idx, string job_dict)
        {
            return IsArmDevice ? HbFunctionsArm.hb_get_preview3_json(hbHandle, preview_idx, job_dict)
                               : HBFunctions64.hb_get_preview3_json(hbHandle, preview_idx, job_dict);
        }
    }
}
