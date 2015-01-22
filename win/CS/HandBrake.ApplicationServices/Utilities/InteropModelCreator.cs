// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InteropModelCreator.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Utility Class to Convert a
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// A Utility Class to Convert a 
    /// </summary>
    public class InteropModelCreator
    {
        /// <summary>
        /// The get encode job.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <returns>
        /// The <see cref="EncodeJob"/>.
        /// </returns>
        public static EncodeJob GetEncodeJob(QueueTask task)
        {
            // Sanity Checking
            if (task == null || task.Task == null || task.Configuration == null)
            {
                return null;
            }

            return GetEncodeJob(task.Task, task.Configuration);
        }

        /// <summary>
        /// Get an EncodeJob model for a LibHB Encode.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// An Interop.EncodeJob model.
        /// </returns>
        public static EncodeJob GetEncodeJob(EncodeTask task, HBConfiguration configuration)
        {
            // The current Job Configuration
            EncodeTask work = task;

            // Which will be converted to this EncodeJob Model.
            EncodeJob job = new EncodeJob();

            // Audio Settings
            job.AudioEncodings = new List<AudioEncoding>();
            foreach (AudioTrack track in work.AudioTracks)
            {
                AudioEncoding newTrack = new AudioEncoding
                                             {
                                                 Bitrate = track.Bitrate, 
                                                 Drc = track.DRC, 
                                                 Gain = track.Gain, 
                                                 Encoder = Converters.GetCliAudioEncoder(track.Encoder), 
                                                 InputNumber = track.Track.HasValue ? track.Track.Value - 1 : 0, // It's 0 based index 
                                                 Mixdown = Converters.GetCliMixDown(track.MixDown), 
                                                 SampleRateRaw = GetSampleRateRaw(track.SampleRate), 
                                                 EncodeRateType = AudioEncodeRateType.Bitrate, 
                                                 Name = track.TrackName, 
                                                 IsPassthru = track.IsPassthru,
                                              };

                job.AudioEncodings.Add(newTrack);
            }

            // Title Settings
            job.OutputPath = work.Destination;
            job.SourcePath = work.Source;
            job.Title = work.Title;

            // job.SourceType = work.Type;
            switch (work.PointToPointMode)
            {
                case PointToPointMode.Chapters:
                    job.RangeType = VideoRangeType.Chapters;
                    break;
                case PointToPointMode.Seconds:
                    job.RangeType = VideoRangeType.Seconds;
                    break;
                case PointToPointMode.Frames:
                    job.RangeType = VideoRangeType.Frames;
                    break;
                case PointToPointMode.Preview:
                    job.RangeType = VideoRangeType.Preview;
                    break;
            }

            if (work.PointToPointMode == PointToPointMode.Seconds)
            {
                job.SecondsEnd = work.EndPoint;
                job.SecondsStart = work.StartPoint;
            }
            if (work.PointToPointMode == PointToPointMode.Chapters)
            {
                job.ChapterStart = work.StartPoint;
                job.ChapterEnd = work.EndPoint;
            }
            if (work.PointToPointMode == PointToPointMode.Frames)
            {
                job.FramesEnd = work.EndPoint;
                job.FramesStart = work.StartPoint;
            }

            if (work.PointToPointMode == PointToPointMode.Preview)
            {
                job.StartAtPreview = work.PreviewEncodeStartAt.HasValue ? work.PreviewEncodeStartAt.Value : 1;
                job.SecondsEnd = work.PreviewEncodeDuration.HasValue ? work.PreviewEncodeDuration.Value : 30;
                job.SeekPoints = configuration.PreviewScanCount;
            }

            job.Angle = work.Angle;

            // Output Settings
            job.IPod5GSupport = work.IPod5GSupport;
            job.Optimize = work.OptimizeMP4;
            switch (work.OutputFormat)
            {
                case OutputFormat.Mp4:
                    job.ContainerName = "av_mp4"; // TODO make part of enum.
                    break;
                case OutputFormat.Mkv:
                    job.ContainerName = "av_mkv"; // TODO make part of enum.
                    break;
            }

            // Picture Settings
            job.Anamorphic = work.Anamorphic;
            job.Cropping = new Cropping { Top = work.Cropping.Top, Bottom = work.Cropping.Bottom, Left = work.Cropping.Left, Right = work.Cropping.Right };
            job.DisplayWidth = work.DisplayWidth.HasValue ? int.Parse(Math.Round(work.DisplayWidth.Value, 0).ToString()) : 0;
            job.PixelAspectX = work.PixelAspectX;
            job.PixelAspectY = work.PixelAspectY;
            job.Height = work.Height.HasValue ? work.Height.Value : 0;
            job.KeepDisplayAspect = work.KeepDisplayAspect;
            job.MaxHeight = work.MaxHeight.HasValue ? work.MaxHeight.Value : 0;
            job.MaxWidth = work.MaxWidth.HasValue ? work.MaxWidth.Value : 0;
            job.Modulus = work.Modulus.HasValue ? work.Modulus.Value : 16;
            job.UseDisplayWidth = true;
            job.Width = work.Width.HasValue ? work.Width.Value : 0;

            // Filter Settings
            job.CustomDecomb = work.CustomDecomb;
            job.CustomDeinterlace = work.CustomDeinterlace;
            job.CustomDenoise = work.CustomDenoise;
            job.DenoisePreset = work.DenoisePreset.ToString().ToLower().Replace(" ", string.Empty);
            job.DenoiseTune = work.DenoiseTune.ToString().ToLower().Replace(" ", string.Empty);
            job.CustomDetelecine = work.CustomDetelecine;
            if (work.Deblock > 4)
            {
                job.Deblock = work.Deblock;
            }
            job.Decomb = work.Decomb;
            job.Deinterlace = work.Deinterlace;
            job.Denoise = work.Denoise;
            job.Detelecine = work.Detelecine;
            job.Grayscale = work.Grayscale;

            // Video Settings
            job.Framerate = work.Framerate.HasValue ? work.Framerate.Value : 0;
            job.ConstantFramerate = work.FramerateMode == FramerateMode.CFR;
            job.PeakFramerate = work.FramerateMode == FramerateMode.PFR;
            job.Quality = work.Quality.HasValue ? work.Quality.Value : 0;
            job.VideoBitrate = work.VideoBitrate.HasValue ? work.VideoBitrate.Value : 0;
            job.VideoEncodeRateType = work.VideoEncodeRateType;
            job.VideoEncoder = Converters.GetVideoEncoder(work.VideoEncoder);
            job.TwoPass = work.TwoPass;
            job.TurboFirstPass = work.TurboFirstPass;

            if (work.VideoEncoder == VideoEncoder.X264 || work.VideoEncoder == VideoEncoder.X265 || work.VideoEncoder == VideoEncoder.QuickSync)
            {
                job.VideoPreset = work.VideoPreset.ShortName;
                job.VideoProfile = work.VideoProfile.ShortName; 
                job.VideoLevel = work.VideoLevel.ShortName;

                if (work.VideoEncoder != VideoEncoder.QuickSync)
                {
                    job.VideoTunes = new List<string>();
                    foreach (var item in work.VideoTunes)
                    {
                        job.VideoTunes.Add(item.ShortName);
                    }
                }     
            }

            // Chapter Markers
            job.IncludeChapterMarkers = work.IncludeChapterMarkers;
            job.CustomChapterNames = work.ChapterNames.Select(item => item.ChapterName).ToList();
            job.UseDefaultChapterNames = work.IncludeChapterMarkers;

            // Advanced Settings
            job.VideoOptions = work.ShowAdvancedTab ? work.AdvancedEncoderOptions : work.ExtraAdvancedArguments;

            // Subtitles
            job.Subtitles = new Subtitles { SourceSubtitles = new List<SourceSubtitle>(), SrtSubtitles = new List<SrtSubtitle>() };
            foreach (SubtitleTrack track in work.SubtitleTracks)
            {
                if (track.IsSrtSubtitle)
                {
                    job.Subtitles.SrtSubtitles.Add(
                        new SrtSubtitle
                            {
                                CharacterCode = track.SrtCharCode, 
                                Default = track.Default, 
                                FileName = track.SrtPath, 
                                LanguageCode = track.SrtLang, 
                                Offset = track.SrtOffset, 
                                BurnedIn = track.Burned
                            });
                }
                else
                {
                    if (track.SourceTrack != null)
                    {
                        job.Subtitles.SourceSubtitles.Add(
                            new SourceSubtitle { BurnedIn = track.Burned, Default = track.Default, Forced = track.Forced, TrackNumber = track.SourceTrack.TrackNumber });
                    }
                }
            }

            return job;
        }

        /// <summary>
        /// Get the Raw Sample Rate
        /// </summary>
        /// <param name="rate">
        /// The rate.
        /// </param>
        /// <returns>
        /// The Raw sample rate as an int
        /// </returns>
        private static int GetSampleRateRaw(double rate)
        {
            if (rate == 22.05)
                return 22050;
            else if (rate == 24)
                return 24000;
            else if (rate == 44.1)
                return 32000;
            else if (rate == 48)
                return 48000;
            else return 48000;
        }
    }
}
