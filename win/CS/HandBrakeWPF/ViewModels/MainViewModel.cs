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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Factories;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Audio;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Model.Subtitle;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using Microsoft.Win32;

    using Ookii.Dialogs.Wpf;

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
        /// Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Source Scan Service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IEncodeServiceWrapper encodeService;

        /// <summary>
        /// Windows 7 API Pack wrapper
        /// </summary>
        private readonly Win7 windowsSeven = new Win7();

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
        /// The last percentage complete value.
        /// </summary>
        private int lastEncodePercentage;

        /// <summary>
        /// The is preset panel showing.
        /// </summary>
        private bool isPresetPanelShowing;

        /// <summary>
        /// The show source selection.
        /// </summary>
        private bool showSourceSelection;

        /// <summary>
        /// The drives.
        /// </summary>
        private BindingList<SourceMenuItem> drives;

        /// <summary>
        /// The can pause.
        /// </summary>
        private bool canPause;

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
        /// <param name="notificationService">
        /// The notification Service.
        /// *** Leave in Constructor. *** 
        /// </param>
        /// <param name="whenDoneService">
        /// The when Done Service.
        /// *** Leave in Constructor. *** 
        /// </param>
        public MainViewModel(IUserSettingService userSettingService, IScan scanService, IEncodeServiceWrapper encodeService, IPresetService presetService,
            IErrorService errorService, IShellViewModel shellViewModel, IUpdateService updateService, INotificationService notificationService,
            IPrePostActionService whenDoneService)
        {
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.shellViewModel = shellViewModel;
            this.updateService = updateService;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueProcessor>();

            // Setup Properties
            this.WindowTitle = Resources.HandBrake_Title;
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
            this.Drives = new BindingList<SourceMenuItem>();
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
                    this.CurrentTask.OptimizeMP4 = selectedPreset.Task.OptimizeMP4;
                    this.CurrentTask.IPod5GSupport = selectedPreset.Task.IPod5GSupport;
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
                this.NotifyOfPropertyChange(() => ScannedSource);
            }
        }

        /// <summary>
        /// Gets or sets the title specific scan.
        /// </summary>
        public int TitleSpecificScan { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the encode serivce supports pausing.
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
                if (ScannedSource == null || ScannedSource.ScanPath == null)
                {
                    return string.Empty;
                }

                // The title that is selected has a source name. This means it's part of a batch scan.
                if (selectedTitle != null && !string.IsNullOrEmpty(selectedTitle.SourceName) && !selectedTitle.SourceName.EndsWith("\\"))
                {
                    return Path.GetFileNameWithoutExtension(selectedTitle.SourceName);
                }

                // Check if we have a Folder, if so, check if it's a DVD / Bluray drive and get the label.
                if (ScannedSource.ScanPath.EndsWith("\\"))
                {
                    foreach (DriveInformation item in GeneralUtilities.GetDrives())
                    {
                        if (item.RootDirectory.Contains(this.ScannedSource.ScanPath.Replace("\\\\", "\\")))
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
                this.CanPause = value && this.encodeService.CanPause;
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
                if (!object.Equals(this.CurrentTask.Destination, value))
                {
                    this.CurrentTask.Destination = value;
                    this.NotifyOfPropertyChange(() => this.Destination);

                    if (!string.IsNullOrEmpty(this.CurrentTask.Destination))
                    {
                        string ext = string.Empty;
                        try
                        {
                            ext = Path.GetExtension(this.CurrentTask.Destination);
                        }
                        catch (ArgumentException)
                        {
                            this.errorService.ShowMessageBox(Resources.Main_InvalidDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                        }

                        switch (ext)
                        {
                            case ".mkv":
                                this.SelectedOutputFormat = OutputFormat.Mkv;
                                break;
                            case ".mp4":
                                this.SelectedOutputFormat = OutputFormat.Mp4;
                                break;
                            case ".m4v":
                                this.SelectedOutputFormat = OutputFormat.Mp4;
                                break;
                        }
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
                        if (this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null)
                        {
                            this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
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
                    if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
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

                if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                    this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                {
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
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
                if (!object.Equals(this.selectedOutputFormat, value))
                {
                    this.selectedOutputFormat = value;
                    this.CurrentTask.OutputFormat = value;
                    this.NotifyOfPropertyChange(() => SelectedOutputFormat);
                    this.NotifyOfPropertyChange(() => this.CurrentTask.OutputFormat);
                    this.NotifyOfPropertyChange(() => IsMkv);
                    this.SetExtension(string.Format(".{0}", this.selectedOutputFormat.ToString().ToLower()));

                    this.VideoViewModel.RefreshTask();
                    this.AudioViewModel.RefreshTask();
                }
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
                if (!object.Equals(this.isPresetPanelShowing, value))
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
                    foreach (SourceMenuItem menuItem in from item in GeneralUtilities.GetDrives()
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

            // Show or Hide the Preset Panel.
            this.IsPresetPanelShowing = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPresetPanel);

            // Setup the presets.
            this.presetService.Load();
            if (this.presetService.CheckIfPresetsAreOutOfDate())
                if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
                    this.errorService.ShowMessageBox(Resources.Main_PresetUpdateNotification,
                                    Resources.Notice, MessageBoxButton.OK, MessageBoxImage.Information);

            // Queue Recovery
            if (!AppArguments.IsInstantHandBrake)
            {
                QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService);
            }

            this.SelectedPreset = this.presetService.DefaultPreset;

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
                ILogViewModel logvm = (ILogViewModel)window.DataContext;
                logvm.SelectedTab = this.IsEncoding ? 0 : 1;
                window.Activate();
            }
            else
            {
                ILogViewModel logvm = IoC.Get<ILogViewModel>();
                logvm.SelectedTab = this.IsEncoding ? 0 : 1;
                this.WindowManager.ShowWindow(logvm);
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
        /// <returns>
        /// True if added, false if error.
        /// </returns>
        public bool AddToQueue()
        {
            if (this.ScannedSource == null || string.IsNullOrEmpty(this.ScannedSource.ScanPath) || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSourceFirst, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            if (string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Main_SetDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            // Sanity check the filename
            if (!string.IsNullOrEmpty(this.Destination) && FileHelper.FilePathHasInvalidChars(this.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Main_InvalidDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                this.NotifyOfPropertyChange(() => this.Destination);
                return false;
            }

            QueueTask task = new QueueTask(new EncodeTask(this.CurrentTask), HBConfigurationFactory.Create());

            if (!this.queueProcessor.CheckForDestinationPathDuplicates(task.Task.Destination))
            {
                this.queueProcessor.Add(task);
            }
            else
            {
                this.errorService.ShowMessageBox(Resources.Main_DuplicateDestinationOnQueue, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Warning);
                return false;
            }

            if (!this.IsEncoding)
            {
                this.ProgramStatusLabel = string.Format(Resources.Main_XEncodesPending, this.queueProcessor.Count);
            }

            return true;
        }

        /// <summary>
        /// Add all Items to the queue
        /// </summary>
        public void AddAllToQueue()
        {
            if (this.ScannedSource == null || this.ScannedSource.Titles == null || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSourceFirst, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (!AutoNameHelper.IsAutonamingEnabled())
            {
                this.errorService.ShowMessageBox(Resources.Main_TurnOnAutoFileNaming, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.CurrentTask != null && this.CurrentTask.SubtitleTracks != null && this.CurrentTask.SubtitleTracks.Count > 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_AutoAdd_AudioAndSubWarning, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Error);
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
                this.errorService.ShowMessageBox(Resources.Main_ScanSourceFirst, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
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
                        this.AddToQueue();
                    }
                });

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
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = Resources.Main_PleaseSelectFolder, UseDescriptionForTitle = true };
            dialog.ShowDialog();

            ShowSourceSelection = false;

            this.StartScan(dialog.SelectedPath, this.TitleSpecificScan);
        }

        /// <summary>
        /// File Scan
        /// </summary>
        public void FileScan()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.*)|*.*" };
            dialog.ShowDialog();

            ShowSourceSelection = false;

            this.StartScan(dialog.FileName, this.TitleSpecificScan);
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
                this.errorService.ShowMessageBox(Resources.Main_AlreadyEncoding, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            // Check if we already have jobs, and if we do, just start the queue.
            if (this.queueProcessor.Count != 0 || this.encodeService.IsPasued)
            {
                if (this.encodeService.IsPasued)
                {
                    this.IsEncoding = true;
                }

                this.queueProcessor.Start(UserSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
                return;
            }

            // Otherwise, perform Santiy Checking then add to the queue and start if everything is ok.
            if (this.SelectedTitle == null)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (string.IsNullOrEmpty(this.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Main_ChooseDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (File.Exists(this.Destination))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(Resources.Main_DestinationOverwrite, Resources.Question, MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            // Create the Queue Task and Start Processing
            if (this.AddToQueue())
            {
                this.queueProcessor.Start(UserSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
                this.IsEncoding = true;
            }
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
            this.scanService.Scan(task.Source, task.Title, QueueEditAction, HBConfigurationFactory.Create());
        }

        /// <summary>
        /// Pause an Encode
        /// </summary>
        public void PauseEncode()
        {
            this.queueProcessor.Pause();

            if (this.encodeService.CanPause)
            {
                this.encodeService.Pause();
                this.IsEncoding = false;
            }
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
                QueryGeneratorUtility.GenerateQuery(this.CurrentTask, HBConfigurationFactory.Create()),
                "CLI Query",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
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
                };

            string extension = Path.GetExtension(this.CurrentTask.Destination);

            saveFileDialog.FilterIndex = !string.IsNullOrEmpty(this.CurrentTask.Destination)
                                         && !string.IsNullOrEmpty(extension)
                                             ? (extension == ".mp4" || extension == ".m4v" ? 1 : 2)
                                             : (this.CurrentTask.OutputFormat == OutputFormat.Mkv ? 2 : 0);

            if (this.CurrentTask != null && !string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                saveFileDialog.InitialDirectory = Directory.Exists(Path.GetDirectoryName(this.CurrentTask.Destination))
                                                      ? Path.GetDirectoryName(this.CurrentTask.Destination) + "\\"
                                                      : null;

                saveFileDialog.FileName = Path.GetFileName(this.CurrentTask.Destination);
            }

            bool? result = saveFileDialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
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
                        case ".m4v":
                            this.SelectedOutputFormat = OutputFormat.Mp4;
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
                string directory = Path.GetDirectoryName(this.Destination);
                if (!string.IsNullOrEmpty(directory))
                {
                    Process.Start(directory);
                }
                else
                {
                    Process.Start(AppDomain.CurrentDomain.BaseDirectory);
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
                    Resources.Main_SelectPresetForUpdate, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);

                return;
            }

            if (this.SelectedPreset.IsBuildIn)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_NoUpdateOfBuiltInPresets, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (this.errorService.ShowMessageBox(Resources.Main_PresetUpdateConfrimation, Resources.AreYouSure, MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                this.SelectedPreset.Update(new EncodeTask(this.CurrentTask), new AudioBehaviours(this.AudioViewModel.AudioBehaviours), new SubtitleBehaviours(this.SubtitleViewModel.SubtitleBehaviours));
                this.presetService.Update(this.SelectedPreset);

                this.errorService.ShowMessageBox(
                        Resources.Main_PresetUpdated, Resources.Updated, MessageBoxButton.OK, MessageBoxImage.Information);
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
                MessageBox.Show(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
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
                MessageBox.Show(string.Format(Resources.Main_NewDefaultPreset, this.selectedPreset.Name), Resources.Main_Presets, MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Import a Preset
        /// </summary>
        public void PresetImport()
        {
            OpenFileDialog dialog = new OpenFileDialog() { Filter = "Plist (*.plist)|*.plist", CheckFileExists = true };
            dialog.ShowDialog();
            string filename = dialog.FileName;

            if (!string.IsNullOrEmpty(filename))
            {
                PList plist = new PList(filename);

                object build;
                plist.TryGetValue("PresetBuildNumber", out build);

                string buildNumber = build as string;
                if (buildNumber == null)
                {
                    MessageBox.Show(
                        Resources.Preset_UnableToImport_Message,
                        Resources.Preset_UnableToImport_Header,
                        MessageBoxButton.YesNo, MessageBoxImage.Question);
                    return;
                }

                if (buildNumber != userSettingService.GetUserSetting<int>(UserSettingConstants.HandBrakeBuild).ToString(CultureInfo.InvariantCulture))
                {
                    MessageBoxResult result = MessageBox.Show(
                        Resources.Preset_OldVersion_Message,
                        Resources.Preset_OldVersion_Header,
                        MessageBoxButton.YesNo, MessageBoxImage.Question);

                    if (result == MessageBoxResult.No)
                    {
                        return;
                    }
                }

                Preset preset = PlistPresetFactory.CreatePreset(plist);

                if (this.presetService.CheckIfPresetExists(preset.Name))
                {
                    if (!presetService.CanUpdatePreset(preset.Name))
                    {
                        MessageBox.Show(Resources.Main_PresetErrorBuiltInName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                        return;
                    }

                    MessageBoxResult result =
                        MessageBox.Show(Resources.Main_PresetOverwriteWarning, Resources.Overwrite, MessageBoxButton.YesNo, MessageBoxImage.Warning);
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
            SaveFileDialog savefiledialog = new SaveFileDialog
                                                     {
                                                         Filter = "plist|*.plist",
                                                         CheckPathExists = true,
                                                         AddExtension = true,
                                                         DefaultExt = ".plist",
                                                         OverwritePrompt = true,
                                                         FilterIndex = 0
                                                     };
            if (this.selectedPreset != null)
            {
                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (!string.IsNullOrEmpty(filename))
                {
                    PlistUtility.Export(
                        savefiledialog.FileName,
                        this.selectedPreset,
                        this.userSettingService.GetUserSetting<int>(UserSettingConstants.HandBrakeBuild).ToString(CultureInfo.InvariantCulture));
                }
            }
            else
            {
                MessageBox.Show(Resources.Main_SelectPreset, Resources.Warning, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Reset built-in presets
        /// </summary>
        public void PresetReset()
        {
            this.presetService.UpdateBuiltInPresets();
            this.NotifyOfPropertyChange(() => this.Presets);
            this.SelectedPreset = this.presetService.DefaultPreset;
            this.errorService.ShowMessageBox(Resources.Presets_ResetComplete, Resources.Presets_ResetHeader, MessageBoxButton.OK, MessageBoxImage.Information);
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
                this.SelectedPreset = preset;
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
                this.scanService.Scan(filename, title, null, HBConfigurationFactory.Create());
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
                    this.StatusLabel = Resources.Main_ScanCompleted;
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
                this.ProgramStatusLabel = Resources.Main_NewUpdate;
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

                    this.ShowStatusWindow = false;
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
                    else if (e.Exception == null && e.ErrorInformation != null)
                    {
                        this.SourceLabel = Resources.Main_ScanFailed_NoReason + e.ErrorInformation;
                        this.StatusLabel = Resources.Main_ScanFailed_NoReason + e.ErrorInformation;
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
        private void EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
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
                        string josPending = string.Empty;
                        if (!AppArguments.IsInstantHandBrake)
                        {
                            josPending = Resources.Main_JobsPending_addon;
                        }

                        this.ProgramStatusLabel =
                            string.Format("{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Elapsed: {4:hh\\:mm\\:ss}" + josPending,
                                e.PercentComplete,
                                e.CurrentFrameRate,
                                e.AverageFrameRate,
                                e.EstimatedTimeLeft,
                                e.ElapsedTime,
                                this.queueProcessor.Count);

                        if (lastEncodePercentage != percent && this.windowsSeven.IsWindowsSeven)
                        {
                            this.windowsSeven.SetTaskBarProgress(percent);
                        }

                        lastEncodePercentage = percent;
                        this.ProgressPercentage = percent;
                        this.NotifyOfPropertyChange(() => ProgressPercentage);
                    }
                    else
                    {
                        this.ProgramStatusLabel = Resources.Main_QueueFinished;
                        this.IsEncoding = false;

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
        void QueueProcessorJobProcessingStarted(object sender, HandBrake.ApplicationServices.EventArgs.QueueProgressEventArgs e)
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
                    this.ProgramStatusLabel = Resources.Main_QueueFinished;
                    this.IsEncoding = false;

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
              });
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
                        this.StartScan(driveInfo.RootDirectory, 0);
                    }

                    this.ShowSourceSelection = false;
                }
            }
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