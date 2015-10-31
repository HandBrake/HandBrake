// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progess Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.Model;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeProgressEventArgs;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

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
        /// Fires when a new Job starts
        /// </summary>
        event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a job finishes.
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
        /// Gets a value indicating whether is pasued.
        /// </summary>
        bool IsPasued { get; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        void Start(EncodeTask job, HBConfiguration configuration);

        /// <summary>
        /// The pause.
        /// </summary>
        void Pause();

        /// <summary>
        /// The resume.
        /// </summary>
        void Resume();

        /// <summary>
        /// Kill the process
        /// </summary>
        void Stop();
    }
}