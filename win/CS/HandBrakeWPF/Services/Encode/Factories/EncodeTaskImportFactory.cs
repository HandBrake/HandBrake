// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeTaskImportFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encode factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    using System;
    using System.Diagnostics;
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Model;
    using HandBrakeWPF.Extensions;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Encode.Model.Models.Video;
    using HandBrakeWPF.Utilities;

    using AudioEncoder = Model.Models.AudioEncoder;
    using AudioEncoderRateType = Model.Models.AudioEncoderRateType;
    using AudioTrack = Model.Models.AudioTrack;
    using Chapter = HandBrake.Interop.Interop.Json.Encode.Chapter;
    using ChapterMarker = Model.Models.ChapterMarker;
    using EncodeTask = Model.EncodeTask;
    using OutputFormat = Model.Models.OutputFormat;
    using PointToPointMode = Model.Models.PointToPointMode;
    using SubtitleTrack = HandBrake.Interop.Interop.Json.Encode.SubtitleTrack;

    /// <summary>
    /// This factory takes the internal EncodeJob object and turns it into a set of JSON models
    /// that can be deserialized by libhb.
    /// </summary>
    internal class EncodeTaskImportFactory
    {
        /*
         * TODO
         * 1. Reconstruct the Config
         * 2. Reconstruct Queue State
         * 3. Update JSON API. See #1481
         */

        internal static EncodeTask Create(JsonEncodeObject job)
        {
            if (job == null)
            {
                return null;
            }

            EncodeTask task = new EncodeTask();
            ParseSource(task, job);
            ParseDestination(task, job);
            ParsePar(task, job);
            ParseVideo(task, job);
            ParseAudio(task, job);
            ParseSubtitle(task, job);
            ParseFilters(task, job);
            ParseMetaData(task, job);

            return task;
        }

        internal static HBConfiguration CreateConfig(JsonEncodeObject job)
        {
            //                    range.SeekPoints = configuration.PreviewScanCount;
            // video.QSV.Decode = SystemInfo.IsQsvAvailable && configuration.EnableQuickSyncDecoding;
            /*
            // The use of the QSV decoder is configurable for non QSV encoders.
            if (video.QSV.Decode && job.VideoEncoder != VideoEncoder.QuickSync && job.VideoEncoder != VideoEncoder.QuickSyncH265 && job.VideoEncoder != VideoEncoder.QuickSyncH26510b)
            {
                video.QSV.Decode = configuration.UseQSVDecodeForNonQSVEnc;
            }*/

            return new HBConfiguration();
        }

        private static void ParseSource(EncodeTask task, JsonEncodeObject job)
        {
            if (job?.Source == null)
            {
                return;
            }

            task.PointToPointMode = EnumHelper<PointToPointMode>.GetValue(job.Source.Range.Type);

            switch (task.PointToPointMode)
            {
                case PointToPointMode.Chapters:
                case PointToPointMode.Frames:
                    task.StartPoint = job.Source.Range.Start.Value;
                    task.EndPoint = job.Source.Range.End.Value;
                    break;
                case PointToPointMode.Seconds:
                    task.StartPoint = job.Source.Range.Start.Value / 90000;
                    task.EndPoint = job.Source.Range.End.Value / 90000;
                    break;
            }

            task.Source = job.Source.Path;
            task.Title = job.Source.Title;
            task.Angle = job.Source.Angle;
        }

        private static void ParseDestination(EncodeTask task, JsonEncodeObject job)
        {
            if (job.Destination == null)
            {
                return;
            }

            task.Destination = job.Destination.File;
            task.OptimizeMP4 = job.Destination.Mp4Options.Mp4Optimize;
            task.IPod5GSupport = job.Destination.Mp4Options.IpodAtom;
            task.AlignAVStart = job.Destination.AlignAVStart;
            task.OutputFormat = EnumHelper<OutputFormat>.GetValue(job.Destination.Mux);

            task.IncludeChapterMarkers = job.Destination.ChapterMarkers;
            int chapter = 0;
            foreach (Chapter item in job.Destination.ChapterList)
            {
                chapter = chapter + 1;
                task.ChapterNames.Add(new ChapterMarker(0, item.Name, TimeSpan.MinValue));
            }
        }

        private static void ParsePar(EncodeTask task, JsonEncodeObject job)
        {
            if (job.PAR == null)
            {
                return;
            }

            task.PixelAspectX = job.PAR.Num;
            task.PixelAspectY = job.PAR.Den;
        }

        private static void ParseVideo(EncodeTask task, JsonEncodeObject job)
        {
            task.VideoEncoder = EnumHelper<VideoEncoder>.GetValue(job.Video.Encoder);
            task.ExtraAdvancedArguments = job.Video.Options;

            HBVideoEncoder videoEncoder = HandBrakeEncoderHelpers.VideoEncoders.FirstOrDefault(e => e.ShortName == job.Video.Encoder);
            if (videoEncoder == null)
            {
                Debug.WriteLine("Video encoder is not supported on this system.");
            }

            task.VideoLevel = new VideoLevel(job.Video.Level, job.Video.Level);
            task.VideoPreset = new VideoPreset(job.Video.Preset, job.Video.Preset);
            task.VideoProfile = new VideoProfile(job.Video.Profile, job.Video.Profile);

            if (!string.IsNullOrEmpty(job.Video.Tune))
            {
                string[] splitTunes = job.Video.Tune.Split(',');
                foreach (string tune in splitTunes)
                {
                    task.VideoTunes.Add(new VideoTune(tune, tune));
                }
            }

            if (job.Video.Quality.HasValue)
            {
                task.VideoEncodeRateType = VideoEncodeRateType.ConstantQuality;
                task.Quality = job.Video.Quality;
            }

            if (job.Video.Bitrate.HasValue)
            {
                task.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                task.TwoPass = job.Video.TwoPass;
                task.TurboFirstPass = job.Video.Turbo;
                task.VideoBitrate = job.Video.Bitrate;
            }

            // job.Video.ColorMatrixCode;  Not currently supported in the GUI.
        }

        private static void ParseAudio(EncodeTask task, JsonEncodeObject job)
        {
            if (job.Audio == null)
            {
                return;
            }

            task.AllowedPassthruOptions.AudioAllowAACPass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.AacPassthru));
            task.AllowedPassthruOptions.AudioAllowAC3Pass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.Ac3Passthrough));
            task.AllowedPassthruOptions.AudioAllowDTSHDPass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.DtsHDPassthrough));
            task.AllowedPassthruOptions.AudioAllowDTSPass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.DtsPassthrough));
            task.AllowedPassthruOptions.AudioAllowEAC3Pass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.EAc3Passthrough));
            task.AllowedPassthruOptions.AudioAllowFlacPass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.FlacPassthru));
            task.AllowedPassthruOptions.AudioAllowMP3Pass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.Mp3Passthru));
            task.AllowedPassthruOptions.AudioAllowTrueHDPass = job.Audio.CopyMask.Contains(EnumHelper<AudioEncoder>.GetShortName(AudioEncoder.TrueHDPassthrough));
            task.AllowedPassthruOptions.AudioEncoderFallback = EnumHelper<AudioEncoder>.GetValue(job.Audio.FallbackEncoder);

            foreach (HandBrake.Interop.Interop.Json.Encode.AudioTrack item in job.Audio.AudioList)
            {
                HBMixdown mixdown = HandBrakeEncoderHelpers.Mixdowns.FirstOrDefault(m => m.Id == item.Mixdown);
                HBRate sampleRate = HandBrakeEncoderHelpers.AudioSampleRates.FirstOrDefault(s => s.Rate == item.Samplerate);

                AudioTrack track = new AudioTrack();
                track.ScannedTrack = new Scan.Model.Audio(item.Track); // When adding to the queue, we don't need the full context. Editing queue will recovery this.
                track.Encoder = EnumHelper<AudioEncoder>.GetValue(item.Encoder);
                track.Gain = (int)item.Gain;
                track.MixDown = mixdown?.ShortName;
                track.TrackName = item.Name;
                track.SampleRate = sampleRate?.Rate ?? 48;  // TODO this may not work. Check
                track.DRC = item.DRC;

                if (!track.IsPassthru && item.Bitrate.HasValue)
                {
                    track.Bitrate = item.Bitrate.Value;
                    track.EncoderRateType = AudioEncoderRateType.Bitrate;
                }

                if (!track.IsPassthru && item.Quality.HasValue)
                {
                    track.Quality = item.Quality.Value;
                    track.EncoderRateType = AudioEncoderRateType.Quality;
                }

                task.AudioTracks.Add(track);
            }
        }

        private static void ParseSubtitle(EncodeTask task, JsonEncodeObject job)
        {
            if (job.Subtitle == null)
            {
                return;
            }

            if (job.Subtitle.Search.Enable)
            {
                Model.Models.SubtitleTrack subtitleTrack = new Model.Models.SubtitleTrack();
                subtitleTrack.Burned = job.Subtitle.Search.Burn;
                subtitleTrack.Default = job.Subtitle.Search.Default;
                subtitleTrack.Forced = job.Subtitle.Search.Forced;
                subtitleTrack.SubtitleType = SubtitleType.ForeignAudioSearch;
                subtitleTrack.SourceTrack = new Scan.Model.Subtitle(0);
                task.SubtitleTracks.Add(subtitleTrack);
            }

            foreach (SubtitleTrack subtitle in job.Subtitle.SubtitleList)
            {
                if (subtitle.ID == 0) 
                {
                    continue; // Foreign Audio Scan
                }

                Model.Models.SubtitleTrack subtitleTrack = new Model.Models.SubtitleTrack();

                subtitle.ID = subtitle.ID;
                subtitleTrack.Burned = subtitle.Burn;
                subtitleTrack.Default = subtitle.Default;
                subtitleTrack.Forced = subtitle.Forced;

                if (!string.IsNullOrEmpty(subtitle.Import.Filename))
                {
                    subtitleTrack.SubtitleType =  subtitle.Import.Filename.EndsWith("srt") ?  SubtitleType.SRT : SubtitleType.SSA;
                    subtitleTrack.SrtCharCode = subtitle.Import.Codeset;
                    subtitleTrack.SrtFileName = subtitle.Import.Filename;
                    subtitleTrack.SrtLangCode = subtitle.Import.Language;
                    subtitleTrack.SrtLang = HandBrakeLanguagesHelper.Get(subtitleTrack.SrtLangCode).EnglishName;
                    subtitleTrack.SrtOffset = subtitleTrack.SrtOffset;
                }

                task.SubtitleTracks.Add(null);
            }
        }

        private static void ParseFilters(EncodeTask task, JsonEncodeObject job)
        {
            // Crop scale
            Filter cropscaleFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_CROP_SCALE);
            if (cropscaleFilter != null)
            {
                var filterSettings = cropscaleFilter.Settings;
                int cropTop = filterSettings.Value<int>("crop-top");
                int cropBottom = filterSettings.Value<int>("crop-bottom");
                int cropLeft = filterSettings.Value<int>("crop-left");
                int cropRight = filterSettings.Value<int>("crop-right");
                int width = filterSettings.Value<int>("width");
                int height = filterSettings.Value<int>("height");

                task.Width = width;
                task.Height = height;
                task.Cropping.Top = cropTop;
                task.Cropping.Bottom = cropBottom;
                task.Cropping.Left = cropLeft;
                task.Cropping.Right = cropRight;
            }

            // Grayscale
            Filter grayscaleFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_GRAYSCALE);
            if (grayscaleFilter != null)
            {
                task.Grayscale = true;
            }

            // Rotate
            Filter rotationFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_ROTATE);
            if (rotationFilter != null)
            {
                var filterSettings = rotationFilter.Settings;
                task.Rotation = filterSettings.Value<int>("angle");
                task.FlipVideo = filterSettings.Value<string>("hflip") == "1";
            }

            // Deblock
            Filter deblockFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_DEBLOCK);
            if (deblockFilter != null)
            {
                var filterSettings = deblockFilter.Settings;
                task.DeblockPreset = null; // TODO Support Preset / Tune
            }

            // Sharpen
            Filter lapsharpFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_LAPSHARP);
            if (lapsharpFilter != null)
            {             
                var filterSettings = lapsharpFilter.Settings;
                task.Sharpen = Sharpen.LapSharp; // TODO Preset / Tune
            }

            Filter unsharpFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_UNSHARP);
            if (unsharpFilter != null)
            {
                var filterSettings = unsharpFilter.Settings;
                task.Sharpen = Sharpen.UnSharp;  // TODO Preset / Tune
            }

            // Denoise
            Filter hqdn3dFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_HQDN3D);
            if (hqdn3dFilter != null)
            {
                var filterSettings = hqdn3dFilter.Settings;
                task.Denoise = Denoise.hqdn3d; // TODO Preset / Tune
            }

            Filter nlmeansFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_NLMEANS);
            if (nlmeansFilter != null)
            {
                var filterSettings = nlmeansFilter.Settings;
                task.Denoise = Denoise.NLMeans;  // TODO Preset / Tune
            }

            // Detelecine
            Filter detelecineFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_DETELECINE);
            if (detelecineFilter != null)
            {
                var filterSettings = detelecineFilter.Settings;
                task.Detelecine = Detelecine.Default; // TODO Handle Custom
            }

            // Deinterlace
            Filter deinterlaceFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_DEINTERLACE);
            if (deinterlaceFilter != null)
            {
                var filterSettings = deinterlaceFilter.Settings;
                task.DeinterlaceFilter = DeinterlaceFilter.Yadif; // TODO Handle Preset / Custom
            }

            Filter decombFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_DECOMB);
            if (decombFilter != null)
            {
                var filterSettings = decombFilter.Settings;
                task.DeinterlaceFilter = DeinterlaceFilter.Decomb; // TODO Handle Preset / Custom
            }

            // Interlace Detection 
            Filter combDetect = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_COMB_DETECT);
            if (combDetect != null)
            {
                var filterSettings = combDetect.Settings;
                task.CombDetect = CombDetect.Default; // TODO Handle Preset / Custom
            }

            // Framerate

            Filter framerateFilter = job.Filters.FilterList.FirstOrDefault(f => f.ID == (int)hb_filter_ids.HB_FILTER_VFR);
            if (framerateFilter != null)
            {
                var filterSettings = framerateFilter.Settings;
                task.FramerateMode = (FramerateMode)filterSettings.Value<int>("mode");  // TODO numbers may not be in correct order.
                string rate = filterSettings.Value<string>("rate"); // TODO Handle Rate.  num/den  hb_video_framerate_get_from_name
            }
        }

        private static void ParseMetaData(EncodeTask task, JsonEncodeObject job)
        {
            if (job.Metadata == null)
            {
                return;
            }

            task.MetaData.Artist = job.Metadata.Artist;
            task.MetaData.Album = job.Metadata.Album;
            task.MetaData.AlbumArtist = job.Metadata.AlbumArtist;
            task.MetaData.Comment = job.Metadata.Comment;
            task.MetaData.Composer = job.Metadata.Composer;
            task.MetaData.Description = job.Metadata.Description;
            task.MetaData.Genre = job.Metadata.Genre;
            task.MetaData.LongDescription = job.Metadata.LongDescription;
            task.MetaData.Name = job.Metadata.Name;
            task.MetaData.ReleaseDate = job.Metadata.ReleaseDate;
        }
    }
}
