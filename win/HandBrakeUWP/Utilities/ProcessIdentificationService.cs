// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessIdentificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An interface for determining Process IDs.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeUWP.Utilities
{
    using System.Diagnostics;
    using System.Linq;
    using HandBrake.Utilities.Interfaces;
    using Windows.System.Diagnostics;

    /// <summary>
    /// UWP Process Identification Service.
    /// </summary>
    public class ProcessIdentificationService : IProcessIdentificationService
    {
        public int ProcessId => Process.GetCurrentProcess().Id;

        public bool IsProccessIdActive(int id)
        {
            // Will only display processes it has access to, so it will only ever return HandBrake.
            var processes = ProcessDiagnosticInfo.GetForProcesses()
                .Where(process => process.ExecutableFileName == "HandBrake.exe") // just in case though
                .Select(process => process.ProcessId)
                .ToList();

            return processes.Contains((uint)id);
        }
    }
}