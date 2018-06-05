// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log manager.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.IO;

    using HandBrake.Interop.Interop;

    using HandBrakeWPF.Utilities;

    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;
    using LogService = HandBrakeWPF.Services.Logging.LogService;

    /// <summary>
    /// Tempory Class to Initialise the logging.
    /// </summary>
    public static class LogManager
    {
        /// <summary>
        /// The init.
        /// </summary>
        public static void Init()
        {
            ILog log = LogService.GetLogger();
            string logDir = DirectoryUtilities.GetLogDirectory();
            string logFile = Path.Combine(logDir, string.Format("activity_log{0}.txt", GeneralUtilities.ProcessId));
            if (!Directory.Exists(Path.GetDirectoryName(logFile)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(logFile));
            }

            log.Enable();
            log.SetupLogHeader(GeneralUtilities.CreateLogHeader().ToString());
            log.EnableLoggingToDisk(logFile, true);
            HandBrakeUtils.MessageLogged += HandBrakeUtils_MessageLogged;
            HandBrakeUtils.ErrorLogged += HandBrakeUtils_ErrorLogged;
        }

        /// <summary>
        /// Subscribe the ErrorLogged event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void HandBrakeUtils_ErrorLogged(object sender, HandBrake.Interop.Interop.EventArgs.MessageLoggedEventArgs e)
        {
        }

        /// <summary>
        ///  Subscribe the MessageLogged event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void HandBrakeUtils_MessageLogged(object sender, HandBrake.Interop.Interop.EventArgs.MessageLoggedEventArgs e)
        {
        }
    }
}