/*  AboutViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using Microsoft.Practices.ServiceLocation;

    /// <summary>
    /// The About View Model
    /// </summary>
    public class AboutViewModel : ViewModelBase
    {
        public AboutViewModel(IServiceLocator locator)
            : base(locator)
        {
        }

    }
}
