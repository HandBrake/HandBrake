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
    using System.Collections.Generic;
    using System.Collections.ObjectModel;


    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.Services.Queue.Model;

    public interface IQueueService
    {
        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        event EventHandler<QueueProgressEventArgs> JobProcessingStarted;

        /// <summary>
        /// Fires when the status of any running job changes on the queue. Including progress.
        /// </summary>
        event EventHandler QueueJobStatusChanged;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        event EventHandler QueueChanged;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        event EventHandler<QueueCompletedEventArgs> QueueCompleted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        event EventHandler QueuePaused;

        event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        /// <summary>
        /// Gets the number of jobs in the queue
        /// </summary>
        int Count { get; }

        int ActiveJobCount { get; }

        /// <summary>
        /// Gets the number of errors detected in the queue.
        /// </summary>
        int ErrorCount { get; }

        /// <summary>
        /// Gets the number of completed jobs.
        /// </summary>
        int CompletedCount { get; }

        /// <summary>
        /// Gets a value indicating whether IsProcessing.
        /// </summary>
        bool IsProcessing { get; }

        bool IsEncoding { get; }

        bool IsPaused { get; }

        /// <summary>
        /// Gets The current queue.
        /// </summary>
        ObservableCollection<QueueTask> Queue { get; }

        /// <summary>
        /// Add a job to the Queue. 
        /// This method is Thread Safe.
        /// </summary>
        /// <param name="job">
        /// The encode Job object.
        /// </param>
        void Add(QueueTask job);

        /// <summary>
        /// Retry a job and update the queue status
        /// </summary>
        /// <param name="task">
        /// The job to retry
        /// </param>
        void RetryJob(QueueTask task);

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
        /// Get the log file paths for jobs still on the queue. (Including completed) 
        /// </summary>
        /// <returns>List of filepaths</returns>
        List<string> GetLogFilePaths();

        /// <summary>
        /// Moves items in the queue list
        /// </summary>
        void MoveToBottom(IList<QueueTask> moveItems);

        /// <summary>
        /// Moves items in the queue list
        /// </summary>
        void MoveToTop(IList<QueueTask> moveItems);

        /// <summary>
        /// Remove a job from the Queue.
        /// This method is Thread Safe
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        void Remove(QueueTask job);

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
        void Start();

        /// <summary>
        /// Stop the current encode and pause the queue.
        /// </summary>
        /// <param name="stopExistingJobs">
        /// Set to false to allow existing jobs to complete.
        /// </param>
        void Stop(bool stopExistingJobs);

        /// <summary>
        /// Pause the queue but allow the current encode to complete.
        /// </summary>
        /// <param name="pauseJobs">
        /// Also pause the active jobs
        /// </param>
        void Pause(bool pauseJobs);

        /// <summary>
        /// Get the status of all running queue jobs.
        /// </summary>
        /// <returns>
        /// A list of QueueProgressStatus items
        /// </returns>
        List<QueueProgressStatus> GetQueueProgressStatus();

        List<string> GetActiveJobDestinationDirectories();
    }
}