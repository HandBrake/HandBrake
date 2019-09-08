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
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    using HandBrake.Interop.Interop.Model;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model.Models;

    using Newtonsoft.Json;

    using AllowedPassthru = Models.AllowedPassthru;
    using AudioTrack = Models.AudioTrack;
    using ChapterMarker = Models.ChapterMarker;
    using DenoisePreset = Models.DenoisePreset;
    using DenoiseTune = Models.DenoiseTune;
    using FramerateMode = Models.FramerateMode;
    using OutputFormat = Models.OutputFormat;
    using PointToPointMode = Models.PointToPointMode;
    using SubtitleTrack = Models.SubtitleTrack;
    using VideoLevel = Models.Video.VideoLevel;
    using VideoPreset = Models.Video.VideoPreset;
    using VideoProfile = Models.Video.VideoProfile;
    using VideoTune = Models.Video.VideoTune;

    public class EncodeTask 
    {
        public EncodeTask()
        {
            this.Cropping = new Cropping();
            this.AudioTracks = new ObservableCollection<AudioTrack>();
            this.SubtitleTracks = new ObservableCollection<SubtitleTrack>();
            this.ChapterNames = new ObservableCollection<ChapterMarker>();
            this.AllowedPassthruOptions = new AllowedPassthru();
            this.Modulus = 16;
            this.MetaData = new MetaData();

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

            this.AlignAVStart = task.AlignAVStart;
            this.ChapterMarkersFilePath = task.ChapterMarkersFilePath;
            this.Cropping = new Cropping(task.Cropping);
            this.CustomDeinterlaceSettings = task.CustomDeinterlaceSettings;
            this.CustomDenoise = task.CustomDenoise;
            this.CustomDetelecine = task.CustomDetelecine;
            this.CustomCombDetect = task.CustomCombDetect;
            this.CombDetect = task.CombDetect;
            this.DeblockPreset = task.DeblockPreset;
            this.DeblockTune = task.DeblockTune;
            this.CustomDeblock = task.CustomDeblock;
            this.DeinterlacePreset = task.DeinterlacePreset;
            this.DeinterlaceFilter = task.DeinterlaceFilter;
            this.Denoise = task.Denoise;
            this.DenoisePreset = task.DenoisePreset;
            this.DenoiseTune = task.DenoiseTune;
            this.Destination = task.Destination;
            this.Detelecine = task.Detelecine;
            this.FlipVideo = task.FlipVideo;
            this.Rotation = task.Rotation;
            this.Sharpen = task.Sharpen;
            this.SharpenPreset = task.SharpenPreset;
            this.SharpenTune = task.SharpenTune;
            this.SharpenCustom = task.SharpenCustom;

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

            this.MetaData = new MetaData(task.MetaData);
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
        public long StartPoint { get; set; }

        /// <summary>
        /// Gets or sets EndPoint.
        /// </summary>
        public long EndPoint { get; set; }

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

        public bool AlignAVStart { get; set; }

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
        public HBPresetTune DeinterlacePreset { get; set; }

        /// <summary>
        /// Gets or sets the comb detect.
        /// </summary>
        public CombDetect CombDetect { get; set; }

        /// <summary>
        /// Gets or sets CustomDecomb.
        /// </summary>
        public string CustomDeinterlaceSettings { get; set; }

        /// <summary>
        /// Gets or sets the custom comb detect.
        /// </summary>
        public string CustomCombDetect { get; set; }

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
        /// Gets or sets a value indicating whether Grayscale.
        /// </summary>
        public bool Grayscale { get; set; }

        /// <summary>
        /// Rotate the Video by x Degrees
        /// </summary>
        public int Rotation { get; set; }

        /// <summary>
        /// Flip the video.
        /// </summary>
        public bool FlipVideo { get; set; }

        public Sharpen Sharpen { get; set; }
        public FilterPreset SharpenPreset { get; set; }
        public FilterTune SharpenTune { get; set; }
        public string SharpenCustom { get; set; }

        public FilterPreset DeblockPreset { get; set; }
        public FilterTune DeblockTune { get; set; }
        public string CustomDeblock { get; set; }
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

        #region MetaData

        /// <summary>
        /// Gets or sets the meta data.
        /// </summary>
        public MetaData MetaData { get; set; }
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
    }
}
