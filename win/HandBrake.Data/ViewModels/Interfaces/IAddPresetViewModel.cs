// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAddPresetViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Add Preset View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels.Interfaces
{
    using HandBrake.Model.Audio;
    using HandBrake.Model.Subtitles;
    using HandBrake.Services.Scan.Model;

    using EncodeTask = HandBrake.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public interface IAddPresetViewModel
    {
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
        void Setup(EncodeTask task, Title title, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours);
    }
}
