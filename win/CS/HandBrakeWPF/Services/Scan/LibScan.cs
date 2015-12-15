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
    using System.IO;
    using System.Text;
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Interfaces;
    using HandBrake.ApplicationServices.Interop.Json.Scan;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Preview;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Chapter = HandBrakeWPF.Services.Scan.Model.Chapter;
    using ScanProgressEventArgs = HandBrake.ApplicationServices.Interop.EventArgs.ScanProgressEventArgs;
    using Subtitle = HandBrakeWPF.Services.Scan.Model.Subtitle;
    using Title = HandBrakeWPF.Services.Scan.Model.Title;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class LibScan : IScan
    {
        #region Private Variables

        /// <summary>
        /// Lock for the log file
        /// </summary>
        static readonly object LogLock = new object();

        /// <summary>
        /// Log data from HandBrakeInstance
        /// </summary>
        private readonly StringBuilder logging;

        /// <summary>
        /// The Log File Header
        /// </summary>
        private readonly StringBuilder header;

        /// <summary>
        /// The Current source scan path.
        /// </summary>
        private string currentSourceScanPath;

        /// <summary>
        /// The log dir.
        /// </summary>
        private static string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";

        /// <summary>
        /// The dvd info path.
        /// </summary>
        private string dvdInfoPath = Path.Combine(logDir, string.Format("last_scan_log{0}.txt", GeneralUtilities.ProcessId));

        /// <summary>
        /// The scan log.
        /// </summary>
        private StreamWriter scanLog;

        /// <summary>
        /// LibHB Instance
        /// </summary>
        private IHandBrakeInstance instance;

        /// <summary>
        /// The post scan operation.
        /// </summary>
        private Action<bool, Source> postScanOperation;

        /// <summary>
        /// Global to handle cancelled scans.
        /// </summary>
        private bool isCancelled = false;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibScan"/> class. 
        /// </summary>
        public LibScan()
        {
            this.logging = new StringBuilder();
            this.header = GeneralUtilities.CreateLogHeader();
            this.IsScanning = false;
        }

        #region Events

        /// <summary>
        /// Scan has Started
        /// </summary>
        public event EventHandler ScanStarted;

        /// <summary>
        /// Scan has completed
        /// </summary>
        public event ScanCompletedStatus ScanCompleted;

        /// <summary>
        /// Encode process has progressed
        /// </summary>
        public event ScanProgessStatus ScanStatusChanged;

        #endregion

        #region Properties

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                string noLog = "There is no log information to display." + Environment.NewLine + Environment.NewLine
                                + "This window will only display logging information after you have scanned a source." + Environment.NewLine
                                + Environment.NewLine + "You can find previous log files in the log directory or by clicking the 'Open Log Directory' button above.";

                return string.IsNullOrEmpty(this.logging.ToString()) ? noLog : this.header + this.logging.ToString();
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the file to scan
        /// </param>
        /// <param name="title">
        /// int title number. 0 for scan all
        /// </param>
        /// <param name="postAction">
        /// The post Action.
        /// </param>
        /// <param name="configuraiton">
        /// The configuraiton.
        /// </param>
        public void Scan(string sourcePath, int title, Action<bool, Source> postAction, HBConfiguration configuraiton)
        {
            // Try to cleanup any previous scan instances.
            if (this.instance != null)
            {
                try
                {
                    lock (LogLock)
                    {
                        this.scanLog.Close();
                        this.scanLog.Dispose();
                        this.scanLog = null;
                    }
                    this.instance.Dispose();
                }
                catch (Exception)
                {
                    // Do Nothing
                }
            }

            // Handle the post scan operation.
            this.postScanOperation = postAction;

            // Clear down the logging
            this.logging.Clear();

            try
            {
                // Make we don't pick up a stale last_scan_log_xyz.txt (and that we have rights to the file)
                if (File.Exists(this.dvdInfoPath))
                {
                    File.Delete(this.dvdInfoPath);
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }

            if (!Directory.Exists(Path.GetDirectoryName(this.dvdInfoPath)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(this.dvdInfoPath));
            }

            // Create a new scan log.
            this.scanLog = new StreamWriter(this.dvdInfoPath);

            // Create a new HandBrake Instance.
            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
            this.instance = HandBrakeInstanceManager.GetScanInstance(configuraiton.Verbosity);
            this.instance.ScanProgress += this.InstanceScanProgress;
            this.instance.ScanCompleted += this.InstanceScanCompleted;

            // Start the scan on a back
            this.ScanSource(sourcePath, title, configuraiton.PreviewScanCount, configuraiton);
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            try
            {
                this.ServiceLogMessage("Stopping Scan.");
                this.IsScanning = false;
                this.instance.StopScan();

                lock (LogLock)
                {
                    if (this.scanLog != null)
                    {
                        this.scanLog.Close();
                        this.scanLog.Dispose();
                        this.scanLog = null;
                    }
                }
            }
            catch (Exception exc)
            {
                this.isCancelled = false;
                this.ScanCompleted?.Invoke(this, new ScanCompletedEventArgs(false, exc, Resources.ScanService_ScanStopFailed, null));
                // Do Nothing.
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
        /// <param name="configuraiton">
        /// The configuraiton.
        /// </param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        public BitmapImage GetPreview(EncodeTask job, int preview, HBConfiguration configuraiton)
        {
            if (this.instance == null)
            {
                return null;
            }

            BitmapImage bitmapImage = null;
            try
            {
                PreviewSettings settings = new PreviewSettings
                                               {
                                                   Cropping = new Cropping(job.Cropping),
                                                   MaxWidth = job.MaxWidth ?? 0,
                                                   MaxHeight = job.MaxHeight ?? 0,
                                                   KeepDisplayAspect = job.KeepDisplayAspect,
                                                   TitleNumber = job.Title,
                                                   Anamorphic = job.Anamorphic,
                                                   Modulus = job.Modulus,
                                                   Width = job.Width ?? 0,
                                                   Height = job.Height ?? 0,
                                                   PixelAspectX = job.PixelAspectX,
                                                   PixelAspectY = job.PixelAspectY
                                               };

                bitmapImage = this.instance.GetPreview(settings, preview);
            }
            catch (AccessViolationException e)
            {
                Console.WriteLine(e);
            }

            return bitmapImage;
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the source file
        /// </param>
        /// <param name="title">
        /// the title number to look at
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        /// <param name="configuraiton">
        /// The configuraiton.
        /// </param>
        private void ScanSource(object sourcePath, int title, int previewCount, HBConfiguration configuraiton)
        {
            try
            {
                this.logging.Clear();

                string source = sourcePath.ToString().EndsWith("\\") ? string.Format("\"{0}\\\\\"", sourcePath.ToString().TrimEnd('\\'))
                              : "\"" + sourcePath + "\"";
                this.currentSourceScanPath = source;

                this.IsScanning = true;

                TimeSpan minDuration = TimeSpan.FromSeconds(configuraiton.MinScanDuration);

                HandBrakeUtils.SetDvdNav(!configuraiton.IsDvdNavDisabled);

                this.ServiceLogMessage("Starting Scan ...");
                this.instance.StartScan(sourcePath.ToString(), previewCount, minDuration, title != 0 ? title : 0);

                if (this.ScanStarted != null)
                    this.ScanStarted(this, System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.ServiceLogMessage("Scan Failed ..." + Environment.NewLine + exc);
                this.Stop();

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new ScanCompletedEventArgs(false, exc, "An Error has occured in ScanService.ScanSource()", null));
            }
        }

        #endregion

        #region HandBrakeInstance Event Handlers
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
            this.ServiceLogMessage("Scan Finished ...");
            bool cancelled = this.isCancelled;
            this.isCancelled = false;

            // Write the log file out before we start processing incase we crash.
            try
            {
                if (this.scanLog != null)
                {
                    this.scanLog.Flush();
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }

            HandBrakeUtils.MessageLogged -= this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged -= this.HandBrakeInstanceErrorLogged;

            // TODO -> Might be a better place to fix this.
            string path = this.currentSourceScanPath;
            if (this.currentSourceScanPath.Contains("\""))
            {
                path = this.currentSourceScanPath.Trim('\"');
            }

            // Process into internal structures.
            Source sourceData = null;
            if (this.instance?.Titles != null)
            {
                sourceData = new Source { Titles = ConvertTitles(this.instance.Titles), ScanPath = path };
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
            }
            else
            {
                this.ScanCompleted?.Invoke(this, new ScanCompletedEventArgs(cancelled, null, string.Empty, sourceData));
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
                HandBrakeWPF.Services.Scan.EventArgs.ScanProgressEventArgs eventArgs =
                    new HandBrakeWPF.Services.Scan.EventArgs.ScanProgressEventArgs
                        {
                            CurrentTitle = e.CurrentTitle,
                            Titles = e.Titles,
                            Percentage = Math.Round((decimal)e.Progress * 100, 0)
                        };

                this.ScanStatusChanged(this, eventArgs);
            }
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceErrorLogged(object sender, MessageLoggedEventArgs e)
        {
            this.LogMessage(e.Message);
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceMessageLogged(object sender, MessageLoggedEventArgs e)
        {
            this.LogMessage(e.Message);
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
        internal static List<Title> ConvertTitles(JsonScanObject titles)
        {
            List<Title> titleList = new List<Title>();
            foreach (SourceTitle title in titles.TitleList)
            {
                Title converted = new Title
                    {
                        TitleNumber = title.Index,
                        Duration = new TimeSpan(0, title.Duration.Hours, title.Duration.Minutes, title.Duration.Seconds),
                        Resolution = new Size(title.Geometry.Width, title.Geometry.Height),
                        AngleCount = title.AngleCount,
                        ParVal = new Size(title.Geometry.PAR.Num, title.Geometry.PAR.Den),
                        AutoCropDimensions = new Cropping
                        {
                            Top = title.Crop[0],
                            Bottom = title.Crop[1],
                            Left = title.Crop[2],
                            Right = title.Crop[3]
                        },
                        Fps = ((double)title.FrameRate.Num) / title.FrameRate.Den,
                        SourceName = title.Path,
                        MainTitle = titles.MainFeature == title.Index,
                        Playlist = title.Type == 1 ? string.Format(" {0:d5}.MPLS", title.Playlist).Trim() : null,
                        FramerateNumerator = title.FrameRate.Num,
                        FramerateDenominator = title.FrameRate.Den
                    };

                int currentTrack = 1;
                foreach (SourceChapter chapter in title.ChapterList)
                {
                    string chapterName = !string.IsNullOrEmpty(chapter.Name) ? chapter.Name : string.Empty;
                    converted.Chapters.Add(new Chapter(currentTrack, chapterName, new TimeSpan(chapter.Duration.Hours, chapter.Duration.Minutes, chapter.Duration.Seconds)));
                    currentTrack++;
                }

                int currentAudioTrack = 1;
                foreach (SourceAudioTrack track in title.AudioList)
                {
                    converted.AudioTracks.Add(new Audio(currentAudioTrack, track.Language, track.LanguageCode, track.Description, string.Empty, track.SampleRate, track.BitRate));
                    currentAudioTrack++;
                }

                int currentSubtitleTrack = 1;
                foreach (SourceSubtitleTrack track in title.SubtitleList)
                {
                    SubtitleType convertedType = new SubtitleType();

                    switch (track.Source)
                    {
                        case 0:
                            convertedType = SubtitleType.VobSub;
                            break;
                        case 4:
                            convertedType = SubtitleType.UTF8Sub;
                            break;
                        case 5:
                            convertedType = SubtitleType.TX3G;
                            break;
                        case 6:
                            convertedType = SubtitleType.SSA;
                            break;
                        case 1:
                            convertedType = SubtitleType.SRT;
                            break;
                        case 2:
                            convertedType = SubtitleType.CC;
                            break;
                        case 3:
                            convertedType = SubtitleType.CC;
                            break;
                        case 7:
                            convertedType = SubtitleType.PGS;
                            break;
                    }

                    bool canBurn = HBFunctions.hb_subtitle_can_burn(track.Source) > 0;
                    bool canSetForcedOnly = HBFunctions.hb_subtitle_can_force(track.Source) > 0;

                    converted.Subtitles.Add(new Subtitle(track.Source, currentSubtitleTrack, track.Language, track.LanguageCode, convertedType, canBurn, canSetForcedOnly));
                    currentSubtitleTrack++;
                }

                titleList.Add(converted);
            }

            return titleList;
        }

        /// <summary>
        /// The log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        private void LogMessage(string message)
        {
            lock (LogLock)
            {
                if (this.scanLog != null)
                {
                    this.scanLog.WriteLine(message);
                }

                this.logging.AppendLine(message);
            }
        }

        /// <summary>
        /// The service log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ServiceLogMessage(string message)
        {
            this.LogMessage(string.Format("# {0}", message));
        }
        #endregion
    }
}