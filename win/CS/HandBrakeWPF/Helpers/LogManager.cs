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

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.EventArgs;

    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using ILog = Services.Logging.Interfaces.ILog;

    public static class LogManager
    {
        private static ILog generalAppLogger;

        public static void Init()
        {
            generalAppLogger = IoC.Get<ILog>();
            string logDir = DirectoryUtilities.GetLogDirectory();
            string filename = string.Format("activity_log_main.{0}.txt", GeneralUtilities.ProcessId);
            string logFile = Path.Combine(logDir, filename);
            generalAppLogger.ConfigureLogging(filename, logFile);

            IoC.Get<ILogInstanceManager>().Register(filename, generalAppLogger, true);
            
            HandBrakeUtils.MessageLogged += HandBrakeUtils_MessageLogged;
            HandBrakeUtils.ErrorLogged += HandBrakeUtils_ErrorLogged;
        }

        private static void HandBrakeUtils_ErrorLogged(object sender, MessageLoggedEventArgs e)
        {
            if (e == null || string.IsNullOrEmpty(e.Message))
            {
                return;
            }

            generalAppLogger?.LogMessage(e.Message);
        }

        private static void HandBrakeUtils_MessageLogged(object sender, MessageLoggedEventArgs e)
        {
            if (e == null || string.IsNullOrEmpty(e.Message))
            {
                return;
            }

            generalAppLogger?.LogMessage(e.Message);
        }
    }
}