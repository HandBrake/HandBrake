// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAudioDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
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
    public interface IAudioDefaultsViewModel : IOverlayPanel
    {
        /// <summary>
        /// Gets the audio behaviours.
        /// </summary>
        AudioBehaviours AudioBehaviours { get; }
    }
}
