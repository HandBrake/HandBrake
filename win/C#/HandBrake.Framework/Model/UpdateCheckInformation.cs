/*  UpdateCheckInformation.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Model
{
    using System;
    using HandBrake.Framework.Helpers;

    /// <summary>
    /// Provides information about an update check.
    /// </summary>
    public struct UpdateCheckInformation
    {
        /// <summary>
        /// Gets or sets a value indicating whether a New Version is Available.
        /// </summary>
        public bool NewVersionAvailable { get; set; }

        /// <summary>
        /// Gets a value indicating whether an Error Occured.
        /// </summary>
        public bool ErrorOccured
        {
            get { return this.Error != null; }
        }

        /// <summary>
        /// Gets or sets information about the new build, if any. This will be null if there is no new verison.
        /// </summary>
        public AppcastReader BuildInformation { get; set; }

        /// <summary>
        /// Gets or sets the error that occurred, if any. This will be null if no error occured.
        /// </summary>
        public Exception Error { get; set; }
    }
}
