// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessIdentificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An interface for determining Process IDs.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using HandBrake.Utilities.Interfaces;

    public class ProcessIdentificationService : IProcessIdentificationService
    {
        public int ProcessId => Win32.ProcessId;

        public bool IsProccessIdActive(int id)
        {
            return Win32.IsPidACurrentHandBrakeInstance(id);
        }
    }
}