// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using HandBrake.Services.Interfaces;
    using HandBrake.ViewModels.Interfaces;

    public class OptionsViewModel : OptionsViewModelBase
    {
        public OptionsViewModel(IUserSettingService userSettingService, IAboutViewModel aboutViewModel, IErrorService errorService)
            : base(userSettingService, aboutViewModel, errorService)
        {
        }
    }
}