/*  Init.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices
{
    using System;
    using System.Reflection;

    /// <summary>
    /// Initialize ApplicationServices
    /// </summary>
    public class Init
    {
        /// <summary>
        /// Gets the Assembly version.
        /// </summary>
        /// <returns>
        /// Version data
        /// </returns>
        public static Version AssemblyVersion()
        {
            return Assembly.GetExecutingAssembly().GetName().Version;
        }

        /// <summary>
        /// The instance ID used by the Main Applicaiton
        /// </summary>
        public static int InstanceId;

        /// <summary>
        /// The Applicaiton that uses this DLL can pass in it's version string.
        /// </summary>
        public static string HandBrakeGuiVersionString;

        /// <summary>
        /// What to do when the encode completes.
        /// </summary>
        public static string CompletionOption;

        /// <summary>
        /// Disable LibDvdNav
        /// </summary>
        public static bool DisableDvdNav;

        /// <summary>
        /// Growl when an encode has finished.
        /// </summary>
        public static bool GrowlEncode;

        /// <summary>
        /// Growl when a queue has finished.
        /// </summary>
        public static bool GrowlQueue;

        /// <summary>
        /// The Process Priority for HandBrakeCLI
        /// </summary>
        public static string ProcessPriority;

        /// <summary>
        /// Path to save log files to.
        /// </summary>
        public static string SaveLogPath;

        /// <summary>
        /// Copy log files to the SaveLogPath
        /// </summary>
        public static bool SaveLogToSpecifiedPath;

        /// <summary>
        /// Save a copy of the log files with the video
        /// </summary>
        public static bool SaveLogWithVideo;

        /// <summary>
        /// Show the CLI window when encoding.
        /// </summary>
        public static bool ShowCliForInGuiEncodeStatus;

        /// <summary>
        /// Prevent system sleep
        /// </summary>
        public static bool PreventSleep;

        /// <summary>
        /// Setup the Settings used by the applicaiton with this library
        /// </summary>
        /// <param name="versionString">
        /// The version / name of the application that's using this DLL.
        /// </param>
        /// <param name="instanceId">
        /// The Instance ID
        /// </param>
        /// <param name="completionOption">
        /// The completion option.
        /// </param>
        /// <param name="disableDvdNav">
        /// The disable dvd nav.
        /// </param>
        /// <param name="growlEncode">
        /// The growl encode.
        /// </param>
        /// <param name="growlQueue">
        /// The growl queue.
        /// </param>
        /// <param name="processPriority">
        /// The process priority.
        /// </param>
        /// <param name="saveLogPath">
        /// The save log path.
        /// </param>
        /// <param name="saveLogToSpecifiedPath">
        /// The save log to specified path.
        /// </param>
        /// <param name="saveLogWithVideo">
        /// The save log with video.
        /// </param>
        /// <param name="showCliForInGuiEncodeStatus">
        /// The show cli for in gui encode status.
        /// </param>
        /// <param name="preventSleep">
        /// Prevent the system from sleeping
        /// </param>
        public static void SetupSettings(string versionString, int instanceId, string completionOption, bool disableDvdNav,
                                  bool growlEncode, bool growlQueue, string processPriority, string saveLogPath, bool saveLogToSpecifiedPath,
                                  bool saveLogWithVideo, bool showCliForInGuiEncodeStatus, bool preventSleep)
        {
            InstanceId = instanceId;
            HandBrakeGuiVersionString = versionString;
            CompletionOption = completionOption;
            DisableDvdNav = disableDvdNav;
            GrowlEncode = growlEncode;
            GrowlQueue = growlQueue;
            ProcessPriority = processPriority;
            SaveLogPath = saveLogPath;
            SaveLogToSpecifiedPath = saveLogToSpecifiedPath;
            SaveLogWithVideo = saveLogWithVideo;
            ShowCliForInGuiEncodeStatus = showCliForInGuiEncodeStatus;
            PreventSleep = preventSleep;
        }
    }
}
