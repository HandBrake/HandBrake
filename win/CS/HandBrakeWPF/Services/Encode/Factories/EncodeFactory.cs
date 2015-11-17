// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encode factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Helpers;
    using HandBrake.ApplicationServices.Interop.Json.Encode;
    using HandBrake.ApplicationServices.Interop.Json.Shared;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Utilities;

    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using AudioEncoderRateType = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType;
    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;
    using ChapterMarker = HandBrakeWPF.Services.Encode.Model.Models.ChapterMarker;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using FramerateMode = HandBrakeWPF.Services.Encode.Model.Models.FramerateMode;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using PointToPointMode = HandBrakeWPF.Services.Encode.Model.Models.PointToPointMode;
    using Subtitle = HandBrake.ApplicationServices.Interop.Json.Encode.Subtitles;
    using SubtitleTrack = HandBrakeWPF.Services.Encode.Model.Models.SubtitleTrack;
    using Validate = HandBrakeWPF.Helpers.Validate;

    /// <summary>
    /// This factory takes the internal EncodeJob object and turns it into a set of JSON models
    /// that can be deserialized by libhb.
    /// </summary>
    internal class EncodeFactory
    {
        /*
         * TODO:
         * 1. OpenCL and HWD Support 
         * 2. Rotate Support
         */

        /// <summary>
        /// The create.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="JsonEncodeObject"/>.
        /// </returns>
        internal static JsonEncodeObject Create(EncodeTask job, HBConfiguration configuration)
        {
            JsonEncodeObject encode = new JsonEncodeObject
                {
                    SequenceID = 0,
                    Audio = CreateAudio(job),
                    Destination = CreateDestination(job),
                    Filters = CreateFilters(job),
                    PAR = CreatePAR(job),
                    Metadata = CreateMetadata(job),
                    Source = CreateSource(job, configuration),
                    Subtitle = CreateSubtitle(job),
                    Video = CreateVideo(job, configuration)
                };

            return encode;
        }

        /// <summary>
        /// The create source.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="Source"/>.
        /// </returns>
        private static Source CreateSource(EncodeTask job, HBConfiguration configuration)
        {
            Range range = new Range();
            switch (job.PointToPointMode)
            {
                case PointToPointMode.Chapters:
                    range.Type = "chapter";
                    range.Start = job.StartPoint;
                    range.End = job.EndPoint;
                    break;
                case PointToPointMode.Seconds:
                    range.Type = "time";
                    range.Start = job.StartPoint * 90000;
                    range.End = (job.EndPoint - job.StartPoint) * 90000;
                    break;
                case PointToPointMode.Frames:
                    range.Type = "frame";
                    range.Start = job.StartPoint;
                    range.End = job.EndPoint;
                    break;
                case PointToPointMode.Preview:
                    range.Type = "preview";
                    range.Start = job.PreviewEncodeStartAt;
                    range.SeekPoints = configuration.PreviewScanCount;
                    range.End = job.PreviewEncodeDuration * 90000;
                    break;
            }

            Source source = new Source
            {
                Title = job.Title,
                Range = range,
                Angle = job.Angle,
                Path = job.Source,
            };
            return source;
        }

        /// <summary>
        /// The create destination.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Destination"/>.
        /// </returns>
        private static Destination CreateDestination(EncodeTask job)
        {
            Destination destination = new Destination
            {
                File = job.Destination,
                Mp4Options = new Mp4Options
                                 {
                                     IpodAtom = job.IPod5GSupport,
                                     Mp4Optimize = job.OptimizeMP4
                                 },
                ChapterMarkers = job.IncludeChapterMarkers,
                Mux = HBFunctions.hb_container_get_from_name(job.OutputFormat == OutputFormat.Mp4 ? "av_mp4" : "av_mkv"), // TODO tidy up.
                ChapterList = new List<Chapter>()
            };

            if (job.IncludeChapterMarkers)
            {
                foreach (ChapterMarker item in job.ChapterNames)
                {
                    Chapter chapter = new Chapter { Name = item.ChapterName };
                    destination.ChapterList.Add(chapter);
                }
            }

            return destination;
        }

        /// <summary>
        /// Create the PAR object
        /// </summary>
        /// <param name="job">
        /// The Job
        /// </param>
        /// <returns>
        /// The produced PAR object.
        /// </returns>
        private static PAR CreatePAR(EncodeTask job)
        {
            return new PAR { Num = job.PixelAspectX, Den = job.PixelAspectY };
        }

        /// <summary>
        /// The create subtitle.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="HandBrake.ApplicationServices.Interop.Json.Encode.Subtitles"/>.
        /// </returns>
        private static Subtitle CreateSubtitle(EncodeTask job)
        {
            Subtitles subtitle = new Subtitles
                {
                    Search =
                        new SubtitleSearch
                            {
                                Enable = false,
                                Default = false,
                                Burn = false,
                                Forced = false
                            },
                    SubtitleList = new List<HandBrake.ApplicationServices.Interop.Json.Encode.SubtitleTrack>()
                };

            foreach (SubtitleTrack item in job.SubtitleTracks)
            {
                if (!item.IsSrtSubtitle)
                {
                    // Handle Foreign Audio Search
                    if (item.SourceTrack.TrackNumber == 0)
                    {
                        subtitle.Search.Enable = true;
                        subtitle.Search.Burn = item.Burned;
                        subtitle.Search.Default = item.Default;
                        subtitle.Search.Forced = item.Forced;
                    }
                    else
                    {
                        HandBrake.ApplicationServices.Interop.Json.Encode.SubtitleTrack track = new HandBrake.ApplicationServices.Interop.Json.Encode.SubtitleTrack
                        {
                            Burn = item.Burned,
                            Default = item.Default,
                            Forced = item.Forced,
                            ID = item.SourceTrack.TrackNumber,
                            Track = (item.SourceTrack.TrackNumber - 1)
                        };

                        subtitle.SubtitleList.Add(track);
                    }
                }
                else
                {
                    HandBrake.ApplicationServices.Interop.Json.Encode.SubtitleTrack track = new HandBrake.ApplicationServices.Interop.Json.Encode.SubtitleTrack
                    {
                        Track = -1, // Indicates SRT
                        Default = item.Default,
                        Offset = item.SrtOffset,
                        Burn = item.Burned,
                        SRT =
                            new SRT
                            {
                                Filename = item.SrtPath,
                                Codeset = item.SrtCharCode,
                                Language = item.SrtLang
                            }
                    };

                    subtitle.SubtitleList.Add(track);
                }
            }

            return subtitle;
        }

        /// <summary>
        /// The create video.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="Video"/>.
        /// </returns>
        private static Video CreateVideo(EncodeTask job, HBConfiguration configuration)
        {
            Video video = new Video();

            HBVideoEncoder videoEncoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(e => e.ShortName == EnumHelper<VideoEncoder>.GetShortName(job.VideoEncoder));
            Validate.NotNull(videoEncoder, "Video encoder " + job.VideoEncoder + " not recognized.");
            if (videoEncoder != null)
            {
                video.Encoder = videoEncoder.Id;
            }

            string advancedOptions = job.ShowAdvancedTab ? job.AdvancedEncoderOptions : string.Empty;
            if (!string.IsNullOrEmpty(advancedOptions))
            {
                video.Options = advancedOptions;
            }
            else
            {
                video.Level = job.VideoLevel != null ? job.VideoLevel.ShortName : null;
                video.Options = job.ExtraAdvancedArguments;
                video.Preset = job.VideoPreset != null ? job.VideoPreset.ShortName : null;
                video.Profile = job.VideoProfile != null ? job.VideoProfile.ShortName : null;
            }

            if (job.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality) video.Quality = job.Quality;
            if (job.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
            {
                video.Bitrate = job.VideoBitrate;
                video.TwoPass = job.TwoPass;
                video.Turbo = job.TurboFirstPass;
            }

            if (job.VideoTunes != null && job.VideoTunes.Count > 0)
            {
                foreach (var item in job.VideoTunes)
                {
                    video.Tune += string.IsNullOrEmpty(video.Tune) ? item.ShortName : "," + item.ShortName;
                }
            }

            video.OpenCL = configuration.ScalingMode == VideoScaler.BicubicCl;
            video.HWDecode = configuration.EnableDxva;
            video.QSV.Decode = !configuration.DisableQuickSyncDecoding;

            return video;
        }

        /// <summary>
        /// The create audio.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Audio"/>.
        /// </returns>
        private static Audio CreateAudio(EncodeTask job)
        {
            Audio audio = new Audio();

            List<uint> copyMaskList = new List<uint>();
            if (job.AllowedPassthruOptions.AudioAllowAACPass) copyMaskList.Add(NativeConstants.HB_ACODEC_AAC_PASS);
            if (job.AllowedPassthruOptions.AudioAllowAC3Pass) copyMaskList.Add(NativeConstants.HB_ACODEC_AC3_PASS);
            if (job.AllowedPassthruOptions.AudioAllowDTSHDPass) copyMaskList.Add(NativeConstants.HB_ACODEC_DCA_HD_PASS);
            if (job.AllowedPassthruOptions.AudioAllowDTSPass) copyMaskList.Add(NativeConstants.HB_ACODEC_DCA_PASS);
            if (job.AllowedPassthruOptions.AudioAllowEAC3Pass) copyMaskList.Add(NativeConstants.HB_ACODEC_EAC3_PASS);
            if (job.AllowedPassthruOptions.AudioAllowFlacPass) copyMaskList.Add(NativeConstants.HB_ACODEC_FLAC_PASS);
            if (job.AllowedPassthruOptions.AudioAllowMP3Pass) copyMaskList.Add(NativeConstants.HB_ACODEC_MP3_PASS);
            if (job.AllowedPassthruOptions.AudioAllowTrueHDPass) copyMaskList.Add(NativeConstants.HB_ACODEC_TRUEHD_PASS);
            audio.CopyMask = copyMaskList.ToArray();

            HBAudioEncoder audioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(job.AllowedPassthruOptions.AudioEncoderFallback));
            audio.FallbackEncoder = audioEncoder.Id;

            audio.AudioList = new List<HandBrake.ApplicationServices.Interop.Json.Encode.AudioTrack>();
            foreach (AudioTrack item in job.AudioTracks)
            {
                HBAudioEncoder encoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(item.Encoder));
                Validate.NotNull(encoder, "Unrecognized audio encoder:" + item.Encoder);

                HBMixdown mixdown = HandBrakeEncoderHelpers.GetMixdown(EnumHelper<Mixdown>.GetShortName(item.MixDown));
                Validate.NotNull(mixdown, "Unrecognized audio mixdown:" + item.MixDown);

                HBRate sampleRate = HandBrakeEncoderHelpers.AudioSampleRates.FirstOrDefault(s => s.Name == item.SampleRate.ToString(CultureInfo.InvariantCulture));

                HandBrake.ApplicationServices.Interop.Json.Encode.AudioTrack audioTrack = new HandBrake.ApplicationServices.Interop.Json.Encode.AudioTrack
                    {
                        Track = (item.Track.HasValue ? item.Track.Value : 0) - 1,
                        DRC = item.DRC,
                        Encoder = encoder.Id,
                        Gain = item.Gain,
                        Mixdown = mixdown.Id,
                        NormalizeMixLevel = false,
                        Samplerate = sampleRate != null ? sampleRate.Rate : 0,
                        Name = item.TrackName,
                    };

                if (!item.IsPassthru)
                {
                     if (item.EncoderRateType == AudioEncoderRateType.Quality)
                     {
                        audioTrack.Quality = item.Quality;
                     }

                     if (item.EncoderRateType == AudioEncoderRateType.Bitrate)
                     {
                        audioTrack.Bitrate = item.Bitrate;
                     }
                }

                audio.AudioList.Add(audioTrack);
            }

            return audio;
        }

        /// <summary>
        /// The create filter.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Filters"/>.
        /// </returns>
        private static Filters CreateFilters(EncodeTask job)
        {
            Filters filter = new Filters
                            {
                                FilterList = new List<Filter>(),
                            };

            // Detelecine
            if (job.Detelecine != Detelecine.Off)
            {
                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DETELECINE, Settings = job.CustomDetelecine };
                filter.FilterList.Add(filterItem);
            }

            // Decomb
            if (job.DeinterlaceFilter == DeinterlaceFilter.Decomb)
            {
                string options;
                if (job.Decomb == Decomb.Fast)
                {
                    options = "7:2:6:9:1:80";
                }
                else if (job.Decomb == Decomb.Bob)
                {
                    options = "455";
                }
                else
                {
                    options = job.CustomDecomb;
                }

                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DECOMB, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // Deinterlace
            if (job.DeinterlaceFilter == DeinterlaceFilter.Deinterlace)
            {
                string options;
                if (job.Deinterlace == Deinterlace.Fast)
                {
                    options = "0";
                }
                else if (job.Deinterlace == Deinterlace.Slow)
                {
                    options = "1";
                }
                else if (job.Deinterlace == Deinterlace.Slower)
                {
                    options = "3";
                }
                else if (job.Deinterlace == Deinterlace.Bob)
                {
                    options = "15";
                }
                else
                {
                    options = job.CustomDeinterlace;
                }

                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DEINTERLACE, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // VFR / CFR
            int fm = job.FramerateMode == FramerateMode.CFR ? 1 : job.FramerateMode == FramerateMode.PFR ? 2 : 0;
            if (job.Framerate != null)
            {
                IntPtr frameratePrt = Marshal.StringToHGlobalAnsi(job.Framerate.Value.ToString(CultureInfo.InvariantCulture));
                int vrate = HBFunctions.hb_video_framerate_get_from_name(frameratePrt);

                int? num = null;
                int? den = null;
                if (vrate > 0)
                {
                    num = 27000000;
                    den = vrate;
                }

                string framerateString = num.HasValue ? string.Format("{0}:{1}:{2}", fm, num, den) : string.Format("{0}", fm); // filter_cfr, filter_vrate.num, filter_vrate.den

                Filter framerateShaper = new Filter { ID = (int)hb_filter_ids.HB_FILTER_VFR, Settings = framerateString };
                filter.FilterList.Add(framerateShaper);
            }

            // Deblock
            if (job.Deblock >= 5)
            {
                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DEBLOCK, Settings = job.Deblock.ToString() };
                filter.FilterList.Add(filterItem);
            }

            // Denoise
            if (job.Denoise != Denoise.Off)
            {
                hb_filter_ids id = job.Denoise == Denoise.hqdn3d
                    ? hb_filter_ids.HB_FILTER_HQDN3D
                    : hb_filter_ids.HB_FILTER_NLMEANS;

                string settings;
                if (!string.IsNullOrEmpty(job.CustomDenoise))
                {
                    settings = job.CustomDenoise;
                }
                else
                {
                    IntPtr settingsPtr = HBFunctions.hb_generate_filter_settings((int)id, job.DenoisePreset.ToString().ToLower().Replace(" ", string.Empty), job.DenoiseTune.ToString().ToLower().Replace(" ", string.Empty));
                    settings = Marshal.PtrToStringAnsi(settingsPtr);
                }

                Filter filterItem = new Filter { ID = (int)id, Settings = settings };
                filter.FilterList.Add(filterItem);
            }

            if (job.Grayscale)
            {
                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_GRAYSCALE, Settings = null };
                filter.FilterList.Add(filterItem);
            }

            // CropScale Filter
            Filter cropScale = new Filter
            {
                ID = (int)hb_filter_ids.HB_FILTER_CROP_SCALE,
                Settings =
                    string.Format(
                        "{0}:{1}:{2}:{3}:{4}:{5}",
                        job.Width,
                        job.Height,
                        job.Cropping.Top,
                        job.Cropping.Bottom,
                        job.Cropping.Left,
                        job.Cropping.Right)
            };
            filter.FilterList.Add(cropScale);

            // Rotate
            /* TODO  NOT SUPPORTED YET. */

            return filter;
        }

        /// <summary>
        /// The create meta data.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Metadata"/>.
        /// </returns>
        private static Metadata CreateMetadata(EncodeTask job)
        {
            Metadata metaData = new Metadata();

            /* TODO  NOT SUPPORTED YET. */
            return metaData;
        }
    }
}
