// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IQueueProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Queue Processor
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Queue Processor
    /// </summary>
    public interface IQueueProcessor
    {
        #region Events

        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        event QueueProcessor.QueueProgressStatus JobProcessingStarted;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        event EventHandler QueueChanged;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        event EventHandler QueueCompleted;

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
        /// Gets the IEncodeService instance.
        /// </summary>
        IEncodeServiceWrapper EncodeService { get; }

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
        BindingList<QueueTask> Queue { get; }

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
        /// Requests a pause of the encode queue.
        /// </summary>
        void Pause();

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
        void Start();

        #endregion
    }
}