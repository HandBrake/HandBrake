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

    /// <summary>
    /// Encode Progress Event Args
    /// </summary>
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
        /// <param name="logPath">
        /// The path and filename of the log for this encode.
        /// </param>
        /// <param name="finalSizeInBytes">
        /// The final size of the file in bytes.
        /// </param>
        public EncodeCompletedEventArgs(bool sucessful, Exception exception, string errorInformation, string filename, string logPath, long finalSizeInBytes)
        {
            this.Successful = sucessful;
            this.Exception = exception;
            this.ErrorInformation = errorInformation;
            this.FileName = filename;
            this.ActivityLogPath = logPath;
            this.FinalFilesizeInBytes = finalSizeInBytes;
        }

        /// <summary>
        /// Gets or sets the file name.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets or sets a value indicating whether Successful.
        /// </summary>
        public bool Successful { get; private set; }

        /// <summary>
        /// Gets or sets Exception.
        /// </summary>
        public Exception Exception { get; private set; }

        /// <summary>
        /// Gets or sets ErrorInformation.
        /// </summary>
        public string ErrorInformation { get; private set; }

        /// <summary>
        /// 
        /// </summary>
        public string ActivityLogPath { get; private set; }

        /// <summary>
        /// Final filesize in bytes
        /// </summary>
        public long FinalFilesizeInBytes { get; private set; }
    }
}
