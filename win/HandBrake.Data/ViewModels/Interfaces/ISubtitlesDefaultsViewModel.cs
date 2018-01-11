// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISubtitlesDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ISubtitlesDefaultsViewMode type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels.Interfaces
{
    using HandBrake.Model.Subtitles;
    using HandBrake.Services.Presets.Model;

    /// <summary>
    /// The Subtiles View Model Interface
    /// </summary>
    public interface ISubtitlesDefaultsViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets the subtitle behaviours.
        /// </summary>
        SubtitleBehaviours SubtitleBehaviours { get; }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        void SetupLanguages(Preset preset);

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        void SetupLanguages(SubtitleBehaviours behaviours);
    }
}
