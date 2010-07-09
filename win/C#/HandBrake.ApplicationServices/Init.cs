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
        /// Setup the Settings used by the applicaiton with this library
        /// </summary>
        /// <param name="cli_minimized">
        /// The cli_minimized.
        /// </param>
        /// <param name="completionOption">
        /// The completion option.
        /// </param>
        /// <param name="disableDvdNav">
        /// The disable dvd nav.
        /// </param>
        /// <param name="enocdeStatusInGui">
        /// The enocde status in gui.
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
        public static void SetupSettings(bool cli_minimized, string completionOption, bool disableDvdNav, bool enocdeStatusInGui,
                                  bool growlEncode, bool growlQueue, string processPriority, string saveLogPath, bool saveLogToSpecifiedPath,
                                  bool saveLogWithVideo, bool showCliForInGuiEncodeStatus, bool preventSleep)
        {
            Properties.Settings.Default.cli_minimized = cli_minimized;
            Properties.Settings.Default.CompletionOption = completionOption;
            Properties.Settings.Default.disableDvdNav = disableDvdNav;
            Properties.Settings.Default.enocdeStatusInGui = enocdeStatusInGui;
            Properties.Settings.Default.growlEncode = growlEncode;
            Properties.Settings.Default.growlQueue = growlQueue;
            Properties.Settings.Default.processPriority = processPriority;
            Properties.Settings.Default.saveLogPath = saveLogPath;
            Properties.Settings.Default.saveLogToSpecifiedPath = saveLogToSpecifiedPath;
            Properties.Settings.Default.saveLogWithVideo = saveLogWithVideo;
            Properties.Settings.Default.showCliForInGuiEncodeStatus = showCliForInGuiEncodeStatus;
            Properties.Settings.Default.preventSleep = preventSleep;

            Properties.Settings.Default.Save();
        }

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
    }
}
