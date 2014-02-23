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
    using HandBrake.Interop.Model.Encoding.x265;

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
                    return "none";
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
        public static AudioEncoder GetAudioEncoderFromCliString(string audioEnc)
        {
            switch (audioEnc)
            {
                case "faac":
                case "ffaac":
                    return AudioEncoder.ffaac;
                case "fdk_aac":
                    return AudioEncoder.fdkaac;
                case "fdk_haac":
                    return AudioEncoder.fdkheaac;
                case "mp3":
                case "lame":
                    return AudioEncoder.Lame;
                case "vorbis":
                    return AudioEncoder.Vorbis;
                case "ac3":
                case "ffac3":
                    return AudioEncoder.Ac3;
                case "flac16":
                case "ffflac":
                    return AudioEncoder.ffflac;
                case "flac24":
                case "ffflac24":
                    return AudioEncoder.ffflac24;
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
                    return AudioEncoder.ffaac;
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
                case "AAC (CoreAudio)":
                case "AAC (ffmpeg)":
                case "AAC (avcodec)":
                    return AudioEncoder.ffaac;
                case "AAC (FDK)":
                    return AudioEncoder.fdkaac;
                case "HE-AAC (FDK)":
                    return AudioEncoder.fdkheaac;
                case "MP3 (lame)":
                case "MP3":
                    return AudioEncoder.Lame;
                case "Vorbis (vorbis)":
                case "Vorbis":
                    return AudioEncoder.Vorbis;
                case "AC3 (ffmpeg)":
                case "AC3":
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
                case "FLAC (ffmpeg)":
                case "FLAC 16-bit":
                    return AudioEncoder.ffflac;
                case "FLAC (24-bit)":
                case "FLAC 24-bit":
                    return AudioEncoder.ffflac24;
                case "Auto Passthru":
                    return AudioEncoder.Passthrough;
                default:
                    return AudioEncoder.ffaac;
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
            return EnumHelper<AudioEncoder>.GetShortName(selectedEncoder);
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
                case "ffmpeg4":
                case "mpeg4":
                    return VideoEncoder.FFMpeg;
                case "ffmpeg2":
                case "mpeg2":
                    return VideoEncoder.FFMpeg2;
                case "x264":
                    return VideoEncoder.X264;
                case "qsv_h264":
                    return VideoEncoder.QuickSync;
                case "theora":
                    return VideoEncoder.Theora;
                case "hevc":
                    return VideoEncoder.X265;
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
                    return "mpeg4";
                case VideoEncoder.FFMpeg2:
                    return "mpeg2";
                case VideoEncoder.X264:
                    return "x264";
                case VideoEncoder.QuickSync:
                    return "qsv_h264";
                case VideoEncoder.Theora:
                    return "theora";
                case VideoEncoder.X265:
                    return "hevc";
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

        #region x265

        /// <summary>
        /// Get the x265Preset from a cli parameter
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <returns>
        /// The x265Preset enum value
        /// </returns>
        public static x265Preset Getx265PresetFromCli(string preset)
        {
            switch (preset)
            {
                case "ultrafast":
                    return x265Preset.Ultrafast;
                case "superfast":
                    return x265Preset.Superfast;
                case "veryfast":
                    return x265Preset.VeryFast;
                case "faster":
                    return x265Preset.Faster;
                case "fast":
                    return x265Preset.Fast;
                case "medium":
                    return x265Preset.Medium;
                case "slow":
                    return x265Preset.Slow;
                case "slower":
                    return x265Preset.Slower;
                case "veryslow":
                    return x265Preset.VerySlow;
                case "placebo":
                    return x265Preset.Placebo;
                default:
                    return x265Preset.Faster;
            }
        }

        /// <summary>
        /// Get the x265 Profile from the cli
        /// </summary>
        /// <param name="profile">
        /// The preset.
        /// </param>
        /// <returns>
        /// The x265Profile enum value
        /// </returns>
        public static x265Profile Getx265ProfileFromCli(string profile)
        {
            switch (profile)
            {
                case "mainstillpicture":
                    return x265Profile.Mainstillpicture;
                case "main":
                    return x265Profile.Main;
                case "main10":
                    return x265Profile.Main10;
                default:
                    return x265Profile.Main;
            }
        }

        /// <summary>
        /// Get x265Tune enum from a cli string
        /// </summary>
        /// <param name="tune">
        /// The tune.
        /// </param>
        /// <returns>
        /// The x265Tune enum value
        /// </returns>
        public static x265Tune Getx265TuneFromCli(string tune)
        {
            switch (tune)
            {
                case "psnr":
                    return x265Tune.Psnr;
                case "ssim":
                    return x265Tune.Ssim;
                default:
                    return x265Tune.Psnr;
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
                    return OutputFormat.Mp4;
                case "mkv":
                    return OutputFormat.Mkv;
                case "x265":
                    return OutputFormat.X265;
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
                case OutputFormat.Mp4:
                    return "m4v";
                case OutputFormat.Mkv:
                    return "mkv";
                case OutputFormat.X265:
                    return "x265";
            }
        }

        #endregion
    }
}
