// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogFileEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.EventArgs
{
    using System;

    using HandBrakeWPF.Services.Logging.Interfaces;

    public class LogFileEventArgs : EventArgs
    {
        public LogFileEventArgs(string fileName, ILog logInstance)
        {
            this.FileName = fileName;
            this.LogInstance = logInstance;
        }

        public string FileName { get; }

        public ILog LogInstance { get; }
    }
}
