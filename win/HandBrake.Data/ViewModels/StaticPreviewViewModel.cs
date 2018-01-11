// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StaticPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Static Preview View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.ExceptionServices;
    using System.Threading;

    using HandBrake;
    using HandBrake.CoreLibrary.Interop.Model.Encoding;
    using HandBrake.CoreLibrary.Model;
    using HandBrake.Model;
    using HandBrake.Model.Prompts;
    using HandBrake.Factories;
    using HandBrake.Properties;
    using HandBrake.Services.Encode.Model.Models;
    using HandBrake.Services.Interfaces;
    using HandBrake.Services.Queue.Model;
    using HandBrake.Services.Scan.Interfaces;
    using HandBrake.Services.Scan.Model;
    using HandBrake.ViewModels.Interfaces;
    using PlatformBindings;
    using EncodeCompletedEventArgs = HandBrake.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrake.Services.Encode.EventArgs.EncodeProgressEventArgs;
    using EncodeTask = HandBrake.Services.Encode.Model.EncodeTask;
    using IEncode = HandBrake.Services.Encode.Interfaces.IEncode;
    using LibEncode = HandBrake.Services.Encode.LibEncode;
    using OutputFormat = HandBrake.Services.Encode.Model.Models.OutputFormat;
    using PointToPointMode = HandBrake.Services.Encode.Model.Models.PointToPointMode;
    using HandBrake.Common;
    using HandBrake.Utilities;

    /// <summary>
    ///     The Static Preview View Model
    /// </summary>
    public class StaticPreviewViewModel : ViewModelBase, IStaticPreviewViewModel
    {
        /*
         * TODO
         * - Window Size / Scale to screen etc.
         */

        #region Fields

        /// <summary>
        ///     The scan service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        /// Backing field for the encode service.
        /// </summary>
        private readonly IEncode encodeService;

        /// <summary>
        /// The error service
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The user Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// System Information.
        /// </summary>
        private readonly ISystemInfo systemInfo;

        /// <summary>
        /// The Launcher Service.
        /// </summary>
        private readonly LauncherServiceBase launcher;

        /// <summary>
        ///     The height.
        /// </summary>
        private int height;

        /// <summary>
        ///     The preview image.
        /// </summary>
        private ImageData previewImage;

        /// <summary>
        ///     The selected preview image.
        /// </summary>
        private int selectedPreviewImage;

        /// <summary>
        ///     The width.
        /// </summary>
        private int width;

        /// <summary>
        /// The preview not available.
        /// </summary>
        private bool previewNotAvailable;

        /// <summary>
        /// The percentage.
        /// </summary>
        private string percentage;

        /// <summary>
        /// The percentage value.
        /// </summary>
        private double percentageValue;

        /// <summary>
        /// The Backing field for IsEncoding
        /// </summary>
        private bool isEncoding;

        /// <summary>
        /// Backing field for use system default player
        /// </summary>
        private bool useSystemDefaultPlayer;

        #endregion Fields

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="StaticPreviewViewModel"/> class.
        /// </summary>
        /// <param name="scanService">
        /// The scan service.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        /// <param name="launcher">
        /// The Launcher Service.
        /// </param>
        /// <param name="systemInfo">
        /// System Information.
        /// </param>
        public StaticPreviewViewModel(IScan scanService, IUserSettingService userSettingService, IErrorService errorService, LauncherServiceBase launcher, ISystemInfo systemInfo)
        {
            this.scanService = scanService;
            this.selectedPreviewImage = 1;
            this.Title = Resources.Preview;
            this.PreviewNotAvailable = true;
            this.systemInfo = systemInfo;

            // Live Preview
            this.userSettingService = userSettingService;
            this.errorService = errorService;
            this.launcher = launcher;
            this.encodeService = new LibEncode(); // Preview needs a seperate instance rather than the shared singleton. This could maybe do with being refactored at some point

            this.Title = "Preview";
            this.Percentage = "0.00%";
            this.PercentageValue = 0;
            this.Duration = 30;
            this.CanPlay = true;

            UseSystemDefaultPlayer = userSettingService.GetUserSetting<bool>(UserSettingConstants.DefaultPlayer);
            this.Duration = userSettingService.GetUserSetting<int>(UserSettingConstants.LastPreviewDuration);
        }

        #endregion Constructors and Destructors

        #region Public Properties

        /// <summary>
        ///     Gets or sets the height.
        /// </summary>
        public int Height
        {
            get
            {
                return this.height;
            }
            set
            {
                if (value == this.height)
                {
                    return;
                }
                this.height = this.FixHeight(value);
                this.NotifyOfPropertyChange(() => this.Height);
            }
        }

        /// <summary>
        ///     Gets or sets the preview image.
        /// </summary>
        public ImageData PreviewImage
        {
            get
            {
                return this.previewImage;
            }
            set
            {
                if (Equals(value, this.previewImage))
                {
                    return;
                }

                this.previewImage = value;
                this.NotifyOfPropertyChange(() => this.PreviewImage);
            }
        }

        /// <summary>
        ///     Gets or sets the selected preview image.
        /// </summary>
        public int SelectedPreviewImage
        {
            get
            {
                return this.selectedPreviewImage;
            }
            set
            {
                if (value == this.selectedPreviewImage)
                {
                    return;
                }
                this.selectedPreviewImage = value;
                this.NotifyOfPropertyChange(() => this.SelectedPreviewImage);

                this.UpdatePreviewFrame();
            }
        }

        /// <summary>
        ///     Gets or sets the task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets the scanned source.
        /// </summary>
        public Source ScannedSource { get; set; }

        /// <summary>
        ///     Gets the total previews.
        /// </summary>
        public int TotalPreviews
        {
            get
            {
                return this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount) - 1;
            }
        }

        /// <summary>
        ///     Gets or sets the width.
        /// </summary>
        public int Width
        {
            get
            {
                return this.width;
            }
            set
            {
                if (value == this.width)
                {
                    return;
                }
                this.width = this.FixWidth(value);
                this.NotifyOfPropertyChange(() => this.Width);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether preview not available.
        /// </summary>
        public bool PreviewNotAvailable
        {
            get
            {
                return this.previewNotAvailable;
            }
            set
            {
                if (value.Equals(this.previewNotAvailable))
                {
                    return;
                }
                this.previewNotAvailable = value;
                this.NotifyOfPropertyChange(() => this.PreviewNotAvailable);
            }
        }

        #endregion Public Properties

        #region LivePreviewProperties

        /// <summary>
        /// Gets AvailableDurations.
        /// </summary>
        public IEnumerable<int> AvailableDurations
        {
            get
            {
                return new List<int> { 5, 10, 30, 45, 60, 75, 90, 105, 120, 150, 180, 210, 240 };
            }
        }

        /// <summary>
        /// Gets or sets Duration.
        /// </summary>
        public int Duration { get; set; }

        /// <summary>
        /// Gets or sets Percentage.
        /// </summary>
        public string Percentage
        {
            get
            {
                return this.percentage;
            }

            set
            {
                this.percentage = value;
                this.NotifyOfPropertyChange(() => this.Percentage);
            }
        }

        /// <summary>
        /// Gets or sets PercentageValue.
        /// </summary>
        public double PercentageValue
        {
            get
            {
                return this.percentageValue;
            }

            set
            {
                this.percentageValue = value;
                this.NotifyOfPropertyChange(() => this.PercentageValue);
            }
        }

        /// <summary>
        /// Gets StartPoints.
        /// </summary>
        public IEnumerable<int> StartPoints
        {
            get
            {
                List<int> startPoints = new List<int>();
                for (int i = 1;
                     i <= this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);
                     i++)
                {
                    startPoints.Add(i);
                }

                return startPoints;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether UseSystemDefaultPlayer.
        /// </summary>
        public bool UseSystemDefaultPlayer
        {
            get
            {
                return this.useSystemDefaultPlayer;
            }
            set
            {
                this.useSystemDefaultPlayer = value;
                this.NotifyOfPropertyChange(() => UseSystemDefaultPlayer);
                this.userSettingService.SetUserSetting(UserSettingConstants.DefaultPlayer, value);
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
                this.CanPlay = !value;
                this.NotifyOfPropertyChange(() => this.CanPlay);
                this.NotifyOfPropertyChange(() => this.IsEncoding);
            }
        }

        /// <summary>
        /// Gets or sets the Currently Playing / Encoding Filename.
        /// </summary>
        public string CurrentlyPlaying { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether can play.
        /// </summary>
        public bool CanPlay { get; set; }

        #endregion LivePreviewProperties

        #region Public Methods and Operators

        /// <summary>
        /// The update preview frame.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="scannedSource">
        /// The scanned Source.
        /// </param>
        public void UpdatePreviewFrame(EncodeTask task, Source scannedSource)
        {
            this.Task = task;
            this.UpdatePreviewFrame();
            this.DisplayName = Resources.StaticPreviewViewModel_Title;
            this.Title = Resources.Preview;
            this.ScannedSource = scannedSource;
        }

        /// <summary>
        /// Gets or sets a value indicating whether is open.
        /// </summary>
        public bool IsOpen { get; set; }

        public void NextPreview()
        {
            int maxPreview = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);
            if ((this.SelectedPreviewImage + 1) == maxPreview)
            {
                return;
            }

            this.SelectedPreviewImage = this.SelectedPreviewImage + 1;
        }

        public void PreviousPreview()
        {
            if (this.SelectedPreviewImage < 1)
            {
                return;
            }

            this.SelectedPreviewImage = this.SelectedPreviewImage - 1;
        }

        /// <summary>
        ///     The update preview frame.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        public void UpdatePreviewFrame()
        {
            // Don't preview for small images.
            if (this.Task.Anamorphic == Anamorphic.Loose && this.Task.Width < 32)
            {
                PreviewNotAvailable = true;
                return;
            }

            if ((this.Task.Anamorphic == Anamorphic.None || this.Task.Anamorphic == Anamorphic.Custom) && (this.Task.Width < 32 || this.Task.Height < 32))
            {
                PreviewNotAvailable = true;
                return;
            }

            ImageData image = null;
            try
            {
                image = this.scanService.GetPreview(this.Task, this.SelectedPreviewImage, HBConfigurationFactory.Create());
            }
            catch (Exception exc)
            {
                PreviewNotAvailable = true;
                Debug.WriteLine(exc);
            }

            if (image != null)
            {
                PreviewNotAvailable = false;
                this.Width = image.Width;
                this.Height = image.Height;
                this.PreviewImage = image;
            }
        }

        /// <summary>
        /// The preview size changed.
        /// </summary>
        /// <param name="ea">
        /// The ea.
        /// </param>
        public int FixWidth(int width)
        {
            var screendimensions = this.systemInfo.ScreenBounds;
            if (width > screendimensions.Width)
            {
                return screendimensions.Width - 50;
            }

            return width;
        }

        public int FixHeight(int height)
        {
            var screendimensions = this.systemInfo.ScreenBounds;
            if (height > screendimensions.Height)
            {
                return screendimensions.Height - 50;
            }

            return height;
        }

        #endregion Public Methods and Operators

        #region Public Method - Live Preview

        #region Public Methods

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }

        /// <summary>
        /// Handle The Initialisation
        /// </summary>
        public override void OnLoad()
        {
        }

        /// <summary>
        /// Encode and play a sample
        /// </summary>
        public void Play()
        {
            try
            {
                this.IsEncoding = true;
                if (File.Exists(this.CurrentlyPlaying))
                    File.Delete(this.CurrentlyPlaying);
            }
            catch (Exception)
            {
                this.IsEncoding = false;
                this.errorService.ShowMessageBox(Resources.StaticPreview_UnableToDeletePreview,
                               Resources.Error, DialogButtonType.OK, DialogType.Error);
            }

            if (this.Task == null || string.IsNullOrEmpty(Task.Source))
            {
                this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_ScanFirst,
                               Resources.Error, DialogButtonType.OK, DialogType.Error);
                return;
            }

            EncodeTask encodeTask = new EncodeTask(this.Task)
            {
                PreviewEncodeDuration = this.Duration,
                PreviewEncodeStartAt = this.SelectedPreviewImage,
                PointToPointMode = PointToPointMode.Preview
            };

            // Filename handling.
            if (encodeTask.Destination == null)
            {
                string filename = Path.ChangeExtension(Path.GetTempFileName(), encodeTask.OutputFormat == OutputFormat.Mkv ? "m4v" : "mkv");
                encodeTask.Destination = new FileData(AsyncHelpers.GetThreadedResult(() => AppServices.Current?.IO?.GetFile(filename)));
                this.CurrentlyPlaying = filename;
            }
            else
            {
                var path = encodeTask.Destination.Path;

                string directory = Path.GetDirectoryName(path) ?? string.Empty;
                string filename = Path.GetFileNameWithoutExtension(path);
                string extension = Path.GetExtension(path);
                string previewFilename = string.Format("{0}_preview{1}", filename, extension);
                string previewFullPath = Path.Combine(directory, previewFilename);

                // This will throw an exception in most platforms if outside of the permissions boundary (Such as UWP).
                // Can this be done with encodeTask.Destination.RenameAsync() ?
                encodeTask.Destination = new FileData(AsyncHelpers.GetThreadedResult(() => AppServices.Current?.IO?.CreateFile(previewFullPath)));
                this.CurrentlyPlaying = previewFullPath;
            }

            // Setup the encode task as a preview encode
            encodeTask.IsPreviewEncode = true;
            encodeTask.PreviewEncodeStartAt = this.SelectedPreviewImage + 1;
            encodeTask.PreviewEncodeDuration = this.Duration;

            SubtitleTrack scanTrack = null;
            foreach (var track in encodeTask.SubtitleTracks)
            {
                if (track.SourceTrack != null && track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch)
                {
                    scanTrack = track;
                    break;
                }
            }

            if (scanTrack != null)
            {
                encodeTask.SubtitleTracks.Remove(scanTrack);
            }

            QueueTask task = new QueueTask(encodeTask, HBConfigurationFactory.Create(), this.ScannedSource.ScanPath);
            ThreadPool.QueueUserWorkItem(this.CreatePreview, task);
        }

        #endregion Public Methods

        #region Private Methods

        /// <summary>
        /// Play the Encoded file
        /// </summary>
        private void PlayFile()
        {
            // Launch VLC and Play video.
            if (this.CurrentlyPlaying != string.Empty)
            {
                if (File.Exists(this.CurrentlyPlaying))
                {
                    this.launcher.PlayFile(this.CurrentlyPlaying);
                }
                else
                {
                    this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_UnableToPlayFile,
                                 Resources.Error, DialogButtonType.OK, DialogType.Warning);
                }
            }
        }

        /// <summary>
        /// Create the Preview.
        /// </summary>
        /// <param name="state">
        /// The state.
        /// </param>
        private void CreatePreview(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (encodeService.IsEncoding)
            {
                this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_AlreadyEncoding,
                               Resources.Error, DialogButtonType.OK, DialogType.Error);
                return;
            }

            this.encodeService.EncodeCompleted += this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged += this.encodeService_EncodeStatusChanged;

            this.encodeService.Start(((QueueTask)state).Task, ((QueueTask)state).Configuration);
            this.userSettingService.SetUserSetting(UserSettingConstants.LastPreviewDuration, this.Duration);
        }

        #endregion Private Methods

        #region Event Handlers

        /// <summary>
        /// Handle Encode Progress Events
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeProgressEventArgs.
        /// </param>
        private void encodeService_EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            this.Percentage = string.Format("{0} %", Math.Round(e.PercentComplete, 2).ToString(CultureInfo.InvariantCulture));
            this.PercentageValue = e.PercentComplete;
        }

        /// <summary>
        /// Handle the Encode Completed Event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        private void encodeService_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.Percentage = "0.00%";
            this.PercentageValue = 0;
            this.IsEncoding = false;

            this.encodeService.EncodeCompleted -= this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.encodeService_EncodeStatusChanged;

            this.PlayFile();
        }

        #endregion Event Handlers
        #endregion Public Method - Live Preview
    }
}