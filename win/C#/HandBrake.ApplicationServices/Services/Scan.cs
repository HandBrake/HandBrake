/*  Scan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.Framework.Services;
    using HandBrake.Framework.Services.Interfaces;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class ScanService : IScan
    {
        /* Private Variables */

        /// <summary>
        /// The Error Service
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// A Lock object
        /// </summary>
        private static readonly object locker = new object();

        /// <summary>
        /// The CLI data parser
        /// </summary>
        private Parser readData;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The line number thats been read to in the log file
        /// </summary>
        private int logFilePosition;

        /// <summary>
        /// The Process belonging to the CLI
        /// </summary>
        private Process hbProc;

        /// <summary>
        /// Initializes a new instance of the <see cref="ScanService"/> class.
        /// </summary>
        public ScanService()
        {
            this.errorService = new ErrorService();
        }

        /* Event Handlers */

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

        /* Properties */

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning { get; private set; }

        /// <summary>
        /// Gets the Scan Status.
        /// </summary>
        public string ScanStatus { get; private set; }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        public DVD SouceData { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (IsScanning)
                    return readData.Buffer.ToString();

                if (logBuffer == null)
                {
                    ResetLogReader(false);
                    ReadLastScanFile();  
                }

                return logBuffer != null ? logBuffer.ToString() : string.Empty;
            }
        }

        /* Public Methods */

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">Path to the file to scan</param>
        /// <param name="title">int title number. 0 for scan all</param>
        public void Scan(string sourcePath, int title)
        {
            Thread t = new Thread(unused => this.ScanSource(sourcePath, title));
            t.Start();
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            try
            {
                this.readData.OnScanProgress -= this.OnScanProgress;

                if (hbProc != null && !hbProc.HasExited)
                    hbProc.Kill();
            }
            catch (Exception ex)
            {
                errorService.ShowError("Unable to kill HandBrakeCLI.exe \n" +
                "You may need to manually kill HandBrakeCLI.exe using the Windows Task Manager if it does not close automatically" + 
                " within the next few minutes. ", ex.ToString());
            }
        }

        /* Private Methods */

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath">Path to the source file</param>
        /// <param name="title">the title number to look at</param>
        private void ScanSource(object sourcePath, int title)
        {
            try
            {
                IsScanning = true;
                if (this.ScanStared != null)
                    this.ScanStared(this, new EventArgs());

                ResetLogReader(true);

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(logDir, string.Format("last_scan_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                    File.Delete(dvdInfoPath);

                string extraArguments = string.Empty;
                if (Init.DisableDvdNav)
                    extraArguments = " --no-dvdnav";

                if (title > 0)
                    extraArguments += " --scan ";

                this.hbProc = new Process
                                  {
                                      StartInfo =
                                          {
                                              FileName = handbrakeCLIPath,
                                              Arguments =
                                                  String.Format(@" -i ""{0}"" -t{1} {2} -v ", sourcePath, title,
                                                                extraArguments),
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
                this.SouceData = DVD.Parse(this.readData);

                // Write the Buffer out to file.
                using (StreamWriter scanLog = new StreamWriter(dvdInfoPath))
                {
                    // Only write the log file to disk if it's less than 100MB.
                    if (this.readData.Buffer.Length < 100000000)
                    {
                        scanLog.WriteLine(Logging.CreateCliLogHeader(null));
                        scanLog.Write(this.readData.Buffer);
                        logBuffer.AppendLine(this.readData.Buffer.ToString());
                    }
                    else
                    {
                        throw new Exception(
                            "The Log file has not been written to disk as it has grown above the 100MB limit. This indicates there was a problem during the scan process.");
                    }
                }

                IsScanning = false;

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new EventArgs()); 
            }
            catch (Exception exc)
            {
                this.Stop();

                errorService.ShowError("An error has occured during the scan process.", exc.ToString());

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new EventArgs());   
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        private void ReadLastScanFile()
        {
            lock (locker)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, string.Format("last_scan_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));
                string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));

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
                        ResetLogReader(true);
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
                catch (Exception exc)
                {
                    Console.WriteLine(exc.ToString());
                    ResetLogReader(true);
                }
            }
        }

        /// <summary>
        /// Reset the Log Reader
        /// </summary>
        /// <param name="addHeader">
        /// The add Header.
        /// </param>
        private void ResetLogReader(bool addHeader)
        {
            logFilePosition = 0;
            logBuffer = new StringBuilder();
            if (addHeader)
                logBuffer.AppendLine(Logging.CreateCliLogHeader(null));
        }

        /// <summary>
        /// Fire an event when the scan process progresses
        /// </summary>
        /// <param name="sender">the sender</param>
        /// <param name="currentTitle">the current title being scanned</param>
        /// <param name="titleCount">the total number of titles</param>
        private void OnScanProgress(object sender, int currentTitle, int titleCount)
        {
            this.ScanStatus = string.Format("Processing Title: {0} of {1}", currentTitle, titleCount);
            if (this.ScanStatusChanged != null)
                this.ScanStatusChanged(this, new EventArgs());
        }
    }
}