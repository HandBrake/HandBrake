// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IQueueService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Queue Processor
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Interfaces
{
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel;

    using HandBrakeWPF.Services.Queue.Model;

    using IEncode = Encode.Interfaces.IEncode;

    /// <summary>
    /// The Queue Processor
    /// </summary>
    public interface IQueueService
    {
        #region Events

        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        event QueueService.QueueProgressStatus JobProcessingStarted;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        event EventHandler QueueChanged;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        event QueueService.QueueCompletedEventDelegate QueueCompleted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        event EventHandler QueuePaused;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the number of jobs in the queue
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Gets the number of errors detected in the queue.
        /// </summary>
        int ErrorCount { get; }

        /// <summary>
        /// Gets the IEncodeService instance.
        /// </summary>
        IEncode EncodeService { get; }

        /// <summary>
        /// Gets a value indicating whether IsProcessing.
        /// </summary>
        bool IsProcessing { get; }

        /// <summary>
        /// Gets or sets Last Processed Job.
        /// This is set when the job is poped of the queue by GetNextJobForProcessing();
        /// </summary>
        QueueTask LastProcessedJob { get; set; }

        /// <summary>
        /// Gets The current queue.
        /// </summary>
        ObservableCollection<QueueTask> Queue { get; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a job to the Queue. 
        /// This method is Thread Safe.
        /// </summary>
        /// <param name="job">
        /// The encode Job object.
        /// </param>
        void Add(QueueTask job);

        /// <summary>
        /// Backup any changes to the queue file
        /// </summary>
        /// <param name="exportPath">
        /// If this is not null or empty, this will be used instead of the standard backup location.
        /// </param>
        void BackupQueue(string exportPath);

        /// <summary>
        /// Export the windows queue in JSON format.
        /// Note: Note compatible with CLI.
        /// </summary>
        /// <param name="exportPath">
        /// The export path.
        /// </param>
        void ExportJson(string exportPath);

        /// <summary>
        /// Restore a JSON queue file.
        /// </summary>
        /// <param name="path">
        /// Path to the file the user wishes to import.
        /// </param>
        void ImportJson(string path);

        /// <summary>
        /// Export the Queue the standardised JSON format for the CLI
        /// </summary>
        /// <param name="exportPath">
        /// The export path.
        /// </param>
        void ExportCliJson(string exportPath);

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">
        /// The destination of the encode.
        /// </param>
        /// <returns>
        /// Whether or not the supplied destination is already in the queue.
        /// </returns>
        bool CheckForDestinationPathDuplicates(string destination);

        /// <summary>
        /// Clear down all Queue Items
        /// </summary>
        void Clear();

        /// <summary>
        /// Clear down the Queue´s completed items
        /// </summary>
        void ClearCompleted();

        /// <summary>
        /// Get the first job on the queue for processing.
        /// This also removes the job from the Queue and sets the LastProcessedJob
        /// </summary>
        /// <returns>
        /// An encode Job object.
        /// </returns>
        QueueTask GetNextJobForProcessing();

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">
        /// The zero-based location of the job in the queue.
        /// </param>
        void MoveDown(int index);

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">
        /// The zero-based location of the job in the queue.
        /// </param>
        void MoveUp(int index);

        /// <summary>
        /// Remove a job from the Queue.
        /// This method is Thread Safe
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        void Remove(QueueTask job);

        /// <summary>
        /// Reset a Queued Item from Error or Completed to Waiting
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        void ResetJobStatusToWaiting(QueueTask job);

        /// <summary>
        /// Restore a Queue from file or from the queue backup file.
        /// </summary>
        /// <param name="importPath">
        /// The import path. String.Empty or null will result in the default file being loaded.
        /// </param>
        void RestoreQueue(string importPath);

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        /// <param name="clearCompleted">
        /// The clear Completed.
        /// </param>
        void Start(bool clearCompleted);

        /// <summary>
        /// Stop the current encode and pause the queue.
        /// </summary>
        void Stop();

        /// <summary>
        /// Pause the queue but allow the current encode to complete.
        /// </summary>
        void Pause();

        /// <summary>
        /// Pause and Encode and the Queue.
        /// </summary>
        void PauseEncode();

        #endregion
    }
}