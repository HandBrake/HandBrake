// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PlistPresetFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Factory to translate a Plist object into a Preset.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Factories
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

    /// <summary>
    /// A Factory to translate a Plist object into a Preset.
    /// </summary>
    public class PlistPresetFactory
    {
        /// <summary>
        /// The create preset.
        /// </summary>
        /// <param name="plist">
        /// The plist.
        /// </param>
        /// <returns>
        /// The <see cref="Preset"/>.
        /// </returns>
        public static Preset CreatePreset(PList plist)
        {
            Preset preset = new Preset { Task = new EncodeTask(), Category = PresetService.UserPresetCatgoryName };

            // Parse the settings out.
            foreach (var item in plist)
            {
                if (item.Key == "AudioList")
                {
                    List<AudioTrack> tracks = ParseAudioTracks(item.Value);
                    preset.Task.AudioTracks = new ObservableCollection<AudioTrack>(tracks);
                }
                else
                {
                    ParseSetting(item, preset);
                }
            }

            // Handle the PictureDecombDeinterlace key
            if (preset.UseDeinterlace)
            {
                preset.Task.Decomb = Decomb.Off;
                preset.Task.CustomDecomb = string.Empty;
            }

            // Depending on the selected preset options, we may need to change some settings around.
            // If the user chose not to use fitlers, remove them.
            if (!preset.UsePictureFilters)
            {
                preset.Task.Detelecine = Detelecine.Off;
                preset.Task.Denoise = Denoise.Off;
                preset.Task.Deinterlace = Deinterlace.Off;
                preset.Task.Decomb = Decomb.Off;
                preset.Task.Deblock = 0;
                preset.Task.Grayscale = false;
            }

            // IF we are using Source Max, Set the Max Width / Height values.
            if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
            {
                preset.Task.MaxWidth = preset.Task.Height;
                preset.Task.MaxHeight = preset.Task.Width;
            }


            return preset;
        }

        /// <summary>
        /// Parse a setting and set it in the given preset.
        /// </summary>
        /// <param name="kvp">
        /// The kvp setting pair.
        /// </param>
        /// <param name="preset">
        /// The preset object.
        /// </param>
        private static void ParseSetting(KeyValuePair<string, dynamic> kvp, Preset preset)
        {
            switch (kvp.Key)
            {
                // Output Settings
                case "FileFormat":
                    preset.Task.OutputFormat = Converters.GetFileFormat(kvp.Value.Replace("file", string.Empty).Trim());
                    break;
                case "Mp4HttpOptimize":
                    preset.Task.OptimizeMP4 = kvp.Value == 1;
                    break;
                case "Mp4LargeFile":
                    preset.Task.LargeFile = kvp.Value == 1;
                    break;
                case "Mp4iPodCompatible":
                    preset.Task.IPod5GSupport = kvp.Value == 1;
                    break;

                // Picture Settings
                case "PictureAutoCrop":
                    preset.Task.HasCropping = kvp.Value != 1;
                    break;
                case "PictureTopCrop":
                    preset.Task.Cropping.Top = kvp.Value;
                    break;
                case "PictureBottomCrop":
                    preset.Task.Cropping.Bottom = kvp.Value;
                    break;
                case "PictureLeftCrop":
                    preset.Task.Cropping.Left = kvp.Value;
                    break;
                case "PictureRightCrop":
                    preset.Task.Cropping.Right = kvp.Value;
                    break;
                case "PictureHeight":
                    preset.Task.Height = kvp.Value == null || kvp.Value == 0 ? null : kvp.Value;
                    break;
                case "PictureWidth":
                    preset.Task.Width = kvp.Value == null || kvp.Value == 0 ? null : kvp.Value;
                    break;
                case "PictureKeepRatio":
                    preset.Task.KeepDisplayAspect = kvp.Value == 1;
                    break;
                case "PicturePAR":
                    preset.Task.Anamorphic = (Anamorphic)kvp.Value;
                    break;
                case "PictureModulus":
                    preset.Task.Modulus = kvp.Value;
                    break;

                // Filters
                case "PictureDeblock":
                    preset.Task.Deblock = kvp.Value;
                    break;
                case "PictureDecomb":
                    preset.Task.Decomb = (Decomb)kvp.Value;
                    break;
                case "PictureDecombCustom":
                    preset.Task.CustomDecomb = kvp.Value;
                    break;
                case "PictureDecombDeinterlace":
                    preset.UseDeinterlace = kvp.Value == 1;
                    break;
                case "PictureDeinterlace":
                    preset.Task.Deinterlace = (Deinterlace)kvp.Value;
                    break;
                case "PictureDeinterlaceCustom":
                    preset.Task.CustomDeinterlace = kvp.Value;
                    break;
                case "PictureDenoise":
                    preset.Task.Denoise = (Denoise)kvp.Value;
                    break;
                case "PictureDenoiseCustom":
                    preset.Task.CustomDenoise = kvp.Value;
                    break;
                case "PictureDetelecine":
                    preset.Task.Detelecine = (Detelecine)kvp.Value;
                    break;
                case "PictureDetelecineCustom":
                    preset.Task.CustomDetelecine = kvp.Value;
                    break;

                // Video Tab
                case "VideoAvgBitrate":
                    if (!string.IsNullOrEmpty(kvp.Value))
                    {
                        preset.Task.VideoBitrate = int.Parse(kvp.Value);
                    }
                    break;
                case "VideoEncoder":
                    preset.Task.VideoEncoder = EnumHelper<VideoEncoder>.GetValue(kvp.Value);
                    break;
                case "VideoFramerate":
                    preset.Task.Framerate = kvp.Value == "Same as source" || string.IsNullOrEmpty(kvp.Value) ? null : double.Parse(kvp.Value, CultureInfo.InvariantCulture);
                    break;
                case "VideoFramerateMode":
                    string parsedValue = kvp.Value;
                    switch (parsedValue)
                    {
                        case "vfr":
                            preset.Task.FramerateMode = FramerateMode.VFR;
                            break;
                        case "cfr":
                            preset.Task.FramerateMode = FramerateMode.CFR;
                            break;
                        default:
                            preset.Task.FramerateMode = FramerateMode.PFR;
                            break;
                    }
                    break;
                case "VideoGrayScale":
                    preset.Task.Grayscale = kvp.Value == 1;
                    break;
                case "VideoQualitySlider":
                    preset.Task.Quality = double.Parse(kvp.Value.ToString(), CultureInfo.InvariantCulture);
                    break;
                case "VideoQualityType": // The Type of Quality Mode used
                    preset.Task.VideoEncodeRateType = (VideoEncodeRateType)kvp.Value;
                    break;
                case "VideoTurboTwoPass":
                    preset.Task.TurboFirstPass = kvp.Value == 1;
                    break;
                case "VideoTwoPass":
                    preset.Task.TwoPass = kvp.Value == 1;
                    break;
                case "x264OptionExtra":
                    preset.Task.AdvancedEncoderOptions = kvp.Value;
                    break;
                case "x264Preset":
                    preset.Task.X264Preset = EnumHelper<x264Preset>.GetValue(kvp.Value, true);
                    break;
                case "h264Profile":
                    preset.Task.H264Profile = EnumHelper<x264Profile>.GetValue(kvp.Value, true);
                    break;
                case "x264Tune":
                    string value = kvp.Value;
                    if (value.Contains("fastdecode"))
                    {
                        preset.Task.FastDecode = true;
                        value = value.Replace("fastdecode", string.Empty).Replace(",", string.Empty);
                    }
                    preset.Task.X264Tune = EnumHelper<x264Tune>.GetValue(value, true);
                    break;
                case "h264Level":
                    preset.Task.H264Level = kvp.Value;
                    break;

                // Chapter Markers Tab
                case "ChapterMarkers":
                    preset.Task.IncludeChapterMarkers = kvp.Value == 1;
                    break;

                // Advanced x264 tab
                case "x264Option":
                    if (preset.Task.VideoEncoder == VideoEncoder.X264) // TODO Temp fix until a better solution is found.
                    {
                        preset.Task.AdvancedEncoderOptions = kvp.Value;
                    }
                    break;

                case "lavcOption":
                    if (preset.Task.VideoEncoder != VideoEncoder.X264) // TODO Temp fix until a better solution is found.
                    {
                        preset.Task.AdvancedEncoderOptions = kvp.Value;
                    }
                    break;

                // Preset Information
                case "PresetBuildNumber":
                    preset.Version = kvp.Value;
                    break;
                case "PresetDescription":
                    preset.Description = kvp.Value;
                    break;
                case "PresetName":
                    preset.Name = kvp.Value;
                    break;
                case "Type":
                    //preset.Task.Type = kvp.Value; // TODO find out what this is
                    break;
                case "UsesMaxPictureSettings":
                    // TODO Not sure if this is used now!?
                    break;
                case "UsesPictureFilters":
                    preset.UsePictureFilters = kvp.Value == 1;
                    break;
                case "UsesPictureSettings":
                    preset.PictureSettingsMode = (PresetPictureSettingsMode)kvp.Value;
                    break;

                // Allowed Passthru
                case "AudioAllowAACPass":
                    preset.Task.AllowedPassthruOptions.AudioAllowAACPass = kvp.Value == 1;
                    break;
                case "AudioAllowAC3Pass":
                    preset.Task.AllowedPassthruOptions.AudioAllowAC3Pass = kvp.Value == 1;
                    break;
                case "AudioAllowDTSHDPass":
                    preset.Task.AllowedPassthruOptions.AudioAllowDTSHDPass = kvp.Value == 1;
                    break;
                case "AudioAllowDTSPass":
                    preset.Task.AllowedPassthruOptions.AudioAllowDTSPass = kvp.Value == 1;
                    break;
                case "AudioAllowMP3Pass":
                    preset.Task.AllowedPassthruOptions.AudioAllowMP3Pass = kvp.Value == 1;
                    break;
                case "AudioEncoderFallback":
                    preset.Task.AllowedPassthruOptions.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetValue(kvp.Value);
                    break;
            }
        }

        /// <summary>
        /// Parse a number of audio tracks
        /// </summary>
        /// <param name="audioList">
        /// The audio list.
        /// </param>
        /// <returns>
        /// The <see cref="List"/> of audio tracks
        /// </returns>
        private static List<AudioTrack> ParseAudioTracks(IEnumerable<dynamic> audioList)
        {
            return audioList.Select(item => ParseAudioTrackParameters(item)).Cast<AudioTrack>().ToList();
        }

        /// <summary>
        /// Parse an audio track's parameters.
        /// </summary>
        /// <param name="audioTrack">
        /// The audio track params
        /// </param>
        /// <returns>
        /// An <see cref="AudioTrack"/> Object
        /// </returns>
        private static AudioTrack ParseAudioTrackParameters(Dictionary<string, dynamic> audioTrack)
        {
            AudioTrack track = new AudioTrack();
            foreach (var item in audioTrack)
            {
                switch (item.Key)
                {
                    case "AudioBitrate":
                        track.Bitrate = int.Parse(item.Value);
                        break;
                    case "AudioEncoder":
                        track.Encoder = Converters.GetAudioEncoder(item.Value.Trim());
                        break;
                    case "AudioMixdown":
                        track.MixDown = Converters.GetAudioMixDown(item.Value.Trim());
                        break;
                    case "AudioSamplerate":
                        track.SampleRate = item.Value == "Auto" ? 0 : double.Parse(item.Value, CultureInfo.InvariantCulture);
                        break;
                    case "AudioTrack":
                        //track.SourceTrack = value; We don't do anything with this one.
                        break;
                    case "AudioTrackDRCSlider":
                        track.DRC = double.Parse(item.Value.ToString(), CultureInfo.InvariantCulture);
                        break;
                    case "AudioTrackGainSlider":
                        track.Gain = int.Parse(item.Value.ToString(), CultureInfo.InvariantCulture);
                        break;
                }
            }

            return track;
        }
    }
}
