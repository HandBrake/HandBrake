// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeTask.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Encode Task
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using System.Collections.ObjectModel;
    using System.Linq;

    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

    using OutputFormat = HandBrake.ApplicationServices.Model.Encoding.OutputFormat;

    /// <summary>
    /// An Encode Task
    /// </summary>
    public class EncodeTask
    {
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
            this.X264Preset = x264Preset.Medium;
            this.H264Profile = x264Profile.None;
            this.X264Tune = x264Tune.None;
            this.Modulus = 16;
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
                this.AudioTracks.Add(new AudioTrack(track));
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
            this.Denoise = task.Denoise;
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
            this.OpenCLSupport = task.OpenCLSupport;
            this.UVDSupport = task.UVDSupport;
            this.KeepDisplayAspect = task.KeepDisplayAspect;
            this.LargeFile = task.LargeFile;
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
            this.X264Preset = task.X264Preset;
            this.H264Profile = task.H264Profile;
            this.X264Tune = task.X264Tune;
            this.H264Level = task.H264Level;
            this.FastDecode = task.FastDecode;
            this.ExtraAdvancedArguments = task.ExtraAdvancedArguments;

            this.PreviewStartAt = task.PreviewStartAt;
            this.PreviewDuration = task.PreviewDuration;
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
        /// Gets or sets a value indicating whether LargeFile.
        /// </summary>
        public bool LargeFile { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Optimize.
        /// </summary>
        public bool OptimizeMP4 { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether IPod5GSupport.
        /// </summary>
        public bool IPod5GSupport { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether OpenCLSupport.
        /// </summary>
        public bool OpenCLSupport { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether UVDSupport.
        /// </summary>
        public bool UVDSupport { get; set; }
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
        /// Gets or sets x264Preset.
        /// </summary>
        public x264Preset X264Preset { get; set; }

        /// <summary>
        /// Gets or sets x264Profile.
        /// </summary>
        public x264Profile H264Profile { get; set; }

        /// <summary>
        /// Gets or sets the x 264 level.
        /// </summary>
        public string H264Level { get; set; }

        /// <summary>
        /// Gets or sets X264Tune.
        /// </summary>
        public x264Tune X264Tune { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether fast decode.
        /// </summary>
        public bool FastDecode { get; set; }

        /// <summary>
        /// Extra Advanced Arguments for the Video Tab.
        /// </summary>
        public string ExtraAdvancedArguments { get; set; }

        #endregion

        #region Preview

        /// <summary>
        /// Gets or sets StartAt.
        /// </summary>
        public int? PreviewStartAt { get; set; }

        /// <summary>
        /// Gets or sets Duration.
        /// </summary>
        public int? PreviewDuration { get; set; }

        #endregion

        #region Helpers

        /// <summary>
        /// Gets a value indicating whether M4v extension is required.
        /// </summary>
        public bool RequiresM4v
        {
            get
            {
                if (this.OutputFormat == OutputFormat.M4V || this.OutputFormat == OutputFormat.Mp4)
                {
                    bool audio = this.AudioTracks.Any(item => item.Encoder == AudioEncoder.Ac3Passthrough || 
                        item.Encoder == AudioEncoder.Ac3 || item.Encoder == AudioEncoder.DtsPassthrough || item.Encoder == AudioEncoder.Passthrough);

                    bool subtitles = this.SubtitleTracks.Any(track => track.SubtitleType != SubtitleType.VobSub);

                    return audio || subtitles;
                }

                return false;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsPreviewEncode.
        /// </summary>
        public bool IsPreviewEncode { get; set; }

        /// <summary>
        /// Gets or sets PreviewEncodeDuration.
        /// </summary>
        public int PreviewEncodeDuration { get; set; }

        /// <summary>
        /// Gets or sets PreviewEncodeStartAt.
        /// </summary>
        public string PreviewEncodeStartAt { get; set; }
        #endregion
    }
}
