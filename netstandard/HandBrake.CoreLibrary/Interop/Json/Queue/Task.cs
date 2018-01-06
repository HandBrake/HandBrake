// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Task.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The task.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.CoreLibrary.Interop.Json.Queue
{
    using HandBrake.CoreLibrary.Interop.Json.Encode;

    /// <summary>
    /// The task.
    /// </summary>
    public class Task
    {
        /// <summary>
        /// Gets or sets the job.
        /// </summary>
        public JsonEncodeObject Job { get; set; }
    }
}
