// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISubtitlesDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ISubtitlesDefaultsViewMode type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Subtiles View Model Interface
    /// </summary>
    public interface ISubtitlesDefaultsViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets the subtitle behaviours.
        /// </summary>
        SubtitleBehaviours SubtitleBehaviours { get; }

        bool IsApplied { get; }

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

        void ResetApplied();
    }
}
