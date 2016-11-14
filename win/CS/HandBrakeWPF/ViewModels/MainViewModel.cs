﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrake.ApplicationServices.Interop;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Commands.Menu;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.EventArgs;
    using HandBrakeWPF.Services.Encode.Interfaces;
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

    using Microsoft.Win32;

    using Ookii.Dialogs.Wpf;

    using Action = System.Action;
    using Execute = Caliburn.Micro.Execute;
    using LogManager = HandBrakeWPF.Helpers.LogManager;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

        private readonly IQueueProcessor queueProcessor;
        private readonly IPresetService presetService;
        private readonly IErrorService errorService;
        private readonly IUpdateService updateService;
        private readonly IWindowManager windowManager;
        private readonly IUserSettingService userSettingService;
        private readonly IScan scanService;
        private readonly IEncode encodeService;
        private readonly Win7 windowsSeven = new Win7();
        private string windowName;
        private string sourceLabel;
        private OutputFormat selectedOutputFormat;
        private bool isMkv;
        private string statusLabel;
        private string programStatusLabel;
        private Source scannedSource;
        private Title selectedTitle;
        private string duration;
        private bool isEncoding;
        private bool showStatusWindow;
        private Preset selectedPreset;
        private EncodeTask queueEditTask;
        private int lastEncodePercentage;
        private bool isPresetPanelShowing;
        private bool showSourceSelection;
        private BindingList<SourceMenuItem> drives;
        private bool canPause;
        private bool showAlertWindow;
        private string alertWindowHeader;
        private string alertWindowText;
        private bool hasSource;

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
        /// <param name="updateService">
        /// The update Service.
        /// </param>
        /// <param name="whenDoneService">
        /// The when Done Service.
        /// *** Leave in Constructor. *** 
        /// </param>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        /// <param name="pictureSettingsViewModel">
        /// The picture Settings View Model.
        /// </param>
        /// <param name="videoViewModel">
        /// The video View Model.
        /// </param>
        /// <param name="filtersViewModel">
        /// The filters View Model.
        /// </param>
        /// <param name="audioViewModel">
        /// The audio View Model.
        /// </param>
        /// <param name="subtitlesViewModel">
        /// The subtitles View Model.
        /// </param>
        /// <param name="advancedViewModel">
        /// The advanced View Model.
        /// </param>
        /// <param name="chaptersViewModel">
        /// The chapters View Model.
        /// </param>
        /// <param name="staticPreviewViewModel">
        /// The static Preview View Model.
        /// </param>
        /// <param name="queueViewModel">
        /// The queue View Model.
        /// </param>
        /// <param name="metaDataViewModel">
        /// The Meta Data View Model
        /// </param>
        public MainViewModel(IUserSettingService userSettingService, IScan scanService, IEncode encodeService, IPresetService presetService, 
            IErrorService errorService, IUpdateService updateService, 
            IPrePostActionService whenDoneService, IWindowManager windowManager, IPictureSettingsViewModel pictureSettingsViewModel, IVideoViewModel videoViewModel, 
            IFiltersViewModel filtersViewModel, IAudioViewModel audioViewModel, ISubtitlesViewModel subtitlesViewModel, 
            IAdvancedViewModel advancedViewModel, IChaptersViewModel chaptersViewModel, IStaticPreviewViewModel staticPreviewViewModel,
            IQueueViewModel queueViewModel, IMetaDataViewModel metaDataViewModel)
        {
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.updateService = updateService;
            this.windowManager = windowManager;
            this.QueueViewModel = queueViewModel;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueProcessor>();

            this.PictureSettingsViewModel = pictureSettingsViewModel;
            this.VideoViewModel = videoViewModel;
            this.MetaDataViewModel = metaDataViewModel;
            this.FiltersViewModel = filtersViewModel;
            this.AudioViewModel = audioViewModel;
            this.SubtitleViewModel = subtitlesViewModel;
            this.ChaptersViewModel = chaptersViewModel;
            this.AdvancedViewModel = advancedViewModel;
            this.StaticPreviewViewModel = staticPreviewViewModel;

            // Setup Properties
            this.WindowTitle = Resources.HandBrake_Title;
            this.CurrentTask = new EncodeTask();
            this.CurrentTask.PropertyChanged += this.CurrentTask_PropertyChanged;
            this.ScannedSource = new Source();
            this.HasSource = false;

            // Setup Events
            this.scanService.ScanStarted += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueueChanged += this.QueueChanged;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
            this.userSettingService.SettingChanged += this.UserSettingServiceSettingChanged;

            this.Presets = this.presetService.Presets;
            this.Drives = new BindingList<SourceMenuItem>();

            // Set Process Priority
            switch (this.userSettingService.GetUserSetting<string>(UserSettingConstants.ProcessPriority))
            {
                case "Realtime":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime;
                    break;
                case "High":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
                    break;
                case "Above Normal":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.AboveNormal;
                    break;
                case "Normal":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Normal;
                    break;
                case "Low":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Idle;
                    break;
                default:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
            }

            // Setup Commands
            this.QueueCommand = new QueueCommands(this.QueueViewModel);

            LogManager.Init();
            HandBrakeInstanceManager.Init();
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

        /// <summary>
        /// Gets or sets the queue view model.
        /// </summary>
        public IQueueViewModel QueueViewModel { get; set; }

        /// <summary>
        /// Gets or sets the static preview view model.
        /// </summary>
        public IStaticPreviewViewModel StaticPreviewViewModel { get; set; }

        /// <summary>
        /// Gets or sets the The MetaData View Model
        /// </summary>
        public IMetaDataViewModel MetaDataViewModel { get; set; }

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
                    this.OptimizeMP4 = selectedPreset.Task.OptimizeMP4;
                    this.IPod5GSupport = selectedPreset.Task.IPod5GSupport;
                    this.SelectedOutputFormat = selectedPreset.Task.OutputFormat;

                    // Tab Settings
                    this.PictureSettingsViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.VideoViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.FiltersViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.AudioViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.SubtitleViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.ChaptersViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.AdvancedViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);
                    this.MetaDataViewModel.SetPreset(this.SelectedPreset, this.CurrentTask);

                    // Do this again to force an update for m4v/mp4 selection
                    this.SelectedOutputFormat = selectedPreset.Task.OutputFormat;
                }

                this.NotifyOfPropertyChange(() => this.SelectedPreset);
            }
        }

        /// <summary>
        /// Optimise MP4 Checkbox
        /// </summary>
        public bool OptimizeMP4
        {
            get
            {
                return this.CurrentTask.OptimizeMP4;
            }
            set
            {
                if (value == this.CurrentTask.OptimizeMP4)
                {
                    return;
                }
                this.CurrentTask.OptimizeMP4 = value;
                this.NotifyOfPropertyChange(() => this.OptimizeMP4);
            }
        }

        /// <summary>
        /// iPod 5G Status
        /// </summary>
        public bool IPod5GSupport
        {
            get
            {
                return this.CurrentTask.IPod5GSupport;
            }
            set
            {
                if (value == this.CurrentTask.IPod5GSupport)
                {
                    return;
                }
                this.CurrentTask.IPod5GSupport = value;
                this.NotifyOfPropertyChange(() => this.IPod5GSupport);
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
                    foreach (DriveInformation item in DriveUtilities.GetDrives())
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
                            this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.SelectedPreset);
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

                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming) && this.ScannedSource.ScanPath != null)
                {
                    if (this.SelectedPointToPoint == PointToPointMode.Chapters && this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != null &&
                        this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat).Contains(Constants.Chapters))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.SelectedPreset);
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
                    this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName, this.SelectedPreset);
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
                if (!Equals(this.selectedOutputFormat, value))
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
        public Action CancelAction
        {
            get
            {
                return this.CancelScan;
            }
        }

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
        public Action AlertWindowClose
        {
            get
            {
                return this.CloseAlertWindow;
            }
        }

        /// <summary>
        /// Gets the add to queue label.
        /// </summary>
        public string QueueLabel
        {
            get
            {
                return string.Format(Resources.Main_QueueLabel, this.queueProcessor.Count > 0 ? string.Format(" ({0})", this.queueProcessor.Count) : string.Empty);
            }
        }

        /// <summary>
        /// Gets the start label.
        /// </summary>
        public string StartLabel
        {
            get
            {
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
            this.NotifyOfPropertyChange(() => this.Presets);

            // Queue Recovery
            bool queueRecovered = QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService, StartupOptions.AutoRestartQueue);

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
            this.encodeService.Stop();

            // Unsubscribe from Events.
            this.scanService.ScanStarted -= this.ScanStared;
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
            IShellViewModel shellViewModel = IoC.Get<IShellViewModel>();
            shellViewModel.DisplayWindow(ShellWindow.OptionsWindow);
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
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowQueueInline))
            {
                this.IsQueueShowingInLine = !this.IsQueueShowingInLine;
                this.NotifyOfPropertyChange(() => this.IsQueueShowingInLine);
            }
            else
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
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenPreviewWindow()
        {
            if (!string.IsNullOrEmpty(this.CurrentTask.Source))
            {
                this.StaticPreviewViewModel.IsOpen = true;
                this.StaticPreviewViewModel.UpdatePreviewFrame(this.CurrentTask, this.ScannedSource);
                this.windowManager.ShowWindow(this.StaticPreviewViewModel);
            }
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
        public bool AddToQueue()
        {
            if (this.ScannedSource == null || string.IsNullOrEmpty(this.ScannedSource.ScanPath) || this.ScannedSource.Titles.Count == 0)
            {
                this.errorService.ShowMessageBox(Resources.Main_ScanSource, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            if (string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Main_SetDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            if (!DirectoryUtilities.IsWritable(Path.GetDirectoryName(this.CurrentTask.Destination)))
            {
                this.errorService.ShowMessageBox(Resources.Main_NoPermissionsOnDirectory, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            // Sanity check the filename
            if (!string.IsNullOrEmpty(this.Destination) && FileHelper.FilePathHasInvalidChars(this.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Main_InvalidDestination, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                this.NotifyOfPropertyChange(() => this.Destination);
                return false;
            }

            if (this.Destination == this.ScannedSource.ScanPath)
            {
                this.errorService.ShowMessageBox(Resources.Main_SourceDestinationMatchError, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                this.Destination = null;
                return false;
            }

            if (this.scannedSource != null && !string.IsNullOrEmpty(this.scannedSource.ScanPath) && this.Destination.ToLower() == this.scannedSource.ScanPath.ToLower())
            {
                this.errorService.ShowMessageBox(Resources.Main_MatchingFileOverwriteWarning, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }

            QueueTask task = new QueueTask(new EncodeTask(this.CurrentTask), HBConfigurationFactory.Create(), this.ScannedSource.ScanPath);

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
                    this.AddToQueue();
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
                ShowSourceSelection = false;

                this.StartScan(dialog.SelectedPath, this.TitleSpecificScan);
            }
        }

        /// <summary>
        /// File Scan
        /// </summary>
        public void FileScan()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.*)|*.*" };
            bool? dialogResult = dialog.ShowDialog();

            if (dialogResult.HasValue && dialogResult.Value)
            {
                ShowSourceSelection = false;

                this.StartScan(dialog.FileName, this.TitleSpecificScan);
            }
        }

        /// <summary>
        /// Cancel a Scan
        /// </summary>
        public void CancelScan()
        {
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
            if (this.queueProcessor.Count != 0 || this.encodeService.IsPasued)
            {
                if (this.encodeService.IsPasued)
                {
                    this.IsEncoding = true;
                }

                this.queueProcessor.Start(this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));
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

            if (this.scannedSource != null && !string.IsNullOrEmpty(this.scannedSource.ScanPath) && this.Destination.ToLower() == this.scannedSource.ScanPath.ToLower())
            {
                this.errorService.ShowMessageBox(Resources.Main_MatchingFileOverwriteWarning, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
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
                this.IsEncoding = true;
                this.queueProcessor.Start(this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue));               
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
            this.encodeService.Pause();
            this.IsEncoding = false;
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
        public void WhenDone(string action)
        {
            this.QueueViewModel?.WhenDone(action);
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
                if (Directory.Exists(Path.GetDirectoryName(this.CurrentTask.Destination)))
                {
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(this.CurrentTask.Destination);
                }

                saveFileDialog.FileName = Path.GetFileName(this.CurrentTask.Destination);
            }

            bool? result = saveFileDialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
                if (saveFileDialog.FileName == this.ScannedSource.ScanPath)
                {
                    this.errorService.ShowMessageBox(Resources.Main_SourceDestinationMatchError, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                    this.Destination = null;
                    return;
                }

                this.Destination = saveFileDialog.FileName;

                // Disk Space Check
                string drive = Path.GetPathRoot(this.Destination);             
                if (drive != null && !drive.StartsWith(@"\\"))
                {
                    DriveInfo c = new DriveInfo(drive);
                    if (c.AvailableFreeSpace < this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseOnLowDiskspaceLevel))
                    {
                        this.errorService.ShowMessageBox(
                            Resources.MainViewModel_LowDiskSpaceWarning,
                            Resources.MainViewModel_LowDiskSpace,
                            MessageBoxButton.OK,
                            MessageBoxImage.Warning);
                    }
                }

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
            this.windowManager.ShowWindow(presetViewModel);
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

            if (this.SelectedPreset.IsBuildIn)
            {
                this.errorService.ShowMessageBox(
                    Resources.Main_NoUpdateOfBuiltInPresets, Resources.Main_NoPresetSelected, MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            IManagePresetViewModel presetViewModel = IoC.Get<IManagePresetViewModel>();
            presetViewModel.Setup(this.SelectedPreset);
            this.windowManager.ShowDialog(presetViewModel);
            Preset preset = presetViewModel.Preset;

            this.SelectedPreset = preset; // Reselect the preset
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
                this.NotifyOfPropertyChange(() => this.Presets);
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
            OpenFileDialog dialog = new OpenFileDialog { Filter = "Preset Files|*.json;*.plist", CheckFileExists = true };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.presetService.Import(dialog.FileName);
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
                Filter = "json|*.json", 
                CheckPathExists = true, 
                AddExtension = true, 
                DefaultExt = ".json", 
                OverwritePrompt = true, 
                FilterIndex = 0
            };
            if (this.selectedPreset != null)
            {
                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (!string.IsNullOrEmpty(filename))
                {
                    this.presetService.Export(savefiledialog.FileName, this.selectedPreset, HBConfigurationFactory.Create());
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
        /// <param name="scannedSource">
        /// The scanned Source.
        /// </param>
        private void QueueEditAction(bool successful, Source scannedSource)
        {
            /* TODO Fix this. */
            Execute.OnUIThread(() =>
            {
                // Copy all the Scan data into the UI
                scannedSource.CopyTo(this.ScannedSource);
                this.NotifyOfPropertyChange(() => this.ScannedSource);
                this.NotifyOfPropertyChange(() => this.ScannedSource.Titles);

                // Select the Users Title
                this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault();
                this.CurrentTask = new EncodeTask(queueEditTask);
                this.NotifyOfPropertyChange(() => this.CurrentTask);
                this.HasSource = true;

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
                this.MetaDataViewModel.UpdateTask(this.CurrentTask);

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
                switch (this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                {
                    case 0: // Auto
                        newExtension = MP4Helper.RequiresM4v(this.CurrentTask) ? ".m4v" : ".mp4";
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

                this.NotifyOfPropertyChange(() => this.OptimizeMP4);
                this.NotifyOfPropertyChange(() => this.IPod5GSupport);
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
                this.PictureSettingsViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.VideoViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.FiltersViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.AudioViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.SubtitleViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.ChaptersViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.AdvancedViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
                this.MetaDataViewModel.SetSource(this.ScannedSource, this.SelectedTitle, this.SelectedPreset, this.CurrentTask);
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
            if (e.ScannedSource != null)
            {
                e.ScannedSource.CopyTo(this.ScannedSource);
            }
            else
            {
                this.ScannedSource = null;
            }

            Execute.OnUIThread(() =>
            {
                if (e.Successful)
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
                        this.ProgramStatusLabel =
                            string.Format(Resources.MainViewModel_EncodeStatusChanged_StatusLabel + Resources.Main_JobsPending_addon,
                                e.Task,
                                e.TaskCount,
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
        void QueueProcessorJobProcessingStarted(object sender, QueueProgressEventArgs e)
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
                  this.NotifyOfPropertyChange(() => this.QueueLabel);
                  this.NotifyOfPropertyChange(() => this.StartLabel);
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
        private void UserSettingServiceSettingChanged(object sender, SettingChangedEventArgs e)
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
        private void CurrentTask_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == UserSettingConstants.ShowAdvancedTab)
            {
                this.NotifyOfPropertyChange(() => this.ShowAdvancedTab);
            }
        }

        #endregion
    }
}