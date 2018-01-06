// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DownloadStatus.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Progress of a File Download
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System;

    /// <summary>
    /// The Progress of a File Download
    /// </summary>
    public class DownloadStatus
    {
        /// <summary>
        /// Gets or sets BytesRead.
        /// </summary>
        public long BytesRead { get; set; }

        /// <summary>
        /// Gets or sets TotalBytes.
        /// </summary>
        public long TotalBytes { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether WasSuccessful.
        /// </summary>
        public bool WasSuccessful { get; set; }

        /// <summary>
        /// Gets or sets Exception.
        /// </summary>
        public Exception Exception { get; set; }

        /// <summary>
        /// Gets or sets Message.
        /// </summary>
        public string Message { get; set; }

        /// <summary>
        /// Gets or sets FilePath.
        /// </summary>
        public string FilePath { get; set; }
    }
}
