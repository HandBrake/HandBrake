/*  OptionsViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Options View Model
    /// </summary>
    [Export(typeof(IOptionsViewModel))]
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public OptionsViewModel(IWindowManager windowManager) : base(windowManager)
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
