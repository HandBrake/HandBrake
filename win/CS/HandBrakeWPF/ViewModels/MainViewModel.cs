// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Main Window
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.Diagnostics;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    [Export(typeof(IMainViewModel))]
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

        /// <summary>
        /// The Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Source Scan Service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IEncode encodeService;

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// The preset service
        /// </summary>
        private readonly IPresetService presetService;

        /// <summary>
        /// HandBrakes Main Window Title
        /// </summary>
        private string windowName;

        /// <summary>
        /// The Source Label
        /// </summary>
        private string sourceLabel;

        /// <summary>
        /// The Toolbar Status Label
        /// </summary>
        private string programStatusLabel;

        /// <summary>
        /// Backing field for the scanned source.
        /// </summary>
        private Source scannedSource;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class.
        /// The viewmodel for HandBrakes main window.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The User Setting Service
        /// </param>
        /// <param name="scanService">
        /// The scan Service.
        /// </param>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="presetService">
        /// The preset Service.
        /// </param>
        [ImportingConstructor]
        public MainViewModel(IWindowManager windowManager, IUserSettingService userSettingService, IScan scanService, IEncode encodeService, IPresetService presetService)
            : base(windowManager)
        {
            this.userSettingService = userSettingService;
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.queueProcessor = new QueueProcessor(Process.GetProcessesByName("HandBrake").Length);

            // Setup Properties
            this.WindowTitle = "HandBrake WPF Test Application";
            this.CurrentTask = new EncodeTask();
            this.ScannedSource = new Source();

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueuePaused += this.QueuePaused;
            this.queueProcessor.EncodeService.EncodeStarted += this.EncodeStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
        }

        #region Properties
        /// <summary>
        /// Gets or sets TestProperty.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return this.windowName;
            }

            set
            {
                if (!object.Equals(this.windowName, value))
                {
                    this.windowName = value;
                }
            }
        }

        /// <summary>
        /// Gets a list of presets
        /// </summary>
        public ObservableCollection<Preset> Presets
        {
            get
            {
                return this.presetService.Presets;
            }
        }

        /// <summary>
        /// Gets or sets The Current Encode Task that the user is building
        /// </summary>
        public EncodeTask CurrentTask { get; set; }

        /// <summary>
        /// Gets or sets the Last Scanned Source
        /// This object contains information about the scanned source.
        /// </summary>
        public Source ScannedSource
        {
            get
            {
                return this.scannedSource;
            }
            set
            {
                this.scannedSource = value;
                this.NotifyOfPropertyChange("ScannedSource");
            }
        }

        /// <summary>
        /// Gets or sets the Source Label
        /// This indicates the status of scans.
        /// </summary>
        public string SourceLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.sourceLabel) ? "Select 'Source' to continue" : this.sourceLabel;
            }

            set
            {
                if (!object.Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                    this.NotifyOfPropertyChange("SourceLabel");
                }
            }
        }

        /// <summary>
        /// Gets or sets the Program Status Toolbar Label
        /// This indicates the status of HandBrake
        /// </summary>
        public string ProgramStatusLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.programStatusLabel) ? "Ready" : this.sourceLabel;
            }

            set
            {
                if (!object.Equals(this.programStatusLabel, value))
                {
                    this.programStatusLabel = value;
                    this.NotifyOfPropertyChange("ProgramStatusLabel");
                }
            }
        }

        #endregion

        #region Load and Shutdown Handling
        /// <summary>
        /// Initialise this view model.
        /// </summary>
        public override void OnLoad()
        {
            // TODO
        }

        /// <summary>
        /// Shutdown this View
        /// </summary>
        public void Shutdown()
        {
            // Unsubscribe from Events.
            this.scanService.ScanStared -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueuePaused -= this.QueuePaused;
            this.queueProcessor.EncodeService.EncodeStarted -= this.EncodeStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }
        #endregion

        #region Menu and Taskbar

        /// <summary>
        /// Open the About Window
        /// </summary>
        public void OpenAboutApplication()
        {
            this.WindowManager.ShowWindow(new AboutViewModel(this.WindowManager, this.userSettingService));
        }

        /// <summary>
        /// Open the Options Window
        /// </summary>
        public void OpenOptionsWindow()
        {
            this.WindowManager.ShowWindow(new OptionsViewModel(this.WindowManager, this.userSettingService));
        }

        /// <summary>
        /// Open the Log Window
        /// </summary>
        public void OpenLogWindow()
        {
            this.WindowManager.ShowWindow(new LogViewModel(this.WindowManager));
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenQueueWindow()
        {
            this.WindowManager.ShowWindow(new QueueViewModel(this.WindowManager));
        }

        /// <summary>
        /// Launch the Help pages.
        /// </summary>
        public void LaunchHelp()
        {
            Process.Start("https://trac.handbrake.fr/wiki/HandBrakeGuide");
        }

        /// <summary>
        /// Check for Updates.
        /// </summary>
        public void CheckForUpdates()
        {
            throw new NotImplementedException("Not Yet Implemented");
        }

        /// <summary>
        /// Folder Scan
        /// </summary>
        public void FolderScan()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true };
            dialog.ShowDialog();
            this.StartScan(dialog.SelectedPath, 0);
        }

        /// <summary>
        /// File Scan
        /// </summary>
        public void FileScan()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "All files (*.*)|*.*" };
            dialog.ShowDialog();
            this.StartScan(dialog.FileName, 0);
        }

        /// <summary>
        /// Cancel a Scan
        /// </summary>
        public void CancelScan()
        {
            this.scanService.Stop();
        }

        /// <summary>
        /// Start an Encode
        /// </summary>
        public void StartEncode()
        {
            throw new NotImplementedException("Not Yet Implemented");
        }

        /// <summary>
        /// Pause an Encode
        /// </summary>
        public void PauseEncode()
        {
            throw new NotImplementedException("Not Yet Implemented");
        }

        /// <summary>
        /// Stop an Encode.
        /// </summary>
        public void StopEncode()
        {
            throw new NotImplementedException("Not Yet Implemented");
        }

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        public void ExitApplication()
        {
            Application.Current.Shutdown();
        }

        #endregion

        #region Private Worker Methods

        /// <summary>
        /// Start a Scan
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        public void StartScan(string filename, int title)
        {
            // TODO 
            // 1. Disable GUI.
            this.scanService.Scan(filename, title, this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount));
        }

        #endregion


        #region Event Handlers
        /// <summary>
        /// Handle the Scan Status Changed Event.
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.ScanProgressEventArgs e)
        {
            this.SourceLabel = "Scanning Title " + e.CurrentTitle + " of " + e.Titles;
        }

        /// <summary>
        /// Handle the Scan Completed Event
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanCompleted(object sender, HandBrake.ApplicationServices.EventArgs.ScanCompletedEventArgs e)
        {
            if (e.Successful)
            {
               this.scanService.SouceData.CopyTo(this.ScannedSource);
               this.NotifyOfPropertyChange("ScannedSource");
               this.NotifyOfPropertyChange("ScannedSource.Titles");
            }

            this.SourceLabel = "Scan Completed";
           
            // TODO Re-enable GUI.
        }

        /// <summary>
        /// Handle the Scan Started Event
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanStared(object sender, EventArgs e)
        {
            // TODO - Disable relevant parts of the UI.
        }

        /// <summary>
        /// The Encode Status has changed Handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The Encode Progress Event Args
        /// </param>
        private void EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            //
        }

        /// <summary>
        /// Encode Started Handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void EncodeStarted(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }

        /// <summary>
        /// The Queue has been paused handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void QueuePaused(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }

        /// <summary>
        /// The Queue has completed handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void QueueCompleted(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }
        #endregion
    }
}