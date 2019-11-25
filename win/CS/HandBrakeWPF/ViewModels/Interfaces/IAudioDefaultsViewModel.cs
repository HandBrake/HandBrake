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
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Audio View Model Interface
    /// </summary>
    public interface IAudioDefaultsViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets the audio behaviours.
        /// </summary>
        AudioBehaviours AudioBehaviours { get; }

        bool IsApplied { get; }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void Setup(Preset preset, EncodeTask task);

        /// <summary>
        /// The refresh task.
        /// </summary>
        void RefreshTask();

        void ResetApplied();
    }
}
