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
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Model.General;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    using HandBrakeWPF.Services.Interfaces;

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

        public string sourcePath;
        private string dvdDrivePath;
        private string dvdDriveLabel;
        private List<DriveInformation> drives;

        /// <summary>
        /// The Toolbar Status Label
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
        [ImportingConstructor]
        public MainViewModel(IWindowManager windowManager, IUserSettingService userSettingService, IScan scanService, IEncode encodeService, IPresetService presetService,
            IErrorService errorService)
        {
            this.scanService = scanService;
            this.encodeService = encodeService;
            this.presetService = presetService;
            this.errorService = errorService;
            this.userSettingService = userSettingService;
            this.queueProcessor = IoC.Get<IQueueProcessor>(); // TODO Instance ID!

            // Setup Properties
            this.WindowTitle = "HandBrake WPF Test Application";
            this.CurrentTask = new EncodeTask();
            this.ScannedSource = new Source();
            this.SelectedPreset = this.presetService.DefaultPreset;

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueuePaused += this.QueuePaused;
            this.queueProcessor.EncodeService.EncodeStarted += this.EncodeStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
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
                if (!object.Equals(this.windowName, value))
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
                this.NotifyOfPropertyChange("SelectedPreset");
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
        /// Gets SourceName.
        /// </summary>
        public string SourceName
        {
            get
            {
                // TODO Disc Label
                //if (this.selectedSourceType == SourceType.DvdDrive)
                //{
                //    return this.dvdDriveLabel;
                //}

                if (selectedTitle != null && !string.IsNullOrEmpty(selectedTitle.SourceName))
                {
                    return Path.GetFileName(selectedTitle.SourceName);
                }

                // We have a drive, selected as a folder.
                if (this.sourcePath.EndsWith("\\"))
                {
                    drives = GeneralUtilities.GetDrives();
                    foreach (DriveInformation item in drives)
                    {
                        if (item.RootDirectory.Contains(this.sourcePath))
                        {
                            return item.VolumeLabel;
                        }
                    }
                }

                if (Path.GetFileNameWithoutExtension(this.sourcePath) != "VIDEO_TS")
                    return Path.GetFileNameWithoutExtension(this.sourcePath);

                return Path.GetFileNameWithoutExtension(Path.GetDirectoryName(this.sourcePath));
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

                return this.SelectedTitle.Chapters.Select(item => item.ChapterNumber).Select(dummy => (int)dummy).ToList();
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
                this.NotifyOfPropertyChange("IsEncoding");
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

                    if (selectedTitle == null)
                    {
                        return;
                    }

                    // Use the Path on the Title, or the Source Scan path if one doesn't exist.
                    this.CurrentTask.Source = !string.IsNullOrEmpty(this.selectedTitle.SourceName) ? this.selectedTitle.SourceName : this.ScannedSource.ScanPath;
                    this.CurrentTask.Title = value.TitleNumber;
                    this.NotifyOfPropertyChange("StartEndRangeItems");
                    this.NotifyOfPropertyChange("SelectedTitle");
                    this.NotifyOfPropertyChange("Angles");

                    // Default the Start and End Point dropdowns
                    this.SelectedStartPoint = 1;
                    this.SelectedEndPoint = selectedTitle.Chapters.Last().ChapterNumber;
                    this.SelectedPointToPoint = PointToPointMode.Chapters;
                    this.SelectedAngle = 1;

                    this.CurrentTask.Destination = AutoNameHelper.AutoName(this.CurrentTask, this.SourceName);
                    this.NotifyOfPropertyChange("CurrentTask");

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
                this.CurrentTask.EndPoint = value;
                this.NotifyOfPropertyChange("SelectedAngle");
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
                this.NotifyOfPropertyChange("SelectedStartPoint");
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
                this.NotifyOfPropertyChange("SelectedEndPoint");
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
                this.NotifyOfPropertyChange("SelectedPointToPoint");
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
                this.NotifyOfPropertyChange("SelectedOutputFormat");
                this.NotifyOfPropertyChange("IsMkv");
                this.SetExtension(string.Format(".{0}", this.selectedOutputFormat.ToString().ToLower())); // TODO, tidy up
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
            this.WindowManager.ShowWindow(IoC.Get<IAboutViewModel>());
        }

        /// <summary>
        /// Open the Options Window
        /// </summary>
        public void OpenOptionsWindow()
        {
            this.WindowManager.ShowWindow(IoC.Get<IOptionsViewModel>());
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
            this.WindowManager.ShowWindow(IoC.Get<IPreviewViewModel>());
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
            // TODO The update service needs refactoring.
            this.userSettingService.SetUserSetting(UserSettingConstants.LastUpdateCheckDate, DateTime.Now);
            string url = userSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakePlatform).Contains("x86_64")
                                                  ? userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_x64)
                                                  : userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_i686);
            UpdateService.BeginCheckForUpdates(UpdateCheckHelper.UpdateCheckDoneMenu, false,
                url, userSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild),
                userSettingService.GetUserSetting<int>(UserSettingConstants.Skipversion));
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

            QueueTask task = new QueueTask
                                 {
                                     Task = this.CurrentTask,
                                     Query = QueryGeneratorUtility.GenerateQuery(this.CurrentTask)
                                 };
            this.queueProcessor.QueueManager.Add(task);

            if (!this.IsEncoding)
            {
                this.ProgramStatusLabel = string.Format("{0} Encodes Pending", this.queueProcessor.QueueManager.Count);
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
            // Santiy Checking.
            if (this.ScannedSource == null || this.CurrentTask == null)
            {
                this.errorService.ShowMessageBox("You must first scan a source.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                this.errorService.ShowMessageBox("The Destination field was empty.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.queueProcessor.IsProcessing)
            {
                this.errorService.ShowMessageBox("HandBrake is already encoding.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (File.Exists(this.CurrentTask.Destination))
            {
                MessageBoxResult result = this.errorService.ShowMessageBox("The current file already exists, do you wish to overwrite it?", "Question", MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.No)
                {
                    return;
                }
            }

            // Create the Queue Task and Start Processing
            QueueTask task = new QueueTask(null)
                {
                    Destination = this.CurrentTask.Destination,
                    Task = this.CurrentTask,
                    Query = QueryGeneratorUtility.GenerateQuery(this.CurrentTask),
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
            this.encodeService.Stop();
        }

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        public void ExitApplication()
        {
            Application.Current.Shutdown();
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
                if (fileNames != null && fileNames.Count() >= 1 && File.Exists(fileNames[0]))
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
                    DefaultExt = ".mp4"
                };
            dialog.ShowDialog();

            this.CurrentTask.Destination = dialog.FileName;
            this.NotifyOfPropertyChange("CurrentTask");
            this.SetExtension(Path.GetExtension(dialog.FileName));
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
                EncodeTask parsed = PlistPresetHandler.Import(filename);
                if (this.presetService.CheckIfPresetExists(parsed.PresetName))
                {
                    if (!presetService.CanUpdatePreset(parsed.PresetName))
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
                        Preset preset = new Preset { Name = parsed.PresetName, CropSettings = parsed.UsesPictureSettings, Task = parsed };

                        presetService.Update(preset);
                    }
                }
                else
                {
                    Preset preset = new Preset { Name = parsed.PresetName, Task = parsed, CropSettings = parsed.UsesPictureSettings, };
                    presetService.Add(preset);
                }

                this.NotifyOfPropertyChange("Presets");
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
                    PlistPresetHandler.Export(savefiledialog.FileName, this.selectedPreset);
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
        /// Set the selected preset.
        /// </summary>
        /// <param name="e">
        /// The RoutedPropertyChangedEventArgs.
        /// </param>
        public void SetSelectedPreset(RoutedPropertyChangedEventArgs<object> e)
        {
            this.SelectedPreset = e.NewValue as Preset;
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
            this.sourcePath = filename;
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

                this.selectedOutputFormat = OutputFormat.Mp4;
                this.IsMkv = false;
            }

            // Now disable controls that are not required. The Following are for MP4 only!
            if (newExtension == ".mkv")
            {
                this.IsMkv = true;
                this.CurrentTask.LargeFile = false;
                this.CurrentTask.OptimizeMP4 = false;
                this.CurrentTask.IPod5GSupport = false;
                this.selectedOutputFormat = OutputFormat.Mkv;
            }

            // Update The browse file extension display
            if (Path.HasExtension(newExtension))
            {
                this.CurrentTask.Destination = Path.ChangeExtension(this.CurrentTask.Destination, newExtension);
            }

            // Update the UI Display
            this.NotifyOfPropertyChange("CurrentTask");
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
            Caliburn.Micro.Execute.OnUIThread(() =>
                {
                    if (e.Successful)
                    {
                        this.scanService.SouceData.CopyTo(this.ScannedSource);
                        this.NotifyOfPropertyChange("ScannedSource");
                        this.NotifyOfPropertyChange("ScannedSource.Titles");
                        this.SelectedTitle = this.ScannedSource.Titles.Where(t => t.MainTitle).FirstOrDefault()
                                             ?? this.ScannedSource.Titles.FirstOrDefault();
                        this.JobContextService.CurrentSource = this.ScannedSource;
                        this.JobContextService.CurrentTask = this.CurrentTask;
                        this.SetupTabs();
                    }

                    this.SourceLabel = "Scan Completed";
                });

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
            ProgramStatusLabel =
                string.Format(
                "{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Elapsed: {4:hh\\:mm\\:ss},  Pending Jobs {5}",
                e.PercentComplete,
                e.CurrentFrameRate,
                e.AverageFrameRate,
                e.EstimatedTimeLeft,
                e.ElapsedTime,
                this.queueProcessor.QueueManager.Count);
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
            this.IsEncoding = false;

            // TODO Handle Updating the UI
        }
        #endregion
    }
}