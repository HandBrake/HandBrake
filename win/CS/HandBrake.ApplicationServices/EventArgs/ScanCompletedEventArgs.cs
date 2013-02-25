// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.EventArgs
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Scan Progress Event Args
    /// </summary>
    [DataContractAttribute]
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
        public ScanCompletedEventArgs(bool cancelled, Exception exception, string errorInformation)
        {
            this.Successful = !cancelled && exception == null && string.IsNullOrEmpty(errorInformation);
            this.Cancelled = cancelled;
            this.Exception = exception;
            this.ErrorInformation = errorInformation;
        }

        /// <summary>
        /// Gets or sets a value indicating whether Successful.
        /// </summary>
        [DataMember]
        public bool Successful { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Cancelled.
        /// </summary>
        [DataMember]
        public bool Cancelled { get; set; }

        /// <summary>
        /// Gets or sets Exception.
        /// </summary>
        [DataMember]
        public Exception Exception { get; set; }

        /// <summary>
        /// Gets or sets ErrorInformation.
        /// </summary>
        [DataMember]
        public string ErrorInformation { get; set; }
    }
}
