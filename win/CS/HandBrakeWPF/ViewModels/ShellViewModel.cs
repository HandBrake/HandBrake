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

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using IQueueService = HandBrakeWPF.Services.Queue.Interfaces.IQueueService;

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

        private bool isMainPanelEnabled;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellViewModel"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        /// <param name="mainViewModel">
        /// The main View Model.
        /// </param>
        /// <param name="optionsViewModel">
        /// The options View Model.
        /// </param>
        public ShellViewModel(IErrorService errorService, IMainViewModel mainViewModel, IOptionsViewModel optionsViewModel)
        {
            this.errorService = errorService;
            this.showMainWindow = true;
            this.showOptions = false;
            this.IsMainPanelEnabled = true;
            this.MainViewModel = mainViewModel;
            this.OptionsViewModel = optionsViewModel;
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
        /// Gets or sets a value indicating whether is main panel enabled.
        /// </summary>
        public bool IsMainPanelEnabled
        {
            get
            {
                return this.isMainPanelEnabled;
            }
            set
            {
                if (value.Equals(this.isMainPanelEnabled))
                {
                    return;
                }
                this.isMainPanelEnabled = value;
                this.NotifyOfPropertyChange(() => this.IsMainPanelEnabled);
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

        /// <summary>
        /// The files dropped on window. Pass this through to the active view model.
        /// </summary>
        /// <param name="e">
        /// The DragEventArgs.
        /// </param>
        public void FilesDroppedOnWindow(DragEventArgs e)
        {
            this.MainViewModel.FilesDroppedOnWindow(e);
        }

        /// <summary>
        /// Checks with the use if this window can be closed.
        /// </summary>
        /// <returns>
        /// Returns true if the window can be closed.
        /// </returns>
        public bool CanClose()
        {
            IQueueService processor = IoC.Get<IQueueService>();
            if (processor != null && processor.EncodeService.IsEncoding)
            {
                MessageBoxResult result =
                    this.errorService.ShowMessageBox(
                        Resources.ShellViewModel_CanClose,
                        Resources.Warning,
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Warning);

                if (result == MessageBoxResult.Yes)
                {
                    processor.Stop();
                    this.MainViewModel?.Shutdown();

                    return true;
                }
                return false;
            }

            this.OptionsViewModel?.Close();

            this.MainViewModel?.Shutdown();

            return true;
        }
    }
}