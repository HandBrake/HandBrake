// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progess Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// Encode Progess Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The EncodeProgressEventArgs.
    /// </param>
    public delegate void EncodeProgessStatus(object sender, EncodeProgressEventArgs e);

    /// <summary>
    /// Encode Progess Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The EncodeProgressEventArgs.
    /// </param>
    public delegate void EncodeCompletedStatus(object sender, EncodeCompletedEventArgs e);

    /// <summary>
    /// The IEncode Interface
    /// </summary>
    public interface IEncode
    {
        /// <summary>
        /// Fires when a new CLI Job starts
        /// </summary>
        event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI job finishes.
        /// </summary>
        event EncodeCompletedStatus EncodeCompleted;

        /// <summary>
        /// Encode process has progressed
        /// </summary>
        event EncodeProgessStatus EncodeStatusChanged;

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        bool IsEncoding { get; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="enableLogging">
        /// The enable Logging.
        /// </param>
        void Start(QueueTask job, bool enableLogging);

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        void Stop();

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        void SafelyStop();

        /// <summary>
        /// Copy the log file to the desired destinations
        /// </summary>
        /// <param name="destination">
        /// The destination.
        /// </param>
        void ProcessLogs(string destination);
    }
}