// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encode factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Factories
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Factories;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Helpers;
    using HandBrake.ApplicationServices.Interop.Json.Anamorphic;
    using HandBrake.ApplicationServices.Interop.Json.Encode;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;

    using AudioTrack = HandBrake.ApplicationServices.Services.Encode.Model.Models.AudioTrack;
    using Subtitle = HandBrake.ApplicationServices.Interop.Json.Encode.Subtitle;

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
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="JsonEncodeObject"/>.
        /// </returns>
        internal static JsonEncodeObject Create(EncodeTask job, SourceVideoInfo title, HBConfiguration configuration)
        {
            JsonEncodeObject encode = new JsonEncodeObject
                {
                    SequenceID = 0, 
                    Audio = CreateAudio(job), 
                    Destination = CreateDestination(job), 
                    Filter = CreateFilter(job, title), 
                    PAR = CreatePAR(job, title), 
                    MetaData = CreateMetaData(job),
                    Source = CreateSource(job, configuration), 
                    Subtitle = CreateSubtitle(job), 
                    Video = CreateVideo(job)
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
                    range.ChapterEnd = job.EndPoint;
                    range.ChapterStart = job.StartPoint;
                    break;
                case PointToPointMode.Seconds:
                    range.PtsToStart = job.StartPoint * 90000;
                    range.PtsToStop = (job.EndPoint - job.StartPoint) * 90000; 
                    break;
                case PointToPointMode.Frames:
                    range.FrameToStart = job.StartPoint;
                    range.FrameToStop = job.EndPoint; 
                    break;
                case PointToPointMode.Preview:
                    range.StartAtPreview = job.PreviewEncodeStartAt;
                    range.SeekPoints = configuration.PreviewScanCount;
                    range.PtsToStop = job.PreviewEncodeDuration * 90000; 
                    break;
            }

            Source source = new Source
            {
                Title = job.Title, 
                Range = range,
                Angle = job.Angle
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
                ChapterList = new List<ChapterList>()
            };

            if (job.IncludeChapterMarkers)
            {
                foreach (ChapterMarker item in job.ChapterNames)
                {
                    ChapterList chapter = new ChapterList { Name = item.ChapterName };
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
        /// <param name="title">
        /// The title.
        /// </param>
        /// <returns>
        /// The produced PAR object.
        /// </returns>
        private static PAR CreatePAR(EncodeTask job, SourceVideoInfo title)
        {
            Geometry resultGeometry = AnamorphicFactory.CreateGeometry(job, title, AnamorphicFactory.KeepSetting.HB_KEEP_WIDTH);
            return new PAR { Num = resultGeometry.PAR.Num, Den = resultGeometry.PAR.Den };
        }

        /// <summary>
        /// The create subtitle.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="HandBrake.ApplicationServices.Interop.Json.Encode.Subtitle"/>.
        /// </returns>
        private static Subtitle CreateSubtitle(EncodeTask job)
        {
            Subtitle subtitle = new Subtitle
                {
                    Search =
                        new Search
                            {
                                Enable = false, 
                                Default = false, 
                                Burn = false, 
                                Forced = false
                            }, 
                    SubtitleList = new List<SubtitleList>()
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
                        SubtitleList track = new SubtitleList { Burn = item.Burned, Default = item.Default, Force = item.Forced, ID = item.SourceTrack.TrackNumber, Track = (item.SourceTrack.TrackNumber - 1) };
                        subtitle.SubtitleList.Add(track);
                    }
                }
                else
                {
                    SubtitleList track = new SubtitleList
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
        /// <returns>
        /// The <see cref="Video"/>.
        /// </returns>
        private static Video CreateVideo(EncodeTask job)
        {
            Video video = new Video();

            HBVideoEncoder videoEncoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(e => e.ShortName == ApplicationServices.Utilities.Converters.GetVideoEncoder(job.VideoEncoder));
            Validate.NotNull(videoEncoder, "Video encoder " + job.VideoEncoder + " not recognized.");
            if (videoEncoder != null)
            {
                video.Codec = videoEncoder.Id;
            }

            string advancedOptions = job.ShowAdvancedTab ? job.AdvancedEncoderOptions : string.Empty;
            if (!string.IsNullOrEmpty(advancedOptions))
            {
                video.Options = advancedOptions;
            }
            else
            {
                video.Level = job.VideoLevel.ShortName;
                video.Options = job.ExtraAdvancedArguments;
                video.Preset = job.VideoPreset.ShortName;
                video.Profile = job.VideoProfile.ShortName;
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
                video.Tune = string.Join(",", job.VideoTunes);
            }

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

            // TODO Handled on the front-end ? Maybe we can offload logic.
            //if (!string.IsNullOrEmpty(job.AudioEncoderFallback))
            //{
            //    HBAudioEncoder audioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(job.AudioEncoderFallback);
            //    Validate.NotNull(audioEncoder, "Unrecognized fallback audio encoder: " + job.AudioEncoderFallback);
            //    audio.FallbackEncoder = audioEncoder.Id;
            //}

            audio.CopyMask = (int)NativeConstants.HB_ACODEC_ANY;

            audio.AudioList = new List<AudioList>();
            foreach (AudioTrack item in job.AudioTracks)
            {
                HBAudioEncoder encoder = HandBrakeEncoderHelpers.GetAudioEncoder(ApplicationServices.Utilities.Converters.GetCliAudioEncoder(item.Encoder) );
                Validate.NotNull(encoder, "Unrecognized audio encoder:" + item.Encoder);

                HBMixdown mixdown = HandBrakeEncoderHelpers.GetMixdown(ApplicationServices.Utilities.Converters.GetCliMixDown(item.MixDown));
                Validate.NotNull(mixdown, "Unrecognized audio mixdown:" + ApplicationServices.Utilities.Converters.GetCliMixDown(item.MixDown));

                AudioList audioTrack = new AudioList
                    {
                        Track = (item.Track.HasValue ? item.Track.Value : 0) - 1,
                        DRC = item.DRC, 
                        Encoder = encoder.Id, 
                        Gain = item.Gain, 
                        Mixdown = mixdown.Id, 
                        NormalizeMixLevel = false,
                        Samplerate = GetSampleRateRaw(item.SampleRate),
                        Name = item.TrackName,
                    };

                if (!item.IsPassthru)
                {
                    // TODO Impiment Quality and Compression. We only support bitrate right now.
                    //if (item.EncodeRateType == AudioEncodeRateType.Quality)
                    //{
                    //    audioTrack.Quality = item.Quality;
                    //}

                    //if (item.EncodeRateType == AudioEncodeRateType.Compression)
                    //{
                    //    audioTrack.CompressionLevel = item.Compression;
                    //}

                    //if (item.EncodeRateType == AudioEncodeRateType.Bitrate)
                   // {
                        audioTrack.Bitrate = item.Bitrate;
                   // }
                }

                audio.AudioList.Add(audioTrack);
            }

            return audio;
        }

        /// <summary>
        /// The get sample rate raw.
        /// </summary>
        /// <param name="rate">
        /// The rate.
        /// </param>
        /// <returns>
        /// The <see cref="int"/>.
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

        /// <summary>
        /// The create filter.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <returns>
        /// The <see cref="Filter"/>.
        /// </returns>
        private static Filter CreateFilter(EncodeTask job, SourceVideoInfo title)
        {
            Filter filter = new Filter
                            {
                                FilterList = new List<FilterList>(), 
                                Grayscale = job.Grayscale
                            };

            // Detelecine
            if (job.Detelecine != Detelecine.Off)
            {
                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DETELECINE, Settings = job.CustomDetelecine };
                filter.FilterList.Add(filterItem);
            }

            // Decomb
            if (job.Decomb != Decomb.Off)
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

                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DECOMB, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // Deinterlace
            if (job.Deinterlace != Deinterlace.Off)
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

                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DEINTERLACE, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // VFR / CFR
            int fm = job.FramerateMode == FramerateMode.CFR ? 1 : job.FramerateMode == FramerateMode.PFR ? 2 : 0;
            IntPtr frameratePrt = Marshal.StringToHGlobalAnsi(job.Framerate.ToString()); // TODO check culture
            int vrate = HBFunctions.hb_video_framerate_get_from_name(frameratePrt);

            int num;
            int den;
            if (vrate > 0)
            {
                num = 27000000;
                den = vrate;
            }
            else
            {
                // cfr or pfr flag with no rate specified implies use the title rate.
                num = title.FramerateNumerator;
                den = title.FramerateDenominator;
            }

            string framerateString = string.Format("{0}:{1}:{2}", fm, num, den); // filter_cfr, filter_vrate.num, filter_vrate.den

            FilterList framerateShaper = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_VFR, Settings = framerateString };
            filter.FilterList.Add(framerateShaper);

            // Deblock
            if (job.Deblock >= 5)
            {
                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DEBLOCK, Settings = job.Deblock.ToString() };
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

                FilterList filterItem = new FilterList { ID = (int)id, Settings = settings };
                filter.FilterList.Add(filterItem);
            }

            // CropScale Filter
            Geometry resultGeometry = AnamorphicFactory.CreateGeometry(job, title, AnamorphicFactory.KeepSetting.HB_KEEP_WIDTH);
            FilterList cropScale = new FilterList
            {
                ID = (int)hb_filter_ids.HB_FILTER_CROP_SCALE, 
                Settings =
                    string.Format(
                        "{0}:{1}:{2}:{3}:{4}:{5}", 
                        resultGeometry.Width, 
                        resultGeometry.Height, 
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
        /// The <see cref="MetaData"/>.
        /// </returns>
        private static MetaData CreateMetaData(EncodeTask job)
        {
            MetaData metaData = new MetaData();

            /* TODO  NOT SUPPORTED YET. */
            return metaData;
        }
    }
}
