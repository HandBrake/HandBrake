// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeUtils.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeUtils type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.ExceptionServices;
    using System.Runtime.InteropServices;
    using System.Text.Json;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Json.Anamorphic;
    using HandBrake.Interop.Interop.Json.Shared;
    using HandBrake.Interop.Utilities;

    /// <summary>
    /// HandBrake Interop Utilities
    /// </summary>
    public static class HandBrakeUtils
    {
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
        /// True if we initialized without hardware support.
        /// </summary>
        private static bool initNoHardware = false;

        /// <summary>
        /// Fires when HandBrake has logged a message.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> MessageLogged;

        /// <summary>
        /// Fires when HandBrake has logged an error.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> ErrorLogged;

        /// <summary>
        /// Ensures the HB global initialize method has been called.
        /// </summary>
        /// <param name="initNoHardwareMode">
        /// Init without hardware encoders.
        /// </param>
        public static void EnsureGlobalInit(bool initNoHardwareMode)
        {
            if (!globalInitialized)
            {
                bool initSuccess;
                try
                {
                    if (initNoHardwareMode)
                    {
                        initNoHardware = true;
                        if (HBFunctions.hb_global_init_no_hardware() == -1)
                        {
                            throw new InvalidOperationException("HB global init failed. (-1)");
                        }

                        initSuccess = true;
                    }
                    else
                    {
                        initSuccess = TryInit();
                    }
                }
                catch (Exception)
                {
                    initSuccess = false;
                }

                // Try without Hardware support. Bad drivers can sometimes cause issues.
                if (!initSuccess)
                {
                    if (HBFunctions.hb_global_init_no_hardware() == -1)
                    {
                        throw new InvalidOperationException("HB global init failed. (-2)");
                    }

                    initNoHardware = true;
                }

                globalInitialized = true;
            }
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
            string encode = JsonSerializer.Serialize(anamorphicGeometry, JsonSettings.Options);
            IntPtr json = HBFunctions.hb_set_anamorphic_size_json(Marshal.StringToHGlobalAnsi(encode));
            string result = Marshal.PtrToStringAnsi(json);
            return JsonSerializer.Deserialize<Geometry>(result, JsonSettings.Options);
        }

        public static void Reduce(long den, long num, out long x, out long y)
        {
            // find the greatest common divisor of num & den by Euclid's algorithm
            long n = num, d = den;
            while (d > 0)
            {
                long t = d;
                d = n % d;
                n = t;
            }

            // at this point n is the gcd. if it's non-zero remove it from num
            // and den. Otherwise just return the original values.
            if (n > 0)
            {
                num /= n;
                den /= n;
            }

            x = num;
            y = den;
        }

        /// <summary>
        /// Sends the message logged event to any registered listeners.
        /// </summary>
        /// <param name="message">
        /// The message to send.
        /// </param>
        public static void SendMessageEvent(string message)
        {
            MessageLogged?.Invoke(null, new MessageLoggedEventArgs(message));
        }

        /// <summary>
        /// Sends the error logged event to any registered listeners.
        /// </summary>
        /// <param name="message">
        /// The message to send
        /// </param>
        public static void SendErrorEvent(string message)
        {
            ErrorLogged?.Invoke(null, new MessageLoggedEventArgs(message));
        }

        /// <summary>
        /// Returns true if we have successfully run global initialization.
        /// </summary>
        /// <returns>True if we have successfully run global initialization.</returns>
        public static bool IsInitialised()
        {
            return globalInitialized;
        }

        /// <summary>
        /// Returns true if we initialized without hardware support.
        /// </summary>
        /// <returns>True if we initialized without hardware support.</returns>
        public static bool IsInitNoHardware()
        {
            return initNoHardware;
        }

        private static bool TryInit()
        {
            try
            {
                if (HBFunctions.hb_global_init() == -1)
                {
                    throw new InvalidOperationException("HB global init failed.");
                }
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }
    }
}
