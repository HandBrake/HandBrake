// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan a Source
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using Parser = HandBrake.ApplicationServices.Parsing.Parser;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class ScanService : IScan
    {
        #region Private Variables

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Log File Header
        /// </summary>
        private readonly StringBuilder header;

        /// <summary>
        /// The CLI data parser
        /// </summary>
        private Parser readData;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The Process belonging to the CLI
        /// </summary>
        private Process hbProc;

        /// <summary>
        /// The stop scan.
        /// </summary>
        private bool cancelScan;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="ScanService"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public ScanService(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
            this.logBuffer = new StringBuilder();

            header = GeneralUtilities.CreateCliLogHeader();
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

        #region Public Properties

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
                return string.IsNullOrEmpty(this.logBuffer.ToString()) ? this.header + noLog : this.header + this.logBuffer.ToString();
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
        /// <param name="postScanAction">
        /// The post Scan Action.
        /// </param>
        public void Scan(string sourcePath, int title, int previewCount, Action<bool> postScanAction)
        {
            Thread t = new Thread(unused => this.ScanSource(sourcePath, title, previewCount, postScanAction));
            t.Start();
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            try
            {
                // Try to clean things up as best as possible.
                if (this.readData != null)
                {
                    this.readData.OnScanProgress -= this.OnScanProgress;
                }

                cancelScan = true;

                if (this.hbProc != null && !this.hbProc.HasExited)
                {
                    this.hbProc.Kill();
                }
            }
            catch
            {
                // We don't really need to notify the user of any errors here.
            }
        }

        /// <summary>
        /// Take a Scan Log file, and process it as if it were from the CLI.
        /// </summary>
        /// <param name="path">
        /// The path to the log file.
        /// </param>
        public void DebugScanLog(string path)
        {
            try
            {
                StreamReader parseLog = new StreamReader(path);
                this.readData = new Parser(parseLog.BaseStream);
                this.SouceData = Source.Parse(this.readData, this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav));
                this.SouceData.ScanPath = path;

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, new ScanCompletedEventArgs(false, null, string.Empty));
                }
            }
            catch (Exception e)
            {
                throw new GeneralApplicationException("Debug Run Failed", string.Empty, e);
            }
        }

        /// <summary>
        /// Shutdown the service.
        /// </summary>
        public void Shutdown()
        {
            // Nothing to do for this implementation.
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
        /// <param name="postScanAction">
        /// The post Scan Action. Disables the Scan Completed Event
        /// </param>
        private void ScanSource(object sourcePath, int title, int previewCount, Action<bool> postScanAction)
        {
            try
            {
                this.IsScanning = true;
                this.cancelScan = false;

                if (this.ScanStared != null)
                {
                    this.ScanStared(this, new EventArgs());
                }

                this.logBuffer = new StringBuilder();

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(
                    logDir,
                    string.Format("last_scan_log{0}.txt", GeneralUtilities.ProcessId));

                if (!Directory.Exists(logDir))
                {
                    Directory.CreateDirectory(logDir);
                }

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                {
                    File.Delete(dvdInfoPath);
                }

                string extraArguments = string.Empty;

                if (previewCount != 10)
                {
                    extraArguments += " --previews " + previewCount;
                }

                if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav))
                {
                    extraArguments += " --no-dvdnav";
                }

                extraArguments += string.Format(" --min-duration={0}", this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.MinScanDuration));

                if (title > 0)
                {
                    extraArguments += " --scan ";
                }

                // Quick fix for "F:\\" style paths. We need \\\\ (Escaped \ twice)
                string source = sourcePath.ToString().EndsWith("\\") ? string.Format("\"{0}\\\\\"", sourcePath.ToString().TrimEnd('\\'))
                                : "\"" + sourcePath + "\"";
                string query = string.Format(@" -i {0} -t{1} {2} -v ", source, title, extraArguments);

                this.hbProc = new Process
                    {
                        StartInfo =
                            {
                                FileName = handbrakeCLIPath,
                                Arguments = string.Format(@" -i {0} -t{1} {2} -v ", source, title, extraArguments),
                                RedirectStandardOutput = true,
                                RedirectStandardError = true,
                                UseShellExecute = false,
                                CreateNoWindow = true
                            }
                    };

                // Start the Scan
                this.hbProc.Start();

                this.readData = new Parser(this.hbProc.StandardError.BaseStream);
                this.readData.OnScanProgress += this.OnScanProgress;
                this.SouceData = Source.Parse(this.readData, this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav));
                this.SouceData.ScanPath = (string)sourcePath;

                // Write the Buffer out to file.
                using (StreamWriter scanLog = new StreamWriter(dvdInfoPath))
                {
                    // Only write the log file to disk if it's less than 50MB.
                    if (this.readData.Buffer.Length < 50000000)
                    {
                        scanLog.WriteLine(header);
                        scanLog.WriteLine(query);
                        scanLog.Write(this.readData.Buffer);

                        this.logBuffer.AppendLine(query);
                        this.logBuffer.AppendLine(this.readData.Buffer.ToString());
                    }
                    else
                    {
                        throw new GeneralApplicationException(
                            "The Log file has not been written to disk as it has grown above the 50MB limit", " This indicates there was a problem during the scan process.", null);
                    }
                }

                this.IsScanning = false;


                if (postScanAction != null)
                {
                    postScanAction(true);
                }
                else
                {
                    if (this.ScanCompleted != null)
                    {
                        if (logBuffer.ToString().Contains("scan: unrecognized file type"))
                            this.ScanCompleted(this, new ScanCompletedEventArgs(false, null, "Unrecognized file type."));
                        else
                            this.ScanCompleted(this, new ScanCompletedEventArgs(this.cancelScan, null, string.Empty));
                    }
                }
            }
            catch (GeneralApplicationException)
            {
                throw;
            }
            catch (Exception exc)
            {
                this.Stop();

                if (postScanAction != null)
                {
                    postScanAction(false);
                }
                else
                {
                    if (this.ScanCompleted != null)
                    {
                        this.ScanCompleted(this, new ScanCompletedEventArgs(false, exc, "An Error has occured in ScanService.ScanSource()"));
                    }
                }
            }
        }

        /// <summary>
        /// Fire an event when the scan process progresses
        /// </summary>
        /// <param name="sender">the sender</param>
        /// <param name="currentTitle">the current title being scanned</param>
        /// <param name="titleCount">the total number of titles</param>
        /// <param name="percentage">The Percentage</param>
        private void OnScanProgress(object sender, int currentTitle, int titleCount, decimal percentage)
        {
            ScanProgressEventArgs eventArgs = new ScanProgressEventArgs
            {
                CurrentTitle = currentTitle,
                Titles = titleCount,
                Percentage = percentage
            };

            if (this.ScanStatusChanged != null)
            {
                this.ScanStatusChanged(this, eventArgs);
            }
        }

        #endregion
    }
}