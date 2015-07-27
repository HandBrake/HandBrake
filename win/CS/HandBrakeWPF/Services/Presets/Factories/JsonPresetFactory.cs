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
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.ApplicationServices.Interop.Json.Presets;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Model;
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
        /// <param name="importedPreset">
        /// The preset.
        /// </param>
        /// <returns>
        /// The <see cref="Preset"/>.
        /// </returns>
        public static Preset ImportPreset(HBPreset importedPreset)
        {
            Preset preset = new Preset();
            preset.Name = importedPreset.PresetName;
            preset.Description = importedPreset.PresetDescription;
            preset.Task = new EncodeTask();

            // Step 1, Create the EncodeTask Object that can be loaded into the UI.

            /* Output Settings */
            preset.Task.OptimizeMP4 = importedPreset.Mp4HttpOptimize;
            preset.Task.IPod5GSupport = importedPreset.Mp4iPodCompatible;
            preset.Task.OutputFormat = GetFileFormat(importedPreset.FileFormat.Replace("file", string.Empty).Trim()); // TOOD null check.

            /* Picture Settings */
            preset.PictureSettingsMode = (PresetPictureSettingsMode)importedPreset.UsesPictureSettings;
            preset.Task.MaxWidth = importedPreset.PictureWidth.HasValue && importedPreset.PictureWidth.Value > 0 ? importedPreset.PictureWidth.Value : (int?)null;
            preset.Task.MaxHeight = importedPreset.PictureHeight.HasValue && importedPreset.PictureHeight.Value > 0 ? importedPreset.PictureHeight.Value : (int?)null;
            preset.Task.Cropping = new Cropping(importedPreset.PictureTopCrop, importedPreset.PictureBottomCrop, importedPreset.PictureLeftCrop, importedPreset.PictureRightCrop);
            preset.Task.HasCropping = !importedPreset.PictureAutoCrop;

            preset.Task.Modulus = importedPreset.PictureModulus;
            preset.Task.KeepDisplayAspect = importedPreset.PictureKeepRatio;
            switch (importedPreset.PicturePAR)
            {
                case "custom":
                    preset.Task.Anamorphic = Anamorphic.Custom;
                    break;
                case "loose":
                    preset.Task.Anamorphic = Anamorphic.Loose;
                    break;
                case "strict":
                    preset.Task.Anamorphic = Anamorphic.Strict;
                    break;
                default:
                    preset.Task.Anamorphic = Anamorphic.Loose;
                    break;
            }

            /* Filter Settings */
            preset.Task.Grayscale = importedPreset.VideoGrayScale;
            preset.Task.Deblock = importedPreset.PictureDeblock;
            switch (importedPreset.PictureDecomb)
            {
                case "custom":
                    preset.Task.Decomb = Decomb.Custom;
                    break;
                case "default":
                    preset.Task.Decomb = Decomb.Default;
                    break;
                case "bob":
                    preset.Task.Decomb = Decomb.Bob;
                    break;
                case "fast":
                    preset.Task.Decomb = Decomb.Fast;
                    break;

                default:
                    preset.Task.Decomb = Decomb.Off;
                    break;
            }

            preset.Task.CustomDecomb = importedPreset.PictureDecombCustom;

            if (!importedPreset.PictureDecombDeinterlace)
            {
                preset.Task.Decomb = Decomb.Off;
            }

            switch (importedPreset.PictureDeinterlace)
            {
                case "custom":
                    preset.Task.Deinterlace = Deinterlace.Custom;
                    break;
                case "bob":
                    preset.Task.Deinterlace = Deinterlace.Bob;
                    break;
                case "gast":
                    preset.Task.Deinterlace = Deinterlace.Fast;
                    break;
                case "slow":
                    preset.Task.Deinterlace = Deinterlace.Slow;
                    break;
                case "slower":
                    preset.Task.Deinterlace = Deinterlace.Slower;
                    break;
                default:
                    preset.Task.Deinterlace = Deinterlace.Off;
                    break;
            }

            preset.Task.CustomDeinterlace = importedPreset.PictureDetelecineCustom;
            preset.Task.CustomDenoise = importedPreset.PictureDenoiseCustom;
            preset.Task.CustomDetelecine = importedPreset.PictureDetelecineCustom;

            switch (importedPreset.PictureDetelecine)
            {
                case "custom":
                    preset.Task.Detelecine = Detelecine.Custom;
                    break;
                case "default":
                    preset.Task.Detelecine = Detelecine.Default;
                    break;
                default:
                    preset.Task.Detelecine = Detelecine.Off;
                    break;
            }

            switch (importedPreset.PictureDenoiseFilter)
            {
                case "nlmeans":
                    preset.Task.Denoise = Denoise.NLMeans;
                    break;
                case "hqdn3d":
                    preset.Task.Denoise = Denoise.hqdn3d;
                    break;
                default:
                    preset.Task.Denoise = Denoise.Off;
                    break;
            }

            switch (importedPreset.PictureDenoisePreset)
            {
                case "custom":
                    preset.Task.DenoisePreset = DenoisePreset.Custom;
                    break;
                case "light":
                    preset.Task.DenoisePreset = DenoisePreset.Light;
                    break;
                case "medium":
                    preset.Task.DenoisePreset = DenoisePreset.Medium;
                    break;
                case "strong":
                    preset.Task.DenoisePreset = DenoisePreset.Strong;
                    break;
                case "ultralight":
                    preset.Task.DenoisePreset = DenoisePreset.Ultralight;
                    break;
                case "weak":
                    preset.Task.DenoisePreset = DenoisePreset.Weak;
                    break;
            }

            switch (importedPreset.PictureDenoiseTune)
            {
                case "animation":
                    preset.Task.DenoiseTune = DenoiseTune.Animation;
                    break;
                case "film":
                    preset.Task.DenoiseTune = DenoiseTune.Film;
                    break;
                case "grain":
                    preset.Task.DenoiseTune = DenoiseTune.Grain;
                    break;
                case "highnotion":
                    preset.Task.DenoiseTune = DenoiseTune.HighMotion;
                    break;

                default:
                    preset.Task.DenoiseTune = DenoiseTune.None;
                    break;
            }

            /* Video Settings */
            preset.Task.VideoEncoder = EnumHelper<VideoEncoder>.GetValue(importedPreset.VideoEncoder);
            preset.Task.VideoBitrate = importedPreset.VideoAvgBitrate;
            preset.Task.TwoPass = importedPreset.VideoTwoPass;
            preset.Task.TurboFirstPass = importedPreset.VideoTurboTwoPass;
            preset.Task.ExtraAdvancedArguments = importedPreset.VideoOptionExtra;
            preset.Task.Quality = double.Parse(importedPreset.VideoQualitySlider.ToString(CultureInfo.InvariantCulture), CultureInfo.InvariantCulture);
            preset.Task.VideoEncodeRateType = (VideoEncodeRateType)importedPreset.VideoQualityType;
            preset.Task.VideoLevel = new VideoLevel(importedPreset.VideoLevel, importedPreset.VideoLevel);
            preset.Task.VideoPreset = new VideoPreset(importedPreset.VideoPreset, importedPreset.VideoPreset);
            preset.Task.VideoProfile = new VideoProfile(importedPreset.VideoProfile, importedPreset.VideoProfile);

            if (!string.IsNullOrEmpty(importedPreset.VideoTune))
            {
                string[] split = importedPreset.VideoTune.Split(',');
                foreach (var item in split)
                {
                    preset.Task.VideoTunes.Add(new VideoTune(item, item));
                }
            }
            preset.Task.Framerate = importedPreset.VideoFramerate == "auto" || string.IsNullOrEmpty(importedPreset.VideoFramerate)
                                 ? (double?)null
                                 : double.Parse(importedPreset.VideoFramerate, CultureInfo.InvariantCulture);
            string parsedValue = importedPreset.VideoFramerateMode;
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

            /* Audio Settings */ 
            preset.AudioTrackBehaviours = new AudioBehaviours();
            preset.Task.AllowedPassthruOptions.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetValue(importedPreset.AudioEncoderFallback);
            preset.AudioTrackBehaviours.SelectedBehaviour = importedPreset.AudioTrackSelectionBehavior == "all"
                                                                     ? AudioBehaviourModes.AllMatching
                                                                     : AudioBehaviourModes.FirstMatch;
            preset.AudioTrackBehaviours.SelectedTrackDefaultBehaviour = AudioTrackDefaultsMode.None;

            if (importedPreset.AudioCopyMask != null)
            {
                foreach (var item in importedPreset.AudioCopyMask)
                {
                    AudioEncoder encoder = EnumHelper<AudioEncoder>.GetValue(item.ToString());
                    switch (encoder)
                    {
                        case AudioEncoder.AacPassthru:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.Ac3Passthrough:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.EAc3Passthrough:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.DtsHDPassthrough:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.DtsPassthrough:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.FlacPassthru:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.Mp3Passthru:
                            preset.Task.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                    }
                }
            }

            if (importedPreset.AudioLanguageList != null)
            {
                foreach (var item in importedPreset.AudioLanguageList)
                {
                    preset.AudioTrackBehaviours.SelectedLangauges.Add(item);
                }
            }

            preset.Task.AudioTracks = new ObservableCollection<AudioTrack>();

            if (importedPreset.AudioList != null)
            {
                foreach (var audioTrack in importedPreset.AudioList)
                {
                    AudioTrack track = new AudioTrack();
                    track.Bitrate = audioTrack.AudioBitrate;

                    // track.CompressionLevel = audioTrack.AudioCompressionLevel;
                    // track.AudioDitherMethod = audioTrack.AudioDitherMethod;
                    track.Encoder = EnumHelper<AudioEncoder>.GetValue(audioTrack.AudioEncoder);  
                    track.MixDown = EnumHelper<Mixdown>.GetValue(audioTrack.AudioMixdown); 

                    // track.AudioNormalizeMixLevel = audioTrack.AudioNormalizeMixLevel;
                    track.SampleRate = audioTrack.AudioSamplerate == "auto" ? 0 : double.Parse(audioTrack.AudioSamplerate);

                    // track.IsQualityBased = audioTrack.AudioTrackQualityEnable;
                    // track.Quality = audioTrack.AudioTrackQuality;
                    track.Gain = (int)audioTrack.AudioTrackGainSlider;
                    track.DRC = audioTrack.AudioTrackDRCSlider;

                    preset.Task.AudioTracks.Add(track);
                }
            }


            /* Subtitle Settings */ 
            preset.SubtitleTrackBehaviours = new SubtitleBehaviours();

            // parsedPreset.SubtitleTrackBehaviours.SelectedBehaviour = preset.SubtitleTrackSelectionBehavior;
            preset.SubtitleTrackBehaviours.AddClosedCaptions = importedPreset.SubtitleAddCC;
            preset.SubtitleTrackBehaviours.AddForeignAudioScanTrack = importedPreset.SubtitleAddForeignAudioSearch;
            if (importedPreset.SubtitleLanguageList != null)
            {
                foreach (var item in importedPreset.SubtitleLanguageList)
                {
                    preset.SubtitleTrackBehaviours.SelectedLangauges.Add(item);
                }
            }

            /* Chapter Marker Settings */ 
            preset.Task.IncludeChapterMarkers = importedPreset.ChapterMarkers;

            /* Advanced Settings */ 
            preset.Task.ShowAdvancedTab = importedPreset.x264UseAdvancedOptions;
            preset.Task.AdvancedEncoderOptions = importedPreset.x264Option;

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

            return preset;
        }

        /// <summary>
        /// The export preset.
        /// </summary>
        /// <param name="export">
        /// The export.
        /// </param>
        /// <param name="config">
        /// HandBrakes configuration options.
        /// </param>
        /// <returns>
        /// The <see cref="Preset"/>.
        /// </returns>
        public static PresetTransportContainer ExportPreset(Preset export, HBConfiguration config)
        {
            PresetTransportContainer container = new PresetTransportContainer();
            container.VersionMajor = "0";
            container.VersionMinor = "10";
            container.VersionMicro = "2";
            container.PresetList = new List<HBPreset> { CreateHbPreset(export, config) };

            return container;
        }

        /// <summary>
        /// The create hb preset.
        /// </summary>
        /// <param name="export">
        /// The export.
        /// </param>
        /// <param name="config">HandBrakes current configuration</param>
        /// <returns>
        /// The <see cref="HBPreset"/>.
        /// </returns>
        private static HBPreset CreateHbPreset(Preset export, HBConfiguration config)
        {
            HBPreset preset = new HBPreset();

            // Preset
            preset.PresetDescription = export.Description;
            preset.PresetName = export.Name;
            preset.Type = 1; // User Preset
            preset.UsesPictureSettings = (int)export.PictureSettingsMode;
            preset.Default = false; // TODO Can other GUI's handle this?

            // Audio
            preset.AudioCopyMask = export.Task.AllowedPassthruOptions.AllowedPassthruOptions.Select(EnumHelper<AudioEncoder>.GetShortName).ToList();
            preset.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetShortName(export.Task.AllowedPassthruOptions.AudioEncoderFallback);
            preset.AudioLanguageList = LanguageUtilities.GetLanguageCodes(export.AudioTrackBehaviours.SelectedLangauges);
            preset.AudioTrackSelectionBehavior = EnumHelper<AudioBehaviourModes>.GetShortName(export.AudioTrackBehaviours.SelectedBehaviour);
            preset.AudioSecondaryEncoderMode = false; // TODO -> Check what this is.
            preset.AudioList = new List<AudioList>();
            foreach (var item in export.Task.AudioTracks)
            {
                AudioList track = new AudioList
                                      {
                                          AudioBitrate = item.Bitrate,
                                          AudioCompressionLevel = 0, // TODO
                                          AudioDitherMethod = null,  // TODO
                                          AudioEncoder = EnumHelper<AudioEncoder>.GetShortName(item.Encoder),
                                          AudioMixdown = EnumHelper<Mixdown>.GetShortName(item.MixDown),
                                          AudioNormalizeMixLevel = false, // TODO
                                          AudioSamplerate = item.SampleRate == 0 ? "auto" : item.SampleRate.ToString(),  // TODO check formatting.
                                          AudioTrackDRCSlider = item.DRC,
                                          AudioTrackGainSlider = item.Gain,
                                          AudioTrackQuality = item.Quality ?? 0,
                                          AudioTrackQualityEnable = item.Quality.HasValue && item.IsQualityVisible
                                      };
               
                preset.AudioList.Add(track);
            }
            

            // Subtitles
            preset.SubtitleAddCC = export.SubtitleTrackBehaviours.AddClosedCaptions;
            preset.SubtitleAddForeignAudioSearch = export.SubtitleTrackBehaviours.AddForeignAudioScanTrack;
            preset.SubtitleBurnBDSub = false; // TODO not supported yet.
            preset.SubtitleBurnDVDSub = false; // TODO not supported yet.
            preset.SubtitleBurnBehavior = EnumHelper<SubtitleBurnInBehaviourModes>.GetShortName(export.SubtitleTrackBehaviours.SelectedBurnInBehaviour);
            preset.SubtitleLanguageList = LanguageUtilities.GetLanguageCodes(export.SubtitleTrackBehaviours.SelectedLangauges);
            preset.SubtitleTrackSelectionBehavior = EnumHelper<SubtitleBehaviourModes>.GetShortName(export.SubtitleTrackBehaviours.SelectedBehaviour);

            // Chapters
            preset.ChapterMarkers = export.Task.IncludeChapterMarkers;

            // Output Settings
            preset.FileFormat = EnumHelper<OutputFormat>.GetShortName(export.Task.OutputFormat);
            preset.Mp4HttpOptimize = export.Task.OptimizeMP4;
            preset.Mp4iPodCompatible = export.Task.IPod5GSupport;

            // Picture Settings
            preset.PictureForceHeight = 0; // TODO
            preset.PictureForceWidth = 0; // TODO
            preset.PictureHeight = preset.UsesPictureSettings >= 1 ? export.Task.MaxHeight : 0; // TODO; // TODO
            preset.PictureItuPAR = false; // TODO Not supported Yet
            preset.PictureKeepRatio = export.Task.KeepDisplayAspect;
            preset.PictureLeftCrop = export.Task.Cropping.Left;
            preset.PictureLooseCrop = false; // TODO Not Supported Yet
            preset.PictureModulus = export.Task.Modulus ?? 16;
            preset.PicturePAR = EnumHelper<Anamorphic>.GetShortName(export.Task.Anamorphic);
            preset.PicturePARHeight = export.Task.PixelAspectY;
            preset.PicturePARWidth = export.Task.PixelAspectX; 
            preset.PictureRightCrop = export.Task.Cropping.Right;
            preset.PictureRotate = 0; // TODO Not supported yet.
            preset.PictureTopCrop = export.Task.Cropping.Top;
            preset.PictureWidth = preset.UsesPictureSettings >= 1 ? export.Task.MaxWidth : 0; // TODO
            preset.PictureDARWidth = export.Task.DisplayWidth.HasValue ? (int)export.Task.DisplayWidth.Value : 0;
            preset.PictureAutoCrop = export.Task.HasCropping;
            preset.PictureBottomCrop = export.Task.Cropping.Bottom;

            // Filters
            preset.PictureDeblock = export.Task.Deblock;
            preset.PictureDecomb = EnumHelper<Decomb>.GetShortName(export.Task.Decomb);
            preset.PictureDecombCustom = export.Task.CustomDecomb;
            preset.PictureDecombDeinterlace = export.Task.Decomb != Decomb.Off;
            preset.PictureDeinterlace = EnumHelper<Deinterlace>.GetShortName(export.Task.Deinterlace);
            preset.PictureDeinterlaceCustom = export.Task.CustomDeinterlace;
            preset.PictureDenoiseCustom = export.Task.CustomDenoise;
            preset.PictureDenoiseFilter = EnumHelper<Denoise>.GetShortName(export.Task.Denoise);
            preset.PictureDenoisePreset = EnumHelper<DenoisePreset>.GetShortName(export.Task.DenoisePreset);
            preset.PictureDenoiseTune = EnumHelper<DenoiseTune>.GetShortName(export.Task.DenoiseTune); 
            preset.PictureDetelecine = EnumHelper<Detelecine>.GetShortName(export.Task.Detelecine);
            preset.PictureDetelecineCustom = export.Task.CustomDetelecine;

            // Video
            preset.VideoEncoder = EnumHelper<VideoEncoder>.GetShortName(export.Task.VideoEncoder);
            preset.VideoFramerate = export.Task.Framerate.ToString();
            preset.VideoFramerateMode = EnumHelper<FramerateMode>.GetShortName(export.Task.FramerateMode);
            preset.VideoGrayScale = export.Task.Grayscale;
            preset.VideoHWDecode = false;
            preset.VideoLevel = export.Task.VideoLevel.ShortName;
            preset.VideoOptionExtra = export.Task.ExtraAdvancedArguments;
            preset.VideoPreset = export.Task.VideoPreset.ShortName;
            preset.VideoProfile = export.Task.VideoProfile.ShortName;
            preset.VideoQSVAsyncDepth = 4; // Defaulted to 4 for now.
            preset.VideoQSVDecode = !config.DisableQuickSyncDecoding;
            preset.VideoQualitySlider = export.Task.Quality.HasValue ? export.Task.Quality.Value : 0;
            preset.VideoQualityType = (int)export.Task.VideoEncodeRateType;
            preset.VideoScaler = EnumHelper<VideoScaler>.GetShortName(config.ScalingMode);
            preset.VideoTune = export.Task.VideoTunes.Aggregate(string.Empty, (current, item) => !string.IsNullOrEmpty(current) ? string.Format("{0}, {1}", current, item.ShortName) : item.ShortName);
            preset.VideoAvgBitrate = export.Task.VideoBitrate ?? 0;
            preset.VideoColorMatrixCode = 0; // TODO not supported.
            preset.VideoTurboTwoPass = export.Task.TurboFirstPass;
            preset.VideoTwoPass = export.Task.TwoPass;

            // Advanced
            preset.x264Option = export.Task.AdvancedEncoderOptions;
            preset.x264UseAdvancedOptions = export.Task.ShowAdvancedTab;

            // Unknown
            preset.ChildrenArray = new List<object>(); // TODO
            preset.Folder = false; // TODO
            preset.FolderOpen = false; // TODO

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
        private static OutputFormat GetFileFormat(string format)
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
