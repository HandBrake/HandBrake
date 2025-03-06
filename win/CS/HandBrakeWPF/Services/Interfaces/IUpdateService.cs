// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IUpdateService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Interface for the Update Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System;

    using HandBrakeWPF.Model;

    /// <summary>
    /// An Interface for the Update Service
    /// </summary>
    public interface IUpdateService
    {
        /// <summary>
        /// Perform an update check at application start, but only daily, weekly or monthly depending on the users settings.
        /// </summary>
        /// <param name="callback">
        /// The callback.
        /// </param>
        void PerformStartupUpdateCheck(Action<UpdateCheckInformation> callback);

        /// <summary>
        /// Perform an Update check and execute the Action when complete.
        /// </summary>
        /// <param name="callback">
        /// The callback.
        /// </param>
        void CheckForUpdates(Action<UpdateCheckInformation> callback);

        /// <summary>
        /// Download the update file.
        /// </summary>
        /// <param name="update">
        /// Update Check Information
        /// </param>
        /// <param name="completed">
        /// The complete.
        /// </param>
        /// <param name="progress">
        /// The progress.
        /// </param>
        void DownloadFile(UpdateCheckInformation update, Action<DownloadStatus> completed, Action<DownloadStatus> progress);
    }
}
