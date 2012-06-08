// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Shell View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Shell View Model
    /// </summary>
    [Export(typeof(IShellViewModel))]
    public class ShellViewModel : ViewModelBase, IShellViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The show main window.
        /// </summary>
        private bool showMainWindow;

        /// <summary>
        /// The show options.
        /// </summary>
        private bool showOptions;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellViewModel"/> class.
        /// </summary>
        public ShellViewModel()
        {
            this.showMainWindow = true;
            this.showOptions = false;
        }

        /// <summary>
        /// Change the page displayed on this window.
        /// </summary>
        /// <param name="window">
        /// The window.
        /// </param>
        public void DisplayWindow(ShellWindow window)
        {
            if (window == ShellWindow.MainWindow)
            {
                this.ShowMainWindow = true;
                this.ShowOptions = false;
            }
            else if (window == ShellWindow.OptionsWindow)
            {
                this.ShowOptions = true;
                this.ShowMainWindow = false;
            } 
            else
            {
                this.ShowMainWindow = true;
                this.ShowOptions = false;
            }
        }

        #region Properties

        /// <summary>
        /// Gets or sets MainViewModel.
        /// </summary>
        public IMainViewModel MainViewModel { get; set; }

        /// <summary>
        /// Gets or sets OptionsViewModel.
        /// </summary>
        public IOptionsViewModel OptionsViewModel { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowMainWindow.
        /// </summary>
        public bool ShowMainWindow
        {
            get
            {
                return this.showMainWindow;
            }
            set
            {
                this.showMainWindow = value;
                this.NotifyOfPropertyChange(() => this.ShowMainWindow);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowOptions.
        /// </summary>
        public bool ShowOptions
        {
            get
            {
                return this.showOptions;
            }
            set
            {
                this.showOptions = value;
                this.NotifyOfPropertyChange(() => this.ShowOptions);
            }
        }

        /// <summary>
        /// Gets WindowTitle.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return "HandBrake";
            }
        }

        #endregion
    }
}