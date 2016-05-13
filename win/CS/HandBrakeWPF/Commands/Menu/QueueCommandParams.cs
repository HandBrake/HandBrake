// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueCommandParams.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueCommandParams type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Menu
{
    /// <summary>
    /// The queue command params.
    /// </summary>
    public enum QueueCommandParams
    {
        ClearCompleted,
        ClearAll,
        ClearSelected,
        Import,
        Export
    }
}
