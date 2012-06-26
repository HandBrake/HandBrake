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
    using System.ComponentModel.Composition;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Media.Imaging;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    using HandBrakeWPF.Services.Interfaces;

    using Image = System.Windows.Controls.Image;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    [Export(typeof(IMainViewModel))]
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

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
        /// The Error Service Backing field.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The Shell View Model
        /// </summary>
        private readonly IShellViewModel shellViewModel;

        /// <summary>
        /// Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

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
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="shellViewModel">
        /// The shell View Model.
        /// </param>
        [ImportingConstructor]
        public MainViewModel(IWindowManager windowManager, IUserSettingService userSettingService, IScan scanService, IEncode encodeService, IPresetService presetService,
            IErrorService errorService, IShellViewModel shellViewModel)
        {
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.shellViewModel = shellViewModel;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueProcessor>(); // TODO Instance ID!

            // Setup Properties
            this.WindowTitle = "HandBrake";
            this.CurrentTask = new EncodeTask();
            this.ScannedSource = new Source();

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;
            this.queueProcessor.JobProcessingStarted += this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;

            this.Presets = this.presetService.Presets;
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
                if (!Equals(this.statusLabel, value))
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
        /// Gets SourceToolbarMenu.
        /// </summary>
        public IEnumerable<MenuItem> SourceToolbarMenu
        {
            get
            {
                // TODO - Find a cleaner way of implementing this

                BindingList<MenuItem> menuItems = new BindingList<MenuItem>();

                // Folder Menu Item
                MenuItem folderMenuItem = new MenuItem
                    {
                        Icon = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/folder.png")), Width = 16, Height = 16 },
                        Header = new TextBlock { Text = "Open Folder", Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center }
                    };
                folderMenuItem.Click += this.folderMenuItem_Click;
                menuItems.Add(folderMenuItem);

                // File Menu Item
                MenuItem fileMenuItem = new MenuItem
                    {
                        Icon = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/Movies.png")), Width = 16, Height = 16 },
                        Header = new TextBlock { Text = "Open File", Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center }
                    };
                fileMenuItem.Click += this.fileMenuItem_Click;
                menuItems.Add(fileMenuItem);

                // File Menu Item
                MenuItem titleSpecific = new MenuItem { Header = new TextBlock { Text = "Title Specific Scan", Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center } };

                MenuItem titleSpecificFolder = new MenuItem
                  {
                      Icon = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/folder.png")), Width = 16, Height = 16 },
                      Header = new TextBlock { Text = "Open Folder", Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center }
                  };
                MenuItem titleSpecificFile = new MenuItem
                  {
                      Icon = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/Movies.png")), Width = 16, Height = 16 },
                      Header = new TextBlock { Text = "Open File", Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center }
                  };
                titleSpecificFolder.Click += this.titleSpecificFolder_Click;
                titleSpecificFile.Click += this.titleSpecificFile_Click;

                titleSpecific.Items.Add(titleSpecificFolder);
                titleSpecific.Items.Add(titleSpecificFile);

                menuItems.Add(titleSpecific);

                // Drives
                foreach (DriveInformation item in GeneralUtilities.GetDrives())
                {
                    MenuItem driveMenuItem = new MenuItem
                        {
                            Icon = new Image { Source = new BitmapImage(new Uri("pack://application:,,,/HandBrake;component/Views/Images/disc_small.png")), Width = 16, Height = 16 },
                            Header = new TextBlock { Text = string.Format("{0} ({1})", item.RootDirectory, item.VolumeLabel), Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center },
                            Tag = item
                        };
                    driveMenuItem.Click += this.driveMenuItem_Click;
                    menuItems.Add(driveMenuItem);
                }


                return menuItems;
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
                // The title that is selected has a source name. This means it's part of a batch scan.
                if (selectedTitle != null && !string.IsNullOrEmpty(selectedTitle.SourceName))
                {
                    return Path.GetFileName(selectedTitle.SourceName);
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
                this.NotifyOfPropertyChange("IsMkv");
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

        #endregion

        #region Properties for Settings

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
                if (!Equals(this.selectedTitle, value))
                {
                    this.selectedTitle = value;

                    if (selectedTitle == null)
                    {
                        return;
                    }

                    // Use the Path on the Title, or the Source Scan path if one doesn't exist.
                    this.CurrentTask.Source = !string.IsNullOrEmpty(this.selectedTitle.SourceName) ? this.selectedTitle.SourceName : this.ScannedSource.ScanPath;
                    this.CurrentTask.Title = value.TitleNumber;
                    this.NotifyOfPropertyChange(() => this.StartEndRangeItems);
                    this.NotifyOfPropertyChange(() => this.SelectedTitle);
                    this.NotifyOfPropertyChange(() => this.Angles);

                    // Default the Start and End Point dropdowns
                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = selectedTitle.Chapters.Last().ChapterNumber;
                    this.SelectedPointToPoint = PointToPointMode.Chapters;
                    this.SelectedAngle = 1;

                    if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
                    {
                        this.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
                    }
                    this.NotifyOfPropertyChange(() => this.CurrentTask);

                    this.Duration = selectedTitle.Duration.ToString();

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

                if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
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

                if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
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
                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = selectedTitle.Chapters.Last().ChapterNumber;
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
                this.SetExtension(string.Format(".{0}", this.selectedOutputFormat.ToString().ToLower())); // TODO, tidy up

                this.VideoViewModel.RefreshTask();
                this.AudioViewModel.RefreshTask();
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
            UpdateCheckHelper.PerformStartupUpdateCheck();

            // Setup the presets.
            if (this.presetService.CheckIfPresetsAreOutOfDate())
                if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
                    this.errorService.ShowMessageBox("HandBrake has determined your built-in presets are out of date... These presets will now be updated.",
                                    "Preset Update", MessageBoxButton.OK, MessageBoxImage.Information);

            // Queue Recovery
            QueueRecoveryHelper.RecoverQueue(this.queueProcessor, this.errorService);

            this.SelectedPreset = this.presetService.DefaultPreset;
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
            this.queueProcessor.JobProcessingStarted -= this.QueueProcessorJobProcessingStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }

        /// <summary>
        /// Handle the Window Closing Event and warn the user if an encode is in-progress
        /// </summary>
        /// <param name="e">
        /// The CancelEventArgs.
        /// </param>
        public void HandleWindowClosing(CancelEventArgs e)
        {
            if (this.encodeService.IsEncoding)
            {
                MessageBoxResult result = this.errorService.ShowMessageBox("HandBrake is currently encoding. Closing now will abort your encode.\nAre you sure you wish to exit?", "Warning", MessageBoxButton.YesNo, MessageBoxImage.Warning);
                if (result == MessageBoxResult.No)
                {
                    e.Cancel = true;
                } 
                else
                {
                    this.encodeService.Stop();
                }
            }
        }

        #endregion

        #region Menu and Taskbar

        /// <summary>
        /// Open the About Window
        /// </summary>
        public void OpenAboutApplication()
        {
            this.WindowManager.ShowWindow(IoC.Get<IAboutViewModel>());
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
            this.WindowManager.ShowWindow(IoC.Get<ILogViewModel>());
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenQueueWindow()
        {
            this.WindowManager.ShowWindow(IoC.Get<IQueueViewModel>());
        }

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        public void OpenPreviewWindow()
        {
            IPreviewViewModel viewModel = IoC.Get<IPreviewViewModel>();
            this.WindowManager.ShowWindow(viewModel);
            viewModel.Task = this.CurrentTask;
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
            UpdateCheckHelper.CheckForUpdates();
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
            if (!this.queueProcessor.QueueManager.CheckForDestinationPathDuplicates(task.Task.Destination))
            {
                this.queueProcessor.QueueManager.Add(task);
            } 
            else
            {
                this.errorService.ShowMessageBox("There are jobs on the queue with the same destination path. Please choose a different path for this job.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            

            if (!this.IsEncoding)
            {
                this.ProgramStatusLabel = string.Format("{0} Encodes Pending", this.queueProcessor.QueueManager.Count);
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
            if (this.queueProcessor.QueueManager.Count != 0)
            {
                this.queueProcessor.Start();
                return;
            }

            // Otherwise, perform Santiy Checking then add to the queue and start if everything is ok.
            if (this.ScannedSource == null || this.CurrentTask == null)
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
            this.queueProcessor.QueueManager.Add(task);
            this.queueProcessor.Start();
            this.IsEncoding = true;
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
                QueryGeneratorUtility.GenerateQuery(this.CurrentTask),
                "CLI Query",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
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
                if (fileNames != null && fileNames.Any() && (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])) )
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
            VistaSaveFileDialog dialog = new VistaSaveFileDialog
                {
                    Filter = "mp4|*.mp4;*.m4v|mkv|*.mkv",
                    AddExtension = true,
                    OverwritePrompt = true,
                    DefaultExt = ".mp4",
                };

            if (this.CurrentTask != null && !string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                if (Directory.Exists(Path.GetDirectoryName(this.CurrentTask.Destination)))
                {
                    dialog.InitialDirectory = Path.GetDirectoryName(this.CurrentTask.Destination) + "\\";
                    dialog.FileName = Path.GetFileName(this.CurrentTask.Destination);
                }
            }

            dialog.ShowDialog();
            this.Destination = dialog.FileName;

            // Set the Extension Dropdown. This will also set Mp4/m4v correctly.
            if (!string.IsNullOrEmpty(dialog.FileName))
            {
                switch (Path.GetExtension(dialog.FileName))
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
                Preset preset = PlistUtility.Import(filename);
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
            VistaSaveFileDialog savefiledialog = new VistaSaveFileDialog { Filter = "plist|*.plist", CheckPathExists = true };
            if (this.selectedPreset != null)
            {
                savefiledialog.ShowDialog();
                string filename = savefiledialog.FileName;

                if (filename != null)
                {
                    PlistUtility.Export(savefiledialog.FileName, this.selectedPreset);
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

        #endregion

        #region Private Methods

        /// <summary>
        /// Start a Scan
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        private void StartScan(string filename, int title)
        {
            // TODO 
            // 1. Disable GUI.
            this.scanService.Scan(filename, title, this.UserSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount));
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
            Execute.OnUIThread(() =>
                {
                    if (e.Successful)
                    {
                        this.scanService.SouceData.CopyTo(this.ScannedSource);
                        this.NotifyOfPropertyChange("ScannedSource");
                        this.NotifyOfPropertyChange("ScannedSource.Titles");
                        this.SelectedTitle = this.ScannedSource.Titles.FirstOrDefault(t => t.MainTitle)
                                             ?? this.ScannedSource.Titles.FirstOrDefault();
                        this.JobContextService.CurrentSource = this.ScannedSource;
                        this.JobContextService.CurrentTask = this.CurrentTask;
                        this.SetupTabs();
                        this.ShowStatusWindow = false;
                    }

                    this.SourceLabel = "Scan Completed";
                    this.StatusLabel = "Scan Completed";
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
                    this.ProgramStatusLabel =
                        string.Format(
                            "{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Elapsed: {4:hh\\:mm\\:ss},  Pending Jobs {5}",
                            e.PercentComplete,
                            e.CurrentFrameRate,
                            e.AverageFrameRate,
                            e.EstimatedTimeLeft,
                            e.ElapsedTime,
                            this.queueProcessor.QueueManager.Count);
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
        /// Drive Scan
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedEventArgs.
        /// </param>
        private void driveMenuItem_Click(object sender, RoutedEventArgs e)
        {
            MenuItem item = e.OriginalSource as MenuItem;
            if (item != null)
            {
                this.StartScan(((DriveInformation)item.Tag).RootDirectory, 0);
            }
        }

        /// <summary>
        /// File Scan
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedEventArgs.
        /// </param>
        private void fileMenuItem_Click(object sender, RoutedEventArgs e)
        {
            this.FileScan();
        }

        /// <summary>
        /// Folder Scan
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedEventArgs.
        /// </param>
        private void folderMenuItem_Click(object sender, RoutedEventArgs e)
        {
            this.FolderScan();
        }

        /// <summary>
        /// Title Specific Scan for File
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void titleSpecificFile_Click(object sender, RoutedEventArgs e)
        {
            this.FileScanTitleSpecific();
        }

        /// <summary>
        /// Title Specific Scan for folder
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void titleSpecificFolder_Click(object sender, RoutedEventArgs e)
        {
            this.FolderScanTitleSpecific();
        }

        #endregion
    }
}