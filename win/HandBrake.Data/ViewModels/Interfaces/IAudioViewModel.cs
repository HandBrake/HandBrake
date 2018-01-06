// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAudioViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IAudioViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Audio;

    /// <summary>
    /// The Audio View Model Interface
    /// </summary>
    public interface IAudioViewModel : ITabInterface
    {
        /// <summary>
        /// Gets the audio behaviours.
        /// </summary>
        AudioBehaviours AudioBehaviours { get; }

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        void RefreshTask();
    }
}
