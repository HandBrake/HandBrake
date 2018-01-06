// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTaskContainer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The queue task container.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Model
{
    /// <summary>
    /// The queue task container.
    /// </summary>
    public class QueueTaskContainer
    {
          /// <summary>
          /// Initializes a new instance of the <see cref="QueueTaskContainer"/> class.
          /// </summary>
          /// <param name="version">
          /// The version.
          /// </param>
          /// <param name="queuetask">
          /// The queuetask.
          /// </param>
        public QueueTaskContainer(int version, string queuetask)
        {
            Version = version;
            QueueTask = queuetask;
        }

        /// <summary>
        /// Gets or sets the version of the presets stored in this container.
        /// </summary>
        public int Version { get; set; }

        /// <summary>
        /// Gets or sets the presets. This is a serialised string.
        /// </summary>
        public string QueueTask { get; set;  }
    }
}
