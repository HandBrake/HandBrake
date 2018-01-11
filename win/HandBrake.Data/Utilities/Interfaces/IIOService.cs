// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IIOService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    public interface IIOService
    {
        /// <summary>
        /// Gets a value indicating whether the App has access to the computer's storage. This will be false with UWP.
        /// </summary>
        bool HasDriveAccess { get; }
    }
}