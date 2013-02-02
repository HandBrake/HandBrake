// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The About View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Windows;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The About View Model
    /// </summary>
    public class PreviewViewModel : ViewModelBase, IPreviewViewModel
    {
        #region Constants and Fields

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

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="PreviewViewModel"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public PreviewViewModel(IEncodeServiceWrapper encodeService, IErrorService errorService, IUserSettingService userSettingService) 
        {
            this.encodeService = encodeService;
            this.errorService = errorService;
            this.userSettingService = userSettingService;
            this.Title = "Preview";
            this.Percentage = "0.00%";
            this.PercentageValue = 0;
            this.StartAt = 1;
            this.Duration = 30;

            UseSystemDefaultPlayer = userSettingService.GetUserSetting<bool>(UserSettingConstants.DefaultPlayer);
            this.Duration = userSettingService.GetUserSetting<int>(UserSettingConstants.LastPreviewDuration);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets AvailableDurations.
        /// </summary>
        public IEnumerable<int> AvailableDurations
        {
            get
            {
                return new List<int> { 5, 10, 30, 45, 60, 75, 90, 105, 120 };
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
        /// Gets or sets StartAt.
        /// </summary>
        public int StartAt { get; set; }

        /// <summary>
        /// Gets StartPoints.
        /// </summary>
        public IEnumerable<int> StartPoints
        {
            get
            {
                List<int> startPoints = new List<int>();
                for (int i = 1;
                     i <= this.UserSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount);
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
                this.NotifyOfPropertyChange(() => this.IsEncoding);
            }
        }

        /// <summary>
        /// Gets or sets the Currently Playing / Encoding Filename.
        /// </summary>
        public string CurrentlyPlaying { get; set; }

        #endregion

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
                this.errorService.ShowMessageBox("Unable to delete previous preview file. You may need to restart the application.",
                                "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (this.Task == null || string.IsNullOrEmpty(Task.Source))
            {
                this.errorService.ShowMessageBox("You must first scan a source and setup your encode before creating a perview.",
                               "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            EncodeTask encodeTask = new EncodeTask(this.Task)
                {
                    PreviewDuration = this.Duration,
                    PreviewStartAt = this.StartAt,
                    PointToPointMode = PointToPointMode.Preview
                };

            // Filename handling.
            if (string.IsNullOrEmpty(encodeTask.Destination))
            {
                string filename = Path.ChangeExtension(Path.GetTempFileName(), encodeTask.OutputFormat == OutputFormat.Mkv ? "m4v" : "mkv");
                encodeTask.Destination = filename;
                this.CurrentlyPlaying = filename;
            }
            else
            {
                string directory =  Path.GetDirectoryName(encodeTask.Destination) ?? string.Empty;
                string filename = Path.GetFileNameWithoutExtension(encodeTask.Destination);
                string extension = Path.GetExtension(encodeTask.Destination);
                string previewFilename = string.Format("{0}_preview{1}", filename, extension);
                string previewFullPath = Path.Combine(directory, previewFilename);
                encodeTask.Destination = previewFullPath;
                this.CurrentlyPlaying = previewFullPath;
            }
            
            // Setup the encode task as a preview encode
            encodeTask.IsPreviewEncode = true;
            encodeTask.PreviewEncodeStartAt = this.StartAt.ToString(CultureInfo.InvariantCulture);
            encodeTask.PreviewEncodeDuration = this.Duration;
            QueueTask task = new QueueTask
                {
                    Task = encodeTask,
                };

            ThreadPool.QueueUserWorkItem(this.CreatePreview, task);
        }

        #endregion

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
                    string args = "\"" + this.CurrentlyPlaying + "\"";

                    if (this.UseSystemDefaultPlayer)
                    {
                        Process.Start(args);
                    }
                    else
                    {
                        if (!File.Exists(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path)))
                        {
                            // Attempt to find VLC if it doesn't exist in the default set location.
                            string vlcPath;

                            if (8 == IntPtr.Size || (!String.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432"))))
                                vlcPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
                            else
                                vlcPath = Environment.GetEnvironmentVariable("ProgramFiles");

                            if (!string.IsNullOrEmpty(vlcPath))
                            {
                                vlcPath = Path.Combine(vlcPath, "VideoLAN\\VLC\\vlc.exe");
                            }

                            if (File.Exists(vlcPath))
                            {
                                UserSettingService.SetUserSetting(UserSettingConstants.VLC_Path, vlcPath);
                            }
                            else
                            {
                                this.errorService.ShowMessageBox("Unable to detect VLC Player. \nPlease make sure VLC is installed and the directory specified in HandBrake's options is correct. (See: \"Tools Menu > Options > Picture Tab\")",
                                                                 "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            }
                        }

                        if (File.Exists(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path)))
                        {
                            ProcessStartInfo vlc = new ProcessStartInfo(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path), args);
                            Process.Start(vlc);
                        }
                    }
                }
                else
                {
                    this.errorService.ShowMessageBox("Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.",
                                 "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
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
                this.errorService.ShowMessageBox("Handbrake is already encoding a video! Only one file can be encoded at any one time.",
                                "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.encodeService.EncodeCompleted += this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged += this.encodeService_EncodeStatusChanged;

            this.encodeService.Start((QueueTask)state, false);
            this.userSettingService.SetUserSetting(UserSettingConstants.LastPreviewDuration, this.Duration);
        }
        #endregion

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
        private void encodeService_EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            this.Percentage = string.Format("{0} %", e.PercentComplete.ToString(CultureInfo.InvariantCulture));
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
        private void encodeService_EncodeCompleted(object sender, HandBrake.ApplicationServices.EventArgs.EncodeCompletedEventArgs e)
        {
            this.Percentage = "0.00%";
            this.PercentageValue = 0;

            this.encodeService.EncodeCompleted -= this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.encodeService_EncodeStatusChanged;

            this.PlayFile();
        }
        #endregion
    }
}