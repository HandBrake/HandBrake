// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeUtils.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeUtils type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.HbLib;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Scan;

    /// <summary>
    /// HandBrake Interop Utilities
    /// </summary>
    public static class HandBrakeUtils
    {
        /// <summary>
        /// Estimated overhead in bytes for each frame in output container.
        /// </summary>
        internal const int ContainerOverheadPerFrame = 6;

        /// <summary>
        /// The callback for log messages from HandBrake.
        /// </summary>
        private static LoggingCallback loggingCallback;

        /// <summary>
        /// The callback for error messages from HandBrake.
        /// </summary>
        private static LoggingCallback errorCallback;

        /// <summary>
        /// True if the global initialize function has been called.
        /// </summary>
        private static bool globalInitialized;

        /// <summary>
        /// Fires when HandBrake has logged a message.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> MessageLogged;

        /// <summary>
        /// Fires when HandBrake has logged an error.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> ErrorLogged;

        /// <summary>
        /// Initializes static members of the HandBrakeUtils class.
        /// </summary>
        static HandBrakeUtils()
        {
            if (!globalInitialized)
            {
                if (HBFunctions.hb_global_init() == -1)
                {
                    throw new InvalidOperationException("HB global init failed.");
                }

                globalInitialized = true;
            }
        }

        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        public static string Version
        {
            get
            {
                var versionPtr = HBFunctions.hb_get_version(IntPtr.Zero); // Pointer isn't actually used.
                return Marshal.PtrToStringAnsi(versionPtr);
            }
        }

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        public static int Build
        {
            get
            {
                return HBFunctions.hb_get_build(IntPtr.Zero);
            }
        }

        /// <summary>
        /// Ensures the HB global initialize method has been called.
        /// </summary>
        public static void EnsureGlobalInit()
        {
            // Does nothing, but invokes static ctor.
        }

        /// <summary>
        /// Enables or disables LibDVDNav. If disabled libdvdread will be used instead.
        /// </summary>
        /// <param name="enableDvdNav">
        /// True to enable LibDVDNav.
        /// </param>
        public static void SetDvdNav(bool enableDvdNav)
        {
            HBFunctions.hb_dvd_set_dvdnav(enableDvdNav ? 1 : 0);
        }

        /// <summary>
        /// Call before app shutdown. Performs global cleanup.
        /// </summary>
        public static void DisposeGlobal()
        {
            HBFunctions.hb_global_close();
        }

        /// <summary>
        /// Register the logger.
        /// </summary>
        public static void RegisterLogger()
        {
            // Register the logger if we have not already
            if (loggingCallback == null)
            {
                // Keep the callback as a member to prevent it from being garbage collected.
                loggingCallback = LoggingHandler;
                errorCallback = ErrorHandler;
                HBFunctions.hb_register_logger(loggingCallback);
                HBFunctions.hb_register_error_handler(errorCallback);
            }
        }

        /// <summary>
        /// Handles log messages from HandBrake.
        /// </summary>
        /// <param name="message">
        /// The log message (including newline).
        /// </param>
        public static void LoggingHandler(string message)
        {
            if (!string.IsNullOrEmpty(message))
            {
                string[] messageParts = message.Split(new[] { "\n" }, StringSplitOptions.RemoveEmptyEntries);

                if (messageParts.Length > 0)
                {
                    message = messageParts[0];
                    SendMessageEvent(message);
                }
            }
        }

        /// <summary>
        /// Handles errors from HandBrake.
        /// </summary>
        /// <param name="message">
        /// The error message.
        /// </param>
        public static void ErrorHandler(string message)
        {
            if (!string.IsNullOrEmpty(message))
            {
                // These errors happen in normal operations. Log them as messages.
				if (message == "dvd: ifoOpen failed" || message.Contains("avformat_seek_file failed") || message.Contains("nav_get_title_list"))
                {
                    SendMessageEvent(message);
                    return;
                }

                SendErrorEvent(message);
            }
        }

        /// <summary>
        /// Gets the standard x264 option name given the starting point.
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The standard x264 option name.
        /// </returns>
        public static string SanitizeX264OptName(string name)
        {
            IntPtr namePtr = Marshal.StringToHGlobalAnsi(name);
            string sanitizedName = Marshal.PtrToStringAnsi(HBFunctions.hb_x264_encopt_name(namePtr));
            Marshal.FreeHGlobal(namePtr);
            return sanitizedName;
        }

        /// <summary>
        /// Checks to see if the given H.264 level is valid given the inputs.
        /// </summary>
        /// <param name="level">
        /// The level to check.
        /// </param>
        /// <param name="width">
        /// The output picture width.
        /// </param>
        /// <param name="height">
        /// The output picture height.
        /// </param>
        /// <param name="fpsNumerator">
        /// The rate numerator.
        /// </param>
        /// <param name="fpsDenominator">
        /// The rate denominator.
        /// </param>
        /// <param name="interlaced">
        /// True if x264 interlaced output is enabled.
        /// </param>
        /// <param name="fakeInterlaced">
        /// True if x264 fake interlacing is enabled.
        /// </param>
        /// <returns>
        /// True if the level is valid.
        /// </returns>
        public static bool IsH264LevelValid(string level, int width, int height, int fpsNumerator, int fpsDenominator, bool interlaced, bool fakeInterlaced)
        {
            return HBFunctions.hb_check_h264_level(
                level, 
                width, 
                height, 
                fpsNumerator, 
                fpsDenominator, 
                interlaced ? 1 : 0, 
                fakeInterlaced ? 1 : 0) == 0;
        }

        /// <summary>
        /// Creates an X264 options string from the given settings.
        /// </summary>
        /// <param name="preset">
        /// The x264 preset.
        /// </param>
        /// <param name="tunes">
        /// The x264 tunes being used.
        /// </param>
        /// <param name="extraOptions">
        /// The extra options string.
        /// </param>
        /// <param name="profile">
        /// The H.264 profile.
        /// </param>
        /// <param name="level">
        /// The H.264 level.
        /// </param>
        /// <param name="width">
        /// The width of the final picture.
        /// </param>
        /// <param name="height">
        /// The height of the final picture.
        /// </param>
        /// <returns>
        /// The full x264 options string from the given inputs.
        /// </returns>
        public static string CreateX264OptionsString(
            string preset, 
            IList<string> tunes, 
            string extraOptions, 
            string profile, 
            string level, 
            int width, 
            int height)
        {
            if (width <= 0)
            {
                throw new ArgumentException("width must be positive.");
            }

            if (height <= 0)
            {
                throw new ArgumentException("height must be positive.");
            }

            IntPtr ptr = HBFunctions.hb_x264_param_unparse(
                preset, 
                string.Join(",", tunes), 
                extraOptions, 
                profile, 
                level, 
                width, 
                height);

            string x264Settings = Marshal.PtrToStringAnsi(ptr);

            return x264Settings;
        }

        /// <summary>
        /// Gets the total number of seconds on the given encode job.
        /// </summary>
        /// <param name="job">
        /// The encode job to query.
        /// </param>
        /// <param name="title">
        /// The title being encoded.
        /// </param>
        /// <returns>
        /// The total number of seconds of video to encode.
        /// </returns>
        internal static double GetJobLengthSeconds(EncodeJob job, Title title)
        {
            switch (job.RangeType)
            {
                case VideoRangeType.All:
                    return title.Duration.TotalSeconds;
                case VideoRangeType.Chapters:
                    TimeSpan duration = TimeSpan.Zero;
                    for (int i = job.ChapterStart; i <= job.ChapterEnd; i++)
                    {
                        duration += title.Chapters[i - 1].Duration;
                    }

                    return duration.TotalSeconds;
                case VideoRangeType.Seconds:
                    return job.SecondsEnd - job.SecondsStart;
                case VideoRangeType.Frames:
                    return (job.FramesEnd - job.FramesStart) / title.Framerate;
            }

            return 0;
        }

        /// <summary>
        /// Gets the number of audio samples used per frame for the given audio encoder.
        /// </summary>
        /// <param name="encoderName">
        /// The encoder to query.
        /// </param>
        /// <returns>
        /// The number of audio samples used per frame for the given
        /// audio encoder.
        /// </returns>
        internal static int GetAudioSamplesPerFrame(string encoderName)
        {
            switch (encoderName)
            {
                case "faac":
                case "ffaac":
                case "copy:aac":
                case "vorbis":
                    return 1024;
                case "lame":
                case "copy:mp3":
                    return 1152;
                case "ffac3":
                case "copy":
                case "copy:ac3":
                case "copy:dts":
                case "copy:dtshd":
                    return 1536;
            }

            // Unknown encoder; make a guess.
            return 1536;
        }

        /// <summary>
        /// Gets the size in bytes for the audio with the given parameters.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="lengthSeconds">
        /// The length of the encode in seconds.
        /// </param>
        /// <param name="title">
        /// The title to encode.
        /// </param>
        /// <param name="outputTrackList">
        /// The list of tracks to encode.
        /// </param>
        /// <returns>
        /// The size in bytes for the audio with the given parameters.
        /// </returns>
        internal static long GetAudioSize(EncodeJob job, double lengthSeconds, Title title, List<Tuple<AudioEncoding, int>> outputTrackList)
        {
            long audioBytes = 0;

            foreach (Tuple<AudioEncoding, int> outputTrack in outputTrackList)
            {
                AudioEncoding encoding = outputTrack.Item1;
                AudioTrack track = title.AudioTracks[outputTrack.Item2 - 1];

                int samplesPerFrame = GetAudioSamplesPerFrame(encoding.Encoder);
                int audioBitrate;

                HBAudioEncoder audioEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(encoding.Encoder);

                if (audioEncoder.IsPassthrough)
                {
                    // Input bitrate is in bits/second.
                    audioBitrate = track.Bitrate / 8;
                }
                else if (encoding.EncodeRateType == AudioEncodeRateType.Quality)
                {
                    // Can't predict size of quality targeted audio encoding.
                    audioBitrate = 0;
                }
                else
                {
                    int outputBitrate;
                    if (encoding.Bitrate > 0)
                    {
                        outputBitrate = encoding.Bitrate;
                    }
                    else
                    {
                        outputBitrate = HandBrakeEncoderHelpers.GetDefaultBitrate(
                            audioEncoder, 
                            encoding.SampleRateRaw == 0 ? track.SampleRate : encoding.SampleRateRaw,
                            HandBrakeEncoderHelpers.SanitizeMixdown(HandBrakeEncoderHelpers.GetMixdown(encoding.Mixdown), audioEncoder, track.ChannelLayout));
                    }

                    // Output bitrate is in kbps.
                    audioBitrate = outputBitrate * 1000 / 8;
                }

                audioBytes += (long)(lengthSeconds * audioBitrate);

                // Audio overhead
                audioBytes += encoding.SampleRateRaw * ContainerOverheadPerFrame / samplesPerFrame;
            }

            return audioBytes;
        }

        /// <summary>
        /// Calculates the video bitrate for the given job and target size.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="sizeMB">
        /// The target size in MB.
        /// </param>
        /// <param name="overallSelectedLengthSeconds">
        /// The currently selected encode length. Used in preview
        /// for calculating bitrate when the target size would be wrong.
        /// </param>
        /// <returns>
        /// The video bitrate in kbps.
        /// </returns>
        public static int CalculateBitrate(EncodeJob job, Title title, int sizeMB, double overallSelectedLengthSeconds = 0)
        {
            long availableBytes = ((long)sizeMB) * 1024 * 1024;

            EncodingProfile profile = job.EncodingProfile;

            double lengthSeconds = overallSelectedLengthSeconds > 0 ? overallSelectedLengthSeconds : GetJobLengthSeconds(job, title);
            lengthSeconds += 1.5;

            double outputFramerate;
            if (profile.Framerate == 0)
            {
                outputFramerate = title.Framerate;
            }
            else
            {
                // Not sure what to do for VFR here hb_calc_bitrate never handled it...
                //   just use the peak for now.
                outputFramerate = profile.Framerate;
            }

            long frames = (long)(lengthSeconds * outputFramerate);

            availableBytes -= frames * ContainerOverheadPerFrame;

            List<Tuple<AudioEncoding, int>> outputTrackList = GetOutputTracks(job, title);
            availableBytes -= GetAudioSize(job, lengthSeconds, title, outputTrackList);

            if (availableBytes < 0)
            {
                return 0;
            }

            // Video bitrate is in kilobits per second, or where 1 kbps is 1000 bits per second.
            // So 1 kbps is 125 bytes per second.
            return (int)(availableBytes / (125 * lengthSeconds));
        }

        /// <summary>
        /// Gives estimated file size (in MB) of the given job and video bitrate.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="videoBitrate">
        /// The video bitrate to be used (kbps).
        /// </param>
        /// <returns>
        /// The estimated file size (in MB) of the given job and video bitrate.
        /// </returns>
        public static double CalculateFileSize(EncodeJob job, Title title, int videoBitrate)
        {
            long totalBytes = 0;

            EncodingProfile profile = job.EncodingProfile;

            double lengthSeconds = GetJobLengthSeconds(job, title);
            lengthSeconds += 1.5;

            double outputFramerate;
            if (profile.Framerate == 0)
            {
                outputFramerate = title.Framerate;
            }
            else
            {
                // Not sure what to do for VFR here hb_calc_bitrate never handled it...
                //   just use the peak for now.
                outputFramerate = profile.Framerate;
            }

            long frames = (long)(lengthSeconds * outputFramerate);

            totalBytes += (long)(lengthSeconds * videoBitrate * 125);
            totalBytes += frames * ContainerOverheadPerFrame;

            List<Tuple<AudioEncoding, int>> outputTrackList = GetOutputTracks(job, title);
            totalBytes += GetAudioSize(job, lengthSeconds, title, outputTrackList);

            return (double)totalBytes / 1024 / 1024;
        }

        /// <summary>
        /// Sends the message logged event to any registered listeners.
        /// </summary>
        /// <param name="message">
        /// The message to send.
        /// </param>
        private static void SendMessageEvent(string message)
        {
            if (MessageLogged != null)
            {
                MessageLogged(null, new MessageLoggedEventArgs { Message = message });
            }

            Debug.WriteLine(message);
        }

        /// <summary>
        /// Sends the error logged event to any registered listeners.
        /// </summary>
        /// <param name="message">
        /// The message to send
        /// </param>
        private static void SendErrorEvent(string message)
        {
            if (ErrorLogged != null)
            {
                ErrorLogged(null, new MessageLoggedEventArgs { Message = message });
            }

            Debug.WriteLine("ERROR: " + message);
        }

        /// <summary>
        /// Gets a list of encodings and target track indices (1-based).
        /// </summary>
        /// <param name="job">The encode job</param>
        /// <param name="title">The title the job is meant to encode.</param>
        /// <returns>A list of encodings and target track indices (1-based).</returns>
        private static List<Tuple<AudioEncoding, int>> GetOutputTracks(EncodeJob job, Title title)
        {
            var list = new List<Tuple<AudioEncoding, int>>();

            foreach (AudioEncoding encoding in job.EncodingProfile.AudioEncodings)
            {
                if (encoding.InputNumber == 0)
                {
                    // Add this encoding for all chosen tracks
                    foreach (int chosenTrack in job.ChosenAudioTracks)
                    {
                        // In normal cases we'll never have a chosen audio track that doesn't exist but when batch encoding
                        // we just choose the first audio track without checking if it exists.
                        if (chosenTrack <= title.AudioTracks.Count)
                        {
                            list.Add(new Tuple<AudioEncoding, int>(encoding, chosenTrack));
                        }
                    }
                }
                else if (encoding.InputNumber <= job.ChosenAudioTracks.Count)
                {
                    // Add this encoding for the specified track, if it exists
                    int trackNumber = job.ChosenAudioTracks[encoding.InputNumber - 1];

                    // In normal cases we'll never have a chosen audio track that doesn't exist but when batch encoding
                    // we just choose the first audio track without checking if it exists.
                    if (trackNumber <= title.AudioTracks.Count)
                    {
                        list.Add(new Tuple<AudioEncoding, int>(encoding, trackNumber));
                    }
                }
            }

            return list;
        }
    }
}
