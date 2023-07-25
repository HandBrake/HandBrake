// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAddPresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;

    using EncodeTask = Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public interface IAddPresetViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets the name of the newly created preset.
        /// </summary>
        string PresetName { get; }

        /// <summary>
        /// Prepare the Preset window to create a Preset Object later.
        /// </summary>
        /// <param name="task">
        /// The Encode Task.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="audioBehaviours">
        /// The audio Behaviours.
        /// </param>
        /// <param name="subtitleBehaviours">
        /// The subtitle Behaviours.
        /// </param>
        /// <param name="presetName">
        /// Optional initial preset name.
        /// </param>
        void Setup(EncodeTask task, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours, string presetName);
    }
}
