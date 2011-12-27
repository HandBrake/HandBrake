// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IUpdateVersionService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IUpdateVersionService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    /// <summary>
    /// The IUpdateVersionService Interface
    /// </summary>
    public interface IUpdateVersionService
    {
        /// <summary>
        /// Get's HandBrakes version data from the CLI.
        /// </summary>
        void SetCliVersionData();
    }
}