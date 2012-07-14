// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IDriveDetectService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IDriveDetectService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System;

    /// <summary>
    /// The DriveDetectService interface.
    /// </summary>
    public interface IDriveDetectService
    {
        /// <summary>
        /// The start detection.
        /// </summary>
        /// <param name="action">
        /// The detection Action.
        /// </param>
        void StartDetection(Action action);

        /// <summary>
        /// Stop the watcher. Must be done before the app shuts down.
        /// </summary>
        void Close();
    }
}