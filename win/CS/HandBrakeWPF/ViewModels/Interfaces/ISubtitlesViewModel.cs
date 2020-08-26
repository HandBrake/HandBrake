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
    /// The Subtitles View Model Interface
    /// </summary>
    public interface ISubtitlesViewModel : ITabInterface
    {
        /// <summary>
        /// Gets the subtitle behaviours.
        /// </summary>
        SubtitleBehaviours SubtitleBehaviours { get; }

        /// <summary>
        /// Import Subtitle files. Used for Drag/drop support which is handled by the MainViewModel.
        /// </summary>
        /// <param name="subtitleFiles">
        /// String array of files.
        /// </param>
        void Import(string[] subtitleFiles);

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        void RefreshTask();

        /// <summary>
        /// Checks the configuration of the subtitles and warns the user about any potential issues.
        /// </summary>
        bool ValidateSubtitles();
    }
}
