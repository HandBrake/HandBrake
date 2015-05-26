// --------------------------------------------------------------------------------------------------------------------
// <copyright file="JsonPresetFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The json preset factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Factories
{
    using System.Collections.ObjectModel;
    using System.Globalization;

    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models.Video;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The json preset factory.
    /// </summary>
    public class JsonPresetFactory
    {
        /// <summary>
        /// The create preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <returns>
        /// The <see cref="Preset"/>.
        /// </returns>
        public static Preset ImportPreset(HandBrake.ApplicationServices.Interop.Json.Presets.HBPreset preset)
        {
            EncodeTask task = new EncodeTask();
            Preset parsedPreset = new Preset();
            parsedPreset.Name = preset.PresetName;
            parsedPreset.Description = preset.PresetDescription;
            parsedPreset.UsePictureFilters = preset.UsesPictureFilters;
            parsedPreset.PictureSettingsMode = (PresetPictureSettingsMode)preset.UsesPictureSettings;
            parsedPreset.UseDeinterlace = preset.PictureDecombDeinterlace;
            parsedPreset.Task = new EncodeTask();

            // Step 1, Create the EncodeTask Object that can be loaded into the UI.

            /* Output Settings */ 
            task.OptimizeMP4 = preset.Mp4HttpOptimize;
            task.IPod5GSupport = preset.Mp4iPodCompatible;
            task.OutputFormat = GetFileFormat(preset.FileFormat.Replace("file", string.Empty).Trim()); // TOOD null check.

            /* Picture Settings */ 
            task.Cropping = new Cropping(preset.PictureTopCrop, preset.PictureBottomCrop, preset.PictureLeftCrop, preset.PictureRightCrop);
            task.HasCropping = preset.PictureAutoCrop;
            task.Width = preset.PictureWidth;
            task.Height = preset.PictureHeight;
            task.Modulus = preset.PictureModulus;
            task.KeepDisplayAspect = preset.PictureKeepRatio;
            switch (preset.PicturePAR)
            {
                case "custom":
                    task.Anamorphic = Anamorphic.Custom;
                    break;
                case "loose":
                    task.Anamorphic = Anamorphic.Loose;
                    break;
                case "strict":
                    task.Anamorphic = Anamorphic.Strict;
                    break;
                default:
                    task.Anamorphic = Anamorphic.Loose;
                    break;
            }

            /* Filter Settings */ 
            task.Grayscale = preset.VideoGrayScale;
            task.Deblock = preset.PictureDeblock;
            switch (preset.PictureDecomb)
            {
                case "custom":
                    task.Decomb = Decomb.Custom;
                    break;
                case "default":
                    task.Decomb = Decomb.Default;
                    break;
                case "bob":
                    task.Decomb = Decomb.Bob;
                    break;
                case "fast":
                    task.Decomb = Decomb.Fast;
                    break;

                default:
                    task.Decomb = Decomb.Off;
                    break;
            }

            task.CustomDecomb = preset.PictureDecombCustom;

            switch (preset.PictureDeinterlace)
            {
                case "custom":
                    task.Deinterlace = Deinterlace.Custom;
                    break;
                case "bob":
                    task.Deinterlace = Deinterlace.Bob;
                    break;
                case "gast":
                    task.Deinterlace = Deinterlace.Fast;
                    break;
                case "slow":
                    task.Deinterlace = Deinterlace.Slow;
                    break;
                case "slower":
                    task.Deinterlace = Deinterlace.Slower;
                    break;
                default:
                    task.Deinterlace = Deinterlace.Off;
                    break;
            }

            task.CustomDeinterlace = preset.PictureDetelecineCustom;
            task.CustomDenoise = preset.PictureDenoiseCustom;
            task.CustomDetelecine = preset.PictureDetelecineCustom;

            switch (preset.PictureDetelecine)
            {
                case "custom":
                    task.Detelecine = Detelecine.Custom;
                    break;
                case "default":
                    task.Detelecine = Detelecine.Default;
                    break;
                default:
                    task.Detelecine = Detelecine.Off;
                    break;
            }

            switch (preset.PictureDenoiseFilter)
            {
                case "nlmeans":
                    task.Denoise = Denoise.NLMeans;
                    break;
                case "hqdn3d":
                    task.Denoise = Denoise.hqdn3d;
                    break;
                default:
                    task.Denoise = Denoise.Off;
                    break;
            }

            switch (preset.PictureDenoisePreset)
            {
                case "custom":
                    task.DenoisePreset = DenoisePreset.Custom;
                    break;
                case "light":
                    task.DenoisePreset = DenoisePreset.Light;
                    break;
                case "medium":
                    task.DenoisePreset = DenoisePreset.Medium;
                    break;
                case "strong":
                    task.DenoisePreset = DenoisePreset.Strong;
                    break;
                case "ultralight":
                    task.DenoisePreset = DenoisePreset.Ultralight;
                    break;
                case "weak":
                    task.DenoisePreset = DenoisePreset.Weak;
                    break;
            }

            switch (preset.PictureDenoiseTune)
            {
                case "animation":
                    task.DenoiseTune = DenoiseTune.Animation;
                    break;
                case "film":
                    task.DenoiseTune = DenoiseTune.Film;
                    break;
                case "grain":
                    task.DenoiseTune = DenoiseTune.Grain;
                    break;
                case "highnotion":
                    task.DenoiseTune = DenoiseTune.HighMotion;
                    break;

                default:
                    task.DenoiseTune = DenoiseTune.None;
                    break;
            }

            /* Video Settings */ 
            task.VideoEncoder = EnumHelper<VideoEncoder>.GetValue(preset.VideoEncoder);
            task.VideoBitrate = preset.VideoAvgBitrate;
            task.TwoPass = preset.VideoTwoPass;
            task.TurboFirstPass = preset.VideoTurboTwoPass;
            task.ExtraAdvancedArguments = preset.VideoOptionExtra;
            task.Quality = double.Parse(preset.VideoQualitySlider.ToString(CultureInfo.InvariantCulture), CultureInfo.InvariantCulture);
            task.VideoEncodeRateType = (VideoEncodeRateType)preset.VideoQualityType;
            task.VideoLevel = new VideoLevel(preset.VideoLevel, preset.VideoLevel);
            task.VideoPreset = new VideoPreset(preset.VideoPreset, preset.VideoPreset);
            string[] split = preset.VideoTune.Split(',');
            foreach (var item in split)
            {
                task.VideoTunes.Add(new VideoTune(item, item));
            }
            task.Framerate = preset.VideoFramerate == "auto" || string.IsNullOrEmpty(preset.VideoFramerate)
                                 ? (double?)null
                                 : double.Parse(preset.VideoFramerate, CultureInfo.InvariantCulture);
            string parsedValue = preset.VideoFramerateMode;
            switch (parsedValue)
            {
                case "vfr":
                    task.FramerateMode = FramerateMode.VFR;
                    break;
                case "cfr":
                    task.FramerateMode = FramerateMode.CFR;
                    break;
                default:
                    task.FramerateMode = FramerateMode.PFR;
                    break;
            }

            /* Audio Settings */ 
            parsedPreset.AudioTrackBehaviours = new AudioBehaviours();
            task.AllowedPassthruOptions.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetValue(preset.AudioEncoderFallback);
            parsedPreset.AudioTrackBehaviours.SelectedBehaviour = preset.AudioTrackSelectionBehavior == "all"
                                                                     ? AudioBehaviourModes.AllMatching
                                                                     : preset.AudioTrackSelectionBehavior == "first"
                                                                           ? AudioBehaviourModes.FirstMatch
                                                                           : AudioBehaviourModes.None;

            if (preset.AudioCopyMask != null)
            {
                foreach (var item in preset.AudioCopyMask)
                {
                    AudioEncoder encoder = EnumHelper<AudioEncoder>.GetValue(item.ToString());
                    switch (encoder)
                    {
                        case AudioEncoder.AacPassthru:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.Ac3Passthrough:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.EAc3Passthrough:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.DtsHDPassthrough:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.DtsPassthrough:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.FlacPassthru:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.Mp3Passthru:
                            task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                    }
                }
            }

            if (preset.AudioLanguageList != null)
            {
                foreach (var item in preset.AudioLanguageList)
                {
                    parsedPreset.AudioTrackBehaviours.SelectedLangauges.Add(item);
                }
            }

            task.AudioTracks = new ObservableCollection<AudioTrack>();

            if (preset.AudioList != null)
            {
                foreach (var audioTrack in preset.AudioList)
                {
                    AudioTrack track = new AudioTrack();
                    track.Bitrate = audioTrack.AudioBitrate;

                    // track.CompressionLevel = audioTrack.AudioCompressionLevel;
                    // track.AudioDitherMethod = audioTrack.AudioDitherMethod;
                    track.Encoder = EnumHelper<AudioEncoder>.GetValue(audioTrack.AudioEncoder);
                    track.MixDown = Converters.GetAudioMixDown(audioTrack.AudioMixdown);

                    // track.AudioNormalizeMixLevel = audioTrack.AudioNormalizeMixLevel;
                    track.SampleRate = audioTrack.AudioSamplerate == "auto" ? 0 : double.Parse(audioTrack.AudioSamplerate);

                    // track.IsQualityBased = audioTrack.AudioTrackQualityEnable;
                    // track.Quality = audioTrack.AudioTrackQuality;
                    track.Gain = (int)audioTrack.AudioTrackGainSlider;
                    track.DRC = audioTrack.AudioTrackDRCSlider;

                    task.AudioTracks.Add(track);
                }
            }


            /* Subtitle Settings */ 
            parsedPreset.SubtitleTrackBehaviours = new SubtitleBehaviours();

            // parsedPreset.SubtitleTrackBehaviours.SelectedBehaviour = preset.SubtitleTrackSelectionBehavior;
            parsedPreset.SubtitleTrackBehaviours.AddClosedCaptions = preset.SubtitleAddCC;
            parsedPreset.SubtitleTrackBehaviours.AddForeignAudioScanTrack = preset.SubtitleAddForeignAudioSearch;
            if (preset.SubtitleLanguageList != null)
            {
                foreach (var item in preset.SubtitleLanguageList)
                {
                    parsedPreset.SubtitleTrackBehaviours.SelectedLangauges.Add(item);
                }
            }

            /* Chapter Marker Settings */ 
            task.IncludeChapterMarkers = preset.ChapterMarkers;

            /* Advanced Settings */ 
            task.ShowAdvancedTab = preset.x264UseAdvancedOptions;
            task.AdvancedEncoderOptions = preset.x264Option;

            /* Not Supported Yet */ 
            // public int VideoColorMatrixCode { get; set; }
            // public bool VideoHWDecode { get; set; }
            // public string VideoScaler { get; set; }
            // public bool VideoQSVDecode { get; set; }
            // public int VideoQSVAsyncDepth { get; set; }
            // public bool SubtitleAddForeignAudioSubtitle { get; set; }
            // public string SubtitleBurnBehavior { get; set; }
            // public bool SubtitleBurnBDSub { get; set; }
            // public bool SubtitleBurnDVDSub { get; set; }
            // public bool PictureItuPAR { get; set; }
            // public bool PictureLooseCrop { get; set; }
            // public int PicturePARWidth { get; set; }
            // public int PicturePARHeight { get; set; }
            // public int PictureRotate { get; set; }
            // public int PictureForceHeight { get; set; }
            // public int PictureForceWidth { get; set; }
            // public bool AudioSecondaryEncoderMode { get; set; }
            // public List<object> ChildrenArray { get; set; }
            // public bool Default { get; set; }
            // public bool Folder { get; set; }
            // public bool FolderOpen { get; set; }
            // public int PictureDARWidth { get; set; }
            // public int Type { get; set; }

            return parsedPreset;
        }

        /// <summary>
        /// The export preset.
        /// </summary>
        /// <param name="export">
        /// The export.
        /// </param>
        /// <returns>
        /// The <see cref="Preset"/>.
        /// </returns>
        public HandBrake.ApplicationServices.Interop.Json.Presets.HBPreset ExportPreset(Preset export)
        {
            HandBrake.ApplicationServices.Interop.Json.Presets.HBPreset preset = new HandBrake.ApplicationServices.Interop.Json.Presets.HBPreset();

            return preset;
        }

        /// <summary>
        /// Get the OutputFormat Enum for a given string
        /// </summary>
        /// <param name="format">
        /// OutputFormat as a string
        /// </param>
        /// <returns>
        /// An OutputFormat Enum
        /// </returns>
        public static OutputFormat GetFileFormat(string format)
        {
            switch (format.ToLower())
            {
                default:
                    return OutputFormat.Mp4;
                case "m4v":
                    return OutputFormat.Mp4;
                case "mkv":
                    return OutputFormat.Mkv;
            }
        }
    }
}
