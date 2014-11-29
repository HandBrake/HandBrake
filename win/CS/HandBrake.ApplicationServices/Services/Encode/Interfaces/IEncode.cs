// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progess Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;

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
        /// Gets the log index. The current log row counter.
        /// </summary>
        int LogIndex { get; }

        /// <summary>
        /// Gets a value indicating whether can pause.
        /// </summary>
        bool CanPause { get; }

        /// <summary>
        /// Gets a value indicating whether is pasued.
        /// </summary>
        bool IsPasued { get; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        void Start(QueueTask job);

        /// <summary>
        /// The pause.
        /// </summary>
        void Pause();

        /// <summary>
        /// The resume.
        /// </summary>
        void Resume();

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        void Stop();

        /// <summary>
        /// Copy the log file to the desired destinations
        /// </summary>
        /// <param name="destination">
        /// The destination.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        void ProcessLogs(string destination, HBConfiguration configuration);

        /// <summary>
        /// Shutdown the service.
        /// </summary>
        void Shutdown();
    }
}