// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.EventArgs
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Encode Progress Event Args
    /// </summary>
    [DataContract]
    public class EncodeCompletedEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="EncodeCompletedEventArgs"/> class.
        /// </summary>
        /// <param name="sucessful">
        /// The sucessful.
        /// </param>
        /// <param name="exception">
        /// The exception.
        /// </param>
        /// <param name="errorInformation">
        /// The error information.
        /// </param>
        /// <param name="filename">
        /// The filename.
        /// </param>
        public EncodeCompletedEventArgs(bool sucessful, Exception exception, string errorInformation, string filename)
        {
            this.Successful = sucessful;
            this.Exception = exception;
            this.ErrorInformation = errorInformation;
            this.FileName = filename;
        }

        /// <summary>
        /// Gets or sets the file name.
        /// </summary>
        [DataMember]
        public string FileName { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Successful.
        /// </summary>
        [DataMember]
        public bool Successful { get; set; }

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
