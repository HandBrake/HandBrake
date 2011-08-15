/*  IQueueManager.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;
    using System.Collections.ObjectModel;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Queue Manager Interface
    /// </summary>
    public interface IQueueManager
    {
        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        event EventHandler QueueChanged;

        /// <summary>
        /// Gets or sets Last Processed Job.
        /// This is set when the job is poped of the queue by GetNextJobForProcessing();
        /// </summary>
        QueueTask LastProcessedJob { get; set; }

        /// <summary>
        /// Gets The current queue.
        /// </summary>
        ReadOnlyCollection<QueueTask> Queue { get; }

        /// <summary>
        /// Gets the number of jobs in the queue
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Add a job to the Queue. 
        /// This method is Thread Safe.
        /// </summary>
        /// <param name="job">
        /// The encode Job object.
        /// </param>
        void Add(QueueTask job);

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
        /// Clear down the Queue´s completed items
        /// </summary>
        void ClearCompleted();

        /// <summary>
        /// Clear down all Queue Items
        /// </summary>
        void Clear();

        /// <summary>
        /// Get the first job on the queue for processing.
        /// This also removes the job from the Queue and sets the LastProcessedJob
        /// </summary>
        /// <returns>
        /// An encode Job object.
        /// </returns>
        QueueTask GetNextJobForProcessing();

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        void MoveUp(int index);

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        void MoveDown(int index);

        /// <summary>
        /// Backup any changes to the queue file
        /// </summary>
        /// <param name="exportPath">
        /// If this is not null or empty, this will be used instead of the standard backup location.
        /// </param>
        void BackupQueue(string exportPath);

        /// <summary>
        /// Restore a Queue from file or from the queue backup file.
        /// </summary>
        /// <param name="importPath">
        /// The import path. String.Empty or null will result in the default file being loaded.
        /// </param>
        void RestoreQueue(string importPath);

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">The destination of the encode.</param>
        /// <returns>Whether or not the supplied destination is already in the queue.</returns>
        bool CheckForDestinationPathDuplicates(string destination);

        /// <summary>
        /// Create a batch script from the queue
        /// </summary>
        /// <param name="path">
        /// The path to the location for the script to be saved.
        /// </param>
        /// <returns>
        /// True if sucessful
        /// </returns>
        bool WriteBatchScriptToFile(string path);
    }
}