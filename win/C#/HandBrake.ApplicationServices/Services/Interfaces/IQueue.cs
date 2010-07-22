/*  IQueue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;
    using System.Collections.ObjectModel;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The IQueue Interface
    /// </summary>
    public interface IQueue : IEncode
    {
        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        event EventHandler QueueStarted;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        event EventHandler QueueListChanged;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        event EventHandler QueuePauseRequested;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        event EventHandler QueueCompleted;

        /// <summary>
        /// Gets or sets the last encode that was processed.
        /// </summary>
        /// <returns></returns> 
        Job LastEncode { get; set; }

        /// <summary>
        /// Gets a value indicating whether Request Pause
        /// </summary>
        bool Paused { get; }

        /// <summary>
        /// Gets the current state of the encode queue.
        /// </summary>
        ReadOnlyCollection<Job> CurrentQueue { get; }

        /// <summary>
        /// Gets the number of items in the queue.
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        bool IsEncoding { get; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// Adds an item to the queue.
        /// </summary>
        /// <param name="query">
        /// The query that will be passed to the HandBrake CLI.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="source">
        /// The location of the source video.
        /// </param>
        /// <param name="destination">
        /// The location where the encoded video will be.
        /// </param>
        /// <param name="customJob">
        /// Custom job
        /// </param>
        void Add(string query, int title, string source, string destination, bool customJob);

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        void Remove(int index);

        /// <summary>
        /// Retrieve a job from the queue
        /// </summary>
        /// <param name="index">the job id</param>
        /// <returns>A job for the given index or blank job object</returns>
        Job GetJob(int index);

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
        /// Writes the current state of the queue to a file.
        /// </summary>
        /// <param name="file">The location of the file to write the queue to.</param>
        void WriteQueueStateToFile(string file);

        /// <summary>
        /// Writes the current state of the queue in the form of a batch (.bat) file.
        /// </summary>
        /// <param name="file">The location of the file to write the batch file to.</param>
        bool WriteBatchScriptToFile(string file);

        /// <summary>
        /// Reads a serialized XML file that represents a queue of encoding jobs.
        /// </summary>
        /// <param name="file">The location of the file to read the queue from.</param>
        void LoadQueueFromFile(string file);

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">The destination of the encode.</param>
        /// <returns>Whether or not the supplied destination is already in the queue.</returns>
        bool CheckForDestinationDuplicate(string destination);

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        void Start();

        /// <summary>
        /// Requests a pause of the encode queue.
        /// </summary>
        void Pause();
    }
}