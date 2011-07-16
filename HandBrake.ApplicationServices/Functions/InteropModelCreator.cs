/*  InteropModelCreator.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Collections.Generic;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;

    using Decomb = HandBrake.ApplicationServices.Model.Encoding.Decomb;
    using Deinterlace = HandBrake.ApplicationServices.Model.Encoding.Deinterlace;
    using Denoise = HandBrake.ApplicationServices.Model.Encoding.Denoise;
    using Detelecine = HandBrake.ApplicationServices.Model.Encoding.Detelecine;
    using OutputFormat = HandBrake.ApplicationServices.Model.Encoding.OutputFormat;
    using VideoEncoder = HandBrake.ApplicationServices.Model.Encoding.VideoEncoder;

    /// <summary>
    /// A Utility Class to Convert a 
    /// </summary>
    public class InteropModelCreator
    {
        /*
         * TODO: This conversion class needs to be finished off before libencode will work.
         */

        /// <summary>
        /// Get an EncodeJob model for a LibHB Encode.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <returns>
        /// An Interop.EncodeJob model.
        /// </returns>
        public static EncodeJob GetEncodeJob(QueueTask task)
        {
            // Sanity Checking
            if (task == null || task.Task == null)
            {
                return null;
            }

            // The current Job Configuration
            EncodeTask work = task.Task;

            // Which will be converted to this EncodeJob Model.
            EncodeJob job = new EncodeJob();
            EncodingProfile profile = new EncodingProfile();
            job.EncodingProfile = profile;
           

            switch (work.Anamorphic)
            {
                case Model.Encoding.Anamorphic.Custom:
                    profile.Anamorphic = Interop.Model.Encoding.Anamorphic.Custom;
                    break;
                case Model.Encoding.Anamorphic.Strict:
                    profile.Anamorphic = Interop.Model.Encoding.Anamorphic.Strict;
                    break;
                case Model.Encoding.Anamorphic.Loose:
                    profile.Anamorphic = Interop.Model.Encoding.Anamorphic.Loose;
                    break;
                case Model.Encoding.Anamorphic.None:
                    profile.Anamorphic = Interop.Model.Encoding.Anamorphic.None;
                    break;
            }


            profile.AudioEncodings = new List<AudioEncoding>();
            foreach (AudioTrack track in work.AudioTracks)
            {
                AudioEncoding newTrack = new AudioEncoding
                    {
                        Bitrate = track.Bitrate,
                        Drc = track.DRC,
                        Gain = track.Gain,
                        //Encoder = track.Encoder,
                        // InputNumber = track.Track,
                        //Mixdown = track.MixDown,
                        //SampleRateRaw = track.SampleRate
                    };

                profile.AudioEncodings.Add(newTrack);
            }

            profile.Cropping = new HandBrake.Interop.Model.Cropping
                {
                    Top = work.Cropping.Top,
                    Bottom = work.Cropping.Bottom,
                    Left = work.Cropping.Left,
                    Right = work.Cropping.Right
                };

            profile.CustomCropping = true;
            profile.CustomDecomb = work.CustomDecomb;
            profile.CustomDeinterlace = work.CustomDeinterlace;
            profile.CustomDenoise = work.CustomDenoise;
            profile.CustomDetelecine = work.CustomDetelecine;
            profile.Deblock = work.Deblock;
            switch (work.Decomb)
            {
                case Decomb.Default:
                    profile.Decomb = Interop.Model.Encoding.Decomb.Default;
                    break;
                case Decomb.Custom:
                    profile.Decomb = Interop.Model.Encoding.Decomb.Custom;
                    break;
                case Decomb.Off:
                    profile.Decomb = Interop.Model.Encoding.Decomb.Off;
                    break;
            }

            switch (work.Deinterlace)
            {
                case Deinterlace.Fast:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Fast;
                    break;
                case Deinterlace.Slow:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Slow;
                    break;
                case Deinterlace.Slower:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Slower;
                    break;
                case Deinterlace.Slowest:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Slower;
                    break;
                case Deinterlace.Custom:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Custom;
                    break;
                case Deinterlace.Off:
                    profile.Deinterlace = Interop.Model.Encoding.Deinterlace.Off;
                    break;
            }


            switch (work.Denoise)
            {
                case Denoise.Off:
                    profile.Denoise = Interop.Model.Encoding.Denoise.Off;
                    break;
                case Denoise.Custom:
                    profile.Denoise = Interop.Model.Encoding.Denoise.Custom;
                    break;
                case Denoise.Strong:
                    profile.Denoise = Interop.Model.Encoding.Denoise.Strong;
                    break;
                case Denoise.Medium:
                    profile.Denoise = Interop.Model.Encoding.Denoise.Medium;
                    break;
                case Denoise.Weak:
                    profile.Denoise = Interop.Model.Encoding.Denoise.Weak;
                    break;
            }

            switch (work.Detelecine)
            {
                case Detelecine.Default:
                    profile.Detelecine = Interop.Model.Encoding.Detelecine.Default;
                    break;
                case Detelecine.Custom:
                    profile.Detelecine = Interop.Model.Encoding.Detelecine.Custom;
                    break;
                case Detelecine.Off:
                    profile.Detelecine = Interop.Model.Encoding.Detelecine.Off;
                    break;
            }

            profile.DisplayWidth = work.DisplayWidth.HasValue
                                       ? int.Parse(Math.Round(work.DisplayWidth.Value, 0).ToString())
                                       : 0;
            profile.Framerate = work.Framerate.HasValue ? work.Framerate.Value : 0;
            profile.Grayscale = work.Grayscale;
            profile.Height = work.Height.HasValue ? work.Height.Value : 0;
            profile.IPod5GSupport = work.IPod5GSupport;
            profile.IncludeChapterMarkers = work.IncludeChapterMarkers;
            profile.KeepDisplayAspect = work.KeepDisplayAspect;
            profile.LargeFile = work.LargeFile;
            profile.MaxHeight = work.MaxHeight.HasValue ? work.MaxHeight.Value : 0;
            profile.MaxWidth = work.MaxWidth.HasValue ? work.MaxWidth.Value : 0;
            profile.Modulus = work.Modulus.HasValue ? work.Modulus.Value : 16;
            profile.Optimize = work.OptimizeMP4;
            switch (work.OutputFormat)
            {
                case OutputFormat.Mp4:
                case OutputFormat.M4V:
                    profile.OutputFormat = Interop.Model.Encoding.OutputFormat.Mp4;
                    break;
                case OutputFormat.Mkv:
                    profile.OutputFormat = Interop.Model.Encoding.OutputFormat.Mkv;
                    break;
            }
            profile.PeakFramerate = work.FramerateMode == FramerateMode.PFR;
            profile.PixelAspectX = work.PixelAspectX;
            profile.PixelAspectY = work.PixelAspectY;

            switch (work.OutputFormat)
            {
                case OutputFormat.Mp4:
                    profile.PreferredExtension = Interop.Model.Encoding.OutputExtension.Mp4;
                    break;
                case OutputFormat.M4V:
                    profile.PreferredExtension = Interop.Model.Encoding.OutputExtension.M4v;
                    break;
            }
            profile.Quality = work.Quality.HasValue ? work.Quality.Value : 0;
            profile.UseDisplayWidth = true;
            profile.VideoBitrate = work.VideoBitrate.HasValue ? work.VideoBitrate.Value : 0;

            switch (work.VideoEncodeRateType)
            {
                case VideoEncodeRateMode.AverageBitrate:
                    profile.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                    break;
                case VideoEncodeRateMode.ConstantQuality:
                    profile.VideoEncodeRateType = VideoEncodeRateType.ConstantQuality;
                    break;
            }

            switch (work.VideoEncoder)
            {
                case VideoEncoder.X264:
                    profile.VideoEncoder = Interop.Model.Encoding.VideoEncoder.X264;
                    break;
                case VideoEncoder.FFMpeg:
                    profile.VideoEncoder = Interop.Model.Encoding.VideoEncoder.FFMpeg;
                    break;
                case VideoEncoder.FFMpeg2:
                    profile.VideoEncoder = Interop.Model.Encoding.VideoEncoder.FFMpeg; // TODO Fix This.
                    break;
                case VideoEncoder.Theora:
                    profile.VideoEncoder = Interop.Model.Encoding.VideoEncoder.Theora;
                    break;

            }

            profile.Width = work.Width.HasValue ? work.Width.Value : 0;
            profile.X264Options = work.AdvancedEncoderOptions;

            if (work.PointToPointMode == PointToPointMode.Chapters)
            {
                job.ChapterStart = work.StartPoint;
                job.ChapterEnd = work.EndPoint;
            }

            job.Angle = work.Angle;
            job.EncodingProfile = profile;
            if (work.PointToPointMode == PointToPointMode.Frames)
            {
                job.FramesEnd = work.EndPoint;
                job.FramesStart = work.StartPoint;
            }

            job.OutputPath = work.Destination;
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
            }


            if (work.PointToPointMode == PointToPointMode.Seconds)
            {
                job.SecondsEnd = work.EndPoint;
                job.SecondsStart = work.StartPoint;
            }

            job.SourcePath = work.Source;
            // job.SourceType = work.Type;
            job.Title = work.Title;
            job.Subtitles = new Subtitles { SourceSubtitles = new List<SourceSubtitle>(), SrtSubtitles = new List<SrtSubtitle>() };
            foreach (SubtitleTrack track in work.SubtitleTracks)
            {
                // TODO
            }


            return job;
        }
    }
}
