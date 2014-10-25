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
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Shell View Model
    /// </summary>
    public class ShellViewModel : ViewModelBase, IShellViewModel
    {
        /// <summary>
        /// Backing field for the error service.
        /// </summary>
        private readonly IErrorService errorService;

        #region Constants and Fields

        /// <summary>
        /// The show main window.
        /// </summary>
        private bool showMainWindow;

        /// <summary>
        /// The show options.
        /// </summary>
        private bool showOptions;

        /// <summary>
        /// The show instant.
        /// </summary>
        private bool showInstant;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellViewModel"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        public ShellViewModel(IErrorService errorService)
        {
            this.errorService = errorService;

            if (!AppArguments.IsInstantHandBrake)
            {
                this.showMainWindow = true;
                this.showOptions = false;
                this.showInstant = false;
            }
            else
            {
                this.showMainWindow = false;
                this.showOptions = false;
                this.showInstant = true;
            }
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
                this.ShowInstant = false;
            }
            else if (window == ShellWindow.OptionsWindow)
            {
                this.ShowOptions = true;
                this.ShowMainWindow = false;
                this.ShowInstant = false;
            }
            else if (window == ShellWindow.InstantMainWindow)
            {
                this.ShowInstant = true;
                this.ShowOptions = false;
                this.ShowMainWindow = false;
            }
            else
            {
                this.ShowMainWindow = true;
                this.ShowOptions = false;
                this.ShowInstant = false;
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
        /// Gets or sets the instant view model.
        /// </summary>
        public IInstantViewModel InstantViewModel { get; set; }

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
        /// Gets or sets a value indicating whether ShowInstant.
        /// </summary>
        public bool ShowInstant
        {
            get
            {
                return this.showInstant;
            }
            set
            {
                this.showInstant = value;
                this.NotifyOfPropertyChange(() => this.ShowInstant);
            }
        }

        /// <summary>
        /// Gets WindowTitle.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return AppArguments.IsInstantHandBrake ? "Instant HandBrake" : "HandBrake";
            }
        }

        #endregion

        /// <summary>
        /// Checks with the use if this window can be closed.
        /// </summary>
        /// <returns>
        /// Returns true if the window can be closed.
        /// </returns>
        public bool CanClose()
        {
            IQueueProcessor processor = IoC.Get<IQueueProcessor>();
            if (processor != null && processor.EncodeService.IsEncoding)
            {
                MessageBoxResult result =
                    errorService.ShowMessageBox(
                        "An Encode is currently running. Exiting HandBrake will stop this encode.\nAre you sure you wish to exit HandBrake?",
                        Resources.Warning,
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Warning);

                if (result == MessageBoxResult.Yes)
                {
                    processor.Pause();
                    processor.EncodeService.Stop();
                    if (this.MainViewModel != null)
                    {
                        this.MainViewModel.Shutdown();
                    }

                    return true;
                }
                return false;
            }

            if (this.MainViewModel != null)
            {
                this.MainViewModel.Shutdown();
            }
            return true;
        }
    }
}