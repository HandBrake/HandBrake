// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainViewModel.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Main Window
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
    using System.Text.Json;
    using System.Threading;
    using System.Windows;
    using System.Windows.Input;

    using HandBrake.App.Core.Model;
    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Commands.DebugTools;
    using HandBrakeWPF.Commands.Menu;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Startup;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using Ookii.Dialogs.Wpf;

    using Action = System.Action;
    using Application = System.Windows.Application;
    using DataFormats = System.Windows.DataFormats;
    using DragEventArgs = System.Windows.DragEventArgs;
    using ILog = Services.Logging.Interfaces.ILog;
    using OpenFileDialog = Microsoft.Win32.OpenFileDialog;
    using SaveFileDialog = Microsoft.Win32.SaveFileDialog;

    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        private readonly IQueueService queueProcessor;
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IUpdateService updateService;
        private readonly IWindowManager windowManager;
        private readonly INotifyIconService notifyIconService;
        private readonly ILog logService;
        private readonly INotificationService notificationService;
        private readonly IUserSettingService userSettingService;
        private readonly IScan scanService;
        private readonly WindowsTaskbar windowsTaskbar = new WindowsTaskbar();
        private readonly DelayedActionProcessor delayedPreviewprocessor = new DelayedActionProcessor();

        private string windowName;
        private string sourceLabel;
        private string statusLabel;
        private string programStatusLabel;
        private Source scannedSource;
        private Title selectedTitle;
        private string duration;
        private bool showStatusWindow;
        private Preset selectedPreset;
        private QueueTask queueEditTask;
        private int lastEncodePercentage;
        private bool showSourceSelection;
        private BindingList<SourceMenuItem> drives;
        private bool showAlertWindow;
        private string alertWindowHeader;
        private string alertWindowText;
        private bool hasSource;
        private bool isSettingPreset;
        private bool isModifiedPreset;
        private bool updateAvailable;
        private bool isNavigationEnabled;

        public MainViewModel(
            IUserSettingService userSettingService,
            IScan scanService,
            IPresetService presetService,
            IErrorService errorService,
            IUpdateService updateService,
            IPrePostActionService whenDoneService,
            IWindowManager windowManager,
            IPictureSettingsViewModel pictureSettingsViewModel,
            IVideoViewModel videoViewModel,
            ISummaryViewModel summaryViewModel,
            IFiltersViewModel filtersViewModel,
            IAudioViewModel audioViewModel,
            ISubtitlesViewModel subtitlesViewModel,
            IChaptersViewModel chaptersViewModel,
            IStaticPreviewViewModel staticPreviewViewModel,
            IQueueViewModel queueViewModel,
            IMetaDataViewModel metaDataViewModel,
            IPresetManagerViewModel presetManagerViewModel,
            INotifyIconService notifyIconService,
            ISystemService systemService,
            ILog logService,
            INotificationService notificationService)
            : base(userSettingService)
        {
            this.scanService = scanService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.updateService = updateService;
            this.windowManager = windowManager;
            this.notifyIconService = notifyIconService;
            this.logService = logService;
            this.notificationService = notificationService;
            this.QueueViewModel = queueViewModel;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoCHelper.Get<IQueueService>();

            this.SummaryViewModel = summaryViewModel;
            this.PictureSettingsViewModel = pictureSettingsViewModel;
            this.VideoViewModel = videoViewModel;
            this.MetaDataViewModel = metaDataViewModel;
            this.FiltersViewModel = filtersViewModel;
            this.AudioViewModel = audioViewModel;
            this.SubtitleViewModel = subtitlesViewModel;
            this.ChaptersViewModel = chaptersViewModel;
            this.StaticPreviewViewModel = staticPreviewViewModel;
            this.PresetManagerViewModel = presetManagerViewModel;

            // Setup Properties
            this.WindowTitle = Resources.HandBrake_Title;
            this.CurrentTask = new EncodeTask();
            this.ScannedSource = new Source();
            this.HasSource = false;
            this.IsNavigationEnabled = true;

            // Setup Events
            this.scanService.ScanStarted += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueChanged;
            this.queueProcessor.QueuePaused += this.QueueProcessor_QueuePaused;
            this.queueProcessor.QueueJobStatusChanged += this.QueueProcessor_QueueJobStatusChanged;
            this.userSettingService.SettingChanged += this.UserSettingServiceSettingChanged;

            this.PresetsCategories = new BindingList<IPresetObject>();
            this.Drives = new BindingList<SourceMenuItem>();

            // Set Process Priority. Only when using in-process encoding. 
            // When process isolation is enabled, we'll stick to "Normal".
            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled))
            {
                switch ((ProcessPriority)this.userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessPriorityInt))
                {
                    case ProcessPriority.High:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
                        break;
                    case ProcessPriority.AboveNormal:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.AboveNormal;
                        break;
                    case ProcessPriority.Normal:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Normal;
                        break;
                    case ProcessPriority.Low:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Idle;
                        break;
                    default:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                        break;
                }
            }


            // Setup Commands
            this.QueueCommand = new QueueCommands(this.QueueViewModel);
            this.ProcessDriveCommand = new SimpleRelayCommand<object>(this.ProcessDrive);
            this.WhenDoneCommand = new SimpleRelayCommand<int>(this.WhenDone);

            // Monitor the system.
            systemService.Start();

            this.Load();
        }

        public SimpleRelayCommand<int> WhenDoneCommand { get; set; }

        /* View Model Properties */

        public IPictureSettingsViewModel PictureSettingsViewModel { get; set; }

        public IAudioViewModel AudioViewModel { get; set; }

        public ISubtitlesViewModel SubtitleViewModel { get; set; }

        public IChaptersViewModel ChaptersViewModel { get; set; }

        public IVideoViewModel VideoViewModel { get; set; }

        public IFiltersViewModel FiltersViewModel { get; set; }

        public IQueueViewModel QueueViewModel { get; set; }

        public IStaticPreviewViewModel StaticPreviewViewModel { get; set; }

        public IMetaDataViewModel MetaDataViewModel { get; set; }

        public ISummaryViewModel SummaryViewModel { get; set; }

        public IPresetManagerViewModel PresetManagerViewModel { get; set; }

        public int SelectedTab { get; set; }

        /* Commands */
        public ICommand AddToQueueQualitySweepCommand => new AddToQueueQualitySweepCommand(this, this.VideoViewModel, this.userSettingService, this.errorService);
        public SimpleRelayCommand<object> ProcessDriveCommand { get; set; }

        /* Properties */

        public string WindowTitle
        {
            get => this.windowName;

            set
            {
                if (!Equals(this.windowName, value))
                {
                    this.windowName = value;
                    this.NotifyOfPropertyChange(() => this.WindowTitle);
                }
            }
        }

        public string ProgramStatusLabel
        {
            get => string.IsNullOrEmpty(this.programStatusLabel) ? Resources.State_Ready : this.programStatusLabel;

            set
            {
                if (!Equals(this.programStatusLabel, value))
                {
                    this.programStatusLabel = value;
                    this.NotifyOfPropertyChange(() => this.ProgramStatusLabel);
                }
            }
        }

        public string StatusLabel
        {
            get => string.IsNullOrEmpty(this.statusLabel) ? Resources.State_Ready : this.statusLabel;

            set
            {
                if (!Equals(this.statusLabel, value))
                {
                    this.statusLabel = value;
                    this.NotifyOfPropertyChange(() => this.StatusLabel);
                }
            }
        }

        public bool QueueRecoveryArchivesExist { get; set; }

        public IEnumerable<IPresetObject> PresetsCategories { get; set; }

        public Preset SelectedPreset
        {
            get => this.selectedPreset;

            set
            {
                if (!object.Equals(this.selectedPreset, value))
                {
                    if (value == null)
                    {
                        this.errorService.ShowError("Null Preset", null, Environment.StackTrace.ToString());
                    }

                    if (value != null)
                    {
                        bool result = this.PresetSelect(value);
                        if (result)
                        {
                            this.selectedPreset = value;
                            this.NotifyOfPropertyChange(() => this.SelectedPreset);
                        }
                    }
                }
            }
        }

        public bool IsModifiedPreset
        {
            get => this.isModifiedPreset;

            set
            {
                if (value == this.isModifiedPreset)
                {
                    return;
                }

                this.isModifiedPreset = value;
                this.NotifyOfPropertyChange(() => this.IsModifiedPreset);
            }
        }

        public EncodeTask CurrentTask { get; set; }

        public Source ScannedSource
        {
            get => this.scannedSource;

            set
            {
                this.scannedSource = value;
                this.NotifyOfPropertyChange(() => this.ScannedSource);
                this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);
            }
        }

        public int TitleSpecificScan { get; set; }

        public string SourceLabel
        {
            get => string.IsNullOrEmpty(this.sourceLabel) ? Resources.Main_SelectSource : this.sourceLabel;

            set
            {
                if (!Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                    this.NotifyOfPropertyChange(() => SourceLabel);
                }
            }
        }

        public BindingList<PointToPointMode> RangeMode { get; } = new BindingList<PointToPointMode> { PointToPointMode.Chapters, PointToPointMode.Seconds, PointToPointMode.Frames };

        public bool ShowTextEntryForPointToPointMode => this.SelectedPointToPoint != PointToPointMode.Chapters;

        public IEnumerable<int> StartEndRangeItems
        {
            get => this.SelectedTitle?.Chapters.Select(item => item.ChapterNumber).Select(dummy => dummy).ToList();
        }

        public IEnumerable<int> Angles
        {
            get
            {
                if (this.SelectedTitle == null)
                {
                    return null;
                }

                List<int> items = new List<int>();
                for (int i = 1; i <= this.selectedTitle.AngleCount; i++)
                {
                    items.Add(i);
                }

                return items;
            }
        }

        public string Duration
        {
            get => string.IsNullOrEmpty(duration) ? "--:--:--" : duration;
            set
            {
                duration = value;
                this.NotifyOfPropertyChange(() => Duration);
            }
        }

        public bool IsEncoding => this.queueProcessor.IsEncoding;

        public bool ShowStatusWindow
        {
            get => this.showStatusWindow;

            set
            {
                this.showStatusWindow = value;
                this.IsNavigationEnabled = !this.showStatusWindow;
                this.NotifyOfPropertyChange(() => this.ShowStatusWindow);
            }
        }

        public IEnumerable<OutputFormat> OutputFormats => new List<OutputFormat> { OutputFormat.Mp4, OutputFormat.Mkv, OutputFormat.WebM };

        public string Destination
        {
            get => this.CurrentTask.Destination;
            set
            {
                value = value?.Replace("\"", string.Empty);

                if (!Equals(this.CurrentTask.Destination, value))
                {
                    if (!string.IsNullOrEmpty(value))
                    {
                        string ext = string.Empty;
                        try
                        {
                            if (FileHelper.FilePathHasInvalidChars(value))
                            {
                                this.errorService.ShowMessageBox(Resources.Main_InvalidDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                                return;
                            }

                            if (value == this.ScannedSource.ScanPath)
                            {
                                this.errorService.ShowMessageBox(Resources.Main_MatchingFileOverwriteWarning, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                                return;
                            }

                            ext = Path.GetExtension(value);
                        }
                        catch (Exception exc)
                        {
                            this.errorService.ShowError(Resources.Main_InvalidDestination, string.Empty, value + Environment.NewLine + exc);
                            return;
                        }

                        this.CurrentTask.Destination = value;
                        this.NotifyOfPropertyChange(() => this.Destination);

                        switch (ext)
                        {
                            case ".mkv":
                                this.SummaryViewModel.SetContainer(OutputFormat.Mkv);
                                break;
                            case ".mp4":
                            case ".m4v":
                                this.SummaryViewModel.SetContainer(OutputFormat.Mp4);
                                break;
                            case ".webm":
                                this.SummaryViewModel.SetContainer(OutputFormat.WebM);
                                break;
                        }
                    }
                    else
                    {
                        this.CurrentTask.Destination = string.Empty;
                        this.NotifyOfPropertyChange(() => this.Destination);
                    }
                }
            }
        }

        public Title SelectedTitle
        {
            get => this.selectedTitle;

            set
            {
                if (!Equals(this.selectedTitle, value))
                {
                    this.selectedTitle = value;

                    if (this.selectedTitle == null)
                    {
                        return;
                    }

                    // Use the Path on the Title, or the Source Scan path if one doesn't exist.
                    this.SourceLabel = this.SelectedTitle?.DisplaySourceName ?? this.ScannedSource?.SourceName ;
                    this.CurrentTask.Source = !string.IsNullOrEmpty(this.selectedTitle.SourceName) ? this.selectedTitle.SourceName : this.ScannedSource?.ScanPath;
                    this.CurrentTask.Title = value.TitleNumber;
                    this.NotifyOfPropertyChange(() => this.StartEndRangeItems);
                    this.NotifyOfPropertyChange(() => this.SelectedTitle);
                    this.NotifyOfPropertyChange(() => this.Angles);
                    this.NotifyOfPropertyChange(() => this.SourceInfo);

                    // Default the Start and End Point dropdowns
                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = this.selectedTitle.Chapters != null &&
                                            this.selectedTitle.Chapters.Count != 0
                                                ? this.selectedTitle.Chapters.Last().ChapterNumber
                                                : 1;

                    this.SelectedPointToPoint = PointToPointMode.Chapters;
                    this.SelectedAngle = 1;

                    if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
                    {
                        if (this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null)
                        {
                            this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
                        }
                    }

                    this.NotifyOfPropertyChange(() => this.CurrentTask);

                    this.Duration = this.DurationCalculation();

                    // Setup the tab controls
                    this.SetupTabs();
                }
            }
        }

        public int SelectedAngle
        {
            get => this.CurrentTask.Angle;

            set
            {
                this.CurrentTask.Angle = value;
                this.NotifyOfPropertyChange(() => this.SelectedAngle);
            }
        }

        public bool IsTimespanRange { get; set; }

        public long SelectedStartPoint
        {
            get => this.CurrentTask.StartPoint;

            set
            {
                this.CurrentTask.StartPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedStartPoint);
                this.Duration = this.DurationCalculation();

                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.ScannedSource.ScanPath != null)
                {
                    if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
                    }
                }

                if (this.SelectedStartPoint > this.SelectedEndPoint)
                {
                    this.SelectedEndPoint = this.SelectedStartPoint;
                }
            }
        }

        public long SelectedEndPoint
        {
            get => this.CurrentTask.EndPoint;

            set
            {
                this.CurrentTask.EndPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedEndPoint);
                this.Duration = this.DurationCalculation();

                if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                    this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                {
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
                }

                if (this.SelectedStartPoint > this.SelectedEndPoint && this.SelectedPointToPoint == PointToPointMode.Chapters)
                {
                    this.SelectedStartPoint = this.SelectedEndPoint;
                }
            }
        }

        public PointToPointMode SelectedPointToPoint
        {
            get => this.CurrentTask.PointToPointMode;
            set
            {
                this.CurrentTask.PointToPointMode = value;
                this.NotifyOfPropertyChange(() => SelectedPointToPoint);
                this.NotifyOfPropertyChange(() => ShowTextEntryForPointToPointMode);

                if (value == PointToPointMode.Chapters && this.SelectedTitle != null)
                {
                    if (this.selectedTitle == null)
                    {
                        return;
                    }

                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = selectedTitle.Chapters != null && selectedTitle.Chapters.Count > 0 ? selectedTitle.Chapters.Last().ChapterNumber : 1;
                }
                else if (value == PointToPointMode.Seconds)
                {
                    if (this.selectedTitle == null)
                    {
                        return;
                    }

                    this.SelectedStartPoint = 0;

                    int timeInSeconds;
                    if (int.TryParse(Math.Round(selectedTitle.Duration.TotalSeconds, 0).ToString(CultureInfo.InvariantCulture), out timeInSeconds))
                    {
                        this.SelectedEndPoint = timeInSeconds;
                    }

                    this.IsTimespanRange = true;
                    this.NotifyOfPropertyChange(() => this.IsTimespanRange);
                }
                else
                {
                    if (this.selectedTitle == null)
                    {
                        return;
                    }

                    // Note this does not account for VFR. It's only a guesstimate. 
                    double estimatedTotalFrames = selectedTitle.Fps * selectedTitle.Duration.TotalSeconds;

                    this.SelectedStartPoint = 0;
                    int totalFrames;
                    if (int.TryParse(Math.Round(estimatedTotalFrames, 0).ToString(CultureInfo.InvariantCulture), out totalFrames))
                    {
                        this.SelectedEndPoint = totalFrames;
                    }

                    this.IsTimespanRange = false;
                    this.NotifyOfPropertyChange(() => this.IsTimespanRange);
                }
            }
        }

        public int ProgressPercentage { get; set; }

        public bool ShowSourceSelection
        {
            get => this.showSourceSelection;
            set
            {
                if (value.Equals(this.showSourceSelection))
                {
                    return;
                }
                this.showSourceSelection = value;
                this.NotifyOfPropertyChange(() => this.ShowSourceSelection);

                this.IsNavigationEnabled = !showSourceSelection;

                // Refresh the drives.
                if (this.showSourceSelection)
                {
                    this.Drives.Clear();
                    foreach (SourceMenuItem menuItem in from item in DriveUtilities.GetDrives()
                                                        let driveInformation = item
                                                        select new SourceMenuItem
                                                        {
                                                            Text = string.Format("{0} ({1})", item.RootDirectory, item.VolumeLabel), 
                                                            Command = new SourceMenuCommand(() => this.ProcessDrive(driveInformation)), 
                                                            Tag = item, 
                                                            IsDrive = true
                                                        })
                    {
                        this.Drives.Add(menuItem);
                    }

                    this.TitleSpecificScan = 0;
                    this.NotifyOfPropertyChange(() => this.TitleSpecificScan);
                }
            }
        }

        public BindingList<SourceMenuItem> Drives
        {
            get => this.drives;
            set
            {
                if (Equals(value, this.drives))
                {
                    return;
                }

                this.drives = value;
                this.NotifyOfPropertyChange(() => this.Drives);
            }
        }

        public Action CancelAction => this.CancelScan;

        public Action OpenLogWindowAction => this.OpenLogWindow;

        public bool ShowAlertWindow
        {
            get => this.showAlertWindow;
            set
            {
                if (value.Equals(this.showAlertWindow))
                {
                    return;
                }
                this.showAlertWindow = value;
                this.IsNavigationEnabled = !this.showAlertWindow;
                this.NotifyOfPropertyChange(() => this.ShowAlertWindow);
            }
        }

        public string AlertWindowHeader
        {
            get => this.alertWindowHeader;
            set
            {
                if (value == this.alertWindowHeader)
                {
                    return;
                }

                this.alertWindowHeader = value;
                this.NotifyOfPropertyChange(() => this.AlertWindowHeader);
            }
        }

        public string AlertWindowText
        {
            get => this.alertWindowText;
            set
            {
                if (value == this.alertWindowText)
                {
                    return;
                }

                this.alertWindowText = value;
                this.NotifyOfPropertyChange(() => this.AlertWindowText);
            }
        }

        public Action AlertWindowClose => this.CloseAlertWindow;

        public int QueueCount => this.queueProcessor.Count;
        
        public bool IsQueueCountVisible => this.queueProcessor.Count > 0;

        public string QueueLabel => string.Format(Resources.Main_QueueLabel, string.Empty);

        public string StartLabel
        {
            get
            {
                if (this.queueProcessor.IsPaused && this.queueProcessor.ActiveJobCount > 0)
                {
                    return Resources.Main_ResumeEncode;
                }

                return this.queueProcessor.Count > 0 ? Resources.Main_StartQueue : Resources.Main_Start;
            } 
        }

        public bool IsNavigationEnabled
        {
            get => this.isNavigationEnabled;
            set
            {
                if (value == this.isNavigationEnabled) return;
                this.isNavigationEnabled = value;
                this.NotifyOfPropertyChange(() => this.IsNavigationEnabled);
                this.NotifyOfPropertyChange(() => this.HasSource);
            }
        }

        public bool HasSource
        {
            get => this.hasSource && this.IsNavigationEnabled;

            set
            {
                if (value.Equals(this.hasSource))
                {
                    return;
                }

                this.hasSource = value;
                this.NotifyOfPropertyChange(() => this.HasSource);
            }
        }

        public string SourceInfo => SourceInfoHelper.GenerateSourceInfo(this.SelectedTitle);

        public bool ShowAddAllToQueue => this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddAllToQueue);

        public bool ShowAddSelectionToQueue => this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddSelectionToQueue);

        public string ShowAddAllMenuName =>
            string.Format("{0} {1}", (!this.ShowAddAllToQueue ? Resources.MainView_Show : Resources.MainView_Hide), Resources.MainView_ShowAddAllToQueue);

        public string ShowAddSelectionMenuName =>
            string.Format("{0} {1}", (!this.ShowAddSelectionToQueue ? Resources.MainView_Show : Resources.MainView_Hide), Resources.MainView_ShowAddSelectionToQueue);

        public bool UpdateAvailable
        {
            get => this.updateAvailable;
            set
            {
                if (value == this.updateAvailable)
                {
                    return;
                }

                this.updateAvailable = value;
                this.NotifyOfPropertyChange(() => this.UpdateAvailable);
            }
        }

        public bool IsOldNightly { get; set; }

        public bool IsMultiProcess { get; set; }

        public bool IsNightly => HandBrakeVersionHelper.IsNightly();

        public bool IsPresetPaneDisplayed { get; set; }

        public bool IsPresetDescriptionVisible { get; set; }

        /* Commands */

        public ICommand QueueCommand { get; set; }

        /* Load and Shutdown Handling */
        
        public override void OnLoad()
        {
            // Perform an update check if required
            this.updateService.PerformStartupUpdateCheck(this.HandleUpdateCheckResults);

            // Setup the presets.
            this.presetService.Load();
            this.PresetsCategories = this.presetService.Presets;
            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.presetService.LoadCategoryStates();

            this.SummaryViewModel.OutputFormatChanged += this.SummaryViewModel_OutputFormatChanged;

            // Queue Recovery
            bool queueRecovered = QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService, StartupOptions.AutoRestartQueue, StartupOptions.QueueRecoveryIds);
            this.QueueRecoveryArchivesExist = QueueRecoveryHelper.ArchivesExist();
            this.NotifyOfPropertyChange(() => this.QueueRecoveryArchivesExist);

            // If the queue is not recovered, show the source selection window by default.
            if (!queueRecovered)
            {
                this.ShowSourceSelection = true;
            }
            else
            {
                this.HasSource = true; // Enable the GUI. Needed for in-line queue.
            }

            // If the user has enabled --auto-start-queue, start the queue.
            if (StartupOptions.AutoRestartQueue && !this.queueProcessor.IsProcessing && this.queueProcessor.Count > 0)
            {
                this.queueProcessor.Start();
            }

            // Preset Selection
            this.SelectDefaultPreset();

            // Reset WhenDone if necessary.
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction))
            {
                this.WhenDone(0);
            }

            // Log Cleaning
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs))
            {
                Thread clearLog = new Thread(() => GeneralUtilities.ClearLogFiles(7));
                clearLog.Start();
            }

            // Preset Panel
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPresetDesc))
            {
                IsPresetDescriptionVisible = true;
                this.NotifyOfPropertyChange(() => this.IsPresetDescriptionVisible);
            }

            this.PictureSettingsViewModel.TabStatusChanged += this.TabStatusChanged;
            this.VideoViewModel.TabStatusChanged += this.TabStatusChanged;
            this.FiltersViewModel.TabStatusChanged += this.TabStatusChanged;
            this.AudioViewModel.TabStatusChanged += this.TabStatusChanged;
            this.SubtitleViewModel.TabStatusChanged += this.TabStatusChanged;
            this.ChaptersViewModel.TabStatusChanged += this.TabStatusChanged;
            this.MetaDataViewModel.TabStatusChanged += this.TabStatusChanged;
            this.SummaryViewModel.TabStatusChanged += this.TabStatusChanged;
        }

        public void Shutdown()
        {
            // Notification Service
            this.notificationService.Shutdown();

            // Shutdown Service
            this.queueProcessor.Stop(true);
            this.presetService.SaveCategoryStates();

            // Unsubscribe from Events.
            this.scanService.ScanStarted -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;
            this.queueProcessor.QueuePaused -= this.QueueProcessor_QueuePaused;
            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueChanged;
          
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.userSettingService.SettingChanged -= this.UserSettingServiceSettingChanged;

            this.SummaryViewModel.OutputFormatChanged -= this.SummaryViewModel_OutputFormatChanged;

            // Tab status events
            this.PictureSettingsViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.VideoViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.FiltersViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.AudioViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.SubtitleViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.ChaptersViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.MetaDataViewModel.TabStatusChanged -= this.TabStatusChanged;
            this.SummaryViewModel.TabStatusChanged -= this.TabStatusChanged;
        }

        /* Menu and Toolbar */

        public void OpenAboutApplication()
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.About);
        }

        public void OpenOptionsWindow()
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(null);
        }
         
        public void OpenLogWindow()
        {
            WindowHelper.ShowWindow<ILogViewModel, LogView>(this.windowManager);
        }

        public void OpenQueueWindow()
        {
            WindowHelper.ShowWindow<IQueueViewModel, QueueView>(this.windowManager);
        }

        public void OpenPreviewWindow()
        {
            if (!string.IsNullOrEmpty(this.CurrentTask.Source) && !this.StaticPreviewViewModel.IsOpen)
            {
                this.StaticPreviewViewModel.IsOpen = true;
                this.StaticPreviewViewModel.UpdatePreviewFrame(this.CurrentTask, this.ScannedSource);
                this.windowManager.ShowWindow<StaticPreviewView>(this.StaticPreviewViewModel);
            }
            else if (this.StaticPreviewViewModel.IsOpen)
            {
                WindowHelper.ShowWindow<IPresetManagerViewModel, StaticPreviewView>(this.windowManager);
            }
        }

        public void OpenPresetWindow()
        {
            if (!this.PresetManagerViewModel.IsOpen)
            {
                this.PresetManagerViewModel.IsOpen = true;
                this.PresetManagerViewModel.SetupWindow(this.HandleManagePresetChanges);
                this.windowManager.ShowWindow<PresetManagerView>(this.PresetManagerViewModel);
            }
            else if (this.PresetManagerViewModel.IsOpen)
            {
                WindowHelper.ShowWindow<IPresetManagerViewModel, PresetManagerView>(this.windowManager);
            }
        }

        private void HandleManagePresetChanges(Preset preset)
        {
            this.PresetsCategories = null;
            this.NotifyOfPropertyChange(() => this.PresetsCategories);

            this.PresetsCategories = this.presetService.Presets;
            this.NotifyOfPropertyChange(() => this.PresetsCategories);

            this.selectedPreset = preset; // Reselect the preset      
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
        }
        
        public void LaunchHelp()
        {
            try
            {
                Process.Start("explorer.exe", "https://handbrake.fr/docs");
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Main_UnableToLoadHelpMessage, Resources.Main_UnableToLoadHelpSolution, exc);
            }
        }

        public void CheckForUpdates()
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.Updates);
        }
        
        public void NightlyUpdate()
        {
            try
            {
                Process.Start("explorer.exe", "https://github.com/HandBrake/HandBrake-snapshots");
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Main_UnableToLoadHelpMessage, Resources.Main_UnableToLoadHelpSolution, exc);
            }
        }

        public AddQueueError AddToQueue(bool batch)
        {
            if (this.ScannedSource == null || string.IsNullOrEmpty(this.ScannedSource.ScanPath) || this.ScannedSource.Titles.Count == 0)
            {
                return new AddQueueError(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                return new AddQueueError(Resources.Main_SetDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (this.Destination.ToLower() == this.ScannedSource.ScanPath.ToLower())
            {
                return new AddQueueError(Resources.Main_MatchingFileOverwriteWarning, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (File.Exists(this.CurrentTask.Destination))
            {
                FileOverwriteBehaviour behaviour = (FileOverwriteBehaviour)this.userSettingService.GetUserSetting<int>(UserSettingConstants.FileOverwriteBehaviour);
                if (behaviour == FileOverwriteBehaviour.Ask)
                {
                    MessageBoxResult result = this.errorService.ShowMessageBox(string.Format(Resources.Main_QueueOverwritePrompt, Path.GetFileName(this.CurrentTask.Destination)), Resources.Question, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                    if (result == MessageBoxResult.No)
                    {
                        return null; // Handled by the above action.
                    }
                }
            }

            if (!DirectoryUtilities.IsWritable(Path.GetDirectoryName(this.CurrentTask.Destination), true))
            {
                return new AddQueueError(Resources.Main_NoPermissionsOrMissingDirectory, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (!batch && !DriveUtilities.HasMinimumDiskSpace(
                this.Destination,
                this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.Main_LowDiskspace, Resources.Warning, MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return null; // Handled by the above action.
                }
            }

            // Sanity check the filename
            if (FileHelper.FilePathHasInvalidChars(this.Destination))
            {
                this.NotifyOfPropertyChange(() => this.Destination);
                return new AddQueueError(Resources.Main_InvalidDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            // defer to subtitle's validation messages
            if (!this.SubtitleViewModel.ValidateSubtitles())
            {
                return new AddQueueError(Resources.Subtitles_WebmSubtitleIncompatibilityHeader, Resources.Main_PleaseFixSubtitleSettings, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            QueueTask task = new QueueTask(new EncodeTask(this.CurrentTask), this.ScannedSource.ScanPath, this.SelectedPreset, this.IsModifiedPreset, this.selectedTitle);

            if (!this.queueProcessor.CheckForDestinationPathDuplicates(task.Task.Destination))
            {
                this.queueProcessor.Add(task);
            }
            else
            {
                return new AddQueueError(Resources.Main_DuplicateDestinationOnQueue, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Warning);
            }

            if (!this.IsEncoding)
            {
                this.ProgramStatusLabel = string.Format(Resources.Main_XEncodesPending, this.queueProcessor.Count);
            }

            return null;
        }

        public void AddToQueueWithErrorHandling()
        {
            var addError = this.AddToQueue(false);
            if (addError != null)
            {
                this.errorService.ShowMessageBox(addError.Message, addError.Header, addError.Buttons, addError.ErrorType);
            }
        }

        public void AddAllToQueue()
        {
            if (this.ScannedSource == null || this.ScannedSource.Titles == null || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!AutoNameHelper.IsAutonamingEnabled())
            {
                this.errorService.ShowMessageBox(Resources.Main_TurnOnAutoFileNaming, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!DriveUtilities.HasMinimumDiskSpace(this.Destination, this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.Main_LowDiskspace, Resources.Warning, MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return; // Handled by the above action.
                }
            }

            foreach (Title title in this.ScannedSource.Titles)
            {
                this.SelectedTitle = title;
                var addError = this.AddToQueue(true);
                if (addError != null)
                {
                    MessageBoxResult result = this.errorService.ShowMessageBox(addError.Message + Environment.NewLine + Environment.NewLine + Resources.Main_ContinueAddingToQueue, addError.Header, MessageBoxButton.YesNo, addError.ErrorType);

                    if (result == MessageBoxResult.No)
                    {
                        break;
                    }
                }
            }
        }

        public void AddSelectionToQueue()
        {
            if (this.ScannedSource == null || this.ScannedSource.Titles == null || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!AutoNameHelper.IsAutonamingEnabled())
            {
                this.errorService.ShowMessageBox(Resources.Main_TurnOnAutoFileNaming, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!DriveUtilities.HasMinimumDiskSpace(this.Destination, this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.Main_LowDiskspace, Resources.Warning, MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return; // Handled by the above action.
                }
            }

            // Always use the current settings when adding to the queue as best as possible.
            Preset temporaryPreset = this.selectedPreset;
            if (this.IsModifiedPreset)
            {
                temporaryPreset = new Preset(this.SelectedPreset);
                temporaryPreset.Name = string.Format(
                    "{0} {1}",
                    temporaryPreset.Name,
                    Resources.MainView_ModifiedPreset);
                temporaryPreset.Task = new EncodeTask(this.CurrentTask);
                temporaryPreset.AudioTrackBehaviours = new AudioBehaviours(this.AudioViewModel.AudioBehaviours);
                temporaryPreset.SubtitleTrackBehaviours = new SubtitleBehaviours(this.SubtitleViewModel.SubtitleBehaviours);
            }

            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(QueueSelectionViewModel));
            IQueueSelectionViewModel viewModel = IoCHelper.Get<IQueueSelectionViewModel>();

            viewModel.Setup(
                this.ScannedSource,
                (tasks) =>
                {
                    foreach (SelectionTitle title in tasks)
                    {
                        this.SelectedTitle = title.Title;
                        var addError = this.AddToQueue(true);
                        if (addError != null)
                        {
                            MessageBoxResult result = this.errorService.ShowMessageBox(addError.Message + Environment.NewLine + Environment.NewLine + Resources.Main_ContinueAddingToQueue, addError.Header, MessageBoxButton.YesNo, addError.ErrorType);

                            if (result == MessageBoxResult.No)
                            {
                                break;
                            }
                        }
                    }
                },
                temporaryPreset);

            if (window != null)
            {
                window.Activate();
            }
            else
            {
                this.windowManager.ShowWindow<QueueSelectionView>(viewModel);
            }
        }

        public void FolderScan()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = Resources.Main_PleaseSelectFolder, UseDescriptionForTitle = true };
            bool? dialogResult = dialog.ShowDialog();

            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.StartScan(dialog.SelectedPath, this.TitleSpecificScan);
            }
        }

        public void FileScan()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.*)|*.*" };

            string mruDir = this.GetMru(Constants.FileScanMru);
            if (!string.IsNullOrEmpty(mruDir) && Directory.Exists(mruDir))
            {
                dialog.InitialDirectory = mruDir;
            }

            bool? dialogResult = null;
            try
            {
                dialogResult = dialog.ShowDialog();
            }
            catch (Exception e)
            {
                this.SetMru(Constants.FileScanMru, string.Empty); // RESET MRU in case it's the fault.
                this.errorService.ShowMessageBox(
                    Resources.MainViewModel_FilePathSelectError,
                    Resources.Error,
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
                this.logService.LogMessage("Attempted to recover from an error fro the File Scan FileDialog: " + e);
                return;
            }
            
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.SetMru(Constants.FileScanMru, Path.GetDirectoryName(dialog.FileName));

                this.StartScan(dialog.FileName, this.TitleSpecificScan);
            }
        }

        public void CancelScan()
        {
            this.ShowStatusWindow = false;
            this.scanService.Cancel();
        }

        public void StartEncode()
        {
            if (this.queueProcessor.IsProcessing)
            {
                this.NotifyOfPropertyChange(() => this.IsEncoding);
                return;
            }

            // Check if we already have jobs, and if we do, just start the queue.
            if (this.queueProcessor.Count != 0 || this.queueProcessor.IsPaused)
            {
                this.NotifyOfPropertyChange(() => this.IsEncoding);
                this.queueProcessor.Start();
                return;
            }

            // Otherwise, perform Sanity Checking then add to the queue and start if everything is ok.
            if (this.SelectedTitle == null)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // Create the Queue Task and Start Processing
            var addError = this.AddToQueue(false);
            if (addError == null)
            {
                this.NotifyOfPropertyChange(() => this.IsEncoding);
                this.queueProcessor.Start();               
            }
            else
            {
                this.errorService.ShowMessageBox(
                    addError.Message,
                    addError.Header,
                    addError.Buttons,
                    addError.ErrorType);
            }
        }

        public void EditQueueJob(QueueTask queueTask)
        {
            // Rescan the source to make sure it's still valid
            EncodeTask task = queueTask.Task;

            this.queueEditTask = queueTask;
            this.scanService.Scan(task.Source, task.Title, QueueEditAction);
        }

        public void PauseEncode()
        {
            if (!this.queueProcessor.IsEncoding || this.queueProcessor.IsPaused)
            {
                return;
            }

            this.queueProcessor.Pause(true);
            this.NotifyOfPropertyChange(() => this.IsEncoding);
        }

        public void StopEncode()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.MainView_StopEncodeConfirm,
                Resources.MainView_StopEncode,
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result == MessageBoxResult.Yes)
            {
                this.queueProcessor.Stop(true);
            }
        }

        public void ExitApplication()
        {
            Application.Current.Shutdown();
        }

        public void SelectSourceWindow()
        {
            ShowSourceSelection = !ShowSourceSelection;
        }

        public void CloseSourceSelection()
        {
            this.ShowSourceSelection = false;
        }

        public void CloseAlertWindow()
        {
            this.ShowAlertWindow = false;
            this.AlertWindowText = string.Empty;
            this.AlertWindowHeader = string.Empty;
        }

        public void WhenDone(int action)
        {
            this.QueueViewModel?.WhenDone(action, true);
        }

        public void ExportSourceData()
        {
            if (this.ScannedSource == null)
            {
                return; 
            }

            string json = JsonSerializer.Serialize(this.ScannedSource, JsonSettings.Options);

            SaveFileDialog savefiledialog = new SaveFileDialog
                                            {
                                                Filter = "json|*.json",
                                                CheckPathExists = true,
                                                AddExtension = true,
                                                DefaultExt = ".json",
                                                OverwritePrompt = true,
                                                FilterIndex = 0,
                                                FileName = "debug.scan_output.json"
            };

            savefiledialog.ShowDialog();

            if (!string.IsNullOrEmpty(savefiledialog.FileName))
            {
                using (StreamWriter writer = new StreamWriter(savefiledialog.FileName))
                {
                    writer.Write(json);
                }
            }
        }

        public void ImportSourceData()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Debug Files|*.json", CheckFileExists = true };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                using (StreamReader reader = new StreamReader(dialog.FileName))
                {
                    string json = reader.ReadToEnd();
                    if (!string.IsNullOrEmpty(json))
                    {
                       Source source = JsonSerializer.Deserialize<Source>(json, JsonSettings.Options);
                       this.ScannedSource = source;
                       this.HasSource = true;
                       this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault(t => t.MainTitle) ?? this.ScannedSource.Titles.FirstOrDefault();
                    }
                }
            }
        }

        public void TogglePresetPane()
        {
            this.IsPresetPaneDisplayed = !this.IsPresetPaneDisplayed;
            this.NotifyOfPropertyChange(() => IsPresetPaneDisplayed);
        }

        public void NextTitle()
        {
            if (this.ScannedSource == null || this.SelectedTitle == null)
            {
                return;
            }

            int index = this.ScannedSource.Titles.IndexOf(this.selectedTitle);
            if (this.ScannedSource.Titles.Count >= (index + 2))
            {
                this.SelectedTitle = this.ScannedSource.Titles[index + 1];
            }
        }

        public void PreviousTitle()
        {
            if (this.ScannedSource == null || this.SelectedTitle == null)
            {
                return;
            }

            int index = this.ScannedSource.Titles.IndexOf(this.selectedTitle);
            if (index >= 1)
            {
                this.SelectedTitle = this.ScannedSource.Titles[index -1];
            }
        }

        /* Main Window Public Methods*/

        public void FilesDroppedOnWindow(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
                if (fileNames != null && fileNames.Any() && (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])))
                {
                    string videoContent = fileNames.FirstOrDefault(f => Path.GetExtension(f)?.ToLower() != ".srt" && Path.GetExtension(f)?.ToLower() != ".ssa");
                    if (!string.IsNullOrEmpty(videoContent))
                    {
                        this.StartScan(videoContent, 0);
                        return;
                    }

                    if (this.SelectedTitle == null)
                    {
                        this.errorService.ShowMessageBox(
                            Resources.MainView_SubtitleBeforeScanError,
                            Resources.Error,
                            MessageBoxButton.OK,
                            MessageBoxImage.Error);
                        return;
                    }

                    // StartScan is not synchronous, so for now we don't support adding both srt and video file at the same time. 
                    string[] subtitleFiles = fileNames.Where(f => Path.GetExtension(f)?.ToLower() == ".srt" || Path.GetExtension(f)?.ToLower() == ".ssa").ToArray();
                    if (subtitleFiles.Any())
                    {
                        this.SwitchTab(5);
                        this.SubtitleViewModel.Import(subtitleFiles);
                    }
                }
            }

            e.Handled = true;
        }

        public void SwitchTab(int i)
        {
            this.SelectedTab = i;
            this.NotifyOfPropertyChange(() => this.SelectedTab);
        }

        public void BrowseDestination()
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog
            {
                Filter = "mp4|*.mp4;*.m4v|mkv|*.mkv|webm|*.webm", 
                CheckPathExists = true, 
                AddExtension = true, 
                DefaultExt = ".mp4",
            };

            saveFileDialog.OverwritePrompt =
                (FileOverwriteBehaviour)this.userSettingService.GetUserSetting<int>(UserSettingConstants.FileOverwriteBehaviour) == FileOverwriteBehaviour.Ask;

            string extension = Path.GetExtension(this.CurrentTask.Destination);

            saveFileDialog.FilterIndex = !string.IsNullOrEmpty(this.CurrentTask.Destination)
                                         && !string.IsNullOrEmpty(extension)
                                             ? (extension == ".mp4" || extension == ".m4v" ? 1 : 2)
                                             : (this.CurrentTask.OutputFormat == OutputFormat.Mkv 
                                                 ? 2 
                                                 : (this.CurrentTask.OutputFormat == OutputFormat.WebM ? 3 : 0));

            string mruDir = this.GetMru(Constants.FileSaveMru);
            if (!string.IsNullOrEmpty(mruDir) && Directory.Exists(mruDir))
            {
                saveFileDialog.InitialDirectory = mruDir;
            }

            // If we have a current directory, override the MRU.
            if (this.CurrentTask != null && !string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                if (Directory.Exists(Path.GetDirectoryName(this.CurrentTask.Destination)))
                {
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(this.CurrentTask.Destination);
                }

                saveFileDialog.FileName = Path.GetFileName(this.CurrentTask.Destination);
            }

            bool? result = saveFileDialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
                this.SetMru(Constants.FileSaveMru, Path.GetDirectoryName(saveFileDialog.FileName));

                this.Destination = saveFileDialog.FileName;

                // Set the Extension Dropdown. This will also set Mp4/m4v correctly.
                if (!string.IsNullOrEmpty(saveFileDialog.FileName))
                {
                    switch (Path.GetExtension(saveFileDialog.FileName))
                    {
                        case ".mkv":
                            this.SummaryViewModel.SetContainer(OutputFormat.Mkv);
                            break;
                        case ".mp4":
                        case ".m4v":
                            this.SummaryViewModel.SetContainer(OutputFormat.Mp4);
                            break;
                        case ".webm":
                            this.SummaryViewModel.SetContainer(OutputFormat.WebM);
                            break;
                    }

                    this.NotifyOfPropertyChange(() => this.CurrentTask);
                }
            }
        }

        public void OpenDestinationDirectory()
        {
            if (!string.IsNullOrEmpty(this.Destination))
            {
                try
                {
                    string directory = Path.GetDirectoryName(this.Destination);
                    if (!string.IsNullOrEmpty(directory) && Directory.Exists(directory))
                    {
                        Process.Start("explorer.exe", directory);
                    }
                    else
                    {
                        MessageBoxResult result =
                            errorService.ShowMessageBox(
                                string.Format(Resources.DirectoryUtils_CreateFolderMsg, directory),
                                Resources.DirectoryUtils_CreateFolder,
                                MessageBoxButton.YesNo,
                                MessageBoxImage.Question);
                        if (result == MessageBoxResult.Yes)
                        {
                            Directory.CreateDirectory(directory);
                            Process.Start("explorer.exe", directory);
                        }
                    }
                }
                catch (Exception exc)
                {
                    this.errorService.ShowError(Resources.MainViewModel_UnableToLaunchDestDir, Resources.MainViewModel_UnableToLaunchDestDirSolution, exc);
                }
            }
        }

        public void PresetAdd()
        {
            IAddPresetViewModel presetViewModel = IoCHelper.Get<IAddPresetViewModel>();
            presetViewModel.Setup(this.CurrentTask, this.SelectedTitle, this.AudioViewModel.AudioBehaviours, this.SubtitleViewModel.SubtitleBehaviours);
            bool? result = this.windowManager.ShowDialog<AddPresetView>(presetViewModel);

            if (result.HasValue && result.Value)
            {
                this.NotifyOfPropertyChange(() => this.PresetsCategories);
                this.SelectedPreset = this.presetService.GetPreset(presetViewModel.PresetName);

                this.IsModifiedPreset = false;
            }
        }

        public void PresetUpdate()
        {
            if (this.SelectedPreset == null)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_SelectPresetForUpdate, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);

                return;
            }

            if (this.selectedPreset.IsBuildIn)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_NoUpdateOfBuiltInPresets, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (this.errorService.ShowMessageBox(Resources.Main_PresetUpdateConfirmation, Resources.AreYouSure, MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                this.selectedPreset.Update(new EncodeTask(this.CurrentTask), new AudioBehaviours(this.AudioViewModel.AudioBehaviours), new SubtitleBehaviours(this.SubtitleViewModel.SubtitleBehaviours));
                this.presetService.Update(this.selectedPreset.Name, this.selectedPreset);
                this.IsModifiedPreset = false;

                this.errorService.ShowMessageBox(
                        Resources.Main_PresetUpdated, Resources.Updated, MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        public void PresetManage()
        {
            if (this.SelectedPreset == null)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_SelectPresetForUpdate, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);

                return;
            }

            IManagePresetViewModel presetViewModel = IoCHelper.Get<IManagePresetViewModel>();
            presetViewModel.Setup(this.selectedPreset);
            this.windowManager.ShowDialog<ManagePresetView>(presetViewModel);
            Preset preset = presetViewModel.Preset;

            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.selectedPreset = preset; // Reselect the preset      
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
        }

        public void PresetRemoveSelected()
        {
            this.PresetRemove(this.SelectedPreset);
        }

        public void PresetRemove(object presetObj)
        {
            Preset preset = presetObj as Preset;
            if (preset != null)
            {
                if (preset.IsDefault)
                {
                    this.errorService.ShowMessageBox(
                      Resources.MainViewModel_CanNotDeleteDefaultPreset, 
                      Resources.Warning, 
                      MessageBoxButton.OK, 
                      MessageBoxImage.Information);

                    return;
                }

                MessageBoxResult result =
                this.errorService.ShowMessageBox(
                   Resources.MainViewModel_PresetRemove_AreYouSure + preset.Name + " ?", 
                   Resources.Question, 
                   MessageBoxButton.YesNo, 
                   MessageBoxImage.Question);

                if (result == MessageBoxResult.No)
                {
                    return;
                }

                this.presetService.Remove(preset.Name);
                this.NotifyOfPropertyChange(() => this.PresetsCategories);
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void PresetSetDefault()
        {
            if (this.selectedPreset != null)
            {
                this.presetService.SetDefault(this.selectedPreset.Name);
                this.errorService.ShowMessageBox(string.Format(Resources.Main_NewDefaultPreset, this.selectedPreset.Name), Resources.Main_Presets, MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void PresetImport()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Preset Files|*.json;*.plist", CheckFileExists = true };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.presetService.Import(dialog.FileName);
                this.NotifyOfPropertyChange(() => this.PresetsCategories);
            }
        }

        public void PresetExport()
        {
            if (this.selectedPreset != null && !this.selectedPreset.IsBuildIn)
            {
                SaveFileDialog savefiledialog = new SaveFileDialog
                                                {
                                                    Filter = "json|*.json",
                                                    CheckPathExists = true,
                                                    AddExtension = true,
                                                    DefaultExt = ".json",
                                                    OverwritePrompt = true,
                                                    FilterIndex = 0
                                                };

                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (!string.IsNullOrEmpty(filename))
                {
                    this.presetService.Export(savefiledialog.FileName, this.selectedPreset.Name);
                }
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        public void PresetReset()
        {
            this.presetService.UpdateBuiltInPresets();
            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.SelectDefaultPreset();
            this.errorService.ShowMessageBox(Resources.Presets_ResetComplete, Resources.Presets_ResetHeader, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        public void PresetDeleteBuildIn()
        {
            this.presetService.DeleteBuiltInPresets();
            this.NotifyOfPropertyChange(() => this.PresetsCategories);

            if (this.presetService.GetDefaultPreset() != null)
            {
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
        }

        public void ExportUserPresets()
        {
            SaveFileDialog savefiledialog = new SaveFileDialog
                                            {
                                                Filter = "json|*.json",
                                                CheckPathExists = true,
                                                AddExtension = true,
                                                DefaultExt = ".json",
                                                OverwritePrompt = true,
                                                FilterIndex = 0
                                            };

            savefiledialog.ShowDialog();
            string filename = savefiledialog.FileName;

            if (!string.IsNullOrEmpty(filename))
            {
                IList<PresetDisplayCategory> userPresets = this.presetService.GetPresetCategories(true);
                this.presetService.ExportCategories(savefiledialog.FileName, userPresets);
            }
        }

        public void PresetReSelect()
        {
            this.PresetSelect(this.SelectedPreset);
        }

        public bool PresetSelect(object tag)
        {
            Preset preset = tag as Preset;
            if (preset != null)
            {
                if (preset.IsPresetDisabled)
                {
                    return false;
                }

                this.selectedPreset = preset;
                this.NotifyOfPropertyChange(() => this.SelectedPreset);

                this.presetService.SetSelected(this.selectedPreset.Name);

                if (this.selectedPreset != null)
                {
                    // Tab Settings
                    this.isSettingPreset = true;
                    this.IsModifiedPreset = false;
                    this.SummaryViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.PictureSettingsViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.VideoViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.FiltersViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.AudioViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.SubtitleViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.ChaptersViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.MetaDataViewModel.SetPreset(this.selectedPreset, this.CurrentTask);
                    this.SummaryViewModel.UpdateDisplayedInfo();

                    this.isSettingPreset = false;

                    if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null)
                    {
                        if (this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Preset))
                        {
                            this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
                        }
                    }

                    return true;
                }
            }

            return false;
        }

        public void ShowHidePresetDesc()
        {
            this.IsPresetDescriptionVisible = !this.IsPresetDescriptionVisible;
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowPresetDesc, this.IsPresetDescriptionVisible);
            this.NotifyOfPropertyChange(() => this.IsPresetDescriptionVisible);
        }

        public void StartScan(string filename, int title)
        {
            if (!string.IsNullOrEmpty(filename))
            {
                ShowSourceSelection = false;
                this.scanService.Scan(filename, title, null);
            }
        }

        public void ProcessDrive(object item)
        {
            if (item != null)
            {
                if (item.GetType() == typeof(DriveInformation))
                {
                    string path = ((DriveInformation)item).RootDirectory;
                    string videoDir = Path.Combine(path, "VIDEO_TS");

                    this.StartScan(Directory.Exists(videoDir) ? videoDir : path, 0);
                }
                else if (item.GetType() == typeof(SourceMenuItem))
                {
                    DriveInformation driveInfo = ((SourceMenuItem)item).Tag as DriveInformation;
                    if (driveInfo != null)
                    {
                        string path = driveInfo.RootDirectory;
                        string videoDir = Path.Combine(driveInfo.RootDirectory, "VIDEO_TS");

                        this.StartScan(Directory.Exists(videoDir) ? videoDir : path, this.TitleSpecificScan);
                    }

                    this.ShowSourceSelection = false;
                }
            }
        }

        public bool CanRecoverQueue()
        {
            return this.QueueRecoveryArchivesExist;
        }

        public void RecoverQueue()
        {
            QueueRecoveryHelper.ResetArchives();
            bool result = QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService, StartupOptions.AutoRestartQueue, StartupOptions.QueueRecoveryIds);
            this.QueueRecoveryArchivesExist = !result && QueueRecoveryHelper.ArchivesExist();
            this.NotifyOfPropertyChange(() => this.QueueRecoveryArchivesExist);
        }

        public void FlipAddAllToQueue()
        {
            bool value = !this.ShowAddAllToQueue;
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAddAllToQueue, value);

            var optionsViewModel = IoCHelper.Get<IOptionsViewModel>();
            optionsViewModel.UpdateSettings();
        }

        public void FlipAddSelectionToQueue()
        {
            bool value = !this.ShowAddSelectionToQueue;
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAddSelectionToQueue, value);

            var optionsViewModel = IoCHelper.Get<IOptionsViewModel>();
            optionsViewModel.UpdateSettings();
        }

        public void ReGenerateAutoName()
        {
            if (this.ScannedSource != null)
            {
                this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
            }
        }

        /* Private Methods*/

        private void QueueEditAction(bool successful, Source scannedSource)
        {
            /* TODO Fix this. */
            ThreadHelper.OnUIThread(() =>
            {
                if (this.queueEditTask != null && !string.IsNullOrEmpty(this.queueEditTask.SelectedPresetKey) && this.selectedPreset.Name != this.queueEditTask.SelectedPresetKey)
                {
                    Preset foundPreset = this.presetService.GetPreset(this.queueEditTask.SelectedPresetKey);
                    if (foundPreset != null)
                    {
                        this.selectedPreset = foundPreset;
                        this.NotifyOfPropertyChange(() => this.SelectedPreset);
                    }
                }

                // Copy all the Scan data into the UI
                this.ScannedSource = new Source(scannedSource);

                // Select the Users Title
                this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault();
                this.CurrentTask = new EncodeTask(this.queueEditTask.Task);
                this.NotifyOfPropertyChange(() => this.CurrentTask);
                this.HasSource = true;
             
                // Update the Main Window
                this.NotifyOfPropertyChange(() => this.Destination);
                this.SelectedAngle = this.CurrentTask.Angle;
                long start = this.CurrentTask.StartPoint;
                long end = this.CurrentTask.EndPoint;
                this.SelectedPointToPoint = this.CurrentTask.PointToPointMode; // Force reset.
                this.SelectedStartPoint = start;
                this.SelectedEndPoint = end;

                // Update the Tab Controls
                this.SummaryViewModel.UpdateTask(this.CurrentTask);
                this.PictureSettingsViewModel.UpdateTask(this.CurrentTask);
                this.VideoViewModel.UpdateTask(this.CurrentTask);
                this.FiltersViewModel.UpdateTask(this.CurrentTask);
                this.AudioViewModel.UpdateTask(this.CurrentTask);
                this.SubtitleViewModel.UpdateTask(this.CurrentTask);
                this.ChaptersViewModel.UpdateTask(this.CurrentTask);
                this.MetaDataViewModel.UpdateTask(this.CurrentTask);
              
                // Cleanup
                this.ShowStatusWindow = false;
                this.SourceLabel = this.SelectedTitle?.DisplaySourceName ?? this.ScannedSource?.SourceName;
                this.StatusLabel = Resources.Main_ScanCompleted;
            });
        }

        private void SetupTabs()
        {
            // Setup the Tabs
            if (this.selectedTitle != null)
            {
                this.isSettingPreset = true;
                this.PictureSettingsViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.VideoViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.FiltersViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.AudioViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.SubtitleViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.ChaptersViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.MetaDataViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.SummaryViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.selectedPreset, this.CurrentTask);
                this.isSettingPreset = false;

                TriggerAutonameChange(ChangedOption.Dimensions);
            }
        }

        private void TabStatusChanged(object sender, TabStatusEventArgs e)
        {
            if (this.isSettingPreset)
            {
                return; // Don't process this when we are setting up.
            }

            // Update Preview if needed
            if (e != null && e.TabKey != null && e.TabKey.Equals(TabStatusEventType.FilterType) && this.StaticPreviewViewModel.IsOpen)
            {
                delayedPreviewprocessor.PerformTask(() => this.StaticPreviewViewModel.UpdatePreviewFrame(this.CurrentTask, this.ScannedSource), 1000);
            }

            // Preset Check
            bool matchesPreset = this.PictureSettingsViewModel.MatchesPreset(this.selectedPreset);

            if (!this.SummaryViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.PictureSettingsViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.VideoViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.FiltersViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.AudioViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.SubtitleViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.ChaptersViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            if (!this.MetaDataViewModel.MatchesPreset(this.selectedPreset))
            {
                matchesPreset = false;
            }

            this.IsModifiedPreset = !matchesPreset;

            if (e != null)
            {
                this.TriggerAutonameChange(e.ChangedOption);
            }
        }

        private void TriggerAutonameChange(ChangedOption option)
        {
            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
            {
                return;
            }

            string autonameFormat = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);

            if (string.IsNullOrEmpty(autonameFormat))
            {
                return;
            }

            if (autonameFormat.Contains(Constants.QualityBitrate) && (option == ChangedOption.Bitrate || option == ChangedOption.Quality))
            {
                this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
            }

            if (autonameFormat.Contains(Constants.EncoderBitDepth) && option == ChangedOption.Encoder)
            {
                this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
            }


            if ((autonameFormat.Contains(Constants.StorageWidth) || autonameFormat.Contains(Constants.StorageHeight)) && option == ChangedOption.Dimensions)
            {
                this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SelectedTitle?.DisplaySourceName, this.ScannedSource?.SourceName, this.selectedPreset);
            }
        }

        private string DurationCalculation()
        {
            if (this.selectedTitle == null)
            {
                return "--:--:--";
            }

            double startEndDuration = this.SelectedEndPoint - this.SelectedStartPoint;
            TimeSpan output;

            switch (this.SelectedPointToPoint)
            {
                case PointToPointMode.Chapters:
                    output = this.SelectedTitle.CalculateDuration(this.SelectedStartPoint, this.SelectedEndPoint);
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
                case PointToPointMode.Seconds:
                    output = TimeSpan.FromSeconds(startEndDuration);
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
                case PointToPointMode.Frames:
                    startEndDuration = startEndDuration / selectedTitle.Fps;
                    output = TimeSpan.FromSeconds(Math.Round(startEndDuration, 2));
                    return string.Format("{0:00}:{1:00}:{2:00}", output.Hours, output.Minutes, output.Seconds);
            }

            return "--:--:--";
        }

        private void HandleUpdateCheckResults(UpdateCheckInformation information)
        {
            if (information.NewVersionAvailable)
            {
                this.UpdateAvailable = true;
                this.ProgramStatusLabel = Resources.Main_NewUpdate;
            }
            else if (HandBrakeVersionHelper.IsNightly())
            { 
                int ageLimit = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck);
                if (HandBrakeVersionHelper.NightlyBuildAge() > ageLimit)
                {
                    // Any nightly build older than 30 days is considered old. Encourage users to update.
                    this.UpdateAvailable = false;
                    this.IsOldNightly = true;
                    this.NotifyOfPropertyChange(() => this.IsOldNightly);
                }
            }
        }

        private void OpenAlertWindow(string header, string message)
        {
            this.ShowAlertWindow = true;
            this.AlertWindowHeader = header;
            this.AlertWindowText = message;
        }

        private void SelectDefaultPreset()
        {
            if (this.presetService.GetDefaultPreset() != null)
            {
                this.SelectedPreset = this.presetService.GetDefaultPreset();
            }
        }

        /* Event Handlers */

        private void ScanStatusChanged(object sender, ScanProgressEventArgs e)
        {
            this.SourceLabel = string.Format(Resources.Main_ScanningTitleXOfY, e.CurrentTitle, e.Titles, e.Percentage);
            this.StatusLabel = string.Format(Resources.Main_ScanningTitleXOfY, e.CurrentTitle, e.Titles, e.Percentage);
        }

        private void ScanCompleted(object sender, ScanCompletedEventArgs e)
        {
            this.ShowStatusWindow = false;

            if (e.ScannedSource != null && !e.Cancelled)
            {
                this.ScannedSource = new Source(e.ScannedSource);
            }
            else
            {
                this.ScannedSource = null;
            }

            ThreadHelper.OnUIThread(() =>
            {
                if (e.Successful && this.ScannedSource != null)
                {
                    this.NotifyOfPropertyChange(() => this.ScannedSource);
                    this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);
                    this.HasSource = true;
                    this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault(t => t.MainTitle) ?? this.ScannedSource.Titles.FirstOrDefault();
                }
                else if (!e.Cancelled)
                {
                    this.OpenAlertWindow(Resources.Main_ScanNoTitlesFound, Resources.Main_ScanNoTitlesFoundMessage);
                }

                if (e.Successful)
                {
                    this.SourceLabel = this.SelectedTitle?.DisplaySourceName ?? this.ScannedSource?.SourceName;
                    this.StatusLabel = Resources.Main_ScanCompleted;
                }
                else if (e.Cancelled)
                {
                    this.SourceLabel = Resources.Main_ScanCancelled;
                    this.StatusLabel = Resources.Main_ScanCancelled;
                }
                else
                {
                    this.SourceLabel = Resources.Main_ScanFailed_CheckLog;
                    this.StatusLabel = Resources.Main_ScanFailed_CheckLog;
                }
            });
        }

        private void ScanStared(object sender, EventArgs e)
        {
            ThreadHelper.OnUIThread(
                () =>
                {
                    this.StatusLabel = Resources.Main_ScanningPleaseWait;
                    this.ShowStatusWindow = true;
                });
        }

        private void QueueProcessorJobProcessingStarted(object sender, QueueProgressEventArgs e)
        {
            ThreadHelper.OnUIThread(
               () =>
               {
                   this.ProgramStatusLabel = Resources.Main_PreparingToEncode;
                   this.NotifyOfPropertyChange(() => this.IsEncoding);
               });
        }

        private void QueueCompleted(object sender, EventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.IsEncoding);
            this.NotifyOfPropertyChange(() => this.StartLabel);

            ThreadHelper.OnUIThread(
                () =>
                {
                    string errorDesc = string.Empty;
                    if (this.queueProcessor.ErrorCount > 0)
                    {
                        errorDesc += string.Format(Resources.Main_QueueFinishedErrors, this.queueProcessor.ErrorCount);
                    }

                    this.ProgramStatusLabel = Resources.Main_QueueFinished + errorDesc;
                    this.WindowTitle = Resources.HandBrake_Title;
                    this.notifyIconService.SetTooltip(this.WindowTitle);
                    this.windowsTaskbar.SetNoProgress();
                });
        }

        private void QueueChanged(object sender, EventArgs e)
        {
            ThreadHelper.OnUIThread(
              () =>
              {
                  if (!this.queueProcessor.IsEncoding)
                  {
                      this.ProgramStatusLabel = string.Format(Resources.Main_XEncodesPending, this.queueProcessor.Count);
                  }

                  this.NotifyOfPropertyChange(() => this.IsQueueCountVisible);
                  this.NotifyOfPropertyChange(() => this.QueueCount);
                  this.NotifyOfPropertyChange(() => this.StartLabel);
                  this.NotifyOfPropertyChange(() => this.IsEncoding);

                  if (!this.queueProcessor.IsEncoding && this.IsMultiProcess)
                  {
                      this.IsMultiProcess = false;
                      this.NotifyOfPropertyChange(() => this.IsMultiProcess);
                  }
              });
        }

        private void QueueProcessor_QueuePaused(object sender, EventArgs e)
        {
            ThreadHelper.OnUIThread(
                () =>
                {
                    this.ProgramStatusLabel = Resources.Main_QueuePaused;
                    this.NotifyOfPropertyChange(() => this.IsQueueCountVisible);
                    this.NotifyOfPropertyChange(() => this.QueueCount);
                    this.NotifyOfPropertyChange(() => this.StartLabel);
                    this.NotifyOfPropertyChange(() => this.IsEncoding);

                    this.windowsTaskbar.SetPaused();
                });
        }
        
        private void QueueProcessor_QueueJobStatusChanged(object sender, EventArgs e)
        {
            List<QueueProgressStatus> queueJobStatuses = this.queueProcessor.GetQueueProgressStatus();
            string jobsPending = "   " + string.Format(Resources.Main_JobsPending_addon, this.queueProcessor.Count);


            ThreadHelper.OnUIThread(() =>
            {
                if (queueJobStatuses.Count == 0)
                {
                    this.ProgramStatusLabel = Resources.Main_QueueFinished;
                    this.NotifyOfPropertyChange(() => this.IsEncoding);
                    this.WindowTitle = Resources.HandBrake_Title;
                    this.notifyIconService.SetTooltip(this.WindowTitle);

                    this.IsMultiProcess = false;
                    this.NotifyOfPropertyChange(() => this.IsMultiProcess);
                    this.windowsTaskbar.SetNoProgress();
                }
            });

            if (this.queueProcessor.IsPaused)
            {
                return;
            }

            ThreadHelper.OnUIThread(
                () =>
                {
                    if (queueJobStatuses.Count == 1)
                    {
                        QueueProgressStatus status = queueJobStatuses.First();
                        this.ProgramStatusLabel = status.JobStatus.Replace(Environment.NewLine, " ") + jobsPending;

                        int percent;
                        int.TryParse(Math.Round(status.ProgressValue).ToString(CultureInfo.InvariantCulture), out percent);

                        if (this.lastEncodePercentage != percent)
                        {
                            this.windowsTaskbar.SetTaskBarProgress(percent);
                        }

                        this.lastEncodePercentage = percent;
                        this.ProgressPercentage = percent;
                        this.NotifyOfPropertyChange(() => this.ProgressPercentage);

                        if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowStatusInTitleBar))
                        {
                            this.WindowTitle = string.Format(Resources.WindowTitleStatus, Resources.HandBrake_Title, String.Format("{0:##0.0}", Math.Round(status.ProgressValue, 1)), status.Task, status.TaskCount);
                            this.notifyIconService.SetTooltip(string.Format(Resources.TaskTrayStatusTitle, Resources.HandBrake_Title));
                        }

                        this.IsMultiProcess = false;
                        this.NotifyOfPropertyChange(() => this.IsMultiProcess);
                    }
                    else if (queueJobStatuses.Count > 1)
                    {
                        this.windowsTaskbar.SetNoProgress();
                        this.ProgramStatusLabel = string.Format(Resources.Main_QueueMultiJobStatus, this.queueProcessor.CompletedCount, Environment.NewLine, queueJobStatuses.Count, this.queueProcessor.Count);

                        this.notifyIconService.SetTooltip(string.Format(Resources.TaskTrayStatusManyTitle, Resources.HandBrake_Title, queueJobStatuses.Count));
                        this.IsMultiProcess = true;
                        this.NotifyOfPropertyChange(() => this.IsMultiProcess);
                    }
                });
        }

        private void UserSettingServiceSettingChanged(object sender, SettingChangedEventArgs e)
        {
            switch (e.Key)
            {
                case UserSettingConstants.WhenCompleteAction:
                    this.QueueViewModel.WhenDone(this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction), false);
                    break;

                case UserSettingConstants.ShowAddAllToQueue:
                case UserSettingConstants.ShowAddSelectionToQueue:
                    this.NotifyOfPropertyChange(() => this.ShowAddAllToQueue);
                    this.NotifyOfPropertyChange(() => this.ShowAddSelectionToQueue);
                    this.NotifyOfPropertyChange(() => this.ShowAddAllMenuName);
                    this.NotifyOfPropertyChange(() => this.ShowAddSelectionMenuName);
                    break;
            }
        }

        private void SummaryViewModel_OutputFormatChanged(object sender, OutputFormatChangedEventArgs e)
        {
            if (!string.IsNullOrEmpty(e.Extension))
            {
                this.Destination = Path.ChangeExtension(this.Destination, e.Extension);
            }

            this.VideoViewModel.RefreshTask();
            this.AudioViewModel.RefreshTask(this.CurrentTask.OutputFormat);
            this.SubtitleViewModel.RefreshTask();
        }
    }
}