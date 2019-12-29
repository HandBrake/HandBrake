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

    using HandBrake.Worker.Logging.Models;

    using HandBrakeWPF.Services.Logging.Model;
    using HandBrakeWPF.Utilities;

    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;

    public static class LogManager
    {
        public static void Init()
        {
            ILog log = IoC.Get<ILog>();
            string logDir = DirectoryUtilities.GetLogDirectory();
            string logFile = Path.Combine(logDir, string.Format("activity_log{0}.txt", GeneralUtilities.ProcessId));
            log.ConfigureLogging(new LogHandlerConfig(true, logFile, true, GeneralUtilities.CreateLogHeader().ToString()));
        }
    }
}