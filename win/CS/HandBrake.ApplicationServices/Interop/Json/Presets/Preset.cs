// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Preset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Presets
{
    using System.Collections.Generic;

    /// <summary>
    /// The preset.
    /// </summary>
    public class Preset
    {
        /// <summary>
        /// Gets or sets the audio allow aac pass.
        /// </summary>
        public int AudioAllowAACPass { get; set; }

        /// <summary>
        /// Gets or sets the audio allow a c 3 pass.
        /// </summary>
        public int AudioAllowAC3Pass { get; set; }

        /// <summary>
        /// Gets or sets the audio allow dtshd pass.
        /// </summary>
        public int AudioAllowDTSHDPass { get; set; }

        /// <summary>
        /// Gets or sets the audio allow dts pass.
        /// </summary>
        public int AudioAllowDTSPass { get; set; }

        /// <summary>
        /// Gets or sets the audio allow m p 3 pass.
        /// </summary>
        public int AudioAllowMP3Pass { get; set; }

        /// <summary>
        /// Gets or sets the audio encoder fallback.
        /// </summary>
        public string AudioEncoderFallback { get; set; }

        /// <summary>
        /// Gets or sets the audio list.
        /// </summary>
        public List<AudioList> AudioList { get; set; }

        /// <summary>
        /// Gets or sets the chapter markers.
        /// </summary>
        public int ChapterMarkers { get; set; }

        /// <summary>
        /// Gets or sets the default.
        /// </summary>
        public int Default { get; set; }

        /// <summary>
        /// Gets or sets the file format.
        /// </summary>
        public string FileFormat { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether folder.
        /// </summary>
        public bool Folder { get; set; }

        /// <summary>
        /// Gets or sets the mp 4 http optimize.
        /// </summary>
        public int Mp4HttpOptimize { get; set; }

        /// <summary>
        /// Gets or sets the mp 4 i pod compatible.
        /// </summary>
        public int Mp4iPodCompatible { get; set; }

        /// <summary>
        /// Gets or sets the picture auto crop.
        /// </summary>
        public int PictureAutoCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture bottom crop.
        /// </summary>
        public int PictureBottomCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture deblock.
        /// </summary>
        public int PictureDeblock { get; set; }

        /// <summary>
        /// Gets or sets the picture decomb.
        /// </summary>
        public int PictureDecomb { get; set; }

        /// <summary>
        /// Gets or sets the picture decomb custom.
        /// </summary>
        public string PictureDecombCustom { get; set; }

        /// <summary>
        /// Gets or sets the picture decomb deinterlace.
        /// </summary>
        public int PictureDecombDeinterlace { get; set; }

        /// <summary>
        /// Gets or sets the picture deinterlace.
        /// </summary>
        public int PictureDeinterlace { get; set; }

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
        /// Gets or sets the picture detelecine.
        /// </summary>
        public int PictureDetelecine { get; set; }

        /// <summary>
        /// Gets or sets the picture detelecine custom.
        /// </summary>
        public string PictureDetelecineCustom { get; set; }

        /// <summary>
        /// Gets or sets the picture height.
        /// </summary>
        public int PictureHeight { get; set; }

        /// <summary>
        /// Gets or sets the picture keep ratio.
        /// </summary>
        public int PictureKeepRatio { get; set; }

        /// <summary>
        /// Gets or sets the picture left crop.
        /// </summary>
        public int PictureLeftCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture modulus.
        /// </summary>
        public int PictureModulus { get; set; }

        /// <summary>
        /// Gets or sets the picture par.
        /// </summary>
        public string PicturePAR { get; set; }

        /// <summary>
        /// Gets or sets the picture right crop.
        /// </summary>
        public int PictureRightCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture top crop.
        /// </summary>
        public int PictureTopCrop { get; set; }

        /// <summary>
        /// Gets or sets the picture width.
        /// </summary>
        public int PictureWidth { get; set; }

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
        /// Gets or sets the uses picture filters.
        /// </summary>
        public int UsesPictureFilters { get; set; }

        /// <summary>
        /// Gets or sets the uses picture settings.
        /// </summary>
        public int UsesPictureSettings { get; set; }

        /// <summary>
        /// Gets or sets the video avg bitrate.
        /// </summary>
        public string VideoAvgBitrate { get; set; }

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
        /// Gets or sets the video gray scale.
        /// </summary>
        public int VideoGrayScale { get; set; }

        /// <summary>
        /// Gets or sets the video level.
        /// </summary>
        public string VideoLevel { get; set; }

        /// <summary>
        /// Gets or sets the video option extra.
        /// </summary>
        public string VideoOptionExtra { get; set; }

        /// <summary>
        /// Gets or sets the video preset.
        /// </summary>
        public string VideoPreset { get; set; }

        /// <summary>
        /// Gets or sets the video profile.
        /// </summary>
        public string VideoProfile { get; set; }

        /// <summary>
        /// Gets or sets the video quality slider.
        /// </summary>
        public double VideoQualitySlider { get; set; }

        /// <summary>
        /// Gets or sets the video quality type.
        /// </summary>
        public int VideoQualityType { get; set; }

        /// <summary>
        /// Gets or sets the video tune.
        /// </summary>
        public string VideoTune { get; set; }

        /// <summary>
        /// Gets or sets the video turbo two pass.
        /// </summary>
        public int VideoTurboTwoPass { get; set; }

        /// <summary>
        /// Gets or sets the video two pass.
        /// </summary>
        public int VideoTwoPass { get; set; }

        /// <summary>
        /// Gets or sets the x 264 option.
        /// </summary>
        public string x264Option { get; set; }

        /// <summary>
        /// Gets or sets the x 264 use advanced options.
        /// </summary>
        public int x264UseAdvancedOptions { get; set; }
    }
}
