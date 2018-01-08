// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeUWP.ViewModels
{
    using HandBrake.Services.Interfaces;
    using HandBrake.ViewModels;
    using HandBrake.ViewModels.Interfaces;

    public class ShellViewModel : ShellViewModelBase
    {
        public ShellViewModel(IErrorService errorService, IMainViewModel mainViewModel, IOptionsViewModel optionsViewModel)
            : base(errorService, mainViewModel, optionsViewModel)
        {
        }
    }
}