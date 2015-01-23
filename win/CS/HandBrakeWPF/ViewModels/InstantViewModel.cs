// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InstantViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The instant view model.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan.EventArgs;
    using HandBrake.ApplicationServices.Services.Scan.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using Microsoft.Win32;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    ///     The instant view model.
    /// </summary>
    public class InstantViewModel : ViewModelBase, IInstantViewModel
    {
        #region Constants and Fields

        /// <summary>
        ///     The encode service.
        /// </summary>
        private readonly IEncodeServiceWrapper encodeService;

        /// <summary>
        ///     The error service.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        ///     The preset service.
        /// </summary>
        private readonly IPresetService presetService;

        /// <summary>
        ///     The queue processor.
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        ///     The scan service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        ///     The shell view model.
        /// </summary>
        private readonly IShellViewModel shellViewModel;

        /// <summary>
        ///     The update service.
        /// </summary>
        private readonly IUpdateService updateService;

        /// <summary>
        ///     The user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        ///     Windows 7 API Pack wrapper
        /// </summary>
        private readonly Win7 windowsSeven = new Win7();

        /// <summary>
        ///     The is encoding.
        /// </summary>
        private bool isEncoding;

        /// <summary>
        ///     The last percentage complete value.
        /// </summary>
        private int lastEncodePercentage;

        /// <summary>
        ///     The ordered by duration.
        /// </summary>
        private bool orderedByDuration;

        /// <summary>
        ///     The ordered by title.
        /// </summary>
        private bool orderedByTitle;

        /// <summary>
        ///     The output directory.
        /// </summary>
        private string outputDirectory;

        /// <summary>
        ///     The program status label.
        /// </summary>
        private string programStatusLabel;

        /// <summary>
        ///     The scanned source.
        /// </summary>
        private Source scannedSource;

        /// <summary>
        ///     The selected preset.
        /// </summary>
        private Preset selectedPreset;

        /// <summary>
        ///     The show status window.
        /// </summary>
        private bool showStatusWindow;

        /// <summary>
        ///     The source label.
        /// </summary>
        private string sourceLabel;

        /// <summary>
        ///     The status label.
        /// </summary>
        private string statusLabel;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="InstantViewModel"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user setting service.
        /// </param>
        /// <param name="scanService">
        /// The scan service.
        /// </param>
        /// <param name="encodeService">
        /// The encode service.
        /// </param>
        /// <param name="presetService">
        /// The preset service.
        /// </param>
        /// <param name="errorService">
        /// The error service.
        /// </param>
        /// <param name="shellViewModel">
        /// The shell view model.
        /// </param>
        /// <param name="updateService">
        /// The update service.
        /// </param>
        /// <param name="notificationService">
        /// The notification service.
        /// </param>
        /// <param name="whenDoneService">
        /// The when done service.
        /// </param>
        public InstantViewModel(
            IUserSettingService userSettingService,
            IScan scanService, 
            IEncodeServiceWrapper encodeService, 
            IPresetService presetService, 
            IErrorService errorService, 
            IShellViewModel shellViewModel, 
            IUpdateService updateService, 
            INotificationService notificationService, 
            IPrePostActionService whenDoneService)
        {
            this.userSettingService = userSettingService;
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.shellViewModel = shellViewModel;
            this.updateService = updateService;

            this.queueProcessor = IoC.Get<IQueueProcessor>();

            // Setup Properties
            this.TitleList = new BindingList<SelectionTitle>();
            this.ScannedSource = new Source();

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;

            this.Presets = this.presetService.Presets;
            this.CancelScanCommand = new CancelScanCommand(this.scanService);
        }

        #endregion

        #region Properties

        /// <summary>
        ///     Gets or sets the cancel scan command.
        /// </summary>
        public CancelScanCommand CancelScanCommand { get; set; }

        /// <summary>
        ///     Gets or sets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding
        {
            get
            {
                return this.isEncoding;
            }

            set
            {
                this.isEncoding = value;
                this.NotifyOfPropertyChange(() => this.IsEncoding);
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating whether ordered by duration.
        /// </summary>
        public bool OrderedByDuration
        {
            get
            {
                return this.orderedByDuration;
            }

            set
            {
                this.orderedByDuration = value;
                this.NotifyOfPropertyChange(() => this.OrderedByDuration);
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating whether ordered by title.
        /// </summary>
        public bool OrderedByTitle
        {
            get
            {
                return this.orderedByTitle;
            }

            set
            {
                this.orderedByTitle = value;
                this.NotifyOfPropertyChange(() => this.OrderedByTitle);
            }
        }

        /// <summary>
        ///     Gets or sets the output directory.
        /// </summary>
        public string OutputDirectory
        {
            get
            {
                return this.outputDirectory;
            }
            set
            {
                this.outputDirectory = value;
                this.NotifyOfPropertyChange(() => this.OutputDirectory);
            }
        }

        /// <summary>
        ///     Gets or sets Presets.
        /// </summary>
        public IEnumerable<Preset> Presets { get; set; }

        /// <summary>
        ///     Gets or sets the Program Status Toolbar Label
        ///     This indicates the status of HandBrake
        /// </summary>
        public string ProgramStatusLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.programStatusLabel) ? "Ready" : this.programStatusLabel;
            }

            set
            {
                if (!Equals(this.programStatusLabel, value))
                {
                    this.programStatusLabel = value;
                    this.NotifyOfPropertyChange(() => this.ProgramStatusLabel);
                }
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating progress percentage.
        /// </summary>
        public int ProgressPercentage { get; set; }

        /// <summary>
        ///     Gets or sets the Last Scanned Source
        ///     This object contains information about the scanned source.
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
        ///     Gets or sets SelectedPreset.
        /// </summary>
        public Preset SelectedPreset
        {
            get
            {
                return this.selectedPreset;
            }
            set
            {
                this.selectedPreset = value;
                this.NotifyOfPropertyChange(() => this.SelectedPreset);
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating whether ShowStatusWindow.
        /// </summary>
        public bool ShowStatusWindow
        {
            get
            {
                return this.showStatusWindow;
            }

            set
            {
                this.showStatusWindow = value;
                this.NotifyOfPropertyChange(() => this.ShowStatusWindow);
            }
        }

        /// <summary>
        ///     Gets or sets the Source Label
        ///     This indicates the status of scans.
        /// </summary>
        public string SourceLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.sourceLabel) ? "Select 'Source' to continue" : this.sourceLabel;
            }

            set
            {
                if (!Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                    this.NotifyOfPropertyChange("SourceLabel");
                }
            }
        }

        /// <summary>
        ///     Gets SourceName.
        /// </summary>
        public string SourceName
        {
            get
            {
                // Sanity Check
                if (this.ScannedSource == null || this.ScannedSource.ScanPath == null)
                {
                    return string.Empty;
                }

                // The title that is selected has a source name. This means it's part of a batch scan.
                // if (selectedTitle != null && !string.IsNullOrEmpty(selectedTitle.SourceName))
                // {
                // return Path.GetFileNameWithoutExtension(selectedTitle.SourceName);
                // }

                // Check if we have a Folder, if so, check if it's a DVD / Bluray drive and get the label.
                if (this.ScannedSource.ScanPath.EndsWith("\\"))
                {
                    foreach (DriveInformation item in GeneralUtilities.GetDrives())
                    {
                        if (item.RootDirectory.Contains(this.ScannedSource.ScanPath))
                        {
                            return item.VolumeLabel;
                        }
                    }
                }

                if (Path.GetFileNameWithoutExtension(this.ScannedSource.ScanPath) != "VIDEO_TS")
                {
                    return Path.GetFileNameWithoutExtension(this.ScannedSource.ScanPath);
                }

                return Path.GetFileNameWithoutExtension(Path.GetDirectoryName(this.ScannedSource.ScanPath));
            }
        }

        /// <summary>
        ///     Gets or sets the Program Status Toolbar Label
        ///     This indicates the status of HandBrake
        /// </summary>
        public string StatusLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.statusLabel) ? "Ready" : this.statusLabel;
            }

            set
            {
                if (!Equals(this.statusLabel, value))
                {
                    this.statusLabel = value;
                    this.NotifyOfPropertyChange(() => this.StatusLabel);
                }
            }
        }

        /// <summary>
        ///     Gets or sets the selected titles.
        /// </summary>
        public BindingList<SelectionTitle> TitleList { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        ///     The Destination Path
        /// </summary>
        public void BrowseDestination()
        {
            var saveFileDialog = new SaveFileDialog
                                     {
                                         Filter = "mp4|*.mp4;*.m4v|mkv|*.mkv", 
                                         CheckPathExists = true, 
                                         AddExtension = true, 
                                         DefaultExt = ".mp4", 
                                         OverwritePrompt = true, 
                                     };

            saveFileDialog.ShowDialog();
            this.OutputDirectory = Path.GetDirectoryName(saveFileDialog.FileName);
        }

        /// <summary>
        ///     Cancel a Scan
        /// </summary>
        public void CancelScan()
        {
            this.scanService.Stop();
        }

        /// <summary>
        ///     File Scan
        /// </summary>
        public void FileScan()
        {
            var dialog = new OpenFileDialog() { Filter = "All files (*.*)|*.*" };
            dialog.ShowDialog();
            this.StartScan(dialog.FileName, 0);
        }

        /// <summary>
        /// Support dropping a file onto the main window to scan.
        /// </summary>
        /// <param name="e">
        /// The DragEventArgs.
        /// </param>
        public void FilesDroppedOnWindow(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
                if (fileNames != null && fileNames.Any() &&
                    (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])))
                {
                    this.StartScan(fileNames[0], 0);
                }
            }

            e.Handled = true;
        }

        /// <summary>
        ///     Folder Scan
        /// </summary>
        public void FolderScan()
        {
            var dialog = new VistaFolderBrowserDialog
                             {
                                 Description = "Please select a folder.", 
                                 UseDescriptionForTitle = true
                             };
            dialog.ShowDialog();
            this.StartScan(dialog.SelectedPath, 0);
        }

        /// <summary>
        ///     Launch the Help pages.
        /// </summary>
        public void LaunchHelp()
        {
            Process.Start("https://trac.handbrake.fr/wiki/HandBrakeGuide");
        }

        /// <summary>
        /// The on load.
        /// </summary>
        public override void OnLoad()
        {
            // Perform an update check if required
            // this.updateService.PerformStartupUpdateCheck(this.HandleUpdateCheckResults);

            // Setup the presets.
            this.presetService.Load();
            if (this.presetService.CheckIfPresetsAreOutOfDate())
            {
                if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
                {
                    this.errorService.ShowMessageBox(
                        "HandBrake has determined your built-in presets are out of date... These presets will now be updated." +
                        Environment.NewLine +
                        "Your custom presets have not been updated so you may have to re-create these by deleting and re-adding them.",
                        "Preset Update",
                        MessageBoxButton.OK,
                        MessageBoxImage.Information);
                }
            }

            this.SelectedPreset = this.presetService.DefaultPreset;

            // Log Cleaning
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs))
            {
                var clearLog = new Thread(() => GeneralUtilities.ClearLogFiles(30));
                clearLog.Start();
            }
            base.OnLoad();
        }

        /// <summary>
        ///     Open the About Window
        /// </summary>
        public void OpenAboutApplication()
        {
            var command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.About);
        }

        /// <summary>
        ///     Open the Log Window
        /// </summary>
        public void OpenLogWindow()
        {
            Window window =
                Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(LogView));

            if (window != null)
            {
                var logvm = (ILogViewModel)window.DataContext;
                logvm.SelectedTab = this.IsEncoding ? 0 : 1;
                window.Activate();
            }
            else
            {
                var logvm = IoC.Get<ILogViewModel>();
                logvm.SelectedTab = this.IsEncoding ? 0 : 1;
                this.WindowManager.ShowWindow(logvm);
            }
        }

        /// <summary>
        ///     Open the Options Window
        /// </summary>
        public void OpenOptionsWindow()
        {
            this.shellViewModel.DisplayWindow(ShellWindow.OptionsWindow);
        }

        /// <summary>
        ///     The order by duration.
        /// </summary>
        public void OrderByDuration()
        {
            this.TitleList =
                new BindingList<SelectionTitle>(this.TitleList.OrderByDescending(o => o.Title.Duration).ToList());
            this.NotifyOfPropertyChange(() => this.TitleList);
            this.OrderedByTitle = false;
            this.OrderedByDuration = true;
        }

        /// <summary>
        ///     The order by title.
        /// </summary>
        public void OrderByTitle()
        {
            this.TitleList = new BindingList<SelectionTitle>(this.TitleList.OrderBy(o => o.Title.TitleNumber).ToList());
            this.NotifyOfPropertyChange(() => this.TitleList);
            this.OrderedByTitle = true;
            this.OrderedByDuration = false;
        }

        /// <summary>
        ///     Pause an Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.Pause();
        }

        /// <summary>
        ///     The select all.
        /// </summary>
        public void SelectAll()
        {
            foreach (SelectionTitle item in this.TitleList)
            {
                item.IsSelected = true;
            }
        }

        /// <summary>
        /// The setup.
        /// </summary>
        /// <param name="scannedSource">
        /// The scanned source.
        /// </param>
        public void Setup(Source scannedSource)
        {
            this.TitleList.Clear();

            if (scannedSource != null)
            {
                IEnumerable<Title> titles = this.orderedByTitle
                                                ? scannedSource.Titles
                                                : scannedSource.Titles.OrderByDescending(o => o.Duration).ToList();

                foreach (Title item in titles)
                {
                    var title = new SelectionTitle(item, item.SourceName) { IsSelected = true };
                    this.TitleList.Add(title);
                }
            }
        }

        /// <summary>
        ///     Shutdown this View
        /// </summary>
        public void Shutdown()
        {
            // Shutdown Service
            this.encodeService.Shutdown();

            // Unsubscribe from Events.
            this.scanService.ScanStared -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueChanged;
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }

        /// <summary>
        ///     Start an Encode
        /// </summary>
        public void StartEncode()
        {
            // if (this.queueProcessor.IsProcessing)
            // {
            // this.errorService.ShowMessageBox("HandBrake is already encoding.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            // return;
            // }

            //// Check if we already have jobs, and if we do, just start the queue.
            // if (this.queueProcessor.Count != 0)
            // {
            // this.queueProcessor.Start();
            // return;
            // }

            //// Otherwise, perform Santiy Checking then add to the queue and start if everything is ok.
            // if (this.SelectedTitle == null)
            // {
            // this.errorService.ShowMessageBox("You must first scan a source.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            // return;
            // }

            // if (string.IsNullOrEmpty(this.Destination))
            // {
            // this.errorService.ShowMessageBox("The Destination field was empty.", Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            // return;
            // }

            // if (File.Exists(this.Destination))
            // {
            // MessageBoxResult result = this.errorService.ShowMessageBox("The current file already exists, do you wish to overwrite it?", "Question", MessageBoxButton.YesNo, MessageBoxImage.Question);
            // if (result == MessageBoxResult.No)
            // {
            // return;
            // }
            // }

            //// Create the Queue Task and Start Processing
            // QueueTask task = new QueueTask
            // {
            // Task = new EncodeTask(this.CurrentTask),
            // CustomQuery = false
            // };
            // this.queueProcessor.Add(task);
            // this.queueProcessor.Start();
            // this.IsEncoding = true;
        }

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
            if (!string.IsNullOrEmpty(filename))
            {
                this.scanService.Scan(
                    filename, 
                    title, 
                    null,
                    HBConfigurationFactory.Create());
            }
        }

        /// <summary>
        ///     Stop an Encode.
        /// </summary>
        public void StopEncode()
        {
            this.queueProcessor.Pause();
            this.encodeService.Stop();
        }

        /// <summary>
        ///     The select all.
        /// </summary>
        public void UnSelectAll()
        {
            foreach (SelectionTitle item in this.TitleList)
            {
                item.IsSelected = false;
            }
        }

        #endregion

        #region Methods

        /// <summary>
        /// The Encode Status has changed Handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The Encode Progress Event Args
        /// </param>
        private void EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            int percent;
            int.TryParse(Math.Round(e.PercentComplete).ToString(CultureInfo.InvariantCulture), out percent);

            Execute.OnUIThread(
                () =>
                    {
                        if (this.queueProcessor.EncodeService.IsEncoding)
                        {
                            string josPending = string.Empty;
                            if (!AppArguments.IsInstantHandBrake)
                            {
                                josPending = ",  Pending Jobs {5}";
                            }

                            this.ProgramStatusLabel =
                                string.Format(
                                    "{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Elapsed: {4:hh\\:mm\\:ss}" +
                                    josPending, 
                                    e.PercentComplete, 
                                    e.CurrentFrameRate, 
                                    e.AverageFrameRate, 
                                    e.EstimatedTimeLeft, 
                                    e.ElapsedTime, 
                                    this.queueProcessor.Count);

                            if (this.lastEncodePercentage != percent && this.windowsSeven.IsWindowsSeven)
                            {
                                this.windowsSeven.SetTaskBarProgress(percent);
                            }

                            this.lastEncodePercentage = percent;
                            this.ProgressPercentage = percent;
                            this.NotifyOfPropertyChange(() => this.ProgressPercentage);
                        }
                        else
                        {
                            this.ProgramStatusLabel = "Queue Finished";
                            this.IsEncoding = false;

                            if (this.windowsSeven.IsWindowsSeven)
                            {
                                this.windowsSeven.SetTaskBarProgressToNoProgress();
                            }
                        }
                    });
        }

        /// <summary>
        /// The queue changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueChanged(object sender, EventArgs e)
        {
            Execute.OnUIThread(
                () => { this.ProgramStatusLabel = string.Format("{0} Encodes Pending", this.queueProcessor.Count); });
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
            this.IsEncoding = false;

            Execute.OnUIThread(
                () =>
                    {
                        this.ProgramStatusLabel = "Queue Finished";
                        this.IsEncoding = false;

                        if (this.windowsSeven.IsWindowsSeven)
                        {
                            this.windowsSeven.SetTaskBarProgressToNoProgress();
                        }
                    });
        }

        /// <summary>
        /// Handle the Queue Starting Event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueProcessorJobProcessingStarted(object sender, QueueProgressEventArgs e)
        {
            Execute.OnUIThread(
                () =>
                    {
                        this.ProgramStatusLabel = "Preparing to encode ...";
                        this.IsEncoding = true;
                    });
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
        private void ScanCompleted(object sender, ScanCompletedEventArgs e)
        {
            this.scanService.SouceData.CopyTo(this.ScannedSource);
            this.NotifyOfPropertyChange(() => this.ScannedSource);

            Execute.OnUIThread(
                () =>
                    {
                        if (this.scannedSource != null)
                        {
                            this.Setup(this.scannedSource);
                        }

                        if (e.Successful)
                        {
                            this.NotifyOfPropertyChange(() => this.ScannedSource);
                            this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);
                        }

                        this.ShowStatusWindow = false;
                        if (e.Successful)
                        {
                            this.SourceLabel = this.SourceName;
                            this.StatusLabel = "Scan Completed";
                        }
                        else if (e.Cancelled)
                        {
                            this.SourceLabel = "Scan Cancelled.";
                            this.StatusLabel = "Scan Cancelled.";
                        }
                        else if (e.Exception == null && e.ErrorInformation != null)
                        {
                            this.SourceLabel = "Scan failed: " + e.ErrorInformation;
                            this.StatusLabel = "Scan failed: " + e.ErrorInformation;
                        }
                        else
                        {
                            this.SourceLabel = "Scan Failed... See Activity Log for details.";
                            this.StatusLabel = "Scan Failed... See Activity Log for details.";
                        }
                    });
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
            Execute.OnUIThread(
                () =>
                    {
                        this.StatusLabel = "Scanning source, please wait...";
                        this.ShowStatusWindow = true;
                    });
        }

        /// <summary>
        /// Handle the Scan Status Changed Event.
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanStatusChanged(object sender, ScanProgressEventArgs e)
        {
            this.SourceLabel = string.Format("Scanning Title {0} of {1} ({2}%)", e.CurrentTitle, e.Titles, e.Percentage);
            this.StatusLabel = string.Format("Scanning Title {0} of {1} ({2}%)", e.CurrentTitle, e.Titles, e.Percentage);
        }

        #endregion
    }
}