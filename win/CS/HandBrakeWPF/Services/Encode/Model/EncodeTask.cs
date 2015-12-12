// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeTask.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Encode Task
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;

    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Utilities;

    using AllowedPassthru = HandBrakeWPF.Services.Encode.Model.Models.AllowedPassthru;
    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;
    using ChapterMarker = HandBrakeWPF.Services.Encode.Model.Models.ChapterMarker;
    using DenoisePreset = HandBrakeWPF.Services.Encode.Model.Models.DenoisePreset;
    using DenoiseTune = HandBrakeWPF.Services.Encode.Model.Models.DenoiseTune;
    using FramerateMode = HandBrakeWPF.Services.Encode.Model.Models.FramerateMode;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using PointToPointMode = HandBrakeWPF.Services.Encode.Model.Models.PointToPointMode;
    using SubtitleTrack = HandBrakeWPF.Services.Encode.Model.Models.SubtitleTrack;
    using SubtitleType = HandBrakeWPF.Services.Encode.Model.Models.SubtitleType;
    using VideoLevel = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoProfile;
    using VideoTune = HandBrakeWPF.Services.Encode.Model.Models.Video.VideoTune;

    /// <summary>
    /// An Encode Task
    /// </summary>
    public class EncodeTask : PropertyChangedBase
    {
        #region Private Fields

        /// <summary>
        /// The advanced panel enabled.
        /// </summary>
        private bool showAdvancedTab;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="EncodeTask"/> class.
        /// </summary>
        public EncodeTask()
        {
            this.Cropping = new Cropping();
            this.AudioTracks = new ObservableCollection<AudioTrack>();
            this.SubtitleTracks = new ObservableCollection<SubtitleTrack>();
            this.ChapterNames = new ObservableCollection<ChapterMarker>();
            this.AllowedPassthruOptions = new AllowedPassthru();
            this.Modulus = 16;

            this.VideoTunes = new List<VideoTune>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="EncodeTask"/> class. 
        /// Copy Constructor
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public EncodeTask(EncodeTask task)
        {
            this.AdvancedEncoderOptions = task.AdvancedEncoderOptions;
            this.AllowedPassthruOptions = new AllowedPassthru(task.AllowedPassthruOptions);
            this.Anamorphic = task.Anamorphic;
            this.Angle = task.Angle;

            this.AudioTracks = new ObservableCollection<AudioTrack>();
            foreach (AudioTrack track in task.AudioTracks)
            {
                this.AudioTracks.Add(new AudioTrack(track, true));
            }

            this.ChapterNames = new ObservableCollection<ChapterMarker>();
            foreach (ChapterMarker track in task.ChapterNames)
            {
                this.ChapterNames.Add(new ChapterMarker(track));
            }

            this.ChapterMarkersFilePath = task.ChapterMarkersFilePath;
            this.Cropping = new Cropping(task.Cropping);
            this.CustomDecomb = task.CustomDecomb;
            this.CustomDeinterlace = task.CustomDeinterlace;
            this.CustomDenoise = task.CustomDenoise;
            this.CustomDetelecine = task.CustomDetelecine;
            this.Deblock = task.Deblock;
            this.Decomb = task.Decomb;
            this.Deinterlace = task.Deinterlace;
            this.DeinterlaceFilter = task.DeinterlaceFilter;
            this.Denoise = task.Denoise;
            this.DenoisePreset = task.DenoisePreset;
            this.DenoiseTune = task.DenoiseTune;
            this.Destination = task.Destination;
            this.Detelecine = task.Detelecine;
            this.DisplayWidth = task.DisplayWidth;
            this.EndPoint = task.EndPoint;
            this.Framerate = task.Framerate;
            this.FramerateMode = task.FramerateMode;
            this.Grayscale = task.Grayscale;
            this.HasCropping = task.HasCropping;
            this.Height = task.Height;
            this.IncludeChapterMarkers = task.IncludeChapterMarkers;
            this.IPod5GSupport = task.IPod5GSupport;
            this.KeepDisplayAspect = task.KeepDisplayAspect;
            this.MaxHeight = task.MaxHeight;
            this.MaxWidth = task.MaxWidth;
            this.Modulus = task.Modulus;
            this.OptimizeMP4 = task.OptimizeMP4;
            this.OutputFormat = task.OutputFormat;
            this.PixelAspectX = task.PixelAspectX;
            this.PixelAspectY = task.PixelAspectY;
            this.PointToPointMode = task.PointToPointMode;
            this.Quality = task.Quality;
            this.Source = task.Source;
            this.StartPoint = task.StartPoint;

            this.SubtitleTracks = new ObservableCollection<SubtitleTrack>();
            foreach (SubtitleTrack subtitleTrack in task.SubtitleTracks)
            {
                this.SubtitleTracks.Add(new SubtitleTrack(subtitleTrack));
            }

            this.Title = task.Title;
            this.TurboFirstPass = task.TurboFirstPass;
            this.TwoPass = task.TwoPass;
            this.VideoBitrate = task.VideoBitrate;
            this.VideoEncoder = task.VideoEncoder;
            this.VideoEncodeRateType = task.VideoEncodeRateType;
            this.Width = task.Width;

            this.VideoLevel = task.VideoLevel;
            this.VideoProfile = task.VideoProfile;
            this.VideoPreset = task.VideoPreset;
            this.VideoTunes = new List<VideoTune>(task.VideoTunes);
            this.ExtraAdvancedArguments = task.ExtraAdvancedArguments;

            this.ShowAdvancedTab = task.ShowAdvancedTab;
        }

        #region Source

        /// <summary>
        /// Gets or sets Source.
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets Title.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets the Angle
        /// </summary>
        public int Angle { get; set; }

        /// <summary>
        /// Gets or sets PointToPointMode.
        /// </summary>
        public PointToPointMode PointToPointMode { get; set; }

        /// <summary>
        /// Gets or sets StartPoint.
        /// </summary>
        public int StartPoint { get; set; }

        /// <summary>
        /// Gets or sets EndPoint.
        /// </summary>
        public int EndPoint { get; set; }

        #endregion

        #region Destination

        /// <summary>
        /// Gets or sets Destination.
        /// </summary>
        public string Destination { get; set; }

        #endregion

        #region Output Settings

        /// <summary>
        /// Gets or sets OutputFormat.
        /// </summary>
        public OutputFormat OutputFormat { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Optimize.
        /// </summary>
        public bool OptimizeMP4 { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether IPod5GSupport.
        /// </summary>
        public bool IPod5GSupport { get; set; }

        #endregion

        #region Picture

        /// <summary>
        /// Gets or sets Width.
        /// </summary>
        public int? Width { get; set; }

        /// <summary>
        /// Gets or sets Height.
        /// </summary>
        public int? Height { get; set; }

        /// <summary>
        /// Gets or sets MaxWidth.
        /// </summary>
        public int? MaxWidth { get; set; }

        /// <summary>
        /// Gets or sets MaxHeight.
        /// </summary>
        public int? MaxHeight { get; set; }

        /// <summary>
        /// Gets or sets Cropping.
        /// </summary>
        public Cropping Cropping { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether HasCropping.
        /// </summary>
        public bool HasCropping { get; set; }

        /// <summary>
        /// Gets or sets Anamorphic.
        /// </summary>
        public Anamorphic Anamorphic { get; set; }

        /// <summary>
        /// Gets or sets DisplayWidth.
        /// </summary>
        public double? DisplayWidth { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether KeepDisplayAspect.
        /// </summary>
        public bool KeepDisplayAspect { get; set; }

        /// <summary>
        /// Gets or sets PixelAspectX.
        /// </summary>
        public int PixelAspectX { get; set; }

        /// <summary>
        /// Gets or sets PixelAspectY.
        /// </summary>
        public int PixelAspectY { get; set; }

        /// <summary>
        /// Gets or sets Modulus.
        /// </summary>
        public int? Modulus { get; set; }

        #endregion

        #region Filters

        /// <summary>
        /// Gets or sets Deinterlace Filter Mode
        /// </summary>
        public DeinterlaceFilter DeinterlaceFilter { get; set; }

        /// <summary>
        /// Gets or sets Deinterlace.
        /// </summary>
        public Deinterlace Deinterlace { get; set; }

        /// <summary>
        /// Gets or sets CustomDeinterlace.
        /// </summary>
        public string CustomDeinterlace { get; set; }

        /// <summary>
        /// Gets or sets Decomb.
        /// </summary>
        public Decomb Decomb { get; set; }

        /// <summary>
        /// Gets or sets CustomDecomb.
        /// </summary>
        public string CustomDecomb { get; set; }

        /// <summary>
        /// Gets or sets Detelecine.
        /// </summary>
        public Detelecine Detelecine { get; set; }

        /// <summary>
        /// Gets or sets CustomDetelecine.
        /// </summary>
        public string CustomDetelecine { get; set; }

        /// <summary>
        /// Gets or sets Denoise.
        /// </summary>
        public Denoise Denoise { get; set; }

        /// <summary>
        /// Gets or sets the denoise preset.
        /// </summary>
        public DenoisePreset DenoisePreset { get; set; }

        /// <summary>
        /// Gets or sets the denoise tune.
        /// </summary>
        public DenoiseTune DenoiseTune { get; set; }

        /// <summary>
        /// Gets or sets CustomDenoise.
        /// </summary>
        public string CustomDenoise { get; set; }

        /// <summary>
        /// Gets or sets Deblock.
        /// </summary>
        public int Deblock { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Grayscale.
        /// </summary>
        public bool Grayscale { get; set; }

        #endregion

        #region Video

        /// <summary>
        /// Gets or sets VideoEncodeRateType.
        /// </summary>
        public VideoEncodeRateType VideoEncodeRateType { get; set; }

        /// <summary>
        /// Gets or sets the VideoEncoder
        /// </summary>
        public VideoEncoder VideoEncoder { get; set; }

        /// <summary>
        /// Gets or sets the Video Encode Mode
        /// </summary>
        public FramerateMode FramerateMode { get; set; }

        /// <summary>
        /// Gets or sets Quality.
        /// </summary>
        public double? Quality { get; set; }

        /// <summary>
        /// Gets or sets VideoBitrate.
        /// </summary>
        public int? VideoBitrate { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether TwoPass.
        /// </summary>
        public bool TwoPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether TurboFirstPass.
        /// </summary>
        public bool TurboFirstPass { get; set; }

        /// <summary>
        /// Gets or sets Framerate.
        /// Null = Same as Source
        /// </summary>
        public double? Framerate { get; set; }

        #endregion

        #region Audio

        /// <summary>
        /// Gets or sets AudioEncodings.
        /// </summary>
        public ObservableCollection<AudioTrack> AudioTracks { get; set; }

        /// <summary>
        /// Gets or sets AllowedPassthruOptions.
        /// </summary>
        public AllowedPassthru AllowedPassthruOptions { get; set; }

        #endregion

        #region Subtitles

        /// <summary>
        /// Gets or sets SubtitleTracks.
        /// </summary>
        public ObservableCollection<SubtitleTrack> SubtitleTracks { get; set; }

        #endregion

        #region Chapters

        /// <summary>
        /// Gets or sets a value indicating whether IncludeChapterMarkers.
        /// </summary>
        public bool IncludeChapterMarkers { get; set; }

        /// <summary>
        /// Gets or sets ChapterMarkersFilePath.
        /// </summary>
        public string ChapterMarkersFilePath { get; set; }

        /// <summary>
        /// Gets or sets ChapterNames.
        /// </summary>
        public ObservableCollection<ChapterMarker> ChapterNames { get; set; }

        #endregion

        #region Advanced

        /// <summary>
        /// Gets or sets AdvancedEncoderOptions.
        /// </summary>
        public string AdvancedEncoderOptions { get; set; }

        /// <summary>
        /// Gets or sets the video profile.
        /// </summary>
        public VideoProfile VideoProfile { get; set; }

        /// <summary>
        /// Gets or sets the video level.
        /// </summary>
        public VideoLevel VideoLevel { get; set; }

        /// <summary>
        /// Gets or sets the video preset.
        /// </summary>
        public VideoPreset VideoPreset { get; set; }

        /// <summary>
        /// Gets or sets the video tunes.
        /// </summary>
        public List<VideoTune> VideoTunes { get; set; }

        /// <summary>
        /// Gets or sets Extra Advanced Arguments for the Video Tab.
        /// </summary>
        public string ExtraAdvancedArguments { get; set; }

        #endregion

        #region Preview

        /// <summary>
        /// Gets or sets a value indicating whether IsPreviewEncode.
        /// </summary>
        public bool IsPreviewEncode { get; set; }

        /// <summary>
        /// Gets or sets PreviewEncodeDuration.
        /// </summary>
        public int? PreviewEncodeDuration { get; set; }

        /// <summary>
        /// Gets or sets PreviewEncodeStartAt.
        /// </summary>
        public int? PreviewEncodeStartAt { get; set; }

        #endregion

        #region Helpers

        /// <summary>
        /// Gets a value indicating whether M4v extension is required.
        /// </summary>
        public bool RequiresM4v
        {
            get
            {
                if (this.OutputFormat == OutputFormat.Mp4)
                {
                    bool audio =
                        this.AudioTracks.Any(
                            item =>
                            item.Encoder == AudioEncoder.Ac3Passthrough || item.Encoder == AudioEncoder.Ac3
                            || item.Encoder == AudioEncoder.DtsPassthrough || item.Encoder == AudioEncoder.Passthrough);

                    bool subtitles = this.SubtitleTracks.Any(track => track.SubtitleType != SubtitleType.VobSub);

                    return audio || subtitles;
                }

                return false;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether advanced panel enabled.
        /// </summary>
        public bool ShowAdvancedTab
        {
            get
            {
                return this.showAdvancedTab;
            }
            set
            {
                if (!Equals(value, this.showAdvancedTab))
                {
                    this.showAdvancedTab = value;
                    this.NotifyOfPropertyChange(() => this.ShowAdvancedTab);
                }
            }
        }

        /// <summary>
        /// Gets the picture settings desc.
        /// </summary>
        public string PictureSettingsDesc
        {
            get
            {
                string resolution = string.Empty; 
                switch (this.Anamorphic)
                {
                    case Anamorphic.Strict:
                        resolution = "Anamorphic: Strict";
                        break;
                    case Anamorphic.Loose:
                        resolution = "Anamorphic: Loose, Width: " + this.Width;
                        break;
                    case Anamorphic.Custom:
                        resolution = "Anamorphic: Custom, Resolution: " + this.Width + "x" + this.Height;
                        break;
                    case Anamorphic.None:
                        resolution = "Resolution: " + this.Width + "x" + this.Height;
                        break;
                }

                return resolution + Environment.NewLine + "Crop Top: " + this.Cropping.Top + ", Botton: " + this.Cropping.Bottom + ", Left: "
                       + this.Cropping.Left + ", Right: " + this.Cropping.Right;
            }
        }

        #endregion
    }
}
