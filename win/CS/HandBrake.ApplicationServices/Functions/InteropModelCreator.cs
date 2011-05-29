/*  InteropModelCreator.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System.Collections.Generic;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop;

    using Cropping = HandBrake.Interop.Cropping;

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
                    profile.Anamorphic = Interop.Anamorphic.Custom;
                    break;
                case Model.Encoding.Anamorphic.Strict:
                    profile.Anamorphic = Interop.Anamorphic.Strict;
                    break;
                case Model.Encoding.Anamorphic.Loose:
                    profile.Anamorphic = Interop.Anamorphic.Loose;
                    break;
                case Model.Encoding.Anamorphic.None:
                    profile.Anamorphic = Interop.Anamorphic.None;
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

            profile.Cropping = new Cropping
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
            //profile.Decomb = work.Decomb;
            //profile.Deinterlace = work.Deinterlace;
            //profile.Denoise = work.Denoise;
            //profile.Detelecine = work.Detelecine;
            //profile.DisplayWidth = work.DisplayWidth;
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
            //profile.OutputFormat = work.OutputFormat;
            profile.PeakFramerate = work.FramerateMode == FramerateMode.PFR;
            profile.PixelAspectX = work.PixelAspectX;
            profile.PixelAspectY = work.PixelAspectY;
            //profile.PreferredExtension = work.OutputFormat;
            profile.Quality = work.Quality.HasValue ? work.Quality.Value : 0;
            profile.UseDisplayWidth = true;
            profile.VideoBitrate = work.VideoBitrate.HasValue ? work.VideoBitrate.Value : 0;
            //profile.VideoEncodeRateType = work.VideoEncodeRateType;
            //profile.VideoEncoder = work.VideoEncoder;
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
            //job.RangeType = work.PointToPointMode;

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
