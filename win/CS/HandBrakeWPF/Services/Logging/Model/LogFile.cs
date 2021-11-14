// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogFile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Model
{
    using System;

    using HandBrakeWPF.Properties;

    public class LogFile
    {
        public LogFile(string logFileName, bool isComplete)
        {
            this.LogFileName = logFileName;
            this.IsComplete = isComplete;
        }

        public string LogFileName { get; private set; }

        public bool IsComplete { get; private set; }

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

        protected bool Equals(LogFile other)
        {
            return this.LogFileName == other.LogFileName && this.IsComplete == other.IsComplete;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return Equals((LogFile)obj);
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(this.LogFileName, this.IsComplete);
        }
    }
}
