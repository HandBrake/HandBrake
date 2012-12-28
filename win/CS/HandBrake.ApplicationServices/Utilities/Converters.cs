// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Converters.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class to convert various things to native C# objects
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Text.RegularExpressions;

    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

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
                case "left_only":
                    return "Mono (Left Only)";
                case "right_only":
                    return "Mono (Right Only)";
                case "stereo":
                    return "Stereo";
                case "dpl1":
                    return "Dolby Surround";
                case "dpl2":
                    return "Dolby Pro Logic II";
                case "5point1":
                    return "5.1 Channels";
                case "6point1":
                    return "6.1 Channels";
                case "7point1":
                    return "7.1 Channels";
                case "5_2_lfe":
                    return "7.1 (5F/2R/LFE)";
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
                case "5.1 Channels":
                    return Mixdown.FivePoint1Channels;
                case "6.1 Channels":
                    return Mixdown.SixPoint1Channels;
                case "7.1 Channels":
                    return Mixdown.SevenPoint1Channels;
                case "7.1 (5F/2R/LFE)":
                    return Mixdown.Five_2_LFE;
                case "None":
                case "Passthru":
                    return Mixdown.None;
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
                case Mixdown.None:
                    return "auto";
                case Mixdown.Mono:
                    return "mono";
                case Mixdown.LeftOnly:
                    return "left_only";
                case Mixdown.RightOnly:
                    return "right_only";
                case Mixdown.Stereo:
                    return "stereo";
                case Mixdown.DolbySurround:
                    return "dpl1";
                case Mixdown.DolbyProLogicII:
                    return "dpl2";
                case Mixdown.FivePoint1Channels:
                    return "5point1";
                case Mixdown.SixPoint1Channels:
                    return "6point1";
                case Mixdown.SevenPoint1Channels:
                    return "7point1";
                case Mixdown.Five_2_LFE:
                    return "5_2_lfe";
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
                case "ffac3":
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
                case "ffflac":
                    return "Flac (ffmpeg)";
                case "copy":
                    return "Auto Passthru";
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
                case "ffac3":
                    return AudioEncoder.Ac3;
                case "ffflac":
                    return AudioEncoder.ffflac;
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
                case "copy":
                    return AudioEncoder.Passthrough;
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
                case "Flac (ffmpeg)":
                    return AudioEncoder.ffflac;
                case "Auto Passthru":
                    return AudioEncoder.Passthrough;
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
                    return "ffac3";
                case AudioEncoder.AacPassthru:
                    return "copy:aac";
                case AudioEncoder.Mp3Passthru:
                    return "copy:mp3";
                case AudioEncoder.Passthrough:
                    return "copy";
                case AudioEncoder.ffflac:
                    return "ffflac";
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
                case "":
                case "ffmpeg":
                case "ffmpeg4":
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
                    return "ffmpeg4";
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

        #region x264

        /// <summary>
        /// Get the x264Preset from a cli parameter
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <returns>
        /// The x264Preset enum value
        /// </returns>
        public static x264Preset Getx264PresetFromCli(string preset)
        {
            switch (preset)
            {
                case "ultrafast":
                    return x264Preset.Ultrafast;
                case "superfast":
                    return x264Preset.Superfast;
                case "veryfast":
                    return x264Preset.VeryFast;
                case "faster":
                    return x264Preset.Faster;
                case "fast":
                    return x264Preset.Fast;
                case "medium":
                    return x264Preset.Medium;
                case "slow":
                    return x264Preset.Slow;
                case "slower":
                    return x264Preset.Slower;
                case "veryslow":
                    return x264Preset.VerySlow;
                case "placebo":
                    return x264Preset.Placebo;
                default:
                    return x264Preset.Faster;
            }
        }

        /// <summary>
        /// Get the x264 Profile from the cli
        /// </summary>
        /// <param name="profile">
        /// The preset.
        /// </param>
        /// <returns>
        /// The x264Profile enum value
        /// </returns>
        public static x264Profile Getx264ProfileFromCli(string profile)
        {
            switch (profile)
            {
                case "baseline":
                    return x264Profile.Baseline;
                case "main":
                    return x264Profile.Main;
                case "high":
                    return x264Profile.High;
                case "high10":
                    return x264Profile.High10;
                case "high422":
                    return x264Profile.High422;
                case "high444":
                    return x264Profile.High444;
                default:
                    return x264Profile.Main;
            }
        }

        /// <summary>
        /// Get x264Tune enum from a cli string
        /// </summary>
        /// <param name="tune">
        /// The tune.
        /// </param>
        /// <returns>
        /// The x264Tune enum value
        /// </returns>
        public static x264Tune Getx264TuneFromCli(string tune)
        {
            switch (tune)
            {
                case "film":
                    return x264Tune.Film;
                case "animation":
                    return x264Tune.Animation;
                case "grain":
                    return x264Tune.Grain;
                case "stillimage":
                    return x264Tune.Stillimage;
                case "psnr":
                    return x264Tune.Psnr;
                case "ssim":
                    return x264Tune.Ssim;
                case "fastdecode":
                    return x264Tune.Fastdecode;
                default:
                    return x264Tune.Film;
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
