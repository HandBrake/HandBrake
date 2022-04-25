﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StaticPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Static Preview View Model
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
    using System.Runtime.ExceptionServices;
    using System.Threading;
    using System.Windows;
    using System.Windows.Media.Imaging;

    using Caliburn.Micro;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeCompletedEventArgs = Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = Services.Encode.EventArgs.EncodeProgressEventArgs;
    using EncodeTask = Services.Encode.Model.EncodeTask;
    using IEncode = Services.Encode.Interfaces.IEncode;
    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;
    using LibEncode = Services.Encode.LibEncode;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using PointToPointMode = Services.Encode.Model.Models.PointToPointMode;

    public class StaticPreviewViewModel : ViewModelBase, IStaticPreviewViewModel
    {
        private readonly IScan scanService;
        private readonly IErrorService errorService;
        private readonly ILog logService;
        private readonly ILogInstanceManager logInstanceManager;
        private readonly IPortService portService;
        private readonly IUserSettingService userSettingService;

        private IEncode encodeService;
        private int height;
        private BitmapSource previewImage;
        private int selectedPreviewImage;
        private int width;
        private bool previewNotAvailable;
        private string percentage;
        private double percentageValue;
        private bool isEncoding;
        private bool useSystemDefaultPlayer;
        private bool showPictureSettingControls;

        public StaticPreviewViewModel(IScan scanService, IUserSettingService userSettingService, IErrorService errorService, ILog logService, ILogInstanceManager logInstanceManager, IPortService portService)
        {
            this.scanService = scanService;
            this.selectedPreviewImage = 1;
            this.Title = Resources.Preview;
            this.PreviewNotAvailable = true;

            // Live Preview
            this.userSettingService = userSettingService;
            this.errorService = errorService;
            this.logService = logService;
            this.logInstanceManager = logInstanceManager;
            this.portService = portService;

            this.Title = "Preview";
            this.Percentage = "0.00%";
            this.PercentageValue = 0;
            this.Duration = 30;
            this.CanPlay = true;

            this.useSystemDefaultPlayer = userSettingService.GetUserSetting<bool>(UserSettingConstants.DefaultPlayer);
            this.showPictureSettingControls = userSettingService.GetUserSetting<bool>(UserSettingConstants.PreviewShowPictureSettingsOverlay);
            this.Duration = userSettingService.GetUserSetting<int>(UserSettingConstants.LastPreviewDuration);
        }
        
        public IPictureSettingsViewModel PictureSettingsViewModel { get; private set; }

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

        public BitmapSource PreviewImage
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

        public EncodeTask Task { get; set; }

        public Source ScannedSource { get; set; }

        public int TotalPreviews
        {
            get
            {
                return this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount) - 1;
            }
        }

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

        public IEnumerable<int> AvailableDurations
        {
            get
            {
                return new List<int> { 5, 10, 30, 45, 60, 75, 90, 105, 120, 150, 180, 210, 240 };
            }
        }

        public int Duration { get; set; }

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

        public string CurrentlyPlaying { get; set; }

        public bool CanPlay { get; set; }

        public bool IsOpen { get; set; }

        public bool ShowPictureSettingControls
        {
            get => this.showPictureSettingControls;
            set
            {
                this.showPictureSettingControls = value;
                this.NotifyOfPropertyChange(() => this.ShowPictureSettingControls);
                this.userSettingService.SetUserSetting(UserSettingConstants.PreviewShowPictureSettingsOverlay, value);
            }
        }

        public void UpdatePreviewFrame(EncodeTask task, Source scannedSource)
        {
            this.Task = task;
            this.UpdatePreviewFrame();
            this.DisplayName = Resources.StaticPreviewViewModel_Title;
            this.Title = Resources.Preview;
            this.ScannedSource = scannedSource;
        }

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

        [HandleProcessCorruptedStateExceptions]
        public void UpdatePreviewFrame()
        {
            if (this.Task.Width < 32 || this.Task.Height < 32)
            {
                PreviewNotAvailable = true;
                return;
            }

            BitmapSource image = null;
            try
            {
                image = this.scanService.GetPreview(this.Task, this.SelectedPreviewImage);
            }
            catch (Exception exc)
            {
                PreviewNotAvailable = true;
                Debug.WriteLine(exc);
            }

            if (image != null)
            {
                PreviewNotAvailable = false;
                this.Width = (int)Math.Ceiling(image.Width);
                this.Height = (int)Math.Ceiling(image.Height);
                this.PreviewImage = image;
            }
        }

        public int FixWidth(int width)
        {
            Rect workArea = SystemParameters.WorkArea;
            if (width > workArea.Width)
            {
                return (int)Math.Round(workArea.Width, 0) - 50;
            }

            return width;
        }

        public int FixHeight(int height)
        {
            Rect workArea = SystemParameters.WorkArea;
            if (height > workArea.Height)
            {
                return (int)Math.Round(workArea.Height, 0) - 50;
            }

            return height;
        }

        public void Close()
        {
            this.IsOpen = false;
        }

        public void SetPictureSettingsInstance(IPictureSettingsViewModel pictureSettingsViewModel)
        {
            this.PictureSettingsViewModel = pictureSettingsViewModel;
        }

        public override void OnLoad()
        {
        }

        public void Play()
        {
            try
            {
                this.IsEncoding = true;
                if (File.Exists(this.CurrentlyPlaying))
                {
                    File.Delete(this.CurrentlyPlaying);
                }
            }
            catch (Exception)
            {
                this.IsEncoding = false;
                this.errorService.ShowMessageBox(Resources.StaticPreview_UnableToDeletePreview, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (this.Task == null || string.IsNullOrEmpty(Task.Source))
            {
                this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_ScanFirst, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            EncodeTask encodeTask = new EncodeTask(this.Task)
            {
                PreviewEncodeDuration = this.Duration, 
                PreviewEncodeStartAt = this.SelectedPreviewImage, 
                PointToPointMode = PointToPointMode.Preview
            };

            // Filename handling.
            if (string.IsNullOrEmpty(encodeTask.Destination))
            {
                string formatExtension;
                switch (encodeTask.OutputFormat)
                {
                    case OutputFormat.WebM:
                        formatExtension = "webm";
                        break;
                    case OutputFormat.Mp4:
                        formatExtension = "m4v";
                        break;
                    case OutputFormat.Mkv:
                    default:
                        formatExtension = "mkv";
                        break;
                }

                string filename = Path.ChangeExtension(Path.GetTempFileName(), formatExtension);
                encodeTask.Destination = filename;
                this.CurrentlyPlaying = filename;
            }
            else
            {
                string directory = Path.GetDirectoryName(encodeTask.Destination) ?? string.Empty;
                string filename = Path.GetFileNameWithoutExtension(encodeTask.Destination);
                string extension = Path.GetExtension(encodeTask.Destination);
                string previewFilename = string.Format("{0}_preview{1}", filename, extension);
                string previewFullPath = Path.Combine(directory, previewFilename);
                encodeTask.Destination = previewFullPath;
                this.CurrentlyPlaying = previewFullPath;
            }

            // Setup the encode task as a preview encode
            encodeTask.IsPreviewEncode = true;
            encodeTask.PreviewEncodeStartAt = this.SelectedPreviewImage + 1;  
            encodeTask.PreviewEncodeDuration = this.Duration;

            SubtitleTrack scanTrack = null;
            foreach (var track in encodeTask.SubtitleTracks)
            {
                if (track.SourceTrack != null && track.SourceTrack.IsFakeForeignAudioScanTrack)
                {
                    scanTrack = track;
                    break;
                }
            }

            if (scanTrack != null)
            {
                encodeTask.SubtitleTracks.Remove(scanTrack);
            }

            QueueTask task = new QueueTask(encodeTask, this.ScannedSource.ScanPath, null, false, null);
            ThreadPool.QueueUserWorkItem(this.CreatePreview, task);
        }

        public void CancelEncode()
        {
            if (this.encodeService.IsEncoding)
            {
                this.encodeService.Stop();
            }
        }

        private void PlayFile()
        {
            // Launch VLC and Play video.
            if (this.CurrentlyPlaying != string.Empty)
            {
                if (File.Exists(this.CurrentlyPlaying))
                {
                    string mediaPlayerPath = userSettingService.GetUserSetting<string>(UserSettingConstants.MediaPlayerPath);
                    string args = "\"" + this.CurrentlyPlaying + "\"";

                    if (this.UseSystemDefaultPlayer)
                    {
                        this.logService.LogMessage(string.Format("# VideoPreview: Using system media player. ({0})", args));

                        try
                        {
                            Process.Start(args);
                        }
                        catch (Win32Exception exc)
                        {
                            Execute.OnUIThread(
                                () => this.errorService.ShowError(
                                    exc.Message,
                                    Resources.QueueViewModel_PlayFileErrorSolution,
                                    exc));
                        }
                    }
                    else
                    {
                        if (File.Exists(mediaPlayerPath))
                        {
                            this.logService.LogMessage(string.Format("# Video Preview: Playing file using defined media player. ({0})", args));
                            this.logService.LogMessage(string.Format("# Video Preview: Media Player Path: {0}", mediaPlayerPath));
                            ProcessStartInfo process = new ProcessStartInfo(mediaPlayerPath, args);
                            Process.Start(process);
                            return;
                        }
                        else
                        {
                            // Fallback to the System Default
                            this.logService.LogMessage(string.Format("# Video Preview: Falling back to system media player. ({0})", args));
                            Process.Start(args);
                        }
                    }
                }
                else
                {
                    this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_UnableToPlayFile, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Warning);
                }
            }
        }

        private void CreatePreview(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (this.encodeService != null && encodeService.IsEncoding)
            {
                this.errorService.ShowMessageBox(Resources.StaticPreviewViewModel_AlreadyEncoding, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            this.encodeService = new LibEncode(userSettingService, logInstanceManager, 0, portService); // Preview needs a separate instance rather than the shared singleton. This could maybe do with being refactored at some point

            this.encodeService.EncodeCompleted += this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged += this.encodeService_EncodeStatusChanged;

            this.encodeService.Start(((QueueTask)state).Task, null);
            this.userSettingService.SetUserSetting(UserSettingConstants.LastPreviewDuration, this.Duration);
        }

        private void encodeService_EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            this.Percentage = string.Format("{0} %", Math.Round(e.PercentComplete, 2).ToString(CultureInfo.InvariantCulture));
            this.PercentageValue = e.PercentComplete;
        }

        private void encodeService_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.Percentage = "0.00%";
            this.PercentageValue = 0;
            this.IsEncoding = false;

            this.encodeService.EncodeCompleted -= this.encodeService_EncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.encodeService_EncodeStatusChanged;

            if (e.ErrorInformation != "1")
            {
                this.PlayFile();
            }
        }
    }
}