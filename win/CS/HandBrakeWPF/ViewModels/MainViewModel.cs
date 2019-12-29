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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Windows;
    using System.Windows.Input;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Commands.Menu;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;
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
    using Execute = Caliburn.Micro.Execute;
    using HandBrakeInstanceManager = Instance.HandBrakeInstanceManager;
    using LogManager = Helpers.LogManager;
    using OpenFileDialog = Microsoft.Win32.OpenFileDialog;
    using SaveFileDialog = Microsoft.Win32.SaveFileDialog;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

        private readonly IQueueService queueProcessor;
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IUpdateService updateService;
        private readonly IWindowManager windowManager;
        private readonly INotifyIconService notifyIconService;
        private readonly IUserSettingService userSettingService;
        private readonly IScan scanService;
        private readonly Win7 windowsSeven = new Win7();
        private string windowName;
        private string sourceLabel;
        private string statusLabel;
        private string programStatusLabel;
        private Source scannedSource;
        private Title selectedTitle;
        private string duration;
        private bool isEncoding;
        private bool showStatusWindow;
        private Preset selectedPreset;
        private QueueTask queueEditTask;
        private int lastEncodePercentage;
        private bool isPresetPanelShowing;
        private bool showSourceSelection;
        private BindingList<SourceMenuItem> drives;
        private bool canPause;
        private bool showAlertWindow;
        private string alertWindowHeader;
        private string alertWindowText;
        private bool hasSource;
        private bool isSettingPreset;
        private IPresetObject selectedPresetCategory;
        private bool isModifiedPreset;
        private bool updateAvailable;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class.
        /// The viewmodel for HandBrakes main window.
        /// </summary>
        /// <remarks>whenDoneService must be a serivce here!</remarks>
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
            INotifyIconService notifyIconService,
            ISystemService systemService)
            : base(userSettingService)
        {
            this.scanService = scanService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.updateService = updateService;
            this.windowManager = windowManager;
            this.notifyIconService = notifyIconService;
            this.QueueViewModel = queueViewModel;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueService>();

            this.SummaryViewModel = summaryViewModel;
            this.PictureSettingsViewModel = pictureSettingsViewModel;
            this.VideoViewModel = videoViewModel;
            this.MetaDataViewModel = metaDataViewModel;
            this.FiltersViewModel = filtersViewModel;
            this.AudioViewModel = audioViewModel;
            this.SubtitleViewModel = subtitlesViewModel;
            this.ChaptersViewModel = chaptersViewModel;
            this.StaticPreviewViewModel = staticPreviewViewModel;

            // Setup Properties
            this.WindowTitle = Resources.HandBrake_Title;
            this.CurrentTask = new EncodeTask();
            this.ScannedSource = new Source();
            this.HasSource = false;

            // Setup Events
            this.scanService.ScanStarted += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueChanged;
            this.queueProcessor.QueuePaused += this.QueueProcessor_QueuePaused;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
            this.userSettingService.SettingChanged += this.UserSettingServiceSettingChanged;

            this.PresetsCategories = new BindingList<IPresetObject>();
            this.Drives = new BindingList<SourceMenuItem>();

            // Set Process Priority
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

            // Setup Commands
            this.QueueCommand = new QueueCommands(this.QueueViewModel);

            // Monitor the system.
            systemService.Start();
        }

        #region View Model Properties

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

        public int SelectedTab { get; set; }

        #endregion

        #region Properties

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
                    this.NotifyOfPropertyChange(() => this.WindowTitle);
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
                return string.IsNullOrEmpty(this.programStatusLabel) ? Resources.State_Ready : this.programStatusLabel;
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
                return string.IsNullOrEmpty(this.statusLabel) ? Resources.State_Ready : this.statusLabel;
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

        public bool QueueRecoveryArchivesExist { get; set; }

        public IEnumerable<IPresetObject> PresetsCategories { get; set; }

        public IPresetObject SelectedPresetCategory
        {
            get
            {
                return this.selectedPresetCategory;
            }
            set
            {
                if (!object.Equals(this.selectedPresetCategory, value))
                {
                    this.selectedPresetCategory = value;
                    this.NotifyOfPropertyChange(() => this.SelectedPresetCategory);
                    this.NotifyOfPropertyChange(() => this.CategoryPresets);
                }
            }
        }

        public IEnumerable<Preset> CategoryPresets
        {
            get
            {
                PresetDisplayCategory category = this.SelectedPresetCategory as PresetDisplayCategory;
                if (category != null && category.Presets != null)
                {
                    if (!category.Presets.Contains(this.SelectedPreset))
                    {
                        this.SelectedPreset = category.Presets.FirstOrDefault();
                    }

                    return new BindingList<Preset>(category.Presets);
                }

                this.SelectedPreset = null;
                return new BindingList<Preset>();
            }
        }

        public Preset SelectedPreset
        {
            get
            {
                return this.selectedPreset;
            }

            set
            {
                if (!object.Equals(this.selectedPreset, value))
                {
                    this.selectedPreset = value;
                    this.NotifyOfPropertyChange(() => this.SelectedPreset);

                    if (value != null)
                    {
                        this.PresetSelect(value);
                    }
                }
            }
        }

        public bool IsModifiedPreset
        {
            get
            {
                return this.isModifiedPreset;
            }
            set
            {
                if (value == this.isModifiedPreset) return;
                this.isModifiedPreset = value;
                this.NotifyOfPropertyChange();
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
                this.NotifyOfPropertyChange(() => ScannedSource);
            }
        }

        /// <summary>
        /// Gets or sets the title specific scan.
        /// </summary>
        public int TitleSpecificScan { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the encode service supports pausing.
        /// </summary>
        public bool CanPause
        {
            get
            {
                return this.canPause;
            }
            set
            {
                if (value.Equals(this.canPause))
                {
                    return;
                }
                this.canPause = value;
                this.NotifyOfPropertyChange(() => this.CanPause);
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
                return string.IsNullOrEmpty(this.sourceLabel) ? Resources.Main_SelectSource : this.sourceLabel;
            }

            set
            {
                if (!Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                    this.NotifyOfPropertyChange(() => SourceLabel);
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
                if (this.ScannedSource == null || this.ScannedSource.ScanPath == null || this.selectedTitle == null)
                {
                    return string.Empty;
                }

                if (File.Exists(this.ScannedSource.ScanPath)) // Scan Path is a File.
                {
                    return Path.GetFileNameWithoutExtension(this.ScannedSource.ScanPath);
                }

                if (Directory.Exists(this.ScannedSource.ScanPath)) // Scan Path is a folder.
                {
                    // Check to see if it's a Drive. If yes, use the volume label.
                    foreach (DriveInformation item in DriveUtilities.GetDrives())
                    {
                        if (item.RootDirectory.Contains(this.ScannedSource.ScanPath.Replace("\\\\", "\\")))
                        {
                            return item.VolumeLabel;
                        }
                    }

                    // Otherwise, it may be a path of files.
                    if (!string.IsNullOrEmpty(this.selectedTitle.SourceName) && File.Exists(this.selectedTitle.SourceName)) // Selected Title is a file
                    {
                        return Path.GetFileNameWithoutExtension(this.selectedTitle.SourceName);
                    }
                    else if (Directory.Exists(this.selectedTitle.SourceName)) // Selected Title is a structured source.
                    {
                        return Path.GetFileName(this.ScannedSource.ScanPath);
                    }
                }

                return null;
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
        public bool ShowTextEntryForPointToPointMode => this.SelectedPointToPoint != PointToPointMode.Chapters;

        /// <summary>
        /// Gets StartEndRangeItems.
        /// </summary>
        public IEnumerable<int> StartEndRangeItems
        {
            get
            {
                return this.SelectedTitle?.Chapters.Select(item => item.ChapterNumber).Select(dummy => dummy).ToList();
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
                for (int i = 1; i <= this.selectedTitle.AngleCount; i++)
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
                this.NotifyOfPropertyChange(() => Duration);
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
                this.CanPause = value;
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
        /// Gets RangeMode.
        /// </summary>
        public IEnumerable<OutputFormat> OutputFormats => new List<OutputFormat> { OutputFormat.Mp4, OutputFormat.Mkv, OutputFormat.WebM };

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
                if (!Equals(this.selectedTitle, value))
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
                            this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.selectedPreset);
                        }
                    }

                    this.NotifyOfPropertyChange(() => this.CurrentTask);

                    this.Duration = this.DurationCalculation();

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
                return this.CurrentTask.Angle;
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
        public long SelectedStartPoint
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

                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.ScannedSource.ScanPath != null)
                {
                    if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.selectedPreset);
                    }
                }

                if (this.SelectedStartPoint > this.SelectedEndPoint)
                {
                    this.SelectedEndPoint = this.SelectedStartPoint;
                }
            }
        }

        /// <summary>
        /// Gets or sets SelectedEndPoint.
        /// </summary>
        public long SelectedEndPoint
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

                if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                    this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                {
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.selectedPreset);
                }

                if (this.SelectedStartPoint > this.SelectedEndPoint && this.SelectedPointToPoint == PointToPointMode.Chapters)
                {
                    this.SelectedStartPoint = this.SelectedEndPoint;
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

        /// <summary>
        /// Gets or sets a value indicating whether is preset panel showing.
        /// </summary>
        public bool IsPresetPanelShowing
        {
            get
            {
                return this.isPresetPanelShowing;
            }
            set
            {
                if (!Equals(this.isPresetPanelShowing, value))
                {
                    this.isPresetPanelShowing = value;
                    this.NotifyOfPropertyChange(() => this.IsPresetPanelShowing);

                    // Save the setting if it has changed.
                    if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPresetPanel) != value)
                    {
                        this.userSettingService.SetUserSetting(UserSettingConstants.ShowPresetPanel, value);
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating progress percentage.
        /// </summary>
        public int ProgressPercentage { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether show source selection.
        /// </summary>
        public bool ShowSourceSelection
        {
            get
            {
                return this.showSourceSelection;
            }
            set
            {
                if (value.Equals(this.showSourceSelection))
                {
                    return;
                }
                this.showSourceSelection = value;
                this.NotifyOfPropertyChange(() => this.ShowSourceSelection);

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

        /// <summary>
        /// Gets or sets the drives.
        /// </summary>
        public BindingList<SourceMenuItem> Drives
        {
            get
            {
                return this.drives;
            }
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

        /// <summary>
        /// Gets the cancel action.
        /// </summary>
        public Action CancelAction => this.CancelScan;

        /// <summary>
        /// Action for the status window.
        /// </summary>
        public Action OpenLogWindowAction => this.OpenLogWindow;

        /// <summary>
        /// Gets or sets a value indicating whether show alert window.
        /// </summary>
        public bool ShowAlertWindow
        {
            get
            {
                return this.showAlertWindow;
            }
            set
            {
                if (value.Equals(this.showAlertWindow))
                {
                    return;
                }
                this.showAlertWindow = value;
                this.NotifyOfPropertyChange(() => this.ShowAlertWindow);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether alert window header.
        /// </summary>
        public string AlertWindowHeader
        {
            get
            {
                return this.alertWindowHeader;
            }
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

        /// <summary>
        /// Gets or sets a value indicating whether alert window text.
        /// </summary>
        public string AlertWindowText
        {
            get
            {
                return this.alertWindowText;
            }
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

        /// <summary>
        /// Gets the alert window close.
        /// </summary>
        public Action AlertWindowClose => this.CloseAlertWindow;

        /// <summary>
        /// Gets the add to queue label.
        /// </summary>
        public string QueueLabel => string.Format(Resources.Main_QueueLabel, this.queueProcessor.Count > 0 ? string.Format(" ({0})", this.queueProcessor.Count) : string.Empty);

        /// <summary>
        /// Gets the start label.
        /// </summary>
        public string StartLabel
        {
            get
            {
                if (this.queueProcessor.EncodeService.IsPasued)
                {
                    return Resources.Main_ResumeEncode;
                }

                return this.queueProcessor.Count > 0 ? Resources.Main_StartQueue : Resources.Main_Start;
            } 
        }

        /// <summary>
        /// Gets or sets a value indicating whether has source.
        /// </summary>
        public bool HasSource
        {
            get
            {
                return this.hasSource;
            }

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

        /// <summary>
        /// Flag to indicate if the queue is showing on the main view. (I.e  inline queue display)
        /// </summary>
        public bool IsQueueShowingInLine { get; set; } = false;

        public bool IsUWP { get; } = UwpDetect.IsUWP();

        public string SourceInfo
        {
            get
            {
                if (this.SelectedTitle != null)
                {
                    int parW = this.SelectedTitle.ParVal.Width;
                    int parH = this.SelectedTitle.ParVal.Height;
                    int displayW = this.SelectedTitle.Resolution.Width * parW / parH;

                    return string.Format("{0}x{1} ({2}x{3}), {4} FPS, {5} {6}, {7} {8}", 
                        this.SelectedTitle.Resolution.Width, 
                        this.SelectedTitle.Resolution.Height,
                        displayW,
                        this.SelectedTitle.Resolution.Height, 
                        Math.Round(this.SelectedTitle.Fps, 2), 
                        this.SelectedTitle.AudioTracks.Count, 
                        Resources.MainView_AudioTrackCount,
                        this.SelectedTitle.Subtitles.Count,
                        Resources.MainView_SubtitleTracksCount);
                }

                return string.Empty;
            }
        }

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
                if (value == this.updateAvailable) return;
                this.updateAvailable = value;
                this.NotifyOfPropertyChange(() => this.UpdateAvailable);
            }
        }

        #endregion

        #region Commands 

        /// <summary>
        /// Gets or sets the queue command.
        /// </summary>
        public ICommand QueueCommand { get; set; }

        #endregion

        #region Load and Shutdown Handling

        /// <summary>
        /// Initialise this view model.
        /// </summary>
        public override void OnLoad()
        {
            // Perform an update check if required
            this.updateService.PerformStartupUpdateCheck(this.HandleUpdateCheckResults);

            // Show or Hide the Preset Panel.
            this.IsPresetPanelShowing = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPresetPanel);

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
                this.queueProcessor.Start(this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
            }

            // Preset Selection
            this.SetDefaultPreset();

            // Reset WhenDone if necessary.
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction))
            {
                this.WhenDone(0);
            }

            // Log Cleaning
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs))
            {
                Thread clearLog = new Thread(() => GeneralUtilities.ClearLogFiles(30));
                clearLog.Start();
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

        private void SummaryViewModel_OutputFormatChanged(object sender, OutputFormatChangedEventArgs e)
        {
            if (!string.IsNullOrEmpty(e.Extension))
            {
                this.Destination = Path.ChangeExtension(this.Destination, e.Extension);
            }

            this.VideoViewModel.RefreshTask();
            this.AudioViewModel.RefreshTask();
            this.SubtitleViewModel.RefreshTask();
        }

        public void Shutdown()
        {
            // Shutdown Service
            this.queueProcessor.Stop();
            this.presetService.SaveCategoryStates();

            // Unsubscribe from Events.
            this.scanService.ScanStarted -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;
            this.queueProcessor.QueuePaused -= this.QueueProcessor_QueuePaused;
            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueueChanged -= this.QueueChanged;
          
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
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
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(null);
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
                ILogViewModel logvm = IoC.Get<ILogViewModel>();
                this.windowManager.ShowWindow(logvm);
            }
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenQueueWindow()
        {
            this.IsQueueShowingInLine = false;
            this.NotifyOfPropertyChange(() => this.IsQueueShowingInLine);

            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(QueueView));
            if (window != null)
            {
                if (window.WindowState == WindowState.Minimized)
                {
                    window.WindowState = WindowState.Normal;
                }

                window.Activate();
            }
            else
            {
                this.windowManager.ShowWindow(IoC.Get<IQueueViewModel>());
            }
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenPreviewWindow()
        {
            if (!string.IsNullOrEmpty(this.CurrentTask.Source) && !this.StaticPreviewViewModel.IsOpen)
            {
                this.StaticPreviewViewModel.IsOpen = true;
                this.StaticPreviewViewModel.UpdatePreviewFrame(this.CurrentTask, this.ScannedSource);
                this.windowManager.ShowWindow(this.StaticPreviewViewModel);
            }
            else if (this.StaticPreviewViewModel.IsOpen)
            {
                Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(StaticPreviewView));
                window?.Focus();
            }
        }

        public void ShowPresetPane()
        {
            this.IsPresetPanelShowing = !this.IsPresetPanelShowing;
        }

        /// <summary>
        /// Launch the Help pages.
        /// </summary>
        public void LaunchHelp()
        {
            try
            {
                Process.Start("https://handbrake.fr/docs");
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Main_UnableToLoadHelpMessage, Resources.Main_UnableToLoadHelpSolution, exc);
            }
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
        /// <returns>
        /// True if added, false if error.
        /// </returns>
        public AddQueueError AddToQueue()
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
                    MessageBoxResult result = this.errorService.ShowMessageBox(string.Format(Resources.Main_QueueOverwritePrompt, Path.GetFileName(this.CurrentTask.Destination)), Resources.Question, MessageBoxButton.YesNo, MessageBoxImage.Question);
                    if (result == MessageBoxResult.No)
                    {
                        return null; // Handled by the above action.
                    }
                }
            }

            if (!DirectoryUtilities.IsWritable(Path.GetDirectoryName(this.CurrentTask.Destination), false, this.errorService))
            {
                return new AddQueueError(Resources.Main_NoPermissionsOrMissingDirectory, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (!DriveUtilities.HasMinimumDiskSpace(
                this.Destination,
                this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel)))
            {
                return new AddQueueError(Resources.Main_LowDiskspace, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
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

            QueueTask task = new QueueTask(new EncodeTask(this.CurrentTask), HBConfigurationFactory.Create(), this.ScannedSource.ScanPath, this.SelectedPreset, this.IsModifiedPreset);

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
            var addError = this.AddToQueue();
            if (addError != null)
            {
                this.errorService.ShowMessageBox(addError.Message, addError.Header, addError.Buttons, addError.ErrorType);
            }
        }

        /// <summary>
        /// Add all Items to the queue
        /// </summary>
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

            if (this.CurrentTask != null && this.CurrentTask.SubtitleTracks != null && this.CurrentTask.SubtitleTracks.Count > 0)
            {
                if ((this.SubtitleViewModel.SubtitleBehaviours == null || this.SubtitleViewModel.SubtitleBehaviours.SelectedBehaviour == SubtitleBehaviourModes.None)
                    && !(this.CurrentTask.SubtitleTracks.Count == 1 && this.CurrentTask.SubtitleTracks.First().SubtitleType == SubtitleType.ForeignAudioSearch))
                {
                    MessageBoxResult result = this.errorService.ShowMessageBox(
                        Resources.Main_AutoAdd_AudioAndSubWarning,
                        Resources.Warning,
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Warning);

                    if (result == MessageBoxResult.No)
                    {
                        return;
                    }
                }
            }

            foreach (Title title in this.ScannedSource.Titles)
            {
                this.SelectedTitle = title;
                var addError = this.AddToQueue();
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

        /// <summary>
        /// The add selection to queue.
        /// </summary>
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

            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(QueueSelectionViewModel));
            IQueueSelectionViewModel viewModel = IoC.Get<IQueueSelectionViewModel>();

            viewModel.Setup(this.ScannedSource, this.SourceName, (tasks) =>
            {
                foreach (SelectionTitle title in tasks)
                {
                    this.SelectedTitle = title.Title;
                    var addError = this.AddToQueue();
                    if (addError != null)
                    {
                        MessageBoxResult result = this.errorService.ShowMessageBox(addError.Message + Environment.NewLine + Environment.NewLine + Resources.Main_ContinueAddingToQueue, addError.Header, MessageBoxButton.YesNo, addError.ErrorType);

                        if (result == MessageBoxResult.No)
                        {
                            break;
                        }
                    }
                }
            }, this.selectedPreset);

            if (window != null)
            {
                window.Activate();
            }
            else
            {
                this.windowManager.ShowWindow(viewModel);
            }
        }

        /// <summary>
        /// Folder Scan
        /// </summary>
        public void FolderScan()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = Resources.Main_PleaseSelectFolder, UseDescriptionForTitle = true };
            bool? dialogResult = dialog.ShowDialog();

            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.StartScan(dialog.SelectedPath, this.TitleSpecificScan);
            }
        }

        /// <summary>
        /// File Scan
        /// </summary>
        public void FileScan()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.*)|*.*" };

            string mruDir = this.GetMru(Constants.FileScanMru);
            if (!string.IsNullOrEmpty(mruDir) && Directory.Exists(mruDir))
            {
                dialog.InitialDirectory = mruDir;
            }
            
            bool? dialogResult = dialog.ShowDialog();

            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.SetMru(Constants.FileScanMru, Path.GetDirectoryName(dialog.FileName));

                this.StartScan(dialog.FileName, this.TitleSpecificScan);
            }
        }

        /// <summary>
        /// Cancel a Scan
        /// </summary>
        public void CancelScan()
        {
            this.ShowStatusWindow = false;
            this.scanService.Cancel();
        }

        /// <summary>
        /// Start an Encode
        /// </summary>
        public void StartEncode()
        {
            if (this.queueProcessor.IsProcessing)
            {
                this.IsEncoding = true;
                return;
            }

            // Check if we already have jobs, and if we do, just start the queue.
            if (this.queueProcessor.Count != 0 || this.queueProcessor.EncodeService.IsPasued)
            {
                if (this.queueProcessor.EncodeService.IsPasued)
                {
                    this.IsEncoding = true;
                }

                this.queueProcessor.Start(this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
                return;
            }

            // Otherwise, perform Sanity Checking then add to the queue and start if everything is ok.
            if (this.SelectedTitle == null)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // Create the Queue Task and Start Processing
            var addError = this.AddToQueue();
            if (addError == null)
            {
                this.IsEncoding = true;
                this.queueProcessor.Start(this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));               
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

        /// <summary>
        /// Edit a Queue Task
        /// </summary>
        /// <param name="queueTask">
        /// The task.
        /// </param>
        public void EditQueueJob(QueueTask queueTask)
        {
            // Rescan the source to make sure it's still valid
            EncodeTask task = queueTask.Task;

            this.queueEditTask = queueTask;
            this.scanService.Scan(task.Source, task.Title, QueueEditAction, HBConfigurationFactory.Create());
        }

        /// <summary>
        /// Pause an Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.PauseEncode();
            this.IsEncoding = false;
        }

        /// <summary>
        /// Stop an Encode.
        /// </summary>
        public void StopEncode()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.MainView_StopEncodeConfirm,
                Resources.MainView_StopEncode,
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result == MessageBoxResult.Yes)
            {
                this.queueProcessor.Stop();
            }
        }

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        public void ExitApplication()
        {
            Application.Current.Shutdown();
        }

        /// <summary>
        /// The select source window.
        /// </summary>
        public void SelectSourceWindow()
        {
            ShowSourceSelection = !ShowSourceSelection;
        }

        /// <summary>
        /// The close source selection.
        /// </summary>
        public void CloseSourceSelection()
        {
            this.ShowSourceSelection = false;
        }

        /// <summary>
        /// The close alert window.
        /// </summary>
        public void CloseAlertWindow()
        {
            this.ShowAlertWindow = false;
            this.AlertWindowText = string.Empty;
            this.AlertWindowHeader = string.Empty;
        }

        /// <summary>
        /// Pass on the "When Done" Action to the queue view model. 
        /// </summary>
        /// <param name="action">action</param>
        public void WhenDone(int action)
        {
            this.QueueViewModel?.WhenDone(action, true);
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

        /// <summary>
        /// The Destination Path
        /// </summary>
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

        /// <summary>
        /// The open destination directory.
        /// </summary>
        public void OpenDestinationDirectory()
        {
            if (!string.IsNullOrEmpty(this.Destination))
            {
                try
                {
                    string directory = Path.GetDirectoryName(this.Destination);
                    if (!string.IsNullOrEmpty(directory) && Directory.Exists(directory))
                    {
                        Process.Start(directory);
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
                            Process.Start(directory);
                        }
                    }
                }
                catch (Exception exc)
                {
                    this.errorService.ShowError(Resources.MainViewModel_UnableToLaunchDestDir, Resources.MainViewModel_UnableToLaunchDestDirSolution, exc);
                }
            }
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void PresetAdd()
        {
            IAddPresetViewModel presetViewModel = IoC.Get<IAddPresetViewModel>();
            presetViewModel.Setup(this.CurrentTask, this.SelectedTitle, this.AudioViewModel.AudioBehaviours, this.SubtitleViewModel.SubtitleBehaviours);
            this.windowManager.ShowDialog(presetViewModel);

            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.NotifyOfPropertyChange(() => this.CategoryPresets);
        }

        /// <summary>
        /// Update a selected preset.
        /// </summary>
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

            if (this.errorService.ShowMessageBox(Resources.Main_PresetUpdateConfrimation, Resources.AreYouSure, MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                this.selectedPreset.Update(new EncodeTask(this.CurrentTask), new AudioBehaviours(this.AudioViewModel.AudioBehaviours), new SubtitleBehaviours(this.SubtitleViewModel.SubtitleBehaviours));
                this.presetService.Update(this.selectedPreset);
                this.IsModifiedPreset = false;

                this.errorService.ShowMessageBox(
                        Resources.Main_PresetUpdated, Resources.Updated, MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        /// <summary>
        /// Manage the current Preset
        /// </summary>
        public void PresetManage()
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

            IManagePresetViewModel presetViewModel = IoC.Get<IManagePresetViewModel>();
            presetViewModel.Setup(this.selectedPreset);
            this.windowManager.ShowDialog(presetViewModel);
            Preset preset = presetViewModel.Preset;

            this.NotifyOfPropertyChange(() => this.CategoryPresets);
            this.selectedPreset = preset; // Reselect the preset      
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
        }

        /// <summary>
        /// Remove a Preset
        /// </summary>
        public void PresetRemove()
        {
            if (this.selectedPreset != null)
            {
                if (this.selectedPreset.IsDefault)
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
                   Resources.MainViewModel_PresetRemove_AreYouSure + this.selectedPreset.Name + " ?", 
                   Resources.Question, 
                   MessageBoxButton.YesNo, 
                   MessageBoxImage.Question);

                if (result == MessageBoxResult.No)
                {
                    return;
                }

                this.presetService.Remove(this.selectedPreset);
                this.NotifyOfPropertyChange(() => this.CategoryPresets);
                this.SelectedPreset = this.CategoryPresets.FirstOrDefault();
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
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
                this.errorService.ShowMessageBox(string.Format(Resources.Main_NewDefaultPreset, this.selectedPreset.Name), Resources.Main_Presets, MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Import a Preset
        /// </summary>
        public void PresetImport()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Preset Files|*.json;*.plist", CheckFileExists = true };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.presetService.Import(dialog.FileName);
                this.NotifyOfPropertyChange(() => this.CategoryPresets);
            }
        }

        /// <summary>
        /// Export a Preset
        /// </summary>
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
                    this.presetService.Export(savefiledialog.FileName, this.selectedPreset, HBConfigurationFactory.Create());
                }
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Reset built-in presets
        /// </summary>
        public void PresetReset()
        {
            this.presetService.UpdateBuiltInPresets();

            this.NotifyOfPropertyChange(() => this.PresetsCategories);
            this.NotifyOfPropertyChange(() => this.CategoryPresets);

            this.SetDefaultPreset();

            this.errorService.ShowMessageBox(Resources.Presets_ResetComplete, Resources.Presets_ResetHeader, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        public void PresetSelect()
        {
            this.PresetSelect(this.SelectedPreset);
        }

        /// <summary>
        /// The preset select.
        /// </summary>
        /// <param name="tag">
        /// The tag.
        /// </param>
        public void PresetSelect(object tag)
        {
            Preset preset = tag as Preset;
            if (preset != null)
            {
                if (this.SelectedPresetCategory == null || this.SelectedPresetCategory.Category != preset.Category)
                {
                    this.SelectedPresetCategory = this.PresetsCategories.FirstOrDefault(c => c.Category == preset.Category);
                }

                this.selectedPreset = preset;
                this.NotifyOfPropertyChange(() => this.SelectedPreset);

                this.presetService.SetSelected(this.selectedPreset);

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
                }
            }
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
                ShowSourceSelection = false;
                this.scanService.Scan(filename, title, null, HBConfigurationFactory.Create());
            }
        }

        /// <summary>
        /// The process drive.
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        public void ProcessDrive(object item)
        {
            if (item != null)
            {
                if (item.GetType() == typeof(DriveInformation))
                {
                    this.StartScan(((DriveInformation)item).RootDirectory, 0);
                }
                else if (item.GetType() == typeof(SourceMenuItem))
                {
                    DriveInformation driveInfo = ((SourceMenuItem)item).Tag as DriveInformation;
                    if (driveInfo != null)
                    {
                        this.StartScan(driveInfo.RootDirectory, this.TitleSpecificScan);
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
        }

        public void FlipAddSelectionToQueue()
        {
            bool value = !this.ShowAddSelectionToQueue;
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAddSelectionToQueue, value);
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Update all the UI Components to allow the user to edit their previous settings.
        /// </summary>
        /// <param name="successful">
        /// The successful.
        /// </param>
        /// <param name="scannedSource">
        /// The scanned Source.
        /// </param>
        private void QueueEditAction(bool successful, Source scannedSource)
        {
            /* TODO Fix this. */
            Execute.OnUIThread(() =>
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
                scannedSource.CopyTo(this.ScannedSource);
                this.NotifyOfPropertyChange(() => this.ScannedSource);
                this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);

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
                this.SourceLabel = this.SourceName;
                this.StatusLabel = Resources.Main_ScanCompleted;
            });
        }

        /// <summary>
        /// Setup the UI tabs. Passes in any relevant models for setup.
        /// </summary>
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
            }
        }

        private void TabStatusChanged(object sender, TabStatusEventArgs e)
        {
            if (this.isSettingPreset)
            {
                return; // Don't process this when we are setting up.
            }

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
                this.UpdateAvailable = true;
                this.ProgramStatusLabel = Resources.Main_NewUpdate;
            }
        }

        /// <summary>
        /// The open alert window.
        /// </summary>
        /// <param name="header">
        /// The header.
        /// </param>
        /// <param name="message">
        /// The message.
        /// </param>
        private void OpenAlertWindow(string header, string message)
        {
            this.ShowAlertWindow = true;
            this.AlertWindowHeader = header;
            this.AlertWindowText = message;
        }

        private void SetDefaultPreset()
        {
            // Preset Selection
            if (this.presetService.DefaultPreset != null)
            {
                PresetDisplayCategory category =
                    (PresetDisplayCategory)this.PresetsCategories.FirstOrDefault(
                        p => p.Category == this.presetService.DefaultPreset.Category);

                this.SelectedPresetCategory = category;
                this.SelectedPreset = this.presetService.DefaultPreset;
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
        private void ScanStatusChanged(object sender, ScanProgressEventArgs e)
        {
            this.SourceLabel = string.Format(Resources.Main_ScanningTitleXOfY, e.CurrentTitle, e.Titles, e.Percentage);
            this.StatusLabel = string.Format(Resources.Main_ScanningTitleXOfY, e.CurrentTitle, e.Titles, e.Percentage);
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
            this.ShowStatusWindow = false;

            if (e.ScannedSource != null && !e.Cancelled)
            {
                if (this.ScannedSource == null)
                {
                    this.ScannedSource = new Source();
                }
                e.ScannedSource.CopyTo(this.ScannedSource);
            }
            else
            {
                this.ScannedSource = null;
            }

            Execute.OnUIThread(() =>
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
                    this.SourceLabel = this.SourceName;
                    this.StatusLabel = Resources.Main_ScanCompleted;
                }
                else if (e.Cancelled)
                {
                    this.SourceLabel = Resources.Main_ScanCancelled;
                    this.StatusLabel = Resources.Main_ScanCancelled;
                }
                else
                {
                    this.SourceLabel = Resources.Main_ScanFailled_CheckLog;
                    this.StatusLabel = Resources.Main_ScanFailled_CheckLog;
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
                    this.StatusLabel = Resources.Main_ScanningPleaseWait;
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
        private void EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            int percent;
            int.TryParse(
                Math.Round(e.PercentComplete).ToString(CultureInfo.InvariantCulture), 
                out percent);

            Execute.OnUIThread(
                () =>
                {
                    if (this.queueProcessor.EncodeService.IsEncoding)
                    {
                        string totalHrsLeft = string.Format(@"{0:hh\:mm\:ss}", e.EstimatedTimeLeft);
                        string elapsedTimeHrs = string.Format(@"{0:hh\:mm\:ss} ", e.ElapsedTime);
                        string jobsPending = string.Format(Resources.Main_JobsPending_addon, this.queueProcessor.Count);

                        if (e.IsSubtitleScan)
                        {
                            this.ProgramStatusLabel = string.Format(
                                Resources.MainViewModel_EncodeStatusChanged_SubScan_StatusLabel,
                                e.Task,
                                e.TaskCount,
                                e.PercentComplete,
                                totalHrsLeft,
                                elapsedTimeHrs,
                                jobsPending);
                        }
                        else if (e.IsMuxing)
                        {
                            this.ProgramStatusLabel = Resources.MainView_Muxing;
                        }
                        else if (e.IsSearching)
                        {
                            this.ProgramStatusLabel = string.Format(Resources.MainView_ProgressStatusWithTask, Resources.MainView_Searching, e.PercentComplete, totalHrsLeft, jobsPending);
                        }
                        else
                        {
                            this.ProgramStatusLabel = string.Format(
                                Resources.MainViewModel_EncodeStatusChanged_StatusLabel,
                                e.Task,
                                e.TaskCount,
                                e.PercentComplete,
                                e.CurrentFrameRate,
                                e.AverageFrameRate,
                                totalHrsLeft,
                                elapsedTimeHrs,
                                jobsPending);
                        }

                        if (this.lastEncodePercentage != percent && this.windowsSeven.IsWindowsSeven)
                        {
                            this.windowsSeven.SetTaskBarProgress(percent);
                        }

                        this.lastEncodePercentage = percent;
                        this.ProgressPercentage = percent;
                        this.NotifyOfPropertyChange(() => this.ProgressPercentage);

                        if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowStatusInTitleBar))
                        {
                            this.WindowTitle = string.Format(Resources.WindowTitleStatus, Resources.HandBrake_Title, this.ProgressPercentage, e.Task, e.TaskCount);
                            this.notifyIconService.SetTooltip(string.Format(Resources.TaskTrayStatusTitle, Resources.HandBrake_Title, this.ProgressPercentage, e.Task, e.TaskCount, e.EstimatedTimeLeft));
                        }
                    }
                    else
                    {
                        this.ProgramStatusLabel = Resources.Main_QueueFinished;
                        this.IsEncoding = false;
                        this.WindowTitle = Resources.HandBrake_Title;
                        this.notifyIconService.SetTooltip(this.WindowTitle);

                        if (this.windowsSeven.IsWindowsSeven)
                        {
                            this.windowsSeven.SetTaskBarProgressToNoProgress();
                        }
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
                   this.ProgramStatusLabel = Resources.Main_PreparingToEncode;
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
                    string errorDesc = string.Empty;
                    if (this.queueProcessor.ErrorCount > 0)
                    {
                        errorDesc += string.Format(Resources.Main_QueueFinishedErrors, this.queueProcessor.ErrorCount);
                    }

                    this.ProgramStatusLabel = Resources.Main_QueueFinished + errorDesc;
                    this.WindowTitle = Resources.HandBrake_Title;
                    this.notifyIconService.SetTooltip(this.WindowTitle);

                    if (this.windowsSeven.IsWindowsSeven)
                    {
                        this.windowsSeven.SetTaskBarProgressToNoProgress();
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
              () =>
              {
                  this.ProgramStatusLabel = string.Format(Resources.Main_XEncodesPending, this.queueProcessor.Count);
                  this.NotifyOfPropertyChange(() => this.QueueLabel);
                  this.NotifyOfPropertyChange(() => this.StartLabel);
              });
        }

        private void QueueProcessor_QueuePaused(object sender, EventArgs e)
        {
            Execute.OnUIThread(
                () =>
                {
                    this.ProgramStatusLabel = Resources.Main_QueuePaused;
                    this.NotifyOfPropertyChange(() => this.QueueLabel);
                    this.NotifyOfPropertyChange(() => this.StartLabel);
                });
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
        #endregion
    }
}