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

    using Castle.Components.DictionaryAdapter;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

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

        /// <summary>
        /// Backing field for the selected title.
        /// </summary>
        private Title selectedTitle;

        /// <summary>
        /// Backing field for duration
        /// </summary>
        private string duration;

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
            this.queueProcessor = IoC.Get<IQueueProcessor>(); // TODO Instance ID!

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

        /* Properties for User Selections */

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
            // Santiy Checking.
            if (this.ScannedSource == null || this.CurrentTask == null)
            {
                throw new GeneralApplicationException("You must first scan a source.", string.Empty, null);
            }

            if (string.IsNullOrEmpty(this.CurrentTask.Destination))
            {
                throw new GeneralApplicationException("The Destination field was empty.", "You must first set a destination for the encoded file.", null);
            }

            if (this.queueProcessor.IsProcessing)
            {
                throw new GeneralApplicationException("HandBrake is already encoding.", string.Empty, null);
            }

            if (File.Exists(this.CurrentTask.Destination))
            {
                // TODO: File Overwrite warning.
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
        /// The Destination Path
        /// </summary>
        public void BrowseDestination()
        {
            VistaSaveFileDialog dialog = new VistaSaveFileDialog { Filter = "MP4 File (*.mp4)|Mkv File(*.mkv)" };
            dialog.ShowDialog();
            dialog.AddExtension = true;
            this.CurrentTask.Destination = dialog.FileName;
            this.NotifyOfPropertyChange("CurrentTask");
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
            this.scanService.Scan(filename, title, this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount));
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
                this.NotifyOfPropertyChange("T");
                this.SelectedTitle = this.ScannedSource.Titles.Where(t => t.MainTitle).FirstOrDefault();
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
            // TODO Handle Updating the UI
        }
        #endregion
    }
}