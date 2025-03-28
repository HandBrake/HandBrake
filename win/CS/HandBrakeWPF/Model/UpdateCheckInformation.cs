// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateCheckInformation.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Provides information about an update check.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System;

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
        /// Gets a value indicating whether an Error Occurred.
        /// </summary>
        public bool ErrorOccurred
        {
            get { return this.Error != null; }
        }

        /// <summary>
        /// Gets or sets Information about an update to HandBrake
        /// </summary>
        public Uri DescriptionUrl { get; set; }

        /// <summary>
        /// Gets or sets HandBrake's version from the appcast.xml file.
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets HandBrake's Build from the appcast.xml file.
        /// </summary>
        public string Build { get; set; }

        /// <summary>
        /// Gets or sets the URL for update file.
        /// </summary>
        public string DownloadFile { get; set; }

        /// <summary>
        /// Gets or sets the error that occurred, if any. This will be null if no error occurred.
        /// </summary>
        public Exception Error { get; set; }

        /// <summary>
        /// Gets or sets the expected RSA 4096bit SHA256 Signature
        /// </summary>
        public string Signature { get; set; }

        public bool UseLargerKey { get; set; }
    }
}
