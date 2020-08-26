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
        public EncodeCompletedEventArgs(bool successful, Exception exception, string errorInformation, string sourceFileName, string filename, string logPath, long finalSizeInBytes)
        {
            this.Successful = successful;
            this.Exception = exception;
            this.ErrorInformation = errorInformation;
            this.SourceFileName = sourceFileName;
            this.FileName = filename;
            this.ActivityLogPath = logPath;
            this.FinalFilesizeInBytes = finalSizeInBytes;
        }

        public string FileName { get; private set; }

        public bool Successful { get; private set; }

        public Exception Exception { get; private set; }

        public string ErrorInformation { get; private set; }

        public string SourceFileName { get; private set; }

        public string ActivityLogPath { get; private set; }

        public long FinalFilesizeInBytes { get; private set; }
    }
}
