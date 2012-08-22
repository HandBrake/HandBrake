// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IIsolatedEncodeService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Isolated Encode Service interface.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Isolation.Interfaces
{
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The Isolated Encode Service interface.
    /// </summary>
    public interface IIsolatedEncodeService : IEncode
    {
        /// <summary>
        /// The disconnect.
        /// </summary>
        void Disconnect();
    }
}