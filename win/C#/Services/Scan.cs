/*  Scan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using Parsing;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class Scan
    {
        /// <summary>
        /// The information for this source
        /// </summary>
        private DVD thisDvd;

        /// <summary>
        /// The CLI data parser
        /// </summary>
        private Parser readData;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// A Lock object
        /// </summary>
        private static object locker = new object();

        /// <summary>
        /// The line number thats been read to in the log file
        /// </summary>
        private int logFilePosition;

        /// <summary>
        /// The Process belonging to the CLI
        /// </summary>
        private Process hbProc;

        /// <summary>
        /// The Progress of the scan
        /// </summary>
        private string scanProgress;

        /// <summary>
        /// Scan has Started
        /// </summary>
        public event EventHandler ScanStared;

        /// <summary>
        /// Scan has completed
        /// </summary>
        public event EventHandler ScanCompleted;

        /// <summary>
        /// Scan process has changed to a new title
        /// </summary>
        public event EventHandler ScanStatusChanged;

        /// <summary>
        /// Gets or sets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning { get; set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (IsScanning)
                    return readData.Buffer;

                ReadFile();
                return logBuffer.ToString();
            }
        }

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">Path to the file to scan</param>
        /// <param name="title">int title number. 0 for scan all</param>
        public void ScanSource(string sourcePath, int title)
        {
            Thread t = new Thread(unused => this.RunScan(sourcePath, title));
            t.Start();
        }

        /// <summary>
        /// Object containing the information parsed in the scan.
        /// </summary>
        /// <returns>The DVD object containing the scan information</returns>
        public DVD SouceData()
        {
            return this.thisDvd;
        }

        /// <summary>
        /// Progress of the scan.
        /// </summary>
        /// <returns>The progress of the scan</returns>
        public string ScanStatus()
        {
            return this.scanProgress;
        }

        /// <summary>
        /// The Scan Process
        /// </summary>
        /// <returns>The CLI process</returns>
        public Process ScanProcess()
        {
            return this.hbProc;
        }

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath">Path to the source file</param>
        /// <param name="title">the title number to look at</param>
        private void RunScan(object sourcePath, int title)
        {
            try
            {
                IsScanning = true;
                if (this.ScanStared != null)
                    this.ScanStared(this, new EventArgs());

                ResetLogReader();

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(logDir, "last_scan_log.txt");

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                    File.Delete(dvdInfoPath);

                string extraArguments = string.Empty;
                if (Properties.Settings.Default.noDvdNav)
                    extraArguments = " --no-dvdnav";

                if (title > 0)
                    extraArguments += " --scan ";

                this.hbProc = new Process
                                      {
                                          StartInfo =
                                              {
                                                  FileName = handbrakeCLIPath,
                                                  Arguments =
                                                      String.Format(@" -i ""{0}"" -t{1} {2} -v ", sourcePath, title, extraArguments),
                                                  RedirectStandardOutput = true,
                                                  RedirectStandardError = true,
                                                  UseShellExecute = false,
                                                  CreateNoWindow = true
                                              }
                                      };

                // Start the Scan
                this.hbProc.Start();

                this.readData = new Parser(this.hbProc.StandardError.BaseStream);
                this.readData.OnScanProgress += new ScanProgressEventHandler(this.OnScanProgress);
                this.thisDvd = DVD.Parse(this.readData);

                // Write the Buffer out to file.
                StreamWriter scanLog = new StreamWriter(dvdInfoPath);
                scanLog.Write(this.readData.Buffer);
                scanLog.Flush();
                scanLog.Close();

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new EventArgs());
                IsScanning = false;
            }
            catch (Exception exc)
            {
                Console.WriteLine("frmMain.cs - scanProcess() " + exc);
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        private void ReadFile()
        {
            lock (locker)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, "last_scan_log.txt");
                string logFile2 = Path.Combine(logDir, "tmp_appReadable_log.txt");

                try
                {
                    // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                    if (File.Exists(logFile2))
                        File.Delete(logFile2);

                    // Copy the log file.
                    if (File.Exists(logFile))
                        File.Copy(logFile, logFile2, true);
                    else
                    {
                        ResetLogReader();
                        return;
                    }

                    // Start the Reader
                    // Only use text which continues on from the last read line
                    StreamReader sr = new StreamReader(logFile2);
                    string line;
                    int i = 1;
                    while ((line = sr.ReadLine()) != null)
                    {
                        if (i > logFilePosition)
                        {
                            logBuffer.AppendLine(line);
                            logFilePosition++;
                        }
                        i++;
                    }
                    sr.Close();
                    sr.Dispose();
                }
                catch (Exception)
                {
                    ResetLogReader();
                }
            }
        }

        /// <summary>
        /// Reset the Log Reader
        /// </summary>
        private void ResetLogReader()
        {
            logFilePosition = 0;
            logBuffer = new StringBuilder();
        }

        /// <summary>
        /// Fire an event when the scan process progresses
        /// </summary>
        /// <param name="sender">the sender</param>
        /// <param name="currentTitle">the current title being scanned</param>
        /// <param name="titleCount">the total number of titles</param>
        private void OnScanProgress(object sender, int currentTitle, int titleCount)
        {
            this.scanProgress = string.Format("Processing Title: {0} of {1}", currentTitle, titleCount);
            if (this.ScanStatusChanged != null)
                this.ScanStatusChanged(this, new EventArgs());
        }
    }
}