// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogHandlerConfig.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogHandlerConfig type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Model
{
    public class LogHandlerConfig
    {
        public LogHandlerConfig(bool enableDiskLogging, string logFile, bool deleteCurrentLogFirst, string header)
        {
            this.EnableDiskLogging = enableDiskLogging;
            this.LogFile = logFile;
            this.DeleteCurrentLogFirst = deleteCurrentLogFirst;
            this.Header = header;
        }

        public LogHandlerConfig()
        {
        }

        public bool EnableDiskLogging { get; set; }

        public string LogFile { get; set; }

        public bool DeleteCurrentLogFirst { get; set; }

        public string Header { get; set; }
    }
}