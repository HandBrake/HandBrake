// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibScan.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan a Source
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop;
    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.Interfaces;

    using AudioTrack = HandBrake.ApplicationServices.Parsing.Audio;
    using ScanProgressEventArgs = HandBrake.Interop.EventArgs.ScanProgressEventArgs;
    using Size = System.Drawing.Size;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class LibScan : IScan
    {
        /*
         * TODO
         * 1. Expose the Previews code.
         * 2. Cleanup old instances.
         * 
         */

        #region Private Variables

        /// <summary>
        /// Lock for the log file
        /// </summary>
        static readonly object LogLock = new object();

        /// <summary>
        /// The user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

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

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibScan"/> class. 
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public LibScan(IUserSettingService userSettingService)
        {
            logging = new StringBuilder();

            header = GeneralUtilities.CreateCliLogHeader();
            this.userSettingService = userSettingService;

            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
        }

        #region Events

        /// <summary>
        /// Scan has Started
        /// </summary>
        public event EventHandler ScanStared;

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
        /// Gets the Souce Data.
        /// </summary>
        public Source SouceData { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                string noLog = "No log data available... Log data will show here after you scan a source. \n\nOpen the log file directory to get previous log files.";
                return string.IsNullOrEmpty(this.logging.ToString()) ? this.header + noLog : this.header + this.logging.ToString();
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
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        /// <param name="postAction">
        /// The post Action.
        /// </param>
        public void Scan(string sourcePath, int title, int previewCount, Action<bool> postAction)
        {
            // Try to cleanup any previous scan instances.
            if (instance != null)
            {
                try
                {
                    this.scanLog.Close();
                    this.scanLog.Dispose();
                    instance.Dispose();
                }
                catch (Exception exc)
                {
                    // Do Nothing
                }
            }

            // Clear down the logging
            this.logging.Clear();

            try
            {
                // Make we don't pick up a stale last_scan_log_xyz.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                {
                    File.Delete(dvdInfoPath);
                }
            }
            catch (Exception)
            {
                // Do nothing.
            }

            if (!Directory.Exists(Path.GetDirectoryName(dvdInfoPath)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(dvdInfoPath));
            }

            // Create a new scan log.
            scanLog = new StreamWriter(dvdInfoPath);

            // Create a new HandBrake Instance.
            instance = new HandBrakeInstance();
            instance.Initialize(1);
            instance.ScanProgress += this.InstanceScanProgress;
            instance.ScanCompleted += this.InstanceScanCompleted;

            // Start the scan on a back
            this.ScanSource(sourcePath, title, previewCount);
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            instance.StopScan();

            try
            {
                if (this.scanLog != null)
                {
                    this.scanLog.Close();
                    this.scanLog.Dispose();
                }
            }
            catch (Exception)
            {
                // Do Nothing.
            }
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
        private void ScanSource(object sourcePath, int title, int previewCount)
        {
            try
            {
                this.logging.Clear();

                string source = sourcePath.ToString().EndsWith("\\") ? string.Format("\"{0}\\\\\"", sourcePath.ToString().TrimEnd('\\'))
                              : "\"" + sourcePath + "\"";
                currentSourceScanPath = source;

                IsScanning = true;
                if (this.ScanStared != null)
                    this.ScanStared(this, new EventArgs());

                TimeSpan minDuration =
                    TimeSpan.FromSeconds(
                        this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.MinScanDuration));

                HandBrakeUtils.SetDvdNav(this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav));

                this.instance.StartScan(sourcePath.ToString(), previewCount, minDuration, title != 0 ? title : 0);
            }
            catch (Exception exc)
            {
                this.Stop();

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new ScanCompletedEventArgs(false, exc, "An Error has occured in ScanService.ScanSource()"));
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
        private void InstanceScanCompleted(object sender, EventArgs e)
        {
            // Write the log file out before we start processing incase we crash.
            try
            {
                if (this.scanLog != null)
                {
                    this.scanLog.Flush();
                }
            }
            catch (Exception)
            {
                // Do Nothing.
            }

            // TODO -> Might be a better place to fix this.
            string path = currentSourceScanPath;
            if (currentSourceScanPath.Contains("\""))
            {
                path = currentSourceScanPath.Trim('\"');
            }

            // Process into internal structures.
            this.SouceData = new Source { Titles = ConvertTitles(this.instance.Titles, this.instance.FeatureTitle), ScanPath = path };

            IsScanning = false;

            if (this.ScanCompleted != null)
                this.ScanCompleted(this, new ScanCompletedEventArgs(false, null, string.Empty));
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
                ApplicationServices.EventArgs.ScanProgressEventArgs eventArgs =
                    new ApplicationServices.EventArgs.ScanProgressEventArgs
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
            lock (LogLock)
            {
                if (this.scanLog != null)
                {
                    this.scanLog.WriteLine(e.Message);
                }

                this.logging.AppendLine(e.Message);
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
        private void HandBrakeInstanceMessageLogged(object sender, MessageLoggedEventArgs e)
        {
            lock (LogLock)
            {
                if (this.scanLog != null)
                {
                    this.scanLog.WriteLine(e.Message);
                }

                this.logging.AppendLine(e.Message);
            }
        }

        /// <summary>
        /// Convert Interop Title objects to App Services Title object
        /// </summary>
        /// <param name="titles">
        /// The titles.
        /// </param>
        /// <param name="featureTitle">
        /// The feature Title.
        /// </param>
        /// <returns>
        /// The convert titles.
        /// </returns>
        private static List<Title> ConvertTitles(IEnumerable<Interop.SourceData.Title> titles, int featureTitle)
        {
            List<Title> titleList = new List<Title>();
            foreach (Interop.SourceData.Title title in titles)
            {
                Title converted = new Title
                    {
                        TitleNumber = title.TitleNumber,
                        Duration = title.Duration,
                        Resolution = new Size(title.Resolution.Width, title.Resolution.Height),
                        AspectRatio = title.AspectRatio,
                        AngleCount = title.AngleCount,
                        ParVal = new Size(title.ParVal.Width, title.ParVal.Height),
                        AutoCropDimensions = title.AutoCropDimensions,
                        Fps = title.Framerate,
                        SourceName = title.Path,
                        MainTitle = title.TitleNumber == featureTitle
                    };

                foreach (Interop.SourceData.Chapter chapter in title.Chapters)
                {
                    converted.Chapters.Add(new Chapter(chapter.ChapterNumber, string.Empty, chapter.Duration));
                }

                foreach (Interop.SourceData.AudioTrack track in title.AudioTracks)
                {
                    converted.AudioTracks.Add(new AudioTrack(track.TrackNumber, track.Language, track.LanguageCode, track.Description, string.Empty, track.SampleRate, track.Bitrate));
                }

                foreach (Interop.SourceData.Subtitle track in title.Subtitles)
                {
                    SubtitleType convertedType = new SubtitleType();

                    switch (track.SubtitleSource)
                    {
                        case Interop.SourceData.SubtitleSource.VobSub:
                            convertedType = SubtitleType.VobSub;
                            break;
                        case Interop.SourceData.SubtitleSource.UTF8:
                            convertedType = SubtitleType.UTF8Sub;
                            break;
                        case Interop.SourceData.SubtitleSource.TX3G:
                            convertedType = SubtitleType.TX3G;
                            break;
                        case Interop.SourceData.SubtitleSource.SSA:
                            convertedType = SubtitleType.SSA;
                            break;
                        case Interop.SourceData.SubtitleSource.SRT:
                            convertedType = SubtitleType.SRT;
                            break;
                        case Interop.SourceData.SubtitleSource.CC608:
                            convertedType = SubtitleType.CC;
                            break;
                        case Interop.SourceData.SubtitleSource.CC708:
                            convertedType = SubtitleType.CC;
                            break;
                    }

                    converted.Subtitles.Add(new Subtitle(track.TrackNumber, track.Language, track.LanguageCode, convertedType));
                }

                titleList.Add(converted);
            }

            return titleList;
        }
        #endregion
    }
}