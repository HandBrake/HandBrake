// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueItemStatus.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Queue Item Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.Model
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// Queue Item Status
    /// </summary>
    public enum QueueItemStatus
    {
        [DisplayName("Waiting")]
        Waiting = 0,

        [DisplayName("In Progress")]
        InProgress,

        [DisplayName("Completed")]
        Completed,

        [DisplayName("Error")]
        Error,

        [DisplayName("Paused")]
        Paused,

        [DisplayName("Cancelled")]
        Cancelled
    }
}
