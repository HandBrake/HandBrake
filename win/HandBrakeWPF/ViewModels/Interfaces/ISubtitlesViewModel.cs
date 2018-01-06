// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISubtitlesViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ISubtitlesViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Subtitles;

    /// <summary>
    /// The Subtiles View Model Interface
    /// </summary>
    public interface ISubtitlesViewModel : ITabInterface
    {
        /// <summary>
        /// Gets the subtitle behaviours.
        /// </summary>
        SubtitleBehaviours SubtitleBehaviours { get; }
    }
}
