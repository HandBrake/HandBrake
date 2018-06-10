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
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.Scan;
    using HandBrake.Interop.Interop.Model;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Interop.Model.Preview;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Chapter = Model.Chapter;
    using ILog = Logging.Interfaces.ILog;
    using LogLevel = Logging.Model.LogLevel;
    using LogMessageType = Logging.Model.LogMessageType;
    using LogService = Logging.LogService;
    using ScanProgressEventArgs = HandBrake.Interop.Interop.EventArgs.ScanProgressEventArgs;
    using Subtitle = Model.Subtitle;
    using Title = Model.Title;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class LibScan : IScan
    {
        #region Private Variables

        private readonly ILog log = LogService.GetLogger();
        private string currentSourceScanPath;
        private IHandBrakeInstance instance;
        private Action<bool, Source> postScanOperation;
        private bool isCancelled = false;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibScan"/> class. 
        /// </summary>
        public LibScan()
        {
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
                    this.instance.Dispose();
                }
                catch (Exception)
                {
                    // Do Nothing
                }
            }

            this.isCancelled = false;

            // Handle the post scan operation.
            this.postScanOperation = postAction;

            // Create a new HandBrake Instance.
            this.instance = HandBrake.Interop.Interop.HandBrakeInstanceManager.GetScanInstance(configuraiton.Verbosity);
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
                this.ServiceLogMessage("Manually Stopping Scan ...");
                this.IsScanning = false;
                this.instance.StopScan();              
            }
            catch (Exception exc)
            {
                this.ServiceLogMessage(exc.ToString());
            }
            finally
            {
                this.ScanCompleted?.Invoke(this, new ScanCompletedEventArgs(this.isCancelled, null, null, null));
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

                var bitmapData = this.instance.GetPreview(settings, preview, job.DeinterlaceFilter != DeinterlaceFilter.Off);
                bitmapImage = BitmapUtilities.ConvertToBitmapImage(BitmapUtilities.ConvertByteArrayToBitmap(bitmapData));
            }
            catch (AccessViolationException e)
            {
                Debug.WriteLine(e);
            }

            return bitmapImage;
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// The service log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{0} # {1}{0}", Environment.NewLine, message), LogMessageType.ScanOrEncode, LogLevel.Info);
        }

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
                string source = sourcePath.ToString().EndsWith("\\") ? string.Format("\"{0}\\\\\"", sourcePath.ToString().TrimEnd('\\'))
                              : "\"" + sourcePath + "\"";
                this.currentSourceScanPath = source;

                this.IsScanning = true;

                TimeSpan minDuration = TimeSpan.FromSeconds(configuraiton.MinScanDuration);

                HandBrakeUtils.SetDvdNav(!configuraiton.IsDvdNavDisabled);

                this.ServiceLogMessage("Starting Scan ...");
                this.instance.StartScan(sourcePath.ToString(), previewCount, minDuration, title != 0 ? title : 0);

                this.ScanStarted?.Invoke(this, System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.ServiceLogMessage("Scan Failed ..." + Environment.NewLine + exc);
                this.Stop();
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
            this.ServiceLogMessage("Processing Scan Information ...");
            bool cancelled = this.isCancelled;
            this.isCancelled = false;

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
                this.ServiceLogMessage("Scan Finished for Queue Edit ...");
            }
            else
            {
                this.ScanCompleted?.Invoke(this, new ScanCompletedEventArgs(cancelled, null, string.Empty, sourceData));
                this.ServiceLogMessage("Scan Finished ...");
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
        private static List<Title> ConvertTitles(JsonScanObject titles)
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
                        FramerateDenominator = title.FrameRate.Den,
                        Type = title.Type
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
                    converted.AudioTracks.Add(new Audio(currentAudioTrack, track.Language, track.LanguageCode, track.Description, track.Codec, track.SampleRate, track.BitRate, track.ChannelLayout));
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
        #endregion
    }
}