// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InitCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the InitCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Worker
{
    public class InitCommand
    {
        public int LogVerbosity { get; set; }

        public string LogDirectory { get; set; }

        public string LogFile { get; set; }

        public bool EnableDiskLogging { get; set; }

        public bool EnableHardwareAcceleration { get; set; }

        public bool EnableLibDvdNav { get; set; }

        public bool AllowDisconnectedWorker { get; set; }
    }
}
