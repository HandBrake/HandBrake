// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogFile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Model
{
    using HandBrakeWPF.Properties;

    public class LogFile
    {
        public LogFile(string logFileName, bool isComplete)
        {
            this.LogFileName = logFileName;
            this.IsComplete = isComplete;
        }

        public string LogFileName { get; set; }

        public bool IsComplete { get; set; }

        public string FileDisplayName
        {
            get
            {
                if (this.IsComplete)
                {
                    return string.Format("{0} {1}", Resources.LogViewModel_Complete, this.LogFileName);
                }

                return this.LogFileName;
            }
        }
    }
}
