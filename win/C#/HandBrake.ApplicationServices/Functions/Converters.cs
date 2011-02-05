/*  Converters.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Text.RegularExpressions;

    using HandBrake.ApplicationServices.Model.Encoding;

    /// <summary>
    /// A class to convert various things to native C# objects
    /// </summary>
    public class Converters
    {
        /**
         * TODO:
         * - Many of these converters can be ditched at a later point. Should be able to model all this within the enums themsevles.
         * 
         **/ 

        /// <summary>
        /// Convert HandBrakes time remaining into a TimeSpan
        /// </summary>
        /// <param name="time">
        /// The time remaining for the encode.
        /// </param>
        /// <returns>
        /// A TimepSpan object
        /// </returns>
        public static TimeSpan EncodeToTimespan(string time)
        {
            TimeSpan converted = new TimeSpan(0, 0, 0, 0);

            Match m = Regex.Match(time.Trim(), @"^([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
            {
                TimeSpan.TryParse(m.Groups[0].Value, out converted);
            }

            return converted;
        }

        /// <summary>
        /// Get the GUI equiv to a CLI mixdown
        /// </summary>
        /// <param name="mixdown">The Audio Mixdown</param>
        /// <returns>The GUI representation of the mixdown</returns>
        public static string GetMixDown(string mixdown)
        {
            switch (mixdown.Trim())
            {
                case "mono":
                    return "Mono";
                case "stereo":
                    return "Stereo";
                case "dpl1":
                    return "Dolby Surround";
                case "dpl2":
                    return "Dolby Pro Logic II";
                case "6ch":
                    return "6 Channel Discrete";
                default:
                    return "Automatic";
            }
        }

        /// <summary>
        /// Get the GUI equiv to a CLI audio encoder
        /// </summary>
        /// <param name="audioEnc">The Audio Encoder</param>
        /// <returns>The GUI representation of that audio encoder</returns>
        public static string GetGUIAudioEncoder(string audioEnc)
        {
            switch (audioEnc)
            {
                case "faac":
                    return "AAC (faac)";
                case "lame":
                    return "MP3 (lame)";
                case "vorbis":
                    return "Vorbis (vorbis)";
                case "ac3":
                    return "AC3 (ffmpeg)";
                case "copy:ac3":
                    return "AC3 Passthru";
                case "copy:dts":
                    return "DTS Passthru";
                default:
                    return "AAC (faac)";
            }
        }

        /// <summary>
        /// Get the Video Encoder for a given string
        /// </summary>
        /// <param name="encoder">
        /// The encoder name
        /// </param>
        /// <returns>
        /// VideoEncoder enum object
        /// </returns>
        public static VideoEncoder GetVideoEncoder(string encoder)
        {
            switch (encoder)
            {
                case "ffmpeg":
                    return VideoEncoder.FFMpeg;
                case "x264":
                    return VideoEncoder.X264;
                case "theora":
                    return VideoEncoder.Theora;
                default:
                    return VideoEncoder.X264;
            }
        }

        /// <summary>
        /// Get a GUI name for a given video Encoder.
        /// </summary>
        /// <param name="encoder">
        /// A VideoEncoder Enum object
        /// </param>
        /// <returns>
        /// A GUI encoder name, default is x264
        /// </returns>
        public static string GetGUIVideoEncoder(VideoEncoder encoder)
        {
            switch (encoder)
            {
                case VideoEncoder.FFMpeg:
                    return "MPEG-4 (FFmpeg)";
                case VideoEncoder.X264:
                    return "H.264 (x264)";
                case VideoEncoder.Theora:
                    return "VP3 (Theora)";
                default:
                    return "H.264 (x264)";
            }
        }


        /// <summary>
        /// Get the OutputFormat Enum for a given string
        /// </summary>
        /// <param name="format">
        /// OutputFormat as a string
        /// </param>
        /// <returns>
        /// An OutputFormat Enum
        /// </returns>
        public static OutputFormat GetFileFormat(string format)
        {
            switch (format.ToLower())
            {
                default:
                    return OutputFormat.Mp4;
                case "m4v":
                    return OutputFormat.M4V;
                case "mkv":
                    return OutputFormat.Mkv;
            }
        }

        /// <summary>
        /// Get the OutputFormat Enum for a given string
        /// </summary>
        /// <param name="format">
        /// OutputFormat as a string
        /// </param>
        /// <returns>
        /// An OutputFormat Enum
        /// </returns>
        public static string GetFileFormat(OutputFormat format)
        {
            switch (format)
            {
                default:
                    return "mp4";
                case OutputFormat.M4V:
                    return "m4v";
                case OutputFormat.Mkv:
                    return "mkv";
            }
        }
    }
}
