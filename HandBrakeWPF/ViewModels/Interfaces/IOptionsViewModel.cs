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
        /// The goto tab.
        /// </summary>
        /// <param name="tab">
        /// The tab.
        /// </param>
        void GotoTab(OptionsTab tab);
    }
}