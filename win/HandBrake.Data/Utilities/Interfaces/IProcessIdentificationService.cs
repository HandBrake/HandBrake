// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IProcessIdentificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An interface for determining Process IDs.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    /// <summary>
    /// An interface for determining Process IDs.
    /// </summary>
    public interface IProcessIdentificationService
    {
        /// <summary>
        /// Gets the current HandBrake process Id.
        /// </summary>
        int ProcessId { get; }

        /// <summary>
        /// The find hand brake instance ids.
        /// </summary>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>. True if it's a running HandBrake instance.
        /// </returns>
        bool IsProccessIdActive(int id);
    }
}