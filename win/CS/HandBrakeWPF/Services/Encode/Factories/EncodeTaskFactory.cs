// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeTaskFactory.cs" company="HandBrake Project (http://handbrake.fr)">
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

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.Shared;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json.Linq;

    using AudioEncoder = Model.Models.AudioEncoder;
    using AudioEncoderRateType = Model.Models.AudioEncoderRateType;
    using AudioTrack = Model.Models.AudioTrack;
    using ChapterMarker = Model.Models.ChapterMarker;
    using EncodeTask = Model.EncodeTask;
    using FramerateMode = Model.Models.FramerateMode;
    using OutputFormat = Model.Models.OutputFormat;
    using PointToPointMode = Model.Models.PointToPointMode;
    using Subtitle = HandBrake.Interop.Interop.Json.Encode.Subtitles;
    using SubtitleTrack = Model.Models.SubtitleTrack;
    using SystemInfo = HandBrake.Interop.Utilities.SystemInfo;
    using Validate = Helpers.Validate;

    /// <summary>
    /// This factory takes the internal EncodeJob object and turns it into a set of JSON models
    /// that can be deserialized by libhb.
    /// </summary>
    internal class EncodeTaskFactory
    {
        // TODO make this non-static
        private static IHbFunctions hbFunctions;

        /// <summary>
        /// The create.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <param name="hbFunctionsWrapper">
        /// An instance of IHbFunctions
        /// </param>
        /// <returns>
        /// The <see cref="JsonEncodeObject"/>.
        /// </returns>
        internal static JsonEncodeObject Create(EncodeTask job, HBConfiguration configuration, IHbFunctions hbFunctionsWrapper)
        {
            hbFunctions = hbFunctionsWrapper;

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
                    range.End = job.EndPoint * 90000;
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
                    IpodAtom = VideoEncoderHelpers.IsH264(job.VideoEncoder) ? job.IPod5GSupport : false,
                    Mp4Optimize = job.OptimizeMP4
                },
                ChapterMarkers = job.IncludeChapterMarkers,
                AlignAVStart = job.AlignAVStart,
                Mux = EnumHelper<OutputFormat>.GetShortName(job.OutputFormat), 
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
        /// The <see cref="HandBrake.Interop.Interop.Json.Encode.Subtitles"/>.
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
                SubtitleList = new List<HandBrake.Interop.Interop.Json.Encode.SubtitleTrack>()
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
                        HandBrake.Interop.Interop.Json.Encode.SubtitleTrack track = new HandBrake.Interop.Interop.Json.Encode.SubtitleTrack
                        {
                            Burn = item.Burned,
                            Default = item.Default,
                            Forced = item.Forced,
                            ID = item.SourceTrack.TrackNumber,
                            Track = (item.SourceTrack.TrackNumber - 1),
                            Name = item.Name
                        };

                        subtitle.SubtitleList.Add(track);
                    }
                }
                else
                {
                    HandBrake.Interop.Interop.Json.Encode.SubtitleTrack track = new HandBrake.Interop.Interop.Json.Encode.SubtitleTrack
                    {
                        Track = -1, // Indicates SRT
                        Default = item.Default,
                        Offset = item.SrtOffset,
                        Burn = item.Burned,
                        Name = item.Name,
                        Import =
                            new SubImport
                            {
                                Format = item.SrtPath.EndsWith("srt") ? "SRT" : "SSA",
                                Filename = item.SrtPath,
                                Codeset = item.SrtCharCode,
                                Language = item.SrtLangCode
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
                video.Encoder = videoEncoder.ShortName;
            }

            video.Level = job.VideoLevel?.ShortName;
            video.Options = job.ExtraAdvancedArguments;
            video.Preset = job.VideoPreset?.ShortName;
            video.Profile = job.VideoProfile?.ShortName;

            if (job.VideoTunes != null && job.VideoTunes.Count > 0)
            {
                foreach (var item in job.VideoTunes)
                {
                    video.Tune += string.IsNullOrEmpty(video.Tune) ? item.ShortName : "," + item.ShortName;
                }
            }

            if (job.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality)
            {
                video.Quality = job.Quality;
            }

            if (job.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
            {
                video.Bitrate = job.VideoBitrate;
                video.TwoPass = job.TwoPass;
                video.Turbo = job.TurboFirstPass;
            }

            video.QSV.Decode = SystemInfo.IsQsvAvailable && configuration.EnableQuickSyncDecoding;

            // The use of the QSV decoder is configurable for non QSV encoders.
            if (video.QSV.Decode && job.VideoEncoder != VideoEncoder.QuickSync && job.VideoEncoder != VideoEncoder.QuickSyncH265 && job.VideoEncoder != VideoEncoder.QuickSyncH26510b)
            {
                video.QSV.Decode = configuration.UseQSVDecodeForNonQSVEnc;
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

            List<string> copyMaskList = new List<string>();
            if (job.AllowedPassthruOptions.AudioAllowAACPass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.AacPassthru));
            if (job.AllowedPassthruOptions.AudioAllowAC3Pass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.Ac3Passthrough));
            if (job.AllowedPassthruOptions.AudioAllowDTSHDPass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.DtsHDPassthrough));
            if (job.AllowedPassthruOptions.AudioAllowDTSPass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.DtsPassthrough));
            if (job.AllowedPassthruOptions.AudioAllowEAC3Pass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.EAc3Passthrough));
            if (job.AllowedPassthruOptions.AudioAllowFlacPass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.FlacPassthru));
            if (job.AllowedPassthruOptions.AudioAllowMP3Pass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.Mp3Passthru));
            if (job.AllowedPassthruOptions.AudioAllowTrueHDPass) copyMaskList.Add(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.TrueHDPassthrough));
            audio.CopyMask = copyMaskList.ToArray();

            HBAudioEncoder audioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(job.AllowedPassthruOptions.AudioEncoderFallback));
            audio.FallbackEncoder = audioEncoder.ShortName;

            audio.AudioList = new List<HandBrake.Interop.Interop.Json.Encode.AudioTrack>();
            foreach (AudioTrack item in job.AudioTracks)
            {
                HBAudioEncoder encoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(item.Encoder));
                Validate.NotNull(encoder, "Unrecognized audio encoder:" + item.Encoder);

                if (item.IsPassthru && (item.ScannedTrack.Codec & encoder.Id) == 0)
                {
                    // We have an unsupported passthru. Rather than let libhb drop the track, switch it to the fallback.
                    encoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(job.AllowedPassthruOptions.AudioEncoderFallback));
                }

                HBMixdown mixdown = HandBrakeEncoderHelpers.GetMixdown(item.MixDown);

                HBRate sampleRate = HandBrakeEncoderHelpers.AudioSampleRates.FirstOrDefault(s => s.Name == item.SampleRate.ToString(CultureInfo.InvariantCulture));

                HandBrake.Interop.Interop.Json.Encode.AudioTrack audioTrack = new HandBrake.Interop.Interop.Json.Encode.AudioTrack
                {
                    Track = (item.Track.HasValue ? item.Track.Value : 0) - 1,
                    DRC = item.DRC,
                    Encoder = encoder.ShortName,
                    Gain = item.Gain,
                    Mixdown = mixdown != null ? mixdown.Id : -1,
                    NormalizeMixLevel = false,
                    Samplerate = sampleRate != null ? sampleRate.Rate : 0,
                    Name = !string.IsNullOrEmpty(item.TrackName) ? item.TrackName : null,
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

            // Note, order is important.

            // Detelecine
            if (job.Detelecine != Detelecine.Off)
            {
                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_DETELECINE, null, null, job.CustomDetelecine);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DETELECINE, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            // Deinterlace
            if (job.DeinterlaceFilter == DeinterlaceFilter.Yadif)
            {
                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_DEINTERLACE, job.DeinterlacePreset?.ShortName,  null, job.CustomDeinterlaceSettings);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken root = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DEINTERLACE, Settings = root };
                    filter.FilterList.Add(filterItem);
                }
            }

            // Decomb
            if (job.DeinterlaceFilter == DeinterlaceFilter.Decomb)
            {
                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_DECOMB, job.DeinterlacePreset?.ShortName, null, job.CustomDeinterlaceSettings);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DECOMB, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            if (job.DeinterlaceFilter == DeinterlaceFilter.Decomb || job.DeinterlaceFilter == DeinterlaceFilter.Yadif)
            {
                if (job.CombDetect != CombDetect.Off)
                {
                    IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_COMB_DETECT, EnumHelper<CombDetect>.GetShortName(job.CombDetect), null, job.CustomCombDetect);
                    string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                    if (!string.IsNullOrEmpty(unparsedJson))
                    {
                        JToken settings = JObject.Parse(unparsedJson);

                        Filter filterItem = new Filter
                                                {
                                                    ID = (int)hb_filter_ids.HB_FILTER_COMB_DETECT,
                                                    Settings = settings
                                                };
                        filter.FilterList.Add(filterItem);
                    }
                }    
            }

            // Denoise
            if (job.Denoise != Denoise.Off)
            {
                hb_filter_ids id = job.Denoise == Denoise.hqdn3d
                    ? hb_filter_ids.HB_FILTER_HQDN3D
                    : hb_filter_ids.HB_FILTER_NLMEANS;

                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)id, job.DenoisePreset.ToString().ToLower().Replace(" ", string.Empty), job.DenoiseTune.ToString().ToLower().Replace(" ", string.Empty), job.CustomDenoise);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);

                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)id, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            // Sharpen
            if (job.Sharpen != Sharpen.Off)
            {
                hb_filter_ids id = job.Sharpen == Sharpen.LapSharp
                    ? hb_filter_ids.HB_FILTER_LAPSHARP
                    : hb_filter_ids.HB_FILTER_UNSHARP;

                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)id, job.SharpenPreset.Key, job.SharpenTune.Key, job.SharpenCustom);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);

                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)id, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            // Deblock
            if (job.DeblockPreset != null && job.DeblockPreset.Key != "off")
            {
                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_DEBLOCK, job.DeblockPreset.Key, job.DeblockTune.Key, job.CustomDeblock);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_DEBLOCK, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            // CropScale Filter
            string cropSettings = string.Format("width={0}:height={1}:crop-top={2}:crop-bottom={3}:crop-left={4}:crop-right={5}", job.Width, job.Height, job.Cropping.Top, job.Cropping.Bottom, job.Cropping.Left, job.Cropping.Right);
            IntPtr cropSettingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_CROP_SCALE, null, null, cropSettings);
            string unparsedCropSettingsJson = Marshal.PtrToStringAnsi(cropSettingsPtr);
            if (!string.IsNullOrEmpty(unparsedCropSettingsJson))
            {
                JToken cropSettingsJson = JObject.Parse(unparsedCropSettingsJson);

                Filter cropScale = new Filter
                                       {
                                           ID = (int)hb_filter_ids.HB_FILTER_CROP_SCALE,
                                           Settings = cropSettingsJson
                                       };
                filter.FilterList.Add(cropScale);
            }

            // Grayscale
            if (job.Grayscale)
            {
                Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_GRAYSCALE, Settings = null };
                filter.FilterList.Add(filterItem);
            }

            // Rotate
            if (job.Rotation != 0 || job.FlipVideo)
            {
                string rotateSettings = string.Format("angle={0}:hflip={1}", job.Rotation, job.FlipVideo ? "1" : "0");
                IntPtr settingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_ROTATE, null, null, rotateSettings);
                string unparsedJson = Marshal.PtrToStringAnsi(settingsPtr);
                if (!string.IsNullOrEmpty(unparsedJson))
                {
                    JToken settings = JObject.Parse(unparsedJson);

                    Filter filterItem = new Filter { ID = (int)hb_filter_ids.HB_FILTER_ROTATE, Settings = settings };
                    filter.FilterList.Add(filterItem);
                }
            }

            // Framerate shaping filter
            int fm = job.FramerateMode == FramerateMode.CFR ? 1 : job.FramerateMode == FramerateMode.PFR ? 2 : 0;
            int? num = null, den = null;
            if (job.Framerate != null)
            {
                IntPtr frameratePrt = Marshal.StringToHGlobalAnsi(job.Framerate.Value.ToString(CultureInfo.InvariantCulture));
                int vrate = hbFunctions.hb_video_framerate_get_from_name(frameratePrt);

                if (vrate > 0)
                {
                    num = 27000000;
                    den = vrate;
                }
            }

            string framerateString = num.HasValue ? string.Format("mode={0}:rate={1}/{2}", fm, num, den) : string.Format("mode={0}", fm); // filter_cfr, filter_vrate.num, filter_vrate.den
            IntPtr framerateSettingsPtr = hbFunctions.hb_generate_filter_settings_json((int)hb_filter_ids.HB_FILTER_VFR, null, null, framerateString);
            string unparsedFramerateJson = Marshal.PtrToStringAnsi(framerateSettingsPtr);
            if (!string.IsNullOrEmpty(unparsedFramerateJson))
            {
                JToken framerateSettings = JObject.Parse(unparsedFramerateJson);

                Filter framerateShaper = new Filter
                                             {
                                                 ID = (int)hb_filter_ids.HB_FILTER_VFR,
                                                 Settings = framerateSettings
                                             };
                filter.FilterList.Add(framerateShaper);
            }

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

            if (job.MetaData != null)
            {
                metaData.Artist = job.MetaData.Artist;
                metaData.Album = job.MetaData.Album;
                metaData.AlbumArtist = job.MetaData.AlbumArtist;
                metaData.Comment = job.MetaData.Comment;
                metaData.Composer = job.MetaData.Composer;
                metaData.Description = job.MetaData.Description;
                metaData.Genre = job.MetaData.Genre;
                metaData.LongDescription = job.MetaData.LongDescription;
                metaData.Name = job.MetaData.Name;
                metaData.ReleaseDate = job.MetaData.ReleaseDate;
            }

            return metaData;
        }
    }
}
