// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Queue Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    using System;

    using HandBrake.ApplicationServices.Model;

    using HandBrakeWPF.Services.Queue.Model;

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
        public QueueProgressEventArgs(QueueTask newJob)
        {
            this.NewJob = newJob;
        }

        /// <summary>
        /// Gets or sets the new job which is about to be processed.
        /// </summary>
        public QueueTask NewJob { get; set; }
    }
}
