// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.EventArgs
{
    using System;

    using HandBrakeWPF.Services.Scan.Model;

    /// <summary>
    /// Scan Progress Event Args
    /// </summary>
    public class ScanCompletedEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ScanCompletedEventArgs"/> class.
        /// </summary>
        /// <param name="cancelled">
        /// Whether the scan was cancelled.
        /// </param>
        /// <param name="exception">
        /// The exception.
        /// </param>
        /// <param name="errorInformation">
        /// The error information.
        /// </param>
        /// <param name="scannedSource">
        /// The scanned Source.
        /// </param>
        public ScanCompletedEventArgs(bool cancelled, Exception exception, string errorInformation, Source scannedSource)
        {
            this.Successful = !cancelled && exception == null && string.IsNullOrEmpty(errorInformation) && scannedSource != null && scannedSource.Titles != null && scannedSource.Titles.Count > 0;
            this.Cancelled = cancelled;
            this.Exception = exception;
            this.ErrorInformation = errorInformation;
            this.ScannedSource = scannedSource;
        }

        /// <summary>
        /// Gets a value indicating whether Successful.
        /// </summary>
        public bool Successful { get; private set; }

        /// <summary>
        /// Gets a value indicating whether Cancelled.
        /// </summary>
        public bool Cancelled { get; private set; }

        /// <summary>
        /// Gets the Exception.
        /// </summary>
        public Exception Exception { get; private set; }

        /// <summary>
        /// Gets ErrorInformation.
        /// </summary>
        public string ErrorInformation { get; private set; }

        /// <summary>
        /// Gets the scanned source.
        /// </summary>
        public Source ScannedSource { get; private set; }
    }
}
