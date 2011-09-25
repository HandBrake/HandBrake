/*  IQueueProcessor.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;

    /// <summary>
    /// The Queue Processor
    /// </summary>
    public interface IQueueProcessor
    {
        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        event QueueProcessor.QueueProgressStatus JobProcessingStarted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        event EventHandler QueuePaused;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        event EventHandler QueueCompleted;

        /// <summary>
        /// Gets the IEncodeService instance.
        /// </summary>
        IEncode EncodeService { get; }

        /// <summary>
        /// Gets the IQueueManager instance.
        /// </summary>
        IQueueManager QueueManager { get; }

        /// <summary>
        /// Gets a value indicating whether IsProcessing.
        /// </summary>
        bool IsProcessing { get; }

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