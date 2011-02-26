/*  QueueViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using Microsoft.Practices.ServiceLocation;

    /// <summary>
    /// The Preview View Model
    /// </summary>
    public class QueueViewModel : ViewModelBase
    {
        public QueueViewModel(IServiceLocator locator)
            : base(locator)
        {
        }
    }
}
