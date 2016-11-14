﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeUtils.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeUtils type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Json.Anamorphic;
    using HandBrake.ApplicationServices.Interop.Json.Shared;
    using HandBrake.ApplicationServices.Services.Logging;
    using HandBrake.ApplicationServices.Services.Logging.Interfaces;
    using HandBrake.ApplicationServices.Services.Logging.Model;

    using Newtonsoft.Json;

    /// <summary>
    /// HandBrake Interop Utilities
    /// </summary>
    public static class HandBrakeUtils
    {
        private static readonly ILog log = LogService.GetLogger();

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
            message = message.TrimEnd();
            if (!string.IsNullOrEmpty(message))
            {
                SendMessageEvent(message);
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
                8,
                preset,
                string.Join(",", tunes),
                extraOptions,
                profile,
                level,
                width,
                height); // TODO add bit-depth support.

            string x264Settings = Marshal.PtrToStringAnsi(ptr);

            return x264Settings;
        }

        /// <summary>
        /// Gets the final size and PAR of the video, given anamorphic inputs.
        /// </summary>
        /// <param name="anamorphicGeometry">Anamorphic inputs.</param>
        /// <returns>The final size and PAR of the video.</returns>
        public static Geometry GetAnamorphicSize(AnamorphicGeometry anamorphicGeometry)
        {
            string encode = JsonConvert.SerializeObject(anamorphicGeometry, Formatting.Indented, new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore });
            log.LogMessage(encode, LogMessageType.API, LogLevel.Debug);
            IntPtr json = HBFunctions.hb_set_anamorphic_size_json(Marshal.StringToHGlobalAnsi(encode));
            string result = Marshal.PtrToStringAnsi(json);
            return JsonConvert.DeserializeObject<Geometry>(result);
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
                log.LogMessage(message, LogMessageType.ScanOrEncode, LogLevel.Info);
                MessageLogged(null, new MessageLoggedEventArgs(message));
            }
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
                log.LogMessage(message, LogMessageType.ScanOrEncode, LogLevel.Error);
                ErrorLogged(null, new MessageLoggedEventArgs(message));
            }
        }
    }
}
