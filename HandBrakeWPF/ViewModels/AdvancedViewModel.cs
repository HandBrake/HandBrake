// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Advanced View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Advanced View Model
    /// </summary>
    [Export(typeof(IAdvancedViewModel))]
    public class AdvancedViewModel : ViewModelBase, IAdvancedViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AdvancedViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public AdvancedViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
        }

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public string Query { get; set; }
    }
}
