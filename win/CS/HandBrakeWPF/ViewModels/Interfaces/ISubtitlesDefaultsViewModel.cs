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
    /// The Subtitles View Model Interface
    /// </summary>
    public interface ISubtitlesDefaultsViewModel : IViewModelBase
    {
        SubtitleBehaviours SubtitleBehaviours { get; }

        bool IsApplied { get; }

        void SetupPreset(Preset preset);

        void SetupPreset(SubtitleBehaviours behaviour);

        bool ShowWindow();
    }
}
