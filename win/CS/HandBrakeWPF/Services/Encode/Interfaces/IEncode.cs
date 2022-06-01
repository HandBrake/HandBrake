// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progress Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Interfaces
{
    using System;

    using HandBrake.Interop.Interop.Interfaces.Model;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeProgressEventArgs;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// Encode Progress Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The EncodeProgressEventArgs.
    /// </param>
    public delegate void EncodeProgressStatus(object sender, EncodeProgressEventArgs e);

    /// <summary>
    /// Encode Progress Status
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
        event EncodeProgressStatus EncodeStatusChanged;

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        bool IsEncoding { get; }

        /// <summary>
        /// Gets a value indicating whether is paused.
        /// </summary>
        bool IsPaused { get; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="basePresetName">
        /// Name of the base preset used for logging purposes.
        /// </param>
        void Start(EncodeTask job, string basePresetName);

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

        /// <summary>
        /// Get a copy of the Active job
        /// </summary>
        EncodeTask GetActiveJob();
    }
}