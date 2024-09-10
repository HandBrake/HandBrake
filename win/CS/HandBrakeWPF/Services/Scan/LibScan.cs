// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibScan.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan a Source
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Drawing;
    using System.Linq;
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.Scan;

    using HandBrakeWPF.Instance;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Factories;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Utilities;

    using ILog = Logging.Interfaces.ILog;
    using ScanProgressEventArgs = HandBrake.Interop.Interop.Interfaces.EventArgs.ScanProgressEventArgs;
    using Source = Model.Source;
    using Title = Model.Title;

    public class LibScan : IScan, IDisposable
    {
        private readonly ILog log = null;
        private readonly IUserSettingService userSettingService;
        private readonly ILogInstanceManager logInstanceManager;

        private TitleFactory titleFactory = new TitleFactory();
        private IScanInstance instance;
        private Action<bool, Source> postScanOperation;
        private bool isCancelled = false;

        public LibScan(ILog logService, IUserSettingService userSettingService, ILogInstanceManager logInstanceManager)
        {
            this.log = logService;
            this.userSettingService = userSettingService;
            this.logInstanceManager = logInstanceManager;
            this.IsScanning = false;
        }
        
        public event EventHandler ScanStarted;

        public event ScanCompletedStatus ScanCompleted;

        public event ScanProgressStatus ScanStatusChanged;

        public bool IsScanning { get; private set; }

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePaths">
        /// Paths to the file to scan
        /// </param>
        /// <param name="title">
        /// int title number. 0 for scan all
        /// </param>
        /// <param name="postAction">
        /// The post Action.
        /// </param>
        public void Scan(List<string> sourcePaths, int title, Action<bool, Source> postAction)
        {
            if (this.IsScanning)
            {
                return;
            }

            // Try to cleanup any previous scan instances.
            if (this.instance != null)
            {
                try
                {
                    this.instance.Dispose();
                }
                catch (Exception)
                {
                    // Do Nothing
                }
            }

            this.isCancelled = false;

            // Reset the log
            this.logInstanceManager.ResetApplicationLog();

            // Handle the post scan operation.
            this.postScanOperation = postAction;

            // Create a new HandBrake Instance.
            this.instance = HandBrakeInstanceManager.GetScanInstance(this.userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity));
            this.instance.ScanProgress += this.InstanceScanProgress;
            this.instance.ScanCompleted += this.InstanceScanCompleted;

            // Start the scan on a back
            this.ScanSource(sourcePaths, title, this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount));
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            try
            {
                this.ServiceLogMessage("Manually Stopping Scan ...");
                this.IsScanning = false;
              
                var handBrakeInstance = this.instance;
                if (handBrakeInstance != null)
                {
                    handBrakeInstance.StopScan();
                    handBrakeInstance.ScanProgress -= this.InstanceScanProgress;
                    handBrakeInstance.ScanCompleted -= this.InstanceScanCompleted;
                    handBrakeInstance.Dispose();
                    this.instance = null;
                }
            }
            catch (Exception exc)
            {
                this.ServiceLogMessage(exc.ToString());
            }
            finally
            {
                this.ScanCompleted?.Invoke(this, new ScanCompletedEventArgs(this.isCancelled, null, null, null));
                this.instance = null;
                this.ServiceLogMessage("Scan Stopped ...");
            }
        }

        /// <summary>
        /// Cancel the current scan.
        /// </summary>
        public void Cancel()
        {
            this.isCancelled = true;
            this.Stop();
        }

        /// <summary>
        /// Get a Preview image for the current job and preview number.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="preview">
        /// The preview.
        /// </param>
        /// <param name="showCropBoundaries">Render crop boundary borders on the preview image.</param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        public BitmapImage GetPreview(EncodeTask job, int preview, bool showCropBoundaries)
        {
            if (this.instance == null)
            {
                return null;
            }

            BitmapImage bitmapImage = null;
            try
            {

                EncodeTaskFactory factory = new EncodeTaskFactory(this.userSettingService, false);
                JsonEncodeObject jobDict = factory.Create(job);

                if (showCropBoundaries)
                {
                    // We want uncropped image so we can render an overlay on top.
                    var cropFilter = jobDict.Filters.FilterList.FirstOrDefault(s => s.ID == (int)hb_filter_ids.HB_FILTER_CROP_SCALE);
                    if (cropFilter != null)
                    {
                        jobDict.Filters.FilterList.Remove(cropFilter);
                    }
                }


                RawPreviewData bitmapData = this.instance.GetPreview(jobDict, preview);
                if (bitmapData != null)
                {
                    Bitmap image = BitmapUtilities.ConvertByteArrayToBitmap(bitmapData);
                    if (showCropBoundaries)
                    {
                        image = PreviewManager.RenderCropBorder(image, job.Cropping);
                    }

                    bitmapImage = BitmapUtilities.ConvertToBitmapImage(image);
                }
            }
            catch (AccessViolationException e)
            {
                Debug.WriteLine(e);
            }

            return bitmapImage;
        }
        
        /// <summary>
        /// The service log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{1} # {0}{1}", message, Environment.NewLine));
        }

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePaths">
        /// Path to the source file
        /// </param>
        /// <param name="title">
        /// the title number to look at
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        private void ScanSource(List<string> sourcePaths, int title, int previewCount)
        {
            try
            {
                this.IsScanning = true;

                TimeSpan minDuration = TimeSpan.FromSeconds(this.userSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration));

                HandBrakeUtils.SetDvdNav(!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav));

                List<string> excludedExtensions = this.userSettingService.GetUserSetting<List<string>>(UserSettingConstants.ExcludedExtensions);

                bool nvdec = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvDecSupport);
                bool directx = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableDirectXDecoding);

                int hwDecode = 0;
                if (nvdec && HandBrakeHardwareEncoderHelper.IsNVDecAvailable)
                {
                    hwDecode = (int)NativeConstants.HB_DECODE_SUPPORT_NVDEC;
                }
                if (directx && HandBrakeHardwareEncoderHelper.IsDirectXAvailable)
                {
                    hwDecode = (int)NativeConstants.HB_DECODE_SUPPORT_MF;
                }

                bool keepDuplicateTitles = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.KeepDuplicateTitles);

                this.ServiceLogMessage("Starting Scan ...");
                this.instance.StartScan(sourcePaths, previewCount, minDuration, title != 0 ? title : 0, excludedExtensions, hwDecode, keepDuplicateTitles);

                this.ScanStarted?.Invoke(this, System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.ServiceLogMessage("Scan Failed ..." + Environment.NewLine + exc);
                this.Stop();
            }
        }
        
        /// <summary>
        /// Scan Completed Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InstanceScanCompleted(object sender, System.EventArgs e)
        {
            try
            {
                bool cancelled = this.isCancelled;
                this.isCancelled = false;

                // Process into internal structures.
                Source sourceData = null;
                if (this.instance?.Titles != null)
                {
                    sourceData = new Source(this.ConvertTitles(this.instance.Titles));
                }

                this.IsScanning = false;

                if (this.postScanOperation != null)
                {
                    try
                    {
                        this.postScanOperation(true, sourceData);
                    }
                    catch (Exception exc)
                    {
                        Debug.WriteLine(exc);
                    }

                    this.postScanOperation = null; // Reset
                    this.ServiceLogMessage("Scan Finished for Queue Edit ...");
                }
                else
                {
                    this.ScanCompleted?.Invoke(
                        this,
                        new ScanCompletedEventArgs(cancelled, null, string.Empty, sourceData));
                    this.ServiceLogMessage("Scan Finished ...");
                }
            }
            finally
            {
                this.IsScanning = false;
                var handBrakeInstance = this.instance;
                if (handBrakeInstance != null)
                {
                    handBrakeInstance.ScanProgress -= this.InstanceScanProgress;
                    handBrakeInstance.ScanCompleted -= this.InstanceScanCompleted;
                }
            }
        }

        /// <summary>
        /// Scan Progress Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InstanceScanProgress(object sender, ScanProgressEventArgs e)
        {
            if (this.ScanStatusChanged != null)
            {
                EventArgs.ScanProgressEventArgs eventArgs =
                    new EventArgs.ScanProgressEventArgs
                        {
                            CurrentTitle = e.CurrentTitle,
                            Titles = e.Titles,
                            Percentage = Math.Round((decimal)e.Progress * 100, 0)
                        };

                this.ScanStatusChanged(this, eventArgs);
            }
        }

        /// <summary>
        /// Convert Interop Title objects to App Services Title object
        /// </summary>
        /// <param name="titles">
        /// The titles.
        /// </param>
        /// <returns>
        /// The convert titles.
        /// </returns>
        private List<Title> ConvertTitles(JsonScanObject titles)
        {
            List<Title> titleList = new List<Title>();
            foreach (SourceTitle title in titles.TitleList)
            {
                Title converted = this.titleFactory.CreateTitle(title, titles.MainFeature);
                titleList.Add(converted);
            }

            return titleList;
        }

        public void Dispose()
        {
            if (this.instance != null)
            {
                try
                {
                    this.instance.Dispose();
                    this.instance = null;
                }
                catch (Exception e)
                {
                    this.ServiceLogMessage("Unable to Dispose of LibScan: " + e);
                }            
            }
        }
    }
}