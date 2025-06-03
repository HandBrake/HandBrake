// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBPreset.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    using System.Collections.Generic;

    public class HBPreset
    {
        public bool AlignAVStart { get; set; }

        /// <summary>
        /// Gets or sets the audio copy mask.
        /// </summary>
        public List<string> AudioCopyMask { get; set; }

        /// <summary>
        /// Gets or sets the audio encoder fallback.
        /// </summary>
        public string AudioEncoderFallback { get; set; }

        /// <summary>
        /// Gets or sets the audio language list.
        /// </summary>
        public List<string> AudioLanguageList { get; set; }

        /// <summary>
        /// Gets or sets the audio list.
        /// </summary>
        public List<AudioList> AudioList { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio secondary encoder mode.
        /// </summary>
        public bool AudioSecondaryEncoderMode { get; set; }

        /// <summary>
        /// Gets or sets the audio track selection behavior.
        /// </summary>
        public string AudioTrackSelectionBehavior { get; set; }

        public bool AudioTrackNamePassthru { get; set; }

        public string AudioAutomaticNamingBehavior { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether chapter markers.
        /// </summary>
        public bool ChapterMarkers { get; set; }

        /// <summary>
        /// Gets or sets the children array.
        /// </summary>
        public List<object> ChildrenArray { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets the file format.
        /// </summary>
        public string FileFormat { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether folder.
        /// </summary>
        public bool Folder { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether folder open.
        /// </summary>
        public bool FolderOpen { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether optimize.
        /// </summary>
        public bool Optimize { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether mp 4 i pod compatible.
        /// </summary>
        public bool Mp4iPodCompatible { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether picture loose crop.
        /// </summary>
        public int PictureCropMode { get; set; }

        /// <summary>
        /// Gets or sets the picture bottom crop.
        /// </summary>
        public int PictureBottomCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture left crop.
        /// </summary>
        public int PictureLeftCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture right crop.
        /// </summary>
        public int PictureRightCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture top crop.
        /// </summary>
        public int PictureTopCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture dar width.
        /// </summary>
        public int PictureDARWidth { get; set; }

        public string PictureDeblockPreset { get; set; }

        public string PictureDeblockTune { get; set; }

        public string PictureDeblockCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether picture decomb deinterlace.
        /// </summary>
        public string PictureDeinterlaceFilter { get; set; }

        /// <summary>
        /// Gets or sets the picture comb detect preset.
        /// </summary>
        public string PictureCombDetectPreset { get; set; }

        /// <summary>
        /// Gets or sets the picture comb detect custom.
        /// </summary>
        public string PictureCombDetectCustom { get; set; }

        /// <summary>
        /// Gets or sets the picture deinterlace preset.
        /// </summary>
        public string PictureDeinterlacePreset { get; set; }

        /// <summary>
        /// Gets or sets the picture deinterlace custom.
        /// </summary>
        public string PictureDeinterlaceCustom { get; set; }

        /// <summary>
        /// Gets or sets the picture denoise custom.
        /// </summary>
        public string PictureDenoiseCustom { get; set; }

        /// <summary>
        /// Gets or sets the picture denoise filter.
        /// </summary>
        public string PictureDenoiseFilter { get; set; }

        /// <summary>
        /// Gets or sets the picture denoise preset.
        /// </summary>
        public string PictureDenoisePreset { get; set; }

        /// <summary>
        /// Gets or sets the picture denoise tune.
        /// </summary>
        public string PictureDenoiseTune { get; set; }

        public string PictureSharpenCustom { get; set; }

        public string PictureSharpenFilter { get; set; }

        public string PictureSharpenPreset { get; set; }

        public string PictureSharpenTune { get; set; }

        /// <summary>
        /// Gets or sets the picture detelecine.
        /// </summary>
        public string PictureDetelecine { get; set; }

        /// <summary>
        /// Gets or sets the picture detelecine custom.
        /// </summary>
        public string PictureDetelecineCustom { get; set; }

        public string PictureColorspacePreset { get; set; }

        public string PictureColorspaceCustom { get; set; }

        public string PictureChromaSmoothPreset { get; set; }

        public string PictureChromaSmoothTune { get; set; }

        public string PictureChromaSmoothCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether picture itu par.
        /// </summary>
        public bool PictureItuPAR { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether picture keep ratio.
        /// </summary>
        public bool PictureKeepRatio { get; set; }

        /// <summary>
        /// Gets or sets the picture par.
        /// </summary>
        public string PicturePAR { get; set; }

        /// <summary>
        /// Gets or sets the picture par width.
        /// </summary>
        public int PicturePARWidth { get; set; }

        /// <summary>
        /// Gets or sets the picture par height.
        /// </summary>
        public int PicturePARHeight { get; set; }

        /// <summary>
        /// Gets or sets the picture rotate.
        /// </summary>
        public string PictureRotate { get; set; }

        /// <summary>
        /// Gets or sets the picture width.
        /// </summary>
        public int? PictureWidth { get; set; }

        /// <summary>
        /// Gets or sets the picture height.
        /// </summary>
        public int? PictureHeight { get; set; }

        public bool PictureUseMaximumSize { get; set; }

        public bool PictureAllowUpscaling { get; set; }

        /// <summary>
        /// Gets or sets the picture force height.
        /// </summary>
        public int PictureForceHeight { get; set; }

        /// <summary>
        /// Gets or sets the picture force width.
        /// </summary>
        public int PictureForceWidth { get; set; }

        public string PicturePadMode { get; set; }

        public int PicturePadTop { get; set; }

        public int PicturePadBottom { get; set; }

        public int PicturePadLeft { get; set; }

        public int PicturePadRight { get; set; }

        public string PicturePadColor { get; set; }
        
        /// <summary>
        /// Gets or sets the preset description.
        /// </summary>
        public string PresetDescription { get; set; }

        /// <summary>
        /// Gets or sets the preset name.
        /// </summary>
        public string PresetName { get; set; }

        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        public int Type { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether subtitle add cc.
        /// </summary>
        public bool SubtitleAddCC { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether subtitle add foreign audio search.
        /// </summary>
        public bool SubtitleAddForeignAudioSearch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether subtitle add foreign audio subtitle.
        /// </summary>
        public bool SubtitleAddForeignAudioSubtitle { get; set; }

        /// <summary>
        /// Gets or sets the subtitle burn behavior.
        /// </summary>
        public string SubtitleBurnBehavior { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether subtitle burn bd sub.
        /// </summary>
        public bool SubtitleBurnBDSub { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether subtitle burn dvd sub.
        /// </summary>
        public bool SubtitleBurnDVDSub { get; set; }

        /// <summary>
        /// Gets or sets the subtitle language list.
        /// </summary>
        public List<string> SubtitleLanguageList { get; set; }

        /// <summary>
        /// Gets or sets the subtitle track selection behavior.
        /// </summary>
        public string SubtitleTrackSelectionBehavior { get; set; }

        public bool SubtitleTrackNamePassthru { get; set; }

        /// <summary>
        /// Gets or sets the video avg bitrate.
        /// </summary>
        public int? VideoAvgBitrate { get; set; }

        /// <summary>
        /// Gets or sets the video color matrix code.
        /// </summary>
        public int VideoColorMatrixCode { get; set; }

        /// <summary>
        /// Gets or sets the video encoder.
        /// </summary>
        public string VideoEncoder { get; set; }

        /// <summary>
        /// Gets or sets the video framerate.
        /// </summary>
        public string VideoFramerate { get; set; }

        /// <summary>
        /// Gets or sets the video framerate mode.
        /// </summary>
        public string VideoFramerateMode { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether video gray scale.
        /// </summary>
        public bool VideoGrayScale { get; set; }

        /// <summary>
        /// Gets or sets the video scaler.
        /// </summary>
        public string VideoScaler { get; set; }

        /// <summary>
        /// Gets or sets the video preset.
        /// </summary>
        public string VideoPreset { get; set; }

        /// <summary>
        /// Gets or sets the video tune.
        /// </summary>
        public string VideoTune { get; set; }

        /// <summary>
        /// Gets or sets the video profile.
        /// </summary>
        public string VideoProfile { get; set; }

        /// <summary>
        /// Gets or sets the video level.
        /// </summary>
        public string VideoLevel { get; set; }

        /// <summary>
        /// Gets or sets the video option extra.
        /// </summary>
        public string VideoOptionExtra { get; set; }

        /// <summary>
        /// Gets or sets the video quality type.
        /// </summary>
        public int VideoQualityType { get; set; }

        /// <summary>
        /// Gets or sets the video quality slider.
        /// </summary>
        public double VideoQualitySlider { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether video multi pass.
        /// </summary>
        public bool VideoMultiPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether video turbo multi pass.
        /// </summary>
        public bool VideoTurboMultiPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating which dynamic metadata formats to preserve.
        /// </summary>
        public string VideoPasshtruHDRDynamicMetadata { get; set; }

        /// <summary>
        /// Gets or sets the x 264 option.
        /// </summary>
        public string x264Option { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether x 264 use advanced options.
        /// </summary>
        public bool x264UseAdvancedOptions { get; set; }

        public bool PresetDisabled { get; set; }

        public bool MetadataPassthru { get; set; }
    }
}
