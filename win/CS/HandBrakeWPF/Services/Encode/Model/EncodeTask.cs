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

    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model.Models;

    using AudioTrack = Models.AudioTrack;
    using ChapterMarker = Models.ChapterMarker;
    using FramerateMode = Models.FramerateMode;
    using OutputFormat = Models.OutputFormat;
    using PointToPointMode = Models.PointToPointMode;
    using SubtitleTrack = Models.SubtitleTrack;
    using VideoEncodeRateType = HandBrakeWPF.Model.Video.VideoEncodeRateType;
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
            this.AudioPassthruOptions = new ObservableCollection<HBAudioEncoder>();
            this.MetaData = new MetaData();
            this.Padding = new PaddingFilter();
            this.VideoTunes = new List<VideoTune>();
        }

        public EncodeTask(EncodeTask task)
        {
            /* Source */
            this.Source = task.Source;
            this.StartPoint = task.StartPoint;
            this.Title = task.Title;
            this.Angle = task.Angle;
            this.EndPoint = task.EndPoint;
            this.PointToPointMode = task.PointToPointMode;

            /* Audio */
            this.AudioFallbackEncoder = task.AudioFallbackEncoder;
            this.AudioPassthruOptions = new ObservableCollection<HBAudioEncoder>();
            foreach (var allowed in task.AudioPassthruOptions)
            {
                this.AudioPassthruOptions.Add(allowed);
            }
            
            this.AudioTracks = new ObservableCollection<AudioTrack>();
            foreach (AudioTrack track in task.AudioTracks)
            {
                this.AudioTracks.Add(new AudioTrack(track, true));
            }

            /* Chapters */
            this.ChapterNames = new ObservableCollection<ChapterMarker>();
            foreach (ChapterMarker track in task.ChapterNames)
            {
                this.ChapterNames.Add(new ChapterMarker(track));
            }

            this.IncludeChapterMarkers = task.IncludeChapterMarkers;
            this.ChapterMarkersFilePath = task.ChapterMarkersFilePath;

            /* Subtitles */
            this.SubtitleTracks = new ObservableCollection<SubtitleTrack>();
            foreach (SubtitleTrack subtitleTrack in task.SubtitleTracks)
            {
                this.SubtitleTracks.Add(new SubtitleTrack(subtitleTrack));
            }

            /* Filter Settings */
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
            this.Sharpen = task.Sharpen;
            this.SharpenPreset = task.SharpenPreset;
            this.SharpenTune = task.SharpenTune;
            this.SharpenCustom = task.SharpenCustom;
            this.Colourspace = task.Colourspace;
            this.CustomColourspace = task.CustomColourspace;
            this.ChromaSmooth = task.ChromaSmooth;
            this.ChromaSmoothTune = task.ChromaSmoothTune;
            this.CustomChromaSmooth = task.CustomChromaSmooth;
            this.Grayscale = task.Grayscale;

            /* Picture Settings*/
            this.DisplayWidth = task.DisplayWidth;
            this.MaxHeight = task.MaxHeight;
            this.MaxWidth = task.MaxWidth;
            this.Width = task.Width;
            this.Height = task.Height;
            this.AllowUpscaling = task.AllowUpscaling;
            this.OptimalSize = task.OptimalSize;
            this.PixelAspectX = task.PixelAspectX;
            this.PixelAspectY = task.PixelAspectY;
            this.Cropping = new Cropping(task.Cropping);
            this.Padding = new PaddingFilter(task.Padding);
            this.FlipVideo = task.FlipVideo;
            this.Rotation = task.Rotation;
            this.Anamorphic = task.Anamorphic;
            this.KeepDisplayAspect = task.KeepDisplayAspect;

            /* Video */
            this.Quality = task.Quality;
            this.Framerate = task.Framerate;
            this.FramerateMode = task.FramerateMode;
            this.TurboAnalysisPass = task.TurboAnalysisPass;
            this.MultiPass = task.MultiPass;
            this.VideoBitrate = task.VideoBitrate;
            this.VideoEncoder = task.VideoEncoder;
            this.VideoEncodeRateType = task.VideoEncodeRateType;
            this.VideoLevel = task.VideoLevel;
            this.VideoProfile = task.VideoProfile;
            this.VideoPreset = task.VideoPreset;
            this.VideoTunes = new List<VideoTune>(task.VideoTunes);
            this.ExtraAdvancedArguments = task.ExtraAdvancedArguments;

            /* Container */
            this.IPod5GSupport = task.IPod5GSupport;
            this.OutputFormat = task.OutputFormat;
            this.OptimizeMP4 = task.OptimizeMP4;
            this.AlignAVStart = task.AlignAVStart;

            /* Other */
            this.MetaData = new MetaData(task.MetaData);
        }

        /* Source */

        public string Source { get; set; }

        public int Title { get; set; }

        public int Angle { get; set; }

        public PointToPointMode PointToPointMode { get; set; }

        public long StartPoint { get; set; }

        public long EndPoint { get; set; }

        /* Destination */

        public string Destination { get; set; }

        /* Output Settings */

        public OutputFormat OutputFormat { get; set; }

        public bool OptimizeMP4 { get; set; }

        public bool IPod5GSupport { get; set; }

        public bool AlignAVStart { get; set; }
        
        /* Picture Settings */

        public int? Width { get; set; }

        public int? Height { get; set; }

        public int? MaxWidth { get; set; }

        public int? MaxHeight { get; set; }

        public Cropping Cropping { get; set; }

        public Anamorphic Anamorphic { get; set; }

        public double? DisplayWidth { get; set; }

        public bool KeepDisplayAspect { get; set; }

        public int PixelAspectX { get; set; }

        public int PixelAspectY { get; set; }
        
        public bool AllowUpscaling { get; set; }

        public bool OptimalSize { get; set; }

        /* Filters */

        public DeinterlaceFilter DeinterlaceFilter { get; set; }

        public HBPresetTune DeinterlacePreset { get; set; }

        public CombDetect CombDetect { get; set; }

        public string CustomDeinterlaceSettings { get; set; }

        public string CustomCombDetect { get; set; }

        public Detelecine Detelecine { get; set; }

        public string CustomDetelecine { get; set; }

        public Denoise Denoise { get; set; }

        public HBPresetTune DenoisePreset { get; set; }

        public HBPresetTune DenoiseTune { get; set; }

        public string CustomDenoise { get; set; }

        public bool Grayscale { get; set; }

        public int Rotation { get; set; }

        public bool FlipVideo { get; set; }

        public Sharpen Sharpen { get; set; }

        public FilterPreset SharpenPreset { get; set; }

        public FilterTune SharpenTune { get; set; }

        public string SharpenCustom { get; set; }

        public FilterPreset DeblockPreset { get; set; }

        public FilterTune DeblockTune { get; set; }

        public string CustomDeblock { get; set; }

        public PaddingFilter Padding { get; set; }

        public FilterPreset Colourspace { get; set; }

        public string CustomColourspace { get; set; }

        public FilterPreset ChromaSmooth { get; set; }

        public FilterTune ChromaSmoothTune { get; set; }

        public string CustomChromaSmooth { get; set; }

        /* Video */

        public VideoEncodeRateType VideoEncodeRateType { get; set; }

        public HBVideoEncoder VideoEncoder { get; set; }

        public VideoProfile VideoProfile { get; set; }

        public VideoLevel VideoLevel { get; set; }

        public VideoPreset VideoPreset { get; set; }

        public List<VideoTune> VideoTunes { get; set; }

        public string ExtraAdvancedArguments { get; set; }

        public FramerateMode FramerateMode { get; set; }

        public double? Quality { get; set; }

        public int? VideoBitrate { get; set; }

        public bool MultiPass { get; set; }

        public bool TurboAnalysisPass { get; set; }

        public double? Framerate { get; set; }


        /* Audio */

        public ObservableCollection<AudioTrack> AudioTracks { get; set; }

        public IList<HBAudioEncoder> AudioPassthruOptions { get; set; }

        public HBAudioEncoder AudioFallbackEncoder { get; set; }


        /* Subtitles */

        public ObservableCollection<SubtitleTrack> SubtitleTracks { get; set; }
        
        /* Chapters */

        public bool IncludeChapterMarkers { get; set; }

        public string ChapterMarkersFilePath { get; set; }

        public ObservableCollection<ChapterMarker> ChapterNames { get; set; }


        /* Metadata */
        
        public MetaData MetaData { get; set; }

        /* Previews */

        public bool IsPreviewEncode { get; set; }

        public int? PreviewEncodeDuration { get; set; }

        public int? PreviewEncodeStartAt { get; set; }
    }
}
