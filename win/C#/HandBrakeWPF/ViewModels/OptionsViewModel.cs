/*  OptionsViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using Microsoft.Practices.ServiceLocation;

    /// <summary>
    /// The Options View Model
    /// </summary>
    public class OptionsViewModel : ViewModelBase
    {
        public OptionsViewModel(IServiceLocator locator)
            : base(locator)
        {
        }
    }
}
