// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Queue Completed Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Queue Completed Event Args
    /// </summary>
    [DataContract]
    public class QueueCompletedEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="QueueCompletedEventArgs"/> class. 
        /// </summary>
        /// <param name="wasManuallyStopped">
        /// The was Manually Stopped.
        /// </param>
        public QueueCompletedEventArgs(bool wasManuallyStopped)
        {
            this.WasManuallyStopped = wasManuallyStopped;
        }

        /// <summary>
        /// Gets a value indicating whether the queue WasManuallyStopped.
        /// </summary>
        [DataMember]
        public bool WasManuallyStopped { get; private set; }
    }
}
