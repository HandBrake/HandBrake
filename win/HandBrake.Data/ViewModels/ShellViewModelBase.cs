// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellViewModelBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Shell View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using Caliburn.Micro;
    using HandBrake.Model.Prompts;
    using HandBrake.Properties;
    using HandBrake.Model;
    using HandBrake.Services.Interfaces;
    using HandBrake.ViewModels.Interfaces;

    using IQueueProcessor = HandBrake.Services.Queue.Interfaces.IQueueProcessor;

    /// <summary>
    /// The Shell View Model
    /// </summary>
    public abstract class ShellViewModelBase : ViewModelBase, IShellViewModel
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

        #endregion Constants and Fields

        /// <summary>
        /// Initializes a new instance of the <see cref="ShellViewModelBase"/> class.
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
        public ShellViewModelBase(IErrorService errorService, IMainViewModel mainViewModel, IOptionsViewModel optionsViewModel)
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

        #endregion Properties

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
                var result =
                    this.errorService.ShowMessageBox(
                        Resources.ShellViewModel_CanClose,
                        Resources.Warning,
                        DialogButtonType.YesNo,
                        DialogType.Warning);

                if (result == DialogResult.Yes)
                {
                    processor.Stop();
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