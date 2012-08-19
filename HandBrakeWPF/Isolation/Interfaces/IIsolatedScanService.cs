// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IIsolatedScanService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Isolated Scan Service interface.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Isolation.Interfaces
{
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The Isolated Scan Service interface.
    /// </summary>
    public interface IIsolatedScanService : IScan
    {
        /// <summary>
        /// The disconnect.
        /// </summary>
        void Disconnect();
    }
}