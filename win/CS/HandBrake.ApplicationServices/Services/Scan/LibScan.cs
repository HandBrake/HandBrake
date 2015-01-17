// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibScan.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan a Source
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Scan
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Scan.EventArgs;
    using HandBrake.ApplicationServices.Services.Scan.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop;
    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.Interfaces;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Scan;

    using Chapter = HandBrake.ApplicationServices.Services.Scan.Model.Chapter;
    using ScanProgressEventArgs = HandBrake.Interop.EventArgs.ScanProgressEventArgs;
    using Size = System.Drawing.Size;
    using Subtitle = HandBrake.ApplicationServices.Services.Scan.Model.Subtitle;
    using SubtitleType = HandBrake.ApplicationServices.Services.Encode.Model.Models.SubtitleType;
    using Title = HandBrake.ApplicationServices.Services.Scan.Model.Title;

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
        private Action<bool> postScanOperation;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibScan"/> class. 
        /// </summary>
        public LibScan()
        {
            this.logging = new StringBuilder();

            this.header = GeneralUtilities.CreateCliLogHeader();

            try
            {
                HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
                HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
            }
            catch (Exception)
            {
                // Do nothing. 
            }
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
        public void Scan(string sourcePath, int title, Action<bool> postAction, HBConfiguration configuraiton)
        {
            // Try to cleanup any previous scan instances.
            if (this.instance != null)
            {
                try
                {
                    this.scanLog.Close();
                    this.scanLog.Dispose();
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
            catch (Exception)
            {
                // Do nothing.
            }

            if (!Directory.Exists(Path.GetDirectoryName(this.dvdInfoPath)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(this.dvdInfoPath));
            }

            // Create a new scan log.
            this.scanLog = new StreamWriter(this.dvdInfoPath);

            // Create a new HandBrake Instance.
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
            this.instance.StopScan();

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

            EncodeJob encodeJob = InteropModelCreator.GetEncodeJob(job, configuraiton);

            BitmapImage bitmapImage = null;
            try
            {
                bitmapImage = this.instance.GetPreview(encodeJob, preview);
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
                if (this.ScanStared != null)
                    this.ScanStared(this, System.EventArgs.Empty);

                TimeSpan minDuration =
                    TimeSpan.FromSeconds(
                       configuraiton.MinScanDuration);

                HandBrakeUtils.SetDvdNav(!configuraiton.IsDvdNavDisabled);

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
        private void InstanceScanCompleted(object sender, System.EventArgs e)
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
            string path = this.currentSourceScanPath;
            if (this.currentSourceScanPath.Contains("\""))
            {
                path = this.currentSourceScanPath.Trim('\"');
            }

            // Process into internal structures.
            this.SouceData = new Source { Titles = ConvertTitles(this.instance.Titles, this.instance.FeatureTitle), ScanPath = path };

            this.IsScanning = false;

            if (this.postScanOperation != null)
            {
                this.postScanOperation(true);
            }
            else
            {
                if (this.ScanCompleted != null) this.ScanCompleted(this, new ScanCompletedEventArgs(false, null, string.Empty));
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
        internal static List<Title> ConvertTitles(IEnumerable<Interop.Model.Scan.Title> titles, int featureTitle)
        {
            List<Title> titleList = new List<Title>();
            foreach (Interop.Model.Scan.Title title in titles)
            {
                Title converted = new Title
                    {
                        TitleNumber = title.TitleNumber,
                        Duration = title.Duration,
                        Resolution = new Size(title.Resolution.Width, title.Resolution.Height),
                        AngleCount = title.AngleCount,
                        ParVal = new Size(title.ParVal.Width, title.ParVal.Height),
                        AutoCropDimensions = title.AutoCropDimensions,
                        Fps = title.Framerate,
                        SourceName = title.Path,
                        MainTitle = title.TitleNumber == featureTitle,
                        Playlist = title.InputType == InputType.Bluray ? string.Format(" {0:d5}.MPLS", title.Playlist).Trim() : null,
                        FramerateNumerator = title.FramerateNumerator,
                        FramerateDenominator = title.FramerateDenominator
                    };

                foreach (Interop.Model.Scan.Chapter chapter in title.Chapters)
                {
                    string chapterName = !string.IsNullOrEmpty(chapter.Name) ? chapter.Name : string.Empty;
                    converted.Chapters.Add(new Chapter(chapter.ChapterNumber, chapterName, chapter.Duration));
                }

                foreach (AudioTrack track in title.AudioTracks)
                {
                    converted.AudioTracks.Add(new Audio(track.TrackNumber, track.Language, track.LanguageCode, track.Description, string.Empty, track.SampleRate, track.Bitrate));
                }

                foreach (Interop.Model.Scan.Subtitle track in title.Subtitles)
                {
                    SubtitleType convertedType = new SubtitleType();

                    switch (track.SubtitleSource)
                    {
                        case SubtitleSource.VobSub:
                            convertedType = SubtitleType.VobSub;
                            break;
                        case SubtitleSource.UTF8:
                            convertedType = SubtitleType.UTF8Sub;
                            break;
                        case SubtitleSource.TX3G:
                            convertedType = SubtitleType.TX3G;
                            break;
                        case SubtitleSource.SSA:
                            convertedType = SubtitleType.SSA;
                            break;
                        case SubtitleSource.SRT:
                            convertedType = SubtitleType.SRT;
                            break;
                        case SubtitleSource.CC608:
                            convertedType = SubtitleType.CC;
                            break;
                        case SubtitleSource.CC708:
                            convertedType = SubtitleType.CC;
                            break;
                        case SubtitleSource.PGS:
                            convertedType = SubtitleType.PGS;
                            break;
                    }

                    converted.Subtitles.Add(new Subtitle(track.SubtitleSourceInt, track.TrackNumber, track.Language, track.LanguageCode, convertedType, track.CanBurn, track.CanSetForcedOnly));
                }

                titleList.Add(converted);
            }

            return titleList;
        }
        #endregion
    }
}