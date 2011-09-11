/*  Converters.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Text.RegularExpressions;

    using HandBrake.Interop.Model.Encoding;

    using OutputFormat = HandBrake.ApplicationServices.Model.Encoding.OutputFormat;

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

        #region Audio

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
        /// Get the GUI equiv to a CLI mixdown
        /// </summary>
        /// <param name="mixdown">The Audio Mixdown</param>
        /// <returns>The GUI representation of the mixdown</returns>
        public static Mixdown GetAudioMixDown(string mixdown)
        {
            switch (mixdown.Trim())
            {
                case "Mono":
                    return Mixdown.Mono;
                case "Stereo":
                    return Mixdown.Stereo;
                case "Dolby Surround":
                    return Mixdown.DolbySurround;
                case "Dolby Pro Logic II":
                    return Mixdown.DolbyProLogicII;
                case "6 Channel Discrete":
                    return Mixdown.SixChannelDiscrete;
                case "Passthru":
                    return Mixdown.Passthrough;
                default:
                    return Mixdown.Auto;
            }
        }

        /// <summary>
        /// Return the CLI Mixdown name
        /// </summary>
        /// <param name="selectedAudio">GUI mixdown name</param>
        /// <returns>CLI mixdown name</returns>
        public static string GetCliMixDown(Mixdown selectedAudio)
        {
            switch (selectedAudio)
            {
                case Mixdown.Auto:
                case Mixdown.Passthrough:
                    return "auto";
                case Mixdown.Mono:
                    return "mono";
                case Mixdown.Stereo:
                    return "stereo";
                case Mixdown.DolbySurround:
                    return "dpl1";
                case Mixdown.DolbyProLogicII:
                    return "dpl2";
                case Mixdown.SixChannelDiscrete:
                    return "6ch";
                default:
                    return "auto";
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
                case "copy:dtshd":
                    return "MP3 Passthru";
                case "copy:mp3":
                    return "AAC Passthru";
                case "copy:aac":
                    return "DTS-HD Passthru";
                case "ffaac":
                    return "AAC (ffmpeg)";
                default:
                    return "AAC (faac)";
            }
        }

        /// <summary>
        /// Get the GUI equiv to a CLI audio encoder
        /// </summary>
        /// <param name="audioEnc">The Audio Encoder</param>
        /// <returns>The GUI representation of that audio encoder</returns>
        public static AudioEncoder GetAudioEncoderFromCliString(string audioEnc)
        {
            switch (audioEnc)
            {
                case "faac":
                    return AudioEncoder.Faac;
                case "ffaac":
                    return AudioEncoder.ffaac;
                case "lame":
                    return AudioEncoder.Lame;
                case "vorbis":
                    return AudioEncoder.Vorbis;
                case "ac3":
                    return AudioEncoder.Ac3;
                case "copy:ac3":
                    return AudioEncoder.Ac3Passthrough;
                case "copy:dts":
                    return AudioEncoder.DtsPassthrough;
                case "copy:dtshd":
                    return AudioEncoder.DtsHDPassthrough;
                case "copy:mp3":
                    return AudioEncoder.Mp3Passthru;
                case "copy:aac":
                    return AudioEncoder.AacPassthru;
                default:
                    return AudioEncoder.Faac;
            }
        }

        /// <summary>
        /// Get the GUI equiv to a GUI audio encoder string
        /// </summary>
        /// <param name="audioEnc">The Audio Encoder</param>
        /// <returns>The GUI representation of that audio encoder</returns>
        public static AudioEncoder GetAudioEncoder(string audioEnc)
        {
            switch (audioEnc)
            {
                case "AAC (faac)":
                    return AudioEncoder.Faac;
                case "AAC (ffmpeg)":
                    return AudioEncoder.ffaac;
                case "AAC (CoreAudio)":
                    return AudioEncoder.Faac;
                case "MP3 (lame)":
                    return AudioEncoder.Lame;
                case "Vorbis (vorbis)":
                    return AudioEncoder.Vorbis;
                case "AC3 (ffmpeg)":
                    return AudioEncoder.Ac3;
                case "AC3 Passthru":
                    return AudioEncoder.Ac3Passthrough;
                case "DTS Passthru":
                    return AudioEncoder.DtsPassthrough;
                case "DTS-HD Passthru":
                    return AudioEncoder.DtsHDPassthrough;
                case "AAC Passthru":
                    return AudioEncoder.AacPassthru;
                case "MP3 Passthru":
                    return AudioEncoder.Mp3Passthru;
                default:
                    return AudioEncoder.Faac;
            }
        }

        /// <summary>
        /// Get the CLI Audio Encoder name
        /// </summary>
        /// <param name="selectedEncoder">
        /// String The GUI Encode name
        /// </param>
        /// <returns>
        /// String CLI encoder name
        /// </returns>
        public static string GetCliAudioEncoder(AudioEncoder selectedEncoder)
        {
            switch (selectedEncoder)
            {
                case AudioEncoder.Faac:
                    return "faac";
                case AudioEncoder.ffaac:
                    return "ffaac";
                case AudioEncoder.Lame:
                    return "lame";
                case AudioEncoder.Vorbis:
                    return "vorbis";
                case AudioEncoder.Ac3Passthrough:
                    return "copy:ac3";
                case AudioEncoder.DtsPassthrough:
                    return "copy:dts";
                case AudioEncoder.DtsHDPassthrough:
                    return "copy:dtshd";
                case AudioEncoder.Ac3:
                    return "ac3";
                case AudioEncoder.AacPassthru:
                    return "copy:aac";
                case AudioEncoder.Mp3Passthru:
                    return "copy:mp3";
 
                default:
                    return "faac";
            }
        }

        #endregion

        #region Video 

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
                case "ffmpeg2":
                    return VideoEncoder.FFMpeg2;
                case "x264":
                    return VideoEncoder.X264;
                case "theora":
                    return VideoEncoder.Theora;
                default:
                    return VideoEncoder.X264;
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
        public static string GetVideoEncoder(VideoEncoder encoder)
        {
            switch (encoder)
            {
                case VideoEncoder.FFMpeg:
                    return "ffmpeg";
                case VideoEncoder.FFMpeg2:
                    return "ffmpeg2";
                case VideoEncoder.X264:
                    return "x264";
                case VideoEncoder.Theora:
                    return "theora";
                default:
                    return "x264";
            }
        }

        #endregion

        #region File Format

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

        #endregion
    }
}
