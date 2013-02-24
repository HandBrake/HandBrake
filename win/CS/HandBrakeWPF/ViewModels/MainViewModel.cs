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
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Windows;
    using System.Windows.Media.Imaging;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Factories;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using Microsoft.Win32;

    using Ookii.Dialogs.Wpf;

    using Image = System.Windows.Controls.Image;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// The preset service
        /// </summary>
        private readonly IPresetService presetService;

        /// <summary>
        /// The Error Service Backing field.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The Shell View Model
        /// </summary>
        private readonly IShellViewModel shellViewModel;

        /// <summary>
        /// Backing field for the update serivce.
        /// </summary>
        private readonly IUpdateService updateService;

        /// <summary>
        /// The drive detect service.
        /// </summary>
        private readonly IDriveDetectService driveDetectService;

        /// <summary>
        /// Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Source Scan Service.
        /// </summary>
        private readonly IScanServiceWrapper scanService;

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IEncodeServiceWrapper encodeService;

        /// <summary>
        /// HandBrakes Main Window Title
        /// </summary>
        private string windowName;

        /// <summary>
        /// The Source Label
        /// </summary>
        private string sourceLabel;

        /// <summary>
        /// The Selected Output Format Backing Field
        /// </summary>
        private OutputFormat selectedOutputFormat;

        /// <summary>
        /// Is a MKV file backing field
        /// </summary>
        private bool isMkv;

        /// <summary>
        /// Support Hardware Decoding
        /// </summary>
        private bool supportHardwareDecoding;

        /// <summary>
        /// Support OpenCL
        /// </summary>
        private bool supportOpenCL;
        /// The Toolbar Status Label
        /// </summary>
        private string statusLabel;

        /// <summary>
        /// Program Status Label
        /// </summary>
        private string programStatusLabel;

        /// <summary>
        /// Backing field for the scanned source.
        /// </summary>
        private Source scannedSource;

        /// <summary>
        /// Backing field for the selected title.
        /// </summary>
        private Title selectedTitle;

        /// <summary>
        /// Backing field for duration
        /// </summary>
        private string duration;

        /// <summary>
        /// Is Encoding Backing Field
        /// </summary>
        private bool isEncoding;

        /// <summary>
        /// An Indicated to show the status window
        /// </summary>
        private bool showStatusWindow;

        /// <summary>
        /// Backing field for the selected preset.
        /// </summary>
        private Preset selectedPreset;

        /// <summary>
        /// Queue Edit Task
        /// </summary>
        private EncodeTask queueEditTask;

        /// <summary>
        /// The Source Menu Backing Field
        /// </summary>
        private IEnumerable<SourceMenuItem> sourceMenu;
        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class.
        /// The viewmodel for HandBrakes main window.
        /// </summary>
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
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="shellViewModel">
        /// The shell View Model.
        /// </param>
        /// <param name="updateService">
        /// The update Service.
        /// </param>
        /// <param name="driveDetectService">
        /// The drive Detect Service.
        /// </param>
        /// <param name="notificationService">
        /// The notification Service.
        /// *** Leave in Constructor. ***  TODO find out why?
        /// </param>
        public MainViewModel(IUserSettingService userSettingService, IScanServiceWrapper scanService, IEncodeServiceWrapper encodeService, IPresetService presetService,
            IErrorService errorService, IShellViewModel shellViewModel, IUpdateService updateService, IDriveDetectService driveDetectService, INotificationService notificationService)
        {
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.shellViewModel = shellViewModel;
            this.updateService = updateService;
            this.driveDetectService = driveDetectService;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueProcessor>();

            // Setup Properties
            this.WindowTitle = "HandBrake";
            this.CurrentTask = new EncodeTask();
            this.CurrentTask.PropertyChanged += this.CurrentTask_PropertyChanged;
            this.ScannedSource = new Source();

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
            this.userSettingService.SettingChanged += this.UserSettingServiceSettingChanged;

            this.Presets = this.presetService.Presets;
            this.CancelScanCommand = new CancelScanCommand(this.scanService);
        }

        #region View Model Properties
        /// <summary>
        /// Gets or sets PictureSettingsViewModel.
        /// </summary>
        public IPictureSettingsViewModel PictureSettingsViewModel { get; set; }

        /// <summary>
        /// Gets or sets AudioViewModel.
        /// </summary>
        public IAudioViewModel AudioViewModel { get; set; }

        /// <summary>
        /// Gets or sets SubtitleViewModel.
        /// </summary>
        public ISubtitlesViewModel SubtitleViewModel { get; set; }

        /// <summary>
        /// Gets or sets ChaptersViewModel.
        /// </summary>
        public IChaptersViewModel ChaptersViewModel { get; set; }

        /// <summary>
        /// Gets or sets AdvancedViewModel.
        /// </summary>
        public IAdvancedViewModel AdvancedViewModel { get; set; }

        /// <summary>
        /// Gets or sets VideoViewModel.
        /// </summary>
        public IVideoViewModel VideoViewModel { get; set; }

        /// <summary>
        /// Gets or sets FiltersViewModel.
        /// </summary>
        public IFiltersViewModel FiltersViewModel { get; set; }

        #endregion

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
                if (!Equals(this.windowName, value))
                {
                    this.windowName = value;
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
        /// Gets or sets the Program Status Toolbar Label
        /// This indicates the status of HandBrake
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
        /// Gets or sets the source menu.
        /// </summary>
        public IEnumerable<SourceMenuItem> SourceMenu
        {
            get
            {
                return this.sourceMenu;
            }
            set
            {
                this.sourceMenu = value;
                this.NotifyOfPropertyChange(() => SourceMenu);
            }
        }

        /// <summary>
        /// Gets or sets Presets.
        /// </summary>
        public IEnumerable<Preset> Presets { get; set; }

        /// <summary>
        /// Gets or sets SelectedPreset.
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

                if (this.SelectedPreset != null)
                {
                    // Main Window Settings
                    this.CurrentTask.LargeFile = selectedPreset.Task.LargeFile;
                    this.CurrentTask.OptimizeMP4 = selectedPreset.Task.OptimizeMP4;
                    this.CurrentTask.IPod5GSupport = selectedPreset.Task.IPod5GSupport;
                    this.CurrentTask.OpenCLSupport = selectedPreset.Task.OpenCLSupport;
                    this.CurrentTask.HWDSupport = selectedPreset.Task.HWDSupport;
                    this.SelectedOutputFormat = selectedPreset.Task.OutputFormat;

                    // Tab Settings
                    this.PictureSettingsViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.VideoViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.FiltersViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.AudioViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.SubtitleViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.ChaptersViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.AdvancedViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);

                    // Do this again to force an update for m4v/mp4 selection
                    this.SelectedOutputFormat = selectedPreset.Task.OutputFormat;
                }

                this.NotifyOfPropertyChange(() => this.SelectedPreset);
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
                if (!Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                    this.NotifyOfPropertyChange("SourceLabel");
                }
            }
        }

        /// <summary>
        /// Gets SourceName.
        /// </summary>
        public string SourceName
        {
            get
            {
                // Sanity Check
                if (ScannedSource == null || ScannedSource.ScanPath == null)
                {
                    return string.Empty;
                }

                // The title that is selected has a source name. This means it's part of a batch scan.
                if (selectedTitle != null && !string.IsNullOrEmpty(selectedTitle.SourceName))
                {
                    return Path.GetFileNameWithoutExtension(selectedTitle.SourceName);
                }

                // Check if we have a Folder, if so, check if it's a DVD / Bluray drive and get the label.
                if (ScannedSource.ScanPath.EndsWith("\\"))
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
                    return Path.GetFileNameWithoutExtension(this.ScannedSource.ScanPath);

                return Path.GetFileNameWithoutExtension(Path.GetDirectoryName(this.ScannedSource.ScanPath));
            }
        }

        /// <summary>
        /// Gets RangeMode.
        /// </summary>
        public IEnumerable<PointToPointMode> RangeMode
        {
            get
            {
                return new List<PointToPointMode>
                    {
                        PointToPointMode.Chapters, PointToPointMode.Seconds, PointToPointMode.Frames 
                    };
            }
        }

        /// <summary>
        /// Gets a value indicating whether ShowTextEntryForPointToPointMode.
        /// </summary>
        public bool ShowTextEntryForPointToPointMode
        {
            get
            {
                return this.SelectedPointToPoint != PointToPointMode.Chapters;
            }
        }

        /// <summary>
        /// Gets StartEndRangeItems.
        /// </summary>
        public IEnumerable<int> StartEndRangeItems
        {
            get
            {
                if (this.SelectedTitle == null)
                {
                    return null;
                }

                return this.SelectedTitle.Chapters.Select(item => item.ChapterNumber).Select(dummy => dummy).ToList();
            }
        }

        /// <summary>
        /// Gets Angles.
        /// </summary>
        public IEnumerable<int> Angles
        {
            get
            {
                if (this.SelectedTitle == null)
                {
                    return null;
                }

                List<int> items = new List<int>();
                for (int i = 1; i <= this.selectedTitle.AngleCount + 1; i++)
                {
                    items.Add(i);
                }
                return items;
            }
        }

        /// <summary>
        /// Gets or sets Duration.
        /// </summary>
        public string Duration
        {
            get
            {
                return string.IsNullOrEmpty(duration) ? "--:--:--" : duration;
            }
            set
            {
                duration = value;
                this.NotifyOfPropertyChange("Duration");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsEncoding.
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
        /// Gets or sets a value indicating whether ShowStatusWindow.
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
        /// Gets or sets a value indicating whether IsMkv.
        /// </summary>
        public bool IsMkv
        {
            get
            {
                return this.isMkv;
            }
            set
            {
                this.isMkv = value;
                this.NotifyOfPropertyChange(() => this.IsMkv);
            }
        }
  
        /// <summary>
        /// Gets or sets a value indicating whether SupportHardwareDecoding.
        /// </summary>
        public bool SupportHardwareDecoding
        {
            get
            {
                return this.supportHardwareDecoding;
            }
            set
            {
                this.supportHardwareDecoding = value;
                this.NotifyOfPropertyChange("SupportHardwareDecoding");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether SupportHardwareDecoding.
        /// </summary>
        public bool SupportOpenCL
        {
            get
            {
                return this.supportOpenCL;
            }
            set
            {
                this.supportOpenCL = value;
                this.NotifyOfPropertyChange("SupportOpenCL");
            }
        }
        /// Gets RangeMode.
        /// </summary>
        public IEnumerable<OutputFormat> OutputFormats
        {
            get
            {
                return new List<OutputFormat>
                    {
                        OutputFormat.Mp4, OutputFormat.Mkv
                    };
            }
        }

        /// <summary>
        /// Gets a value indicating whether show debug menu.
        /// </summary>
        public bool ShowDebugMenu
        {
            get
            {
                return this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableDebugFeatures);
            }
        }

        /// <summary>
        /// Gets or sets the cancel scan command.
        /// </summary>
        public CancelScanCommand CancelScanCommand { get; set; }

        /// <summary>
        /// Gets or sets Destination.
        /// </summary>
        public string Destination
        {
            get
            {
                return this.CurrentTask.Destination;
            }
            set
            {
                this.CurrentTask.Destination = value;
                this.NotifyOfPropertyChange(() => this.Destination);
            }
        }

        /// <summary>
        /// Gets or sets SelectedTitle.
        /// </summary>
        public Title SelectedTitle
        {
            get
            {
                return this.selectedTitle;
            }
            set
            {
                if (!object.Equals(this.selectedTitle, value))
                {
                    this.selectedTitle = value;

                    if (this.selectedTitle == null)
                    {
                        return;
                    }

                    // Use the Path on the Title, or the Source Scan path if one doesn't exist.
                    this.SourceLabel = this.SourceName;
                    this.CurrentTask.Source = !string.IsNullOrEmpty(this.selectedTitle.SourceName) ? this.selectedTitle.SourceName : this.ScannedSource.ScanPath;
                    this.CurrentTask.Title = value.TitleNumber;
                    this.NotifyOfPropertyChange(() => this.StartEndRangeItems);
                    this.NotifyOfPropertyChange(() => this.SelectedTitle);
                    this.NotifyOfPropertyChange(() => this.Angles);

                    // Default the Start and End Point dropdowns
                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = this.selectedTitle.Chapters != null &&
                                            this.selectedTitle.Chapters.Count != 0
                                                ? this.selectedTitle.Chapters.Last().ChapterNumber
                                                : 1;

                    this.SelectedPointToPoint = PointToPointMode.Chapters;
                    this.SelectedAngle = 1;

                    if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
                    }
                    this.NotifyOfPropertyChange(() => this.CurrentTask);

                    this.Duration = this.selectedTitle.Duration.ToString();

                    // Setup the tab controls
                    this.SetupTabs();
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedAngle.
        /// </summary>
        public int SelectedAngle
        {
            get
            {
                return this.CurrentTask.StartPoint;
            }

            set
            {
                this.CurrentTask.Angle = value;
                this.NotifyOfPropertyChange(() => this.SelectedAngle);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether is timespan range.
        /// </summary>
        public bool IsTimespanRange { get; set; }

        /// <summary>
        /// Gets or sets SelectedStartPoint.
        /// </summary>
        public int SelectedStartPoint
        {
            get
            {
                return this.CurrentTask.StartPoint;
            }
            set
            {
                this.CurrentTask.StartPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedStartPoint);
                this.Duration = this.DurationCalculation();

                if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.ScannedSource.ScanPath != null)
                {
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedEndPoint.
        /// </summary>
        public int SelectedEndPoint
        {
            get
            {
                return this.CurrentTask.EndPoint;
            }
            set
            {
                this.CurrentTask.EndPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedEndPoint);
                this.Duration = this.DurationCalculation();

                if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.ScannedSource.ScanPath != null)
                {
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedPointToPoint.
        /// </summary>
        public PointToPointMode SelectedPointToPoint
        {
            get
            {
                return this.CurrentTask.PointToPointMode;
            }
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
                    this.SelectedEndPoint = selectedTitle.Chapters.Last().ChapterNumber;
                } 
                else if (value == PointToPointMode.Seconds)
                {
                    if (this.selectedTitle == null)
                    {
                        return;
                    }


                    this.SelectedStartPoint = 0;

                    int timeInSeconds;
                    if (int.TryParse(selectedTitle.Duration.TotalSeconds.ToString(CultureInfo.InvariantCulture), out timeInSeconds))
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
                    if (int.TryParse(estimatedTotalFrames.ToString(CultureInfo.InvariantCulture), out totalFrames))
                    {
                        this.SelectedEndPoint = totalFrames;
                    }

                    this.IsTimespanRange = false;
                    this.NotifyOfPropertyChange(() => this.IsTimespanRange);
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedOutputFormat.
        /// </summary>
        public OutputFormat SelectedOutputFormat
        {
            get
            {
                return this.selectedOutputFormat;
            }
            set
            {
                this.selectedOutputFormat = value;
                this.CurrentTask.OutputFormat = value;
                this.NotifyOfPropertyChange(() => SelectedOutputFormat);
                this.NotifyOfPropertyChange(() => this.CurrentTask.OutputFormat);
                this.NotifyOfPropertyChange(() => IsMkv);
                this.NotifyOfPropertyChange(() => SupportHardwareDecoding);
                this.NotifyOfPropertyChange(() => SupportOpenCL);
                this.SetExtension(string.Format(".{0}", this.selectedOutputFormat.ToString().ToLower())); // TODO, tidy up

                this.VideoViewModel.RefreshTask();
                this.AudioViewModel.RefreshTask();
            }
        }

        /// <summary>
        /// Gets a value indicating whether show advanced tab.
        /// </summary>
        public bool ShowAdvancedTab
        {
            get
            {
                return this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedTab);
            }
        }

        #endregion

        #region Load and Shutdown Handling
        /// <summary>
        /// Initialise this view model.
        /// </summary>
        public override void OnLoad()
        {
            // Check the CLI Executable.
            CliCheckHelper.CheckCLIVersion();

            // Perform an update check if required
            this.updateService.PerformStartupUpdateCheck(this.HandleUpdateCheckResults);

            // Setup the presets.
            if (this.presetService.CheckIfPresetsAreOutOfDate())
                if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
                    this.errorService.ShowMessageBox("HandBrake has determined your built-in presets are out of date... These presets will now be updated.",
                                    "Preset Update", MessageBoxButton.OK, MessageBoxImage.Information);

            // Queue Recovery
            QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService);

            this.SelectedPreset = this.presetService.DefaultPreset;

            // Populate the Source menu with drives.
            this.SourceMenu = this.GenerateSourceMenu();

            this.driveDetectService.StartDetection(this.DriveTrayChanged);

            // Log Cleaning
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs))
            {
                Thread clearLog = new Thread(() => GeneralUtilities.ClearLogFiles(30));
                clearLog.Start();
            }
        }

        /// <summary>
        /// Shutdown this View
        /// </summary>
        public void Shutdown()
        {
            // Shutdown Service
            this.driveDetectService.Close();

            this.scanService.Shutdown();
            this.encodeService.Shutdown();

            // Unsubscribe from Events.
            this.scanService.ScanStared -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueChanged;
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
            this.userSettingService.SettingChanged -= this.UserSettingServiceSettingChanged;
        }

        #endregion

        #region Menu and Taskbar

        /// <summary>
        /// Open the About Window
        /// </summary>
        public void OpenAboutApplication()
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.About);
        }

        /// <summary>
        /// Open the Options Window
        /// </summary>
        public void OpenOptionsWindow()
        {
            this.shellViewModel.DisplayWindow(ShellWindow.OptionsWindow);
        }

        /// <summary>
        /// Open the Log Window
        /// </summary>
        public void OpenLogWindow()
        {
            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(LogView));

            if (window != null)
            {
                window.Activate();
            }
            else
            {
                this.WindowManager.ShowWindow(IoC.Get<ILogViewModel>());
            }
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenQueueWindow()
        {
            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(QueueView));

            if (window != null)
            {
                window.Activate();
            }
            else
            {
                this.WindowManager.ShowWindow(IoC.Get<IQueueViewModel>());
            }
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenPreviewWindow()
        {
            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(PreviewView));
            IPreviewViewModel viewModel = IoC.Get<IPreviewViewModel>();

            if (window != null)
            {
                viewModel.Task = this.CurrentTask;
                window.Activate();
            }
            else
            {
                viewModel.Task = this.CurrentTask;
                this.WindowManager.ShowWindow(viewModel);
            }
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
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.Updates);
        }

        /// <summary>
        /// Add the current task to the queue.
        /// </summary>
        public void AddToQueue()
        {
            if (this.ScannedSource == null || string.IsNullOrEmpty(this.ScannedSource.ScanPath) || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox("You must first scan a source and setup your job before adding to the queue.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            QueueTask task = new QueueTask { Task = new EncodeTask(this.CurrentTask) };
            if (!this.queueProcessor.CheckForDestinationPathDuplicates(task.Task.Destination))
            {
                this.queueProcessor.Add(task);
            }
            else
            {
                this.errorService.ShowMessageBox("There are jobs on the queue with the same destination path. Please choose a different path for this job.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            }

            if (!this.IsEncoding)
            {
                this.ProgramStatusLabel = string.Format("{0} Encodes Pending", this.queueProcessor.Count);
            }
        }

        /// <summary>
        /// Add all Items to the queue
        /// </summary>
        public void AddAllToQueue()
        {
            if (this.ScannedSource == null || this.ScannedSource.Titles == null || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox("You must first scan a source and setup your job before adding to the queue.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!AutoNameHelper.IsAutonamingEnabled())
            {
                this.errorService.ShowMessageBox("You must turn on automatic file naming in preferences before you can add to the queue.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            foreach (Title title in this.ScannedSource.Titles)
            {
                this.SelectedTitle = title;
                this.AddToQueue();
            }
        }

        /// <summary>
        /// The add selection to queue.
        /// </summary>
        public void AddSelectionToQueue()
        {
            if (this.ScannedSource == null || this.ScannedSource.Titles == null || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox("You must first scan a source and setup your job before adding to the queue.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!AutoNameHelper.IsAutonamingEnabled())
            {
                this.errorService.ShowMessageBox("You must turn on automatic file naming in preferences before you can add to the queue.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(QueueSelectionViewModel));
            IQueueSelectionViewModel viewModel = IoC.Get<IQueueSelectionViewModel>();

            viewModel.Setup(this.ScannedSource, this.SourceName);

            if (window != null)
            {
                window.Activate();
            }
            else
            {
                this.WindowManager.ShowWindow(viewModel);
            }
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
        /// Folder Scan
        /// </summary>
        public void FolderScanTitleSpecific()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true };
            dialog.ShowDialog();

            if (string.IsNullOrEmpty(dialog.SelectedPath))
            {
                return;
            }

            ITitleSpecificViewModel titleSpecificView = IoC.Get<ITitleSpecificViewModel>();
            this.WindowManager.ShowDialog(titleSpecificView);

            if (titleSpecificView.SelectedTitle.HasValue)
            {
                this.StartScan(dialog.SelectedPath, titleSpecificView.SelectedTitle.Value);
            }
        }

        /// <summary>
        /// File Scan
        /// </summary>
        public void FileScanTitleSpecific()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "All files (*.*)|*.*" };
            dialog.ShowDialog();

            if (string.IsNullOrEmpty(dialog.FileName))
            {
                return;
            }

            ITitleSpecificViewModel titleSpecificView = IoC.Get<ITitleSpecificViewModel>();
            this.WindowManager.ShowDialog(titleSpecificView);

            if (titleSpecificView.SelectedTitle.HasValue)
            {
                this.StartScan(dialog.FileName, titleSpecificView.SelectedTitle.Value);
            }
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
            if (this.queueProcessor.IsProcessing)
            {
                this.errorService.ShowMessageBox("HandBrake is already encoding.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // Check if we already have jobs, and if we do, just start the queue.
            if (this.queueProcessor.Count != 0)
            {
                this.queueProcessor.Start();
                return;
            }

            // Otherwise, perform Santiy Checking then add to the queue and start if everything is ok.
            if (this.SelectedTitle == null)
            {
                this.errorService.ShowMessageBox("You must first scan a source.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (string.IsNullOrEmpty(this.Destination))
            {
                this.errorService.ShowMessageBox("The Destination field was empty.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (File.Exists(this.Destination))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox("The current file already exists, do you wish to overwrite it?", "Question", MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            // Create the Queue Task and Start Processing
            QueueTask task = new QueueTask
                {
                    Task = new EncodeTask(this.CurrentTask),
                    CustomQuery = false
                };
            this.queueProcessor.Add(task);
            this.queueProcessor.Start();
            this.IsEncoding = true;
        }

        /// <summary>
        /// Edit a Queue Task
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void EditQueueJob(EncodeTask task)
        {
            // Rescan the source to make sure it's still valid
            this.queueEditTask = task;
            this.scanService.Scan(task.Source, task.Title, this.UserSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount), QueueEditAction);
        }

        /// <summary>
        /// Pause an Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.Pause();
        }

        /// <summary>
        /// Stop an Encode.
        /// </summary>
        public void StopEncode()
        {
            this.queueProcessor.Pause();
            this.encodeService.Stop();
        }

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        public void ExitApplication()
        {
            Application.Current.Shutdown();
        }

        /// <summary>
        /// DEBUG: Show CLI Query for settings+6
        /// </summary>
        public void ShowCliQuery()
        {
            this.errorService.ShowMessageBox(
                QueryGeneratorUtility.GenerateQuery(this.CurrentTask,
                userSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount),
                userSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity),
                userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav)),
                "CLI Query",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
        }

        /// <summary>
        /// The debug scan log.
        /// </summary>
        public void DebugScanLog()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog();

            dialog.ShowDialog();

            if (File.Exists(dialog.FileName))
            {
                this.scanService.DebugScanLog(dialog.FileName);
            }
        }

        #endregion

        #region Main Window Public Methods

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
                string[] fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
                if (fileNames != null && fileNames.Any() && (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])))
                {
                    this.StartScan(fileNames[0], 0);
                }
            }

            e.Handled = true;
        }

        /// <summary>
        /// The Destination Path
        /// </summary>
        public void BrowseDestination()
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog
                {
                    Filter = "mp4|*.mp4;*.m4v|mkv|*.mkv",
                    CheckPathExists = true,
                    AddExtension = true,
                    DefaultExt = ".mp4",
                    OverwritePrompt = true,
                    FilterIndex = this.CurrentTask.OutputFormat == OutputFormat.Mkv ? 1 : 0,
                };

            if (this.CurrentTask != null && !string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                if (Directory.Exists(Path.GetDirectoryName(this.CurrentTask.Destination)))
                {
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(this.CurrentTask.Destination) + "\\";
                }

                saveFileDialog.FileName = Path.GetFileName(this.CurrentTask.Destination);
            }

            saveFileDialog.ShowDialog();
            this.Destination = saveFileDialog.FileName;

            // Set the Extension Dropdown. This will also set Mp4/m4v correctly.
            if (!string.IsNullOrEmpty(saveFileDialog.FileName))
            {
                switch (Path.GetExtension(saveFileDialog.FileName))
                {
                    case ".mkv":
                        this.SelectedOutputFormat = OutputFormat.Mkv;
                        break;
                    case ".mp4":
                        this.SelectedOutputFormat = OutputFormat.Mp4;
                        break;
                    case ".m4v":
                        this.SelectedOutputFormat = OutputFormat.M4V;
                        break;
                }

                this.NotifyOfPropertyChange(() => this.CurrentTask);
            }
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void PresetAdd()
        {
            IAddPresetViewModel presetViewModel = IoC.Get<IAddPresetViewModel>();
            presetViewModel.Setup(this.CurrentTask);
            this.WindowManager.ShowWindow(presetViewModel);
        }

        /// <summary>
        /// Update a selected preset.
        /// </summary>
        public void PresetUpdate()
        {
            if (this.SelectedPreset == null)
            {
                this.errorService.ShowMessageBox(
                    "Please select a preset to update.", "No Preset selected", MessageBoxButton.OK, MessageBoxImage.Warning);

                return;
            }

            if (this.SelectedPreset.IsBuildIn)
            {
                this.errorService.ShowMessageBox(
                    "You can not modify built in presets. Please select one of your own presets.", "No Preset selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (this.errorService.ShowMessageBox("Are you sure you wish to update the selected preset?", "Are you sure?", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                this.SelectedPreset.Task = new EncodeTask(this.CurrentTask);
                this.presetService.Update(this.SelectedPreset);

                this.errorService.ShowMessageBox(
                        "The Preset has now been updated with your current settings.", "Preset Updated", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        /// <summary>
        /// Remove a Preset
        /// </summary>
        public void PresetRemove()
        {
            if (this.selectedPreset != null)
            {
                this.presetService.Remove(this.selectedPreset);
            }
            else
            {
                MessageBox.Show("Please select a preset.", "Presets", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Set a default preset
        /// </summary>
        public void PresetSetDefault()
        {
            if (this.selectedPreset != null)
            {
                this.presetService.SetDefault(this.selectedPreset);
                MessageBox.Show(string.Format("New Default Preset Set: {0}", this.selectedPreset.Name), "Presets", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            else
            {
                MessageBox.Show("Please select a preset.", "Presets", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Import a Preset
        /// </summary>
        public void PresetImport()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "Plist (*.plist)|*.plist", CheckFileExists = true };
            dialog.ShowDialog();
            string filename = dialog.FileName;

            if (!string.IsNullOrEmpty(filename))
            {
                PList plist = new PList(filename);
                Preset preset = PlistPresetFactory.CreatePreset(plist);

                if (this.presetService.CheckIfPresetExists(preset.Name))
                {
                    if (!presetService.CanUpdatePreset(preset.Name))
                    {
                        MessageBox.Show(
                            "You can not import a preset with the same name as a built-in preset.",
                            "Error",
                            MessageBoxButton.OK,
                            MessageBoxImage.Error);
                        return;
                    }

                    MessageBoxResult result =
                        MessageBox.Show(
                            "This preset appears to already exist. Would you like to overwrite it?",
                            "Overwrite preset?",
                            MessageBoxButton.YesNo,
                            MessageBoxImage.Warning);
                    if (result == MessageBoxResult.Yes)
                    {
                        presetService.Update(preset);
                    }
                }
                else
                {
                    presetService.Add(preset);
                }

                this.NotifyOfPropertyChange(() => this.Presets);
            }
        }

        /// <summary>
        /// Export a Preset
        /// </summary>
        public void PresetExport()
        {
            VistaSaveFileDialog savefiledialog = new VistaSaveFileDialog { Filter = "plist|*.plist", CheckPathExists = true, AddExtension = true };
            if (this.selectedPreset != null)
            {
                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (filename != null)
                {
                    PlistUtility.Export(savefiledialog.FileName, this.selectedPreset, userSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild).ToString(CultureInfo.InvariantCulture));
                }
            }
            else
            {
                MessageBox.Show("Please select a preset.", "Presets", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Reset built-in presets
        /// </summary>
        public void PresetReset()
        {
            this.presetService.UpdateBuiltInPresets();
            this.NotifyOfPropertyChange("Presets");
            this.SelectedPreset = this.presetService.DefaultPreset;
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
                this.scanService.Scan(filename, title, this.UserSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount), null);
            }
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Update all the UI Components to allow the user to edit their previous settings.
        /// </summary>
        /// <param name="successful">
        /// The successful.
        /// </param>
        private void QueueEditAction(bool successful)
        {
            Execute.OnUIThread(() =>
                {
                    // Copy all the Scan data into the UI
                    this.scanService.SouceData.CopyTo(this.ScannedSource);
                    this.NotifyOfPropertyChange(() => this.ScannedSource);
                    this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);

                    // Select the Users Title
                    this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault(t => t.TitleNumber == this.CurrentTask.Title);
                    this.CurrentTask = new EncodeTask(queueEditTask);
                    this.NotifyOfPropertyChange(() => this.CurrentTask);

                    // Update the Main Window
                    this.NotifyOfPropertyChange(() => this.Destination);
                    this.NotifyOfPropertyChange(() => this.SelectedStartPoint);
                    this.NotifyOfPropertyChange(() => this.SelectedEndPoint);
                    this.NotifyOfPropertyChange(() => this.SelectedAngle);
                    this.NotifyOfPropertyChange(() => this.SelectedPointToPoint);
                    this.NotifyOfPropertyChange(() => this.SelectedOutputFormat);
                    this.NotifyOfPropertyChange(() => IsMkv);

                    // Update the Tab Controls
                    this.PictureSettingsViewModel.UpdateTask(this.CurrentTask);
                    this.VideoViewModel.UpdateTask(this.CurrentTask);
                    this.FiltersViewModel.UpdateTask(this.CurrentTask);
                    this.AudioViewModel.UpdateTask(this.CurrentTask);
                    this.SubtitleViewModel.UpdateTask(this.CurrentTask);
                    this.ChaptersViewModel.UpdateTask(this.CurrentTask);
                    this.AdvancedViewModel.UpdateTask(this.CurrentTask);

                    // Tell the Preivew Window
                    IPreviewViewModel viewModel = IoC.Get<IPreviewViewModel>();
                    viewModel.Task = this.CurrentTask;

                    // Cleanup
                    this.ShowStatusWindow = false;
                    this.SourceLabel = this.SourceName;
                    this.StatusLabel = "Scan Completed";
                });
        }

        /// <summary>
        /// Make sure the correct file extension is set based on user preferences and setup the GUI for the file container selected.
        /// </summary>
        /// <param name="newExtension">
        /// The new extension.
        /// </param>
        private void SetExtension(string newExtension)
        {
            // Make sure the output extension is set correctly based on the users preferences and selection.
            if (newExtension == ".mp4" || newExtension == ".m4v")
            {
                switch (this.UserSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                {
                    case 0: // Auto
                        newExtension = this.CurrentTask.RequiresM4v ? ".m4v" : ".mp4";
                        break;
                    case 1: // MP4
                        newExtension = ".mp4";
                        break;
                    case 2: // M4v
                        newExtension = ".m4v";
                        break;
                }

                this.IsMkv = false;
            }

            // Now disable controls that are not required. The Following are for MP4 only!
            if (newExtension == ".mkv")
            {
                this.IsMkv = true;
                this.CurrentTask.LargeFile = false;
                this.CurrentTask.OptimizeMP4 = false;
                this.CurrentTask.IPod5GSupport = false;
            }

            // Update The browse file extension display
            if (Path.HasExtension(newExtension))
            {
                this.Destination = Path.ChangeExtension(this.Destination, newExtension);
            }

            // Update the UI Display
            this.NotifyOfPropertyChange(() => this.CurrentTask);
        }

        /// <summary>
        /// Setup the UI tabs. Passes in any relevant models for setup.
        /// </summary>
        private void SetupTabs()
        {
            // Setup the Tabs
            if (this.selectedTitle != null)
            {
                this.PictureSettingsViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.VideoViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.FiltersViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.AudioViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.SubtitleViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.ChaptersViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.AdvancedViewModel.SetSource(this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
            }
        }

        /// <summary>
        /// Calculate the duration between the end and start point
        /// </summary>
        /// <returns>
        /// The duration calculation.
        /// </returns>
        private string DurationCalculation()
        {
            if (this.selectedTitle == null)
            {
                return "--:--:--";
            }

            double startEndDuration = this.SelectedEndPoint - this.SelectedStartPoint;
            switch (this.SelectedPointToPoint)
            {
                case PointToPointMode.Chapters:
                    return this.SelectedTitle.CalculateDuration(this.SelectedStartPoint, this.SelectedEndPoint).ToString();
                case PointToPointMode.Seconds:
                    return TimeSpan.FromSeconds(startEndDuration).ToString();
                case PointToPointMode.Frames:
                    startEndDuration = startEndDuration / selectedTitle.Fps;
                    return TimeSpan.FromSeconds(startEndDuration).ToString();
            }

            return "--:--:--";
        }

        /// <summary>
        /// Handle Update Check Results
        /// </summary>
        /// <param name="information">
        /// The information.
        /// </param>
        private void HandleUpdateCheckResults(UpdateCheckInformation information)
        {
            if (information.NewVersionAvailable)
            {
                this.ProgramStatusLabel = "A New Update is Available. Goto Tools Menu > Options to Install";
            }
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
            this.StatusLabel = "Scanning Title " + e.CurrentTitle + " of " + e.Titles;
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
            this.scanService.SouceData.CopyTo(this.ScannedSource);
            Execute.OnUIThread(() =>
                {
                    if (e.Successful)
                    {
                        this.NotifyOfPropertyChange(() => this.ScannedSource);
                        this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);
                        this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault(t => t.MainTitle)
                                             ?? this.ScannedSource.Titles.FirstOrDefault();
                    }
                    if (e.Successful && this.selectedTitle != null)
                    {
                        if (this.selectedTitle.OpenCLSupport == 0)
                        {
                            this.SupportOpenCL = true;
                        }
                        else
                        {
                            this.SupportOpenCL = false;
                        }
                        if (this.selectedTitle.HWDSupport == 0)
                        {
                            this.SupportHardwareDecoding = true;
                        }
                        else
                        {
                            this.SupportHardwareDecoding = false;
                        }
                    }

                    this.ShowStatusWindow = false;
                    if (e.Successful)
                    {
                        this.SourceLabel = this.SourceName;
                        this.StatusLabel = "Scan Completed";
                    }
                    else if (!e.Successful && e.Exception == null)
                    {
                        this.SourceLabel = "Scan Cancelled.";
                        this.StatusLabel = "Scan Cancelled.";
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
            Execute.OnUIThread(
                () =>
                {
                    if (this.IsEncoding)
                    {
                        this.ProgramStatusLabel =
                            string.Format(
                                "{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Elapsed: {4:hh\\:mm\\:ss},  Pending Jobs {5}",
                                e.PercentComplete,
                                e.CurrentFrameRate,
                                e.AverageFrameRate,
                                e.EstimatedTimeLeft,
                                e.ElapsedTime,
                                this.queueProcessor.Count);
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
        void QueueProcessorJobProcessingStarted(object sender, HandBrake.ApplicationServices.EventArgs.QueueProgressEventArgs e)
        {
            Execute.OnUIThread(
               () =>
               {
                   this.ProgramStatusLabel = "Preparing to encode ...";
                   this.IsEncoding = true;
               });
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
              () =>
              {
                  this.ProgramStatusLabel = string.Format("{0} Encodes Pending", this.queueProcessor.Count);
              });
        }

        /// <summary>
        /// The process drive.
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        private void ProcessDrive(object item)
        {
            if (item != null)
            {
                this.StartScan(((DriveInformation)item).RootDirectory, 0);
            }
        }

        /// <summary>
        /// The generate source menu.
        /// </summary>
        /// <returns>
        /// The System.Collections.Generic.IEnumerable`1[T -&gt; HandBrakeWPF.Model.SourceMenuItem].
        /// </returns>
        private IEnumerable<SourceMenuItem> GenerateSourceMenu()
        {
            List<SourceMenuItem> menuItems = new List<SourceMenuItem>();

            SourceMenuItem folderScan = new SourceMenuItem
            {
                Image = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/folder.png")), Width = 16, Height = 16 },
                Text = "Open Folder",
                Command = new SourceMenuCommand(this.FolderScan)
            };
            SourceMenuItem fileScan = new SourceMenuItem
            {
                Image = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/Movies.png")), Width = 16, Height = 16 },
                Text = "Open File",
                Command = new SourceMenuCommand(this.FileScan)
            };

            SourceMenuItem titleSpecific = new SourceMenuItem { Text = "Title Specific Scan" };
            SourceMenuItem folderScanTitle = new SourceMenuItem
            {
                Image = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/folder.png")), Width = 16, Height = 16 },
                Text = "Open Folder",
                Command = new SourceMenuCommand(this.FolderScanTitleSpecific)
            };
            SourceMenuItem fileScanTitle = new SourceMenuItem
            {
                Image = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/Movies.png")), Width = 16, Height = 16 },
                Text = "Open File",
                Command = new SourceMenuCommand(this.FileScanTitleSpecific)
            };
            titleSpecific.Children.Add(folderScanTitle);
            titleSpecific.Children.Add(fileScanTitle);

            menuItems.Add(folderScan);
            menuItems.Add(fileScan);
            menuItems.Add(titleSpecific);

            // Drives
            menuItems.AddRange(
                from item in GeneralUtilities.GetDrives()
                let driveInformation = item
                select
                    new SourceMenuItem
                        {
                            Image = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/disc_small.png")), Width = 16, Height = 16 },
                            Text = string.Format("{0} ({1})", item.RootDirectory, item.VolumeLabel),
                            Command = new SourceMenuCommand(() => this.ProcessDrive(driveInformation)),
                            Tag = item
                        });

            return menuItems;
        }

        /// <summary>
        /// The drive tray changed.
        /// </summary>
        private void DriveTrayChanged()
        {
            Caliburn.Micro.Execute.OnUIThread(() => this.SourceMenu = this.GenerateSourceMenu());
        }

        /// <summary>
        /// Allows the main window to respond to setting changes.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void UserSettingServiceSettingChanged(object sender, HandBrake.ApplicationServices.EventArgs.SettingChangedEventArgs e)
        {
            if (e.Key == UserSettingConstants.ShowAdvancedTab)
            {
                this.NotifyOfPropertyChange(() => this.ShowAdvancedTab);
            }
        }

        /// <summary>
        /// Handle the property changed event of the encode task. 
        /// Allows the main window to respond to changes.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CurrentTask_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
           if (e.PropertyName == UserSettingConstants.ShowAdvancedTab)
           {
               this.NotifyOfPropertyChange(() => this.ShowAdvancedTab);
           }
        }

        #endregion
    }
}