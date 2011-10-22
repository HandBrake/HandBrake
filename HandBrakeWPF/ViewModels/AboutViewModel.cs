/*  AboutViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The About View Model
    /// </summary>
    [Export(typeof(IAboutViewModel))]
    public class AboutViewModel : ViewModelBase, IAboutViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AboutViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public AboutViewModel(IWindowManager windowManager) : base(windowManager)
        {
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {       
        }
    }
}
