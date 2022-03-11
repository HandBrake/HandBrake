﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.Model;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;
    using HandBrake.Interop.Interop.Interfaces.Model.Presets;
    using HandBrake.Interop.Interop.Json.Presets;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Model.Video;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;

    using AudioEncoder = Encode.Model.Models.AudioEncoder;
    using AudioTrack = Encode.Model.Models.AudioTrack;
    using DenoisePreset = Encode.Model.Models.DenoisePreset;
    using DenoiseTune = Encode.Model.Models.DenoiseTune;
    using EncodeTask = Encode.Model.EncodeTask;
    using FramerateMode = Encode.Model.Models.FramerateMode;
    using OutputFormat = Encode.Model.Models.OutputFormat;
    using VideoEncoder = HandBrakeWPF.Model.Video.VideoEncoder;
    using VideoEncodeRateType = HandBrakeWPF.Model.Video.VideoEncodeRateType;
    using VideoLevel = Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = Encode.Model.Models.Video.VideoProfile;
    using VideoTune = Encode.Model.Models.Video.VideoTune;

    public class JsonPresetFactory
    {
        public static Preset ImportPreset(HBPreset importedPreset)
        {
            Preset preset = new Preset();
            preset.Name = importedPreset.PresetName;
            preset.Description = importedPreset.PresetDescription;
            preset.Task = new EncodeTask();
            preset.IsDefault = importedPreset.Default;
            preset.IsBuildIn = importedPreset.Type == 0;

            // Step 1, Create the EncodeTask Object that can be loaded into the UI.

            /* Output Settings */
            preset.Task.OptimizeMP4 = importedPreset.Mp4HttpOptimize;
            preset.Task.IPod5GSupport = importedPreset.Mp4iPodCompatible;
            preset.Task.OutputFormat = GetFileFormat(importedPreset.FileFormat.Replace("file", string.Empty).Trim());
            preset.Task.AlignAVStart = importedPreset.AlignAVStart;
            preset.Task.MetaData.PassthruMetadataEnabled = importedPreset.MetadataPassthrough;

            /* Picture Settings */
            preset.Task.MaxWidth = importedPreset.PictureWidth.HasValue && importedPreset.PictureWidth.Value > 0 ? importedPreset.PictureWidth.Value : (int?)null;
            preset.Task.MaxHeight = importedPreset.PictureHeight.HasValue && importedPreset.PictureHeight.Value > 0 ? importedPreset.PictureHeight.Value : (int?)null;
            preset.Task.Cropping = new Cropping(importedPreset.PictureTopCrop, importedPreset.PictureBottomCrop, importedPreset.PictureLeftCrop, importedPreset.PictureRightCrop);
            preset.Task.HasCropping = !importedPreset.PictureAutoCrop;
            preset.Task.KeepDisplayAspect = importedPreset.PictureKeepRatio;
            preset.Task.AllowUpscaling = importedPreset.PictureAllowUpscaling;
            preset.Task.OptimalSize = importedPreset.PictureUseMaximumSize;
            preset.Task.Padding = new PaddingFilter();
            preset.Task.Padding.Set(importedPreset.PicturePadTop, importedPreset.PicturePadBottom, importedPreset.PicturePadLeft, importedPreset.PicturePadRight, importedPreset.PicturePadColor, importedPreset.PicturePadMode);
            
            switch (importedPreset.PicturePAR)
            {
                case "custom":
                    preset.Task.Anamorphic = Anamorphic.Custom;
                    preset.Task.DisplayWidth = importedPreset.PictureDARWidth;
                    preset.Task.PixelAspectX = importedPreset.PicturePARWidth == 0 ? 1 : importedPreset.PicturePARWidth;
                    preset.Task.PixelAspectY = importedPreset.PicturePARHeight == 0 ? 1 : importedPreset.PicturePARHeight;
                    break;
                case "auto":
                case "loose":
                    preset.Task.Anamorphic = Anamorphic.Automatic;
                    break;
                default:
                    preset.Task.Anamorphic = Anamorphic.None;
                    break;
            }

            /* Filter Settings */
            preset.Task.Grayscale = importedPreset.VideoGrayScale;

            if (!string.IsNullOrEmpty(importedPreset.PictureColorspacePreset))
            {
                preset.Task.Colourspace = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_COLORSPACE).FirstOrDefault(s => s.ShortName == importedPreset.PictureColorspacePreset));
                preset.Task.CustomColourspace = importedPreset.PictureColorspaceCustom;
            }
            else
            {
                preset.Task.Colourspace = new FilterPreset("Off", "off");
            }
            
            if (!string.IsNullOrEmpty(importedPreset.PictureChromaSmoothPreset))
            {
                preset.Task.ChromaSmooth = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_CHROMA_SMOOTH).FirstOrDefault(s => s.ShortName == importedPreset.PictureChromaSmoothPreset));
                preset.Task.ChromaSmoothTune = new FilterTune(HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_CHROMA_SMOOTH).FirstOrDefault(s => s.ShortName == importedPreset.PictureChromaSmoothTune));
                preset.Task.CustomChromaSmooth = importedPreset.PictureChromaSmoothCustom;
            }
            else
            {
                preset.Task.ChromaSmooth = new FilterPreset("Off", "off");
            }

            if (!string.IsNullOrEmpty(importedPreset.PictureDeblockPreset))
            {
                preset.Task.DeblockPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DEBLOCK).FirstOrDefault(s => s.ShortName == importedPreset.PictureDeblockPreset));
            }
            else
            {
                preset.Task.DeblockPreset = new FilterPreset("Off", "off");
            }

            if (!string.IsNullOrEmpty(importedPreset.PictureDeblockTune))
            {
                preset.Task.DeblockTune = new FilterTune(HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_DEBLOCK).FirstOrDefault(s => s.ShortName == importedPreset.PictureDeblockTune));
            }
            else
            {
                preset.Task.DeblockTune = new FilterTune("Off", "off");
            }
           
            preset.Task.CustomDeblock = importedPreset.PictureDeblockCustom;

            if (importedPreset.PictureSharpenFilter != null)
            {
                preset.Task.Sharpen = EnumHelper<Sharpen>.GetValue(importedPreset.PictureSharpenFilter);
                hb_filter_ids filterId = hb_filter_ids.HB_FILTER_INVALID;
                switch (preset.Task.Sharpen)
                {
                    case Sharpen.LapSharp:
                        filterId = hb_filter_ids.HB_FILTER_LAPSHARP;
                        break;
                    case Sharpen.UnSharp:
                        filterId = hb_filter_ids.HB_FILTER_UNSHARP;
                        break;
                }

                if (filterId != hb_filter_ids.HB_FILTER_INVALID)
                {
                    preset.Task.SharpenPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)filterId).FirstOrDefault(s => s.ShortName == importedPreset.PictureSharpenPreset));
                    preset.Task.SharpenTune = new FilterTune(HandBrakeFilterHelpers.GetFilterTunes((int)filterId).FirstOrDefault(s => s.ShortName == importedPreset.PictureSharpenTune));
                    preset.Task.SharpenCustom = importedPreset.PictureSharpenCustom;
                }
                else
                {
                    // Default Values.
                    preset.Task.SharpenPreset = new FilterPreset("Medium", "medium");
                    preset.Task.SharpenTune = new FilterTune("None", "none");
                    preset.Task.SharpenCustom = string.Empty;
                }
            }

            switch (importedPreset.PictureDeinterlaceFilter)
            {
                case "decomb":
                    preset.Task.DeinterlaceFilter = DeinterlaceFilter.Decomb;
                    break;
                case "yadif":
                    preset.Task.DeinterlaceFilter = DeinterlaceFilter.Yadif;
                    break;
                case "bwdif":
                    preset.Task.DeinterlaceFilter = DeinterlaceFilter.Bwdif;
                    break;
                default:
                    preset.Task.DeinterlaceFilter = DeinterlaceFilter.Off;
                    break;
            }

            if (preset.Task.DeinterlaceFilter == DeinterlaceFilter.Decomb)
            {
                List<HBPresetTune> filterPresets = HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DECOMB);
                HBPresetTune presetTune = filterPresets.FirstOrDefault(f => f.ShortName == importedPreset.PictureDeinterlacePreset);
                preset.Task.DeinterlacePreset = presetTune ?? new HBPresetTune("Default", "default");
                preset.Task.CustomDeinterlaceSettings = importedPreset.PictureDeinterlaceCustom;
            }

            if (preset.Task.DeinterlaceFilter == DeinterlaceFilter.Yadif)
            {
                List<HBPresetTune> filterPresets = HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_YADIF);
                HBPresetTune presetTune = filterPresets.FirstOrDefault(f => f.ShortName == importedPreset.PictureDeinterlacePreset);
                preset.Task.DeinterlacePreset = presetTune ?? new HBPresetTune("Default", "default");
                preset.Task.CustomDeinterlaceSettings = importedPreset.PictureDeinterlaceCustom;
            }

            if (preset.Task.DeinterlaceFilter == DeinterlaceFilter.Bwdif)
            {
                List<HBPresetTune> filterPresets = HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_BWDIF);
                HBPresetTune presetTune = filterPresets.FirstOrDefault(f => f.ShortName == importedPreset.PictureDeinterlacePreset);
                preset.Task.DeinterlacePreset = presetTune ?? new HBPresetTune("Default", "default");
                preset.Task.CustomDeinterlaceSettings = importedPreset.PictureDeinterlaceCustom;
            }

            if (preset.Task.DeinterlaceFilter != DeinterlaceFilter.Off)
            {
                switch (importedPreset.PictureCombDetectPreset)
                {
                    case "off":
                        preset.Task.CombDetect = CombDetect.Off;
                        break;
                    case "custom":
                        preset.Task.CombDetect = CombDetect.Custom;
                        break;
                    case "default":
                        preset.Task.CombDetect = CombDetect.Default;
                        break;
                    case "permissive":
                        preset.Task.CombDetect = CombDetect.LessSensitive;
                        break;
                    case "fast":
                        preset.Task.CombDetect = CombDetect.Fast;
                        break;
                    default:
                        preset.Task.CombDetect = CombDetect.Off;
                        break;
                }
            }

            preset.Task.CustomDenoise = importedPreset.PictureDenoiseCustom;
            preset.Task.CustomDetelecine = importedPreset.PictureDetelecineCustom;
            preset.Task.CustomCombDetect = importedPreset.PictureCombDetectCustom;

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
                case "highmotion":
                    preset.Task.DenoiseTune = DenoiseTune.HighMotion;
                    break;
                case "tape":
                    preset.Task.DenoiseTune = DenoiseTune.Tape;
                    break;
                case "sprite":
                    preset.Task.DenoiseTune = DenoiseTune.Sprite;
                    break;

                default:
                    preset.Task.DenoiseTune = DenoiseTune.None;
                    break;
            }

            // Rotation and Flip
            if (!string.IsNullOrEmpty(importedPreset.PictureRotate))
            {
                string[] rotation = importedPreset.PictureRotate.Split(':');
                if (rotation.Length == 2)
                {
                    int rotate;
                    if (int.TryParse(rotation[0], out rotate))
                    {
                        preset.Task.Rotation = int.Parse(rotation[0]);
                        preset.Task.FlipVideo = rotation[1] == "1";
                    }
                }
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
                    preset.Task.VideoTunes.Add(new VideoTune(item?.Trim(), item?.Trim()));
                }
            }

            if (importedPreset.VideoFramerate == "auto" || importedPreset.VideoFramerate == "Same as source" || string.IsNullOrEmpty(importedPreset.VideoFramerate))
            {
                preset.Task.Framerate = null;
            }
            else
            {
                double parsedFramerate;
                if (double.TryParse(importedPreset.VideoFramerate, NumberStyles.Any, CultureInfo.CurrentCulture, out parsedFramerate) || double.TryParse(importedPreset.VideoFramerate, NumberStyles.Any, CultureInfo.InvariantCulture, out parsedFramerate))
                {
                    preset.Task.Framerate = parsedFramerate;
                }              
            }

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
            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetValue(importedPreset.AudioEncoderFallback);
            preset.AudioTrackBehaviours.SelectedBehaviour = importedPreset.AudioTrackSelectionBehavior == "all"
                                                                     ? AudioBehaviourModes.AllMatching
                                                                     : AudioBehaviourModes.FirstMatch;

            preset.AudioTrackBehaviours.SelectedTrackDefaultBehaviour = importedPreset.AudioSecondaryEncoderMode ? AudioTrackDefaultsMode.FirstTrack : AudioTrackDefaultsMode.AllTracks;

            if (importedPreset.AudioCopyMask != null)
            {
                preset.AudioTrackBehaviours.AllowedPassthruOptions.SetFalse();
                foreach (var item in importedPreset.AudioCopyMask)
                {
                    AudioEncoder encoder = EnumHelper<AudioEncoder>.GetValue(item);
                    switch (encoder)
                    {
                        case AudioEncoder.AacPassthru:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowAACPass = true;
                            break;
                        case AudioEncoder.Ac3Passthrough:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowAC3Pass = true;
                            break;
                        case AudioEncoder.EAc3Passthrough:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowEAC3Pass = true;
                            break;
                        case AudioEncoder.DtsHDPassthrough:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowDTSHDPass = true;
                            break;
                        case AudioEncoder.DtsPassthrough:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowDTSPass = true;
                            break;
                        case AudioEncoder.FlacPassthru:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowFlacPass = true;
                            break;
                        case AudioEncoder.Mp2Passthru:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowMP2Pass = true;
                            break;
                        case AudioEncoder.Mp3Passthru:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowMP3Pass = true;
                            break;
                        case AudioEncoder.TrueHDPassthrough:
                            preset.AudioTrackBehaviours.AllowedPassthruOptions.AudioAllowTrueHDPass = true;
                            break;
                    }
                }
            }

            if (importedPreset.AudioLanguageList != null)
            {
                IList<string> names = LanguageUtilities.GetLanguageNames(importedPreset.AudioLanguageList);
                foreach (var name in names)
                {
                    preset.AudioTrackBehaviours.SelectedLanguages.Add(name);
                }
            }

            preset.Task.AudioTracks = new ObservableCollection<AudioTrack>();

            if (importedPreset.AudioList != null)
            {
                foreach (var audioTrack in importedPreset.AudioList)
                {
                    AudioBehaviourTrack track = new AudioBehaviourTrack(EnumHelper<AudioEncoder>.GetValue(importedPreset.AudioEncoderFallback));
                    
                    // track.CompressionLevel = audioTrack.AudioCompressionLevel;
                    // track.AudioDitherMethod = audioTrack.AudioDitherMethod;
                    if (audioTrack.AudioEncoder == "ca_aac")
                    {
                        audioTrack.AudioEncoder = "av_aac"; // No Core Audio support on windows.
                    }

                    track.Encoder = EnumHelper<AudioEncoder>.GetValue(audioTrack.AudioEncoder);
                    track.MixDown = HandBrakeEncoderHelpers.GetMixdown(audioTrack.AudioMixdown);
                    track.Bitrate = audioTrack.AudioBitrate;

                    // track.AudioNormalizeMixLevel = audioTrack.AudioNormalizeMixLevel;

                    if ("auto".Equals(audioTrack.AudioSamplerate))
                    {
                        track.SampleRate = 0;
                    }
                    else if (!string.IsNullOrEmpty(audioTrack.AudioSamplerate))
                    {
                        double sampleRate = 0;
                        if (double.TryParse(audioTrack.AudioSamplerate, NumberStyles.Any, CultureInfo.InvariantCulture, out sampleRate))
                        {
                            track.SampleRate = sampleRate;
                        }
                    }

                    track.EncoderRateType = audioTrack.AudioTrackQualityEnable ? AudioEncoderRateType.Quality : AudioEncoderRateType.Bitrate;
                    track.Quality = audioTrack.AudioTrackQuality;
                    track.Gain = (int)audioTrack.AudioTrackGainSlider;
                    track.DRC = audioTrack.AudioTrackDRCSlider;

                    preset.AudioTrackBehaviours.BehaviourTracks.Add(track);
                }
            }

            /* Subtitle Settings */
            preset.SubtitleTrackBehaviours = new SubtitleBehaviours();
            preset.SubtitleTrackBehaviours.SelectedBehaviour = EnumHelper<SubtitleBehaviourModes>.GetValue(importedPreset.SubtitleTrackSelectionBehavior);
            preset.SubtitleTrackBehaviours.SelectedBurnInBehaviour = EnumHelper<SubtitleBurnInBehaviourModes>.GetValue(importedPreset.SubtitleBurnBehavior);

            preset.SubtitleTrackBehaviours.AddClosedCaptions = importedPreset.SubtitleAddCC;
            preset.SubtitleTrackBehaviours.AddForeignAudioScanTrack = importedPreset.SubtitleAddForeignAudioSearch;
            if (importedPreset.SubtitleLanguageList != null)
            {
                IList<string> names = LanguageUtilities.GetLanguageNames(importedPreset.SubtitleLanguageList);
                foreach (var name in names)
                {
                    preset.SubtitleTrackBehaviours.SelectedLanguages.Add(name);
                }
            }

            /* Chapter Marker Settings */
            preset.Task.IncludeChapterMarkers = importedPreset.ChapterMarkers;

            /* Not Supported Yet */
            // public int VideoColorMatrixCode { get; set; }
            // public bool VideoQSVDecode { get; set; }
            // public int VideoQSVAsyncDepth { get; set; }
            // public bool SubtitleAddForeignAudioSubtitle { get; set; }
            // public bool SubtitleBurnBDSub { get; set; }
            // public bool SubtitleBurnDVDSub { get; set; }
            // public bool PictureItuPAR { get; set; }
            // public bool PictureLooseCrop { get; set; }
            // public int PictureForceHeight { get; set; }
            // public int PictureForceWidth { get; set; }
            // public List<object> ChildrenArray { get; set; }
            // public int Type { get; set; }

            return preset;
        }

        public static PresetTransportContainer ExportPreset(Preset export, HBConfiguration config)
        {
            PresetVersion presetVersion = HandBrakePresetService.GetCurrentPresetVersion();
            PresetTransportContainer container = new PresetTransportContainer(presetVersion.Major, presetVersion.Minor, presetVersion.Micro);

            container.PresetList = new List<object> { CreateHbPreset(export, config) };

            return container;
        }

        public static PresetTransportContainer ExportPresets(IEnumerable<Preset> exportList, HBConfiguration config)
        {
            PresetVersion presetVersion = HandBrakePresetService.GetCurrentPresetVersion();
            PresetTransportContainer container = new PresetTransportContainer(presetVersion.Major, presetVersion.Minor, presetVersion.Micro);

            List<HBPreset> presets = exportList.Select(item => CreateHbPreset(item, config)).ToList();

            container.PresetList = new List<object>();
            container.PresetList.AddRange(presets);

            return container;
        }

        public static PresetTransportContainer ExportPresetCategories(IList<PresetDisplayCategory> categories, HBConfiguration config)
        {
            PresetVersion presetVersion = HandBrakePresetService.GetCurrentPresetVersion();
            PresetTransportContainer container = new PresetTransportContainer(presetVersion.Major, presetVersion.Minor, presetVersion.Micro);

            List<object> presets = new List<object>();
            foreach (var category in categories)
            {
                presets.Add(CreatePresetCategory(category, config));
            }

            container.PresetList = presets;

            return container;
        }

        public static HBPresetCategory CreatePresetCategory(PresetDisplayCategory category, HBConfiguration config)
        {
            HBPresetCategory preset = new HBPresetCategory();
            preset.Folder = true;
            preset.PresetName = category.Category;
            preset.PresetDescription = string.Empty;
            preset.ChildrenArray = new List<HBPreset>();

            foreach (Preset singlePreset in category.Presets)
            {
                preset.ChildrenArray.Add(CreateHbPreset(singlePreset, config));
            }

            return preset;
        }

        public static HBPreset CreateHbPreset(Preset export, HBConfiguration config)
        {
            HBPreset preset = new HBPreset();

            // Preset
            preset.PresetDescription = export.Description;
            preset.PresetName = export.Name;
            preset.Type = export.IsBuildIn ? 0 : 1;
            preset.Default = export.IsDefault;

            // Audio
            preset.AudioCopyMask = export.AudioTrackBehaviours.AllowedPassthruOptions.AllowedPassthruOptions.Select(EnumHelper<AudioEncoder>.GetShortName).ToList();
            preset.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetShortName(export.AudioTrackBehaviours.AllowedPassthruOptions.AudioEncoderFallback);
            preset.AudioLanguageList = LanguageUtilities.GetLanguageCodes(export.AudioTrackBehaviours.SelectedLanguages);
            preset.AudioTrackSelectionBehavior = EnumHelper<AudioBehaviourModes>.GetShortName(export.AudioTrackBehaviours.SelectedBehaviour);
            preset.AudioSecondaryEncoderMode = export.AudioTrackBehaviours.SelectedTrackDefaultBehaviour == AudioTrackDefaultsMode.FirstTrack; // 1 = First Track, 0 = All
            preset.AudioList = new List<AudioList>();
            foreach (var item in export.AudioTrackBehaviours.BehaviourTracks)
            {
                AudioList track = new AudioList
                {
                    AudioBitrate = item.Bitrate,
                    AudioCompressionLevel = 0, // TODO
                    AudioDitherMethod = null,  // TODO
                    AudioEncoder = EnumHelper<AudioEncoder>.GetShortName(item.Encoder),
                    AudioMixdown = item.MixDown != null ? item.MixDown.ShortName : "dpl2",
                    AudioNormalizeMixLevel = false, // TODO
                    AudioSamplerate = item.SampleRate == 0 ? "auto" : item.SampleRate.ToString(CultureInfo.InvariantCulture),  // TODO check formatting.
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
            preset.SubtitleLanguageList = LanguageUtilities.GetLanguageCodes(export.SubtitleTrackBehaviours.SelectedLanguages);
            preset.SubtitleTrackSelectionBehavior = EnumHelper<SubtitleBehaviourModes>.GetShortName(export.SubtitleTrackBehaviours.SelectedBehaviour);

            // Chapters
            preset.ChapterMarkers = export.Task.IncludeChapterMarkers;

            // Output Settings
            preset.FileFormat = EnumHelper<OutputFormat>.GetShortName(export.Task.OutputFormat);
            preset.Mp4HttpOptimize = export.Task.OptimizeMP4;
            preset.Mp4iPodCompatible = export.Task.IPod5GSupport;
            preset.AlignAVStart = export.Task.AlignAVStart;
            preset.MetadataPassthrough = export.Task.MetaData?.PassthruMetadataEnabled ?? false;

            // Picture Settings
            preset.PictureForceHeight = 0; // TODO
            preset.PictureForceWidth = 0; // TODO
            preset.PictureHeight = export.Task.MaxHeight;
            preset.PictureItuPAR = false; // TODO Not supported Yet
            preset.PictureKeepRatio = export.Task.KeepDisplayAspect;
            preset.PictureLeftCrop = export.Task.Cropping.Left;
            preset.PictureLooseCrop = false; // TODO Not Supported Yet
            preset.PicturePAR = EnumHelper<Anamorphic>.GetShortName(export.Task.Anamorphic);
            preset.PicturePARHeight = export.Task.PixelAspectY;
            preset.PicturePARWidth = export.Task.PixelAspectX;
            preset.PictureRightCrop = export.Task.Cropping.Right;

            preset.PicturePadMode = export.Task.Padding.Mode;
            preset.PicturePadTop = export.Task.Padding.Y;
            preset.PicturePadBottom = export.Task.Padding.Bottom;
            preset.PicturePadLeft = export.Task.Padding.X;
            preset.PicturePadRight = export.Task.Padding.Right;
            preset.PicturePadColor = export.Task.Padding.Color;
            preset.PictureUseMaximumSize = export.Task.OptimalSize;
            preset.PictureAllowUpscaling = export.Task.AllowUpscaling;

            if (export.Task.Rotation != 0 || export.Task.FlipVideo)
            {
                preset.PictureRotate = string.Format("{0}:{1}", export.Task.Rotation, export.Task.FlipVideo ? "1" : "0");
            }

            preset.PictureTopCrop = export.Task.Cropping.Top;
            preset.PictureWidth = export.Task.MaxWidth;
            preset.PictureDARWidth = export.Task.DisplayWidth.HasValue ? (int)export.Task.DisplayWidth.Value : 0;
            preset.PictureAutoCrop = !export.Task.HasCropping;
            preset.PictureBottomCrop = export.Task.Cropping.Bottom;

            // Filters
            preset.PictureDeblockPreset = export.Task.DeblockPreset?.Key;
            preset.PictureDeblockTune = export.Task.DeblockTune?.Key;
            preset.PictureDeblockCustom = export.Task.CustomDeblock;

            preset.PictureDeinterlaceFilter = export.Task.DeinterlaceFilter == DeinterlaceFilter.Decomb ? "decomb"
                : export.Task.DeinterlaceFilter == DeinterlaceFilter.Yadif ? "yadif"
                : export.Task.DeinterlaceFilter == DeinterlaceFilter.Bwdif ? "bwdif" : "off";
            preset.PictureDeinterlacePreset = export.Task.DeinterlacePreset?.ShortName;
            preset.PictureDeinterlaceCustom = export.Task.CustomDeinterlaceSettings;

            preset.PictureCombDetectPreset = EnumHelper<CombDetect>.GetShortName(export.Task.CombDetect);
            preset.PictureCombDetectCustom = export.Task.CustomCombDetect;

            preset.PictureDenoiseCustom = export.Task.CustomDenoise;
            preset.PictureDenoiseFilter = EnumHelper<Denoise>.GetShortName(export.Task.Denoise);
            preset.PictureDenoisePreset = EnumHelper<DenoisePreset>.GetShortName(export.Task.DenoisePreset);
            preset.PictureDenoiseTune = EnumHelper<DenoiseTune>.GetShortName(export.Task.DenoiseTune);
            preset.PictureDetelecine = EnumHelper<Detelecine>.GetShortName(export.Task.Detelecine);

            preset.PictureDetelecineCustom = export.Task.CustomDetelecine;

            preset.PictureSharpenFilter = EnumHelper<Sharpen>.GetShortName(export.Task.Sharpen);
            preset.PictureSharpenPreset = export.Task.SharpenPreset != null ? export.Task.SharpenPreset.Key : string.Empty; 
            preset.PictureSharpenTune = export.Task.SharpenTune != null ? export.Task.SharpenTune.Key : string.Empty;
            preset.PictureSharpenCustom = export.Task.SharpenCustom;

            preset.PictureColorspacePreset = export.Task.Colourspace?.Key;
            preset.PictureColorspaceCustom = export.Task.CustomColourspace;

            preset.PictureChromaSmoothPreset = export.Task.ChromaSmooth?.Key;
            preset.PictureChromaSmoothTune = export.Task.ChromaSmoothTune?.Key;
            preset.PictureChromaSmoothCustom = export.Task.CustomChromaSmooth;
            
            // Video
            preset.VideoEncoder = EnumHelper<VideoEncoder>.GetShortName(export.Task.VideoEncoder);
            preset.VideoFramerate = export.Task.Framerate.HasValue ? export.Task.Framerate.ToString() : null;
            preset.VideoFramerateMode = EnumHelper<FramerateMode>.GetShortName(export.Task.FramerateMode);
            preset.VideoGrayScale = export.Task.Grayscale;
            preset.VideoLevel = export.Task.VideoLevel != null ? export.Task.VideoLevel.ShortName : null;
            preset.VideoOptionExtra = export.Task.ExtraAdvancedArguments;
            preset.VideoPreset = export.Task.VideoPreset != null ? export.Task.VideoPreset.ShortName : null;
            preset.VideoProfile = export.Task.VideoProfile != null ? export.Task.VideoProfile.ShortName : null;
            preset.VideoQSVDecode = config.EnableQuickSyncDecoding;
            preset.VideoQualitySlider = export.Task.Quality.HasValue ? export.Task.Quality.Value : 0;
            preset.VideoQualityType = (int)export.Task.VideoEncodeRateType;
            preset.VideoScaler = EnumHelper<VideoScaler>.GetShortName(VideoScaler.Lanczos);
            preset.VideoTune = export.Task.VideoTunes.Aggregate(string.Empty, (current, item) => !string.IsNullOrEmpty(current) ? string.Format("{0},{1}", current, item.ShortName) : item.ShortName);
            preset.VideoAvgBitrate = export.Task.VideoBitrate ?? 0;
            preset.VideoColorMatrixCode = 0; // TODO not supported.
            preset.VideoTurboTwoPass = export.Task.TurboFirstPass;
            preset.VideoTwoPass = export.Task.TwoPass;

            // Unknown
            preset.ChildrenArray = new List<object>(); 
            preset.Folder = false;
            preset.FolderOpen = false;

            return preset;
        }

        private static OutputFormat GetFileFormat(string format)
        {
            switch (format.ToLower())
            {
                default:
                    return OutputFormat.Mp4;
                case "m4v":
                case "mp4":
                case "av_mp4":
                    return OutputFormat.Mp4;
                case "mkv":
                case "av_mkv":
                    return OutputFormat.Mkv;
                case "webm":
                case "av_webm":
                    return OutputFormat.WebM;
            }
        }
    }
}
