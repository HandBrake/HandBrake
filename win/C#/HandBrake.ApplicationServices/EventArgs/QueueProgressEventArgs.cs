/*  QueueProgressEventArgs.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.EventArgs
{
    using System;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// Queue Progress Event Args
    /// </summary>
    public class QueueProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="QueueProgressEventArgs"/> class.
        /// </summary>
        /// <param name="newJob">
        /// The new job.
        /// </param>
        public QueueProgressEventArgs(Job newJob)
        {
            this.NewJob = newJob;
        }

        /// <summary>
        /// Gets or sets the new job which is about to be processed.
        /// </summary>
        public Job NewJob { get; set; }
    }
}
