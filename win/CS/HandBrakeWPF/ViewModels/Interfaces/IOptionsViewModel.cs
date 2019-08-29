// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IOptionsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Options Screen View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model;

    /// <summary>
    /// The Options Screen View Model Interface
    /// </summary>
    public interface IOptionsViewModel
    {
        /// <summary>
        /// The currently selected Tab in the options pane.
        /// </summary>
        OptionsTab SelectedTab { get; }

        /// <summary>
        /// The goto tab.
        /// </summary>
        /// <param name="tab">
        /// The tab.
        /// </param>
        void GotoTab(OptionsTab tab);

        /// <summary>
        /// Refresh certain UI controls that can be updated outside preferences.
        /// </summary>
        void UpdateSettings();

        void Close();

        void PerformUpdateCheck();
    }
}