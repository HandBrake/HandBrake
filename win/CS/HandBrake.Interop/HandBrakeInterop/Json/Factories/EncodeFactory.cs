// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encode factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Factories
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.HbLib;
    using HandBrake.Interop.Helpers;
    using HandBrake.Interop.Json.Anamorphic;
    using HandBrake.Interop.Json.Encode;
    using HandBrake.Interop.Json.Scan;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;

    using Newtonsoft.Json;

    using AudioList = HandBrake.Interop.Json.Encode.AudioList;
    using ChapterList = HandBrake.Interop.Json.Encode.ChapterList;
    using MetaData = HandBrake.Interop.Json.Encode.MetaData;
    using PAR = HandBrake.Interop.Json.Anamorphic.PAR;
    using SubtitleList = HandBrake.Interop.Json.Encode.SubtitleList;

    /// <summary>
    /// This factory takes the internal EncodeJob object and turns it into a set of JSON models
    /// that can be deserialized by libhb.
    /// </summary>
    internal class EncodeFactory
    {

        /*
         * <j45> maxWidth and maxHeight are frontend issues.  You pass those into hb_set_anamorphic_size when calculating geometry settings.  width and height are given to the CROP_SCALE filter.  No need for the frontend to set it in the job.  The job will get the final dimensions from the filter settings anyway.
         * <j45> for example, both crop_scale and rotate filters modify job width and height settings
         *
         */

        /// <summary>
        /// The create.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <returns>
        /// The <see cref="JsonEncodeObject"/>.
        /// </returns>
        internal static JsonEncodeObject Create(EncodeJob job, JsonScanObject scannedSource)
        {
            JsonEncodeObject encode = new JsonEncodeObject
                {
                    SequenceID = 0,
                    Audio = CreateAudio(job),
                    Destination = CreateDestination(job),
                    Filter = CreateFilter(job),
                    PAR = CreatePAR(job),
                    MetaData = CreateMetaData(job),
                    Source = CreateSource(job),
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
        /// <returns>
        /// The <see cref="Source"/>.
        /// </returns>
        private static Source CreateSource(EncodeJob job)
        {
            Source source = new Source
            {
                Title = job.Title,
                Range =
                    new Range
                        {
                            ChapterEnd = job.ChapterEnd,
                            ChapterStart = job.ChapterStart,
                            FrameToStart = job.FramesStart,
                            FrameToStop = job.FramesEnd,
                            PtsToStart = (int)(job.SecondsStart * 90000),
                            PtsToStop = (int)((job.SecondsEnd - job.SecondsStart) * 90000),
                        },
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
        private static Destination CreateDestination(EncodeJob job)
        {
            Destination destination = new Destination
            {
                File = job.OutputPath,
                Mp4Options =
                    new Mp4Options
                        {
                            IpodAtom = job.EncodingProfile.IPod5GSupport,
                           // LargeFileSize = job.EncodingProfile.LargeFile,
                            Mp4Optimize = job.EncodingProfile.Optimize
                        },
                ChapterMarkers = job.EncodingProfile.IncludeChapterMarkers,
                Mux = HBFunctions.hb_container_get_from_name(job.EncodingProfile.ContainerName),
                ChapterList = new List<ChapterList>()
            };

            foreach (string item in job.CustomChapterNames)
            {
                ChapterList chapter = new ChapterList { Name = item };
                destination.ChapterList.Add(chapter);
            }

            return destination;
        }

        /// <summary>
        /// Create the PAR object
        /// </summary>
        /// <param name="job">The Job</param>
        /// <returns>The produced PAR object.</returns>
        private static PAR CreatePAR(EncodeJob job)
        {
            return new PAR {Num = job.EncodingProfile.PixelAspectX, Den = job.EncodingProfile.PixelAspectY} ;
        }

        /// <summary>
        /// The create subtitle.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Subtitle"/>.
        /// </returns>
        private static Subtitle CreateSubtitle(EncodeJob job)
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

            foreach (SourceSubtitle item in job.Subtitles.SourceSubtitles)
            {
                SubtitleList track = new SubtitleList
                    {
                        Burn = item.BurnedIn,
                        Default = item.Default,
                        Force = item.Forced,
                        ID = item.TrackNumber,
                        Track = item.TrackNumber
                    };

                subtitle.SubtitleList.Add(track);
            }

            foreach (SrtSubtitle item in job.Subtitles.SrtSubtitles)
            {
                SubtitleList track = new SubtitleList
                    {
                        Default = item.Default,
                        Offset = item.Offset,
                        SRT =
                            new SRT
                                {
                                    Filename = item.FileName,
                                    Codeset = item.CharacterCode,
                                    Language = item.LanguageCode
                                }
                    };

                subtitle.SubtitleList.Add(track);
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
        private static Video CreateVideo(EncodeJob job)
        {
            Video video = new Video();

            HBVideoEncoder videoEncoder = Encoders.VideoEncoders.FirstOrDefault(e => e.ShortName == job.EncodingProfile.VideoEncoder);
            Validate.NotNull(videoEncoder, "Video encoder " + job.EncodingProfile.VideoEncoder + " not recognized.");
            if (videoEncoder != null)
            {
                video.Codec = videoEncoder.Id;
            }

            video.Level = job.EncodingProfile.VideoLevel;
            video.Options = job.EncodingProfile.VideoOptions;
            video.Preset = job.EncodingProfile.VideoPreset;
            video.Profile = job.EncodingProfile.VideoProfile;
            video.Quality = job.EncodingProfile.Quality;
            if (job.EncodingProfile.VideoTunes != null && job.EncodingProfile.VideoTunes.Count > 0)
            {
                video.Tune = string.Join(",", job.EncodingProfile.VideoTunes);
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
        private static Audio CreateAudio(EncodeJob job)
        {
            Audio audio = new Audio();

            if (!string.IsNullOrEmpty(job.EncodingProfile.AudioEncoderFallback))
            {
                HBAudioEncoder audioEncoder = Encoders.GetAudioEncoder(job.EncodingProfile.AudioEncoderFallback);
                Validate.NotNull(audioEncoder, "Unrecognized fallback audio encoder: " + job.EncodingProfile.AudioEncoderFallback);
                audio.FallbackEncoder = audioEncoder.Id;
            }

            audio.CopyMask = (int)NativeConstants.HB_ACODEC_ANY;

            audio.AudioList = new List<AudioList>();
            int numTracks = 0;
            foreach (AudioEncoding item in job.EncodingProfile.AudioEncodings)
            {
                HBAudioEncoder encoder = Encoders.GetAudioEncoder(item.Encoder);
                Validate.NotNull(encoder, "Unrecognized audio encoder:" + item.Encoder);

                HBMixdown mixdown = Encoders.GetMixdown(item.Mixdown);
                Validate.NotNull(mixdown, "Unrecognized audio mixdown:" + item.Mixdown);

                AudioList audioTrack = new AudioList
                    {
                        Track = numTracks++,
                        Bitrate = item.Bitrate,
                        CompressionLevel = item.Compression,
                        DRC = item.Drc,
                        Encoder = encoder.Id,
                        Gain = item.Gain,
                        Mixdown = mixdown.Id,
                        NormalizeMixLevel = false,
                        Quality = item.Quality,
                        Samplerate = item.SampleRateRaw
                    };

                audio.AudioList.Add(audioTrack);
            }

            return audio;
        }

        /// <summary>
        /// The create filter. TODO
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Filter"/>.
        /// </returns>
        private static Filter CreateFilter(EncodeJob job)
        {
            Filter filter = new Filter
                            {
                                FilterList = new List<FilterList>(),
                                Grayscale = job.EncodingProfile.Grayscale
                            };

            // Detelecine
            if (job.EncodingProfile.Detelecine != Detelecine.Off)
            {
                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DETELECINE, Settings = job.EncodingProfile.CustomDetelecine };
                filter.FilterList.Add(filterItem);
            }

            // Decomb
            if (job.EncodingProfile.Decomb != Decomb.Off)
            {
                string options = "";
                if (job.EncodingProfile.Decomb == Decomb.Fast)
                {
                    options = "7:2:6:9:1:80";
                }
                else if (job.EncodingProfile.Decomb == Decomb.Bob)
                {
                    options = "455";
                }
                else
                {
                    options = job.EncodingProfile.CustomDecomb;
                }

                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DECOMB, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // Deinterlace
            if (job.EncodingProfile.Deinterlace != Deinterlace.Off)
            {
                string options = string.Empty;
                if (job.EncodingProfile.Deinterlace == Deinterlace.Fast)
                {
                    options = "0";
                }
                else if (job.EncodingProfile.Deinterlace == Deinterlace.Slow)
                {
                    options = "1";
                }
                else if (job.EncodingProfile.Deinterlace == Deinterlace.Slower)
                {
                    options = "3";
                }
                else if (job.EncodingProfile.Deinterlace == Deinterlace.Bob)
                {
                    options = "15";
                }
                else
                {
                    options = job.EncodingProfile.CustomDeinterlace;
                }

                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DEINTERLACE, Settings = options };
                filter.FilterList.Add(filterItem);
            }

            // VFR / CFR  TODO
            FilterList framerateShaper = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_VFR, Settings = string.Empty };
            filter.FilterList.Add(framerateShaper);

            // Deblock
            if (job.EncodingProfile.Deblock < 5)
            {
                FilterList filterItem = new FilterList { ID = (int)hb_filter_ids.HB_FILTER_DEBLOCK, Settings = job.EncodingProfile.Deblock.ToString() };
                filter.FilterList.Add(filterItem);
            }

            // Denoise
            if (job.EncodingProfile.Denoise != Denoise.Off)
            {
                hb_filter_ids id = job.EncodingProfile.Denoise == Denoise.hqdn3d
                    ? hb_filter_ids.HB_FILTER_HQDN3D
                    : hb_filter_ids.HB_FILTER_NLMEANS;

                string settings;
                if (job.EncodingProfile.Denoise == Denoise.hqdn3d
                    && !string.IsNullOrEmpty(job.EncodingProfile.CustomDenoise))
                {
                    settings = job.EncodingProfile.CustomDenoise;
                }
                else
                {
                    IntPtr settingsPtr = HBFunctions.hb_generate_filter_settings((int)id, job.EncodingProfile.DenoisePreset, job.EncodingProfile.DenoiseTune);
                    settings = Marshal.PtrToStringAnsi(settingsPtr);
                }

                FilterList filterItem = new FilterList { ID = (int)id, Settings = settings };
                filter.FilterList.Add(filterItem);
            }

            // CropScale Filter
            FilterList cropScale = new FilterList
            {
                ID = (int)hb_filter_ids.HB_FILTER_CROP_SCALE,
                Settings =
                    string.Format(
                        "{0}:{1}:{2}:{3}:{4}:{5}",
                        job.EncodingProfile.Width,
                        job.EncodingProfile.Height,
                        job.EncodingProfile.Cropping.Top,
                        job.EncodingProfile.Cropping.Bottom,
                        job.EncodingProfile.Cropping.Left,
                        job.EncodingProfile.Cropping.Right)
            };
            filter.FilterList.Add(cropScale);

            // Rotate
            /* TODO  NOT SUPPORTED YET. */

            return filter;
        }

        /// <summary>
        /// The create geometry.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <returns>
        /// The <see cref="Geometry"/>.
        /// </returns>
        private static Json.Anamorphic.Geometry CreateGeometry(EncodeJob job, JsonScanObject scannedSource)
        {
            TitleList title = scannedSource.TitleList.FirstOrDefault(c => c.Index == job.Title);

            // Sanatise the Geometry First.
            AnamorphicGeometry anamorphicGeometry = new AnamorphicGeometry
                {
                    DestGeometry = new DestSettings(),
                    SourceGeometry =
                        new SourceGeometry
                            {
                                Width = title.Geometry.Width,
                                Height = title.Geometry.Height,
                                PAR = new Json.Anamorphic.PAR { Num = title.Geometry.PAR.Num, Den = title.Geometry.PAR.Den }
                            }
                };

            anamorphicGeometry.DestGeometry.AnamorphicMode = (int)job.EncodingProfile.Anamorphic;
            anamorphicGeometry.DestGeometry.Geometry.Width = job.EncodingProfile.Width;
            anamorphicGeometry.DestGeometry.Geometry.Height = job.EncodingProfile.Height;
            anamorphicGeometry.DestGeometry.Keep = job.EncodingProfile.KeepDisplayAspect;
            anamorphicGeometry.DestGeometry.Geometry.PAR = new Json.Anamorphic.PAR { Num = job.EncodingProfile.PixelAspectX, Den = job.EncodingProfile.PixelAspectY };
            anamorphicGeometry.DestGeometry.Crop = new List<int> { job.EncodingProfile.Cropping.Top, job.EncodingProfile.Cropping.Bottom, job.EncodingProfile.Cropping.Left, job.EncodingProfile.Cropping.Right };

            string encode = JsonConvert.SerializeObject(anamorphicGeometry, Formatting.Indented, new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore });
            IntPtr json = HBFunctions.hb_set_anamorphic_size_json(Marshal.StringToHGlobalAnsi(encode));
            string statusJson = Marshal.PtrToStringAnsi(json);
            AnamorphicResult sanatisedGeometry = JsonConvert.DeserializeObject<AnamorphicResult>(statusJson);

            // Setup the Destination Gemotry.
            Json.Anamorphic.Geometry geometry = new Json.Anamorphic.Geometry();
            geometry.Width = sanatisedGeometry.Width;
            geometry.Height = sanatisedGeometry.Height;
            geometry.PAR = sanatisedGeometry.PAR;
            return geometry;
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
        private static MetaData CreateMetaData(EncodeJob job)
        {
            MetaData metaData = new MetaData();
            return metaData;
        }


        /// <summary>
        /// Adds a filter to the given filter list.
        /// </summary>
        /// <param name="filterList">The list to add the filter to.</param>
        /// <param name="filterType">The type of filter.</param>
        /// <param name="settings">Settings for the filter.</param>
        /// <param name="allocatedMemory">The list of allocated memory.</param>
        private void AddFilter(List<hb_filter_object_s> filterList, int filterType, string settings, List<IntPtr> allocatedMemory)
        {
            IntPtr settingsNativeString = Marshal.StringToHGlobalAnsi(settings);
            hb_filter_object_s filter = InteropUtilities.ToStructureFromPtr<hb_filter_object_s>(HBFunctions.hb_filter_init(filterType));
            filter.settings = settingsNativeString;

            allocatedMemory.Add(settingsNativeString);
            filterList.Add(filter);
        }

        /// <summary>
        /// Adds a filter to the given filter list with the provided preset and tune.
        /// </summary>
        /// <param name="filterList">The list to add the filter to.</param>
        /// <param name="filterType">The type of filter.</param>
        /// <param name="preset">The preset name.</param>
        /// <param name="tune">The tune name.</param>
        private void AddFilterFromPreset(List<hb_filter_object_s> filterList, int filterType, string preset, string tune)
        {
            IntPtr settingsPtr = HBFunctions.hb_generate_filter_settings(filterType, preset, tune);

            hb_filter_object_s filter = InteropUtilities.ToStructureFromPtr<hb_filter_object_s>(HBFunctions.hb_filter_init(filterType));
            filter.settings = settingsPtr;

            filterList.Add(filter);
        }
    }
}
