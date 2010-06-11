/*  Encode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Properties;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using Timer = System.Threading.Timer;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode : IEncode
    {
        /* Private Variables */

        /// <summary>
        /// An Encode Job
        /// </summary>
        private Job job;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The line number thats been read to in the log file
        /// </summary>
        private int logFilePosition;

        /// <summary>
        /// A Timer for this window
        /// </summary>
        private Timer windowTimer;

        /// <summary>
        /// Gets The Process Handle
        /// </summary>
        private IntPtr processHandle;

        /// <summary>
        /// Gets the Process ID
        /// </summary>
        private int processID;

        /* Constructor */

        /// <summary>
        /// Initializes a new instance of the <see cref="Encode"/> class.
        /// </summary>
        public Encode()
        {
            this.EncodeStarted += Encode_EncodeStarted;
        }

        /* Delegates */

        /// <summary>
        /// Encode Progess Status
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeProgressEventArgs.
        /// </param>
        public delegate void EncodeProgessStatus(object sender, EncodeProgressEventArgs e);

        /* Event Handlers */

        /// <summary>
        /// Fires when a new CLI Job starts
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI job finishes.
        /// </summary>
        public event EventHandler EncodeEnded;

        /// <summary>
        /// Encode process has progressed
        /// </summary>
        public event EncodeProgessStatus EncodeStatusChanged;

        /* Properties */

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        protected Process HbProcess { get; set; }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding { get; private set; }

        /* Public Methods */

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (logBuffer == null)
                {
                    ResetLogReader();
                    ReadFile(null);
                }

                return logBuffer != null ? logBuffer.ToString() : string.Empty;
            }
        }

        /// <summary>
        /// Create a preview sample video
        /// </summary>
        /// <param name="query">
        /// The CLI Query
        /// </param>
        public void CreatePreviewSample(string query)
        {
            this.Run(new Job { Query = query }, true);
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            if (this.HbProcess != null)
                this.HbProcess.Kill();

            Process[] list = Process.GetProcessesByName("HandBrakeCLI");
            foreach (Process process in list)
                process.Kill();

            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyClose()
        {
            if ((int)this.processHandle == 0)
                return;

            // Allow the CLI to exit cleanly
            Win32.SetForegroundWindow((int)this.processHandle);
            SendKeys.Send("^C");
            SendKeys.Flush();

            // HbProcess.StandardInput.AutoFlush = true;
            // HbProcess.StandardInput.WriteLine("^C");
        }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="encJob">
        /// The enc Job.
        /// </param>
        /// <param name="requireStandardOuput">
        /// Set to True to show no window and force standard output redirect
        /// </param>
        protected void Run(Job encJob, bool requireStandardOuput)
        {
            this.job = encJob;
            try
            {
                ResetLogReader();
                IsEncoding = true;

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs", "last_encode_log.txt");
                string strCmdLine = String.Format(@" /C """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, encJob.Query, logPath);
                var cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                if (Settings.Default.enocdeStatusInGui || requireStandardOuput)
                {
                    cliStart.RedirectStandardOutput = true;
                    cliStart.UseShellExecute = false;
                    if (!Settings.Default.showCliForInGuiEncodeStatus || requireStandardOuput)
                        cliStart.CreateNoWindow = true;
                }
                if (Settings.Default.cli_minimized)
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;

                Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                HbProcess = Process.Start(cliStart);
                this.processID = Main.GetCliProcess(before);

                if (HbProcess != null)
                    this.processHandle = HbProcess.MainWindowHandle; // Set the process Handle

                // Start the Log Monitor
                windowTimer = new Timer(new TimerCallback(ReadFile), null, 1000, 1000);

                // Set the process Priority
                Process hbCliProcess = null;
                if (this.processID != -1)
                {
                    hbCliProcess = Process.GetProcessById(this.processID);
                    hbCliProcess.EnableRaisingEvents = true;
                    hbCliProcess.Exited += new EventHandler(HbProcess_Exited);
                }

                if (hbCliProcess != null)
                    switch (Settings.Default.processPriority)
                    {
                        case "Realtime":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.RealTime;
                            break;
                        case "High":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.High;
                            break;
                        case "Above Normal":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                            break;
                        case "Normal":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.Normal;
                            break;
                        case "Low":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.Idle;
                            break;
                        default:
                            hbCliProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                            break;
                    }

                // Fire the Encode Started Event
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());
            }
            catch (Exception exc)
            {
                Main.ShowExceptiowWindow("It would appear that HandBrakeCLI has not started correctly. You should take a look at the Activity log as it may indicate the reason why.\n\nDetailed Error Information: error occured in runCli()", exc.ToString());
            }
        }

        /// <summary>
        /// The HandBrakeCLI process has exited.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void HbProcess_Exited(object sender, EventArgs e)
        {
            IsEncoding = false;

            windowTimer.Dispose();
            ReadFile(null);

            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());
        }

        /// <summary>
        /// Function to run the CLI directly rather than via CMD
        /// TODO: Code to handle the Log data has yet to be written.
        /// TODO: Code to handle the % / ETA info has to be written.
        /// </summary>
        /// <param name="query">
        /// The query.
        /// </param>
        protected void DirectRun(string query)
        {
            try
            {
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());

                IsEncoding = true;

                ResetLogReader();

                // Setup the job
                string handbrakeCLIPath = Path.Combine(Environment.CurrentDirectory, "HandBrakeCLI.exe");
                HbProcess = new Process
                                {
                                    StartInfo =
                                        {
                                            FileName = handbrakeCLIPath,
                                            Arguments = query,
                                            UseShellExecute = false,
                                            RedirectStandardOutput = true,
                                            RedirectStandardError = true,
                                            RedirectStandardInput = true,
                                            CreateNoWindow = false,
                                            WindowStyle = ProcessWindowStyle.Minimized
                                        }
                                };

                // Setup event handlers for rediected data
                HbProcess.ErrorDataReceived += new DataReceivedEventHandler(HbProcErrorDataReceived);
                HbProcess.OutputDataReceived += new DataReceivedEventHandler(HbProcOutputDataReceived);

                // Start the process
                HbProcess.Start();

                // Setup the asynchronous reading of stdin and stderr
                HbProcess.BeginErrorReadLine();
                HbProcess.BeginOutputReadLine();

                // Set the Process Priority);
                switch (Settings.Default.processPriority)
                {
                    case "Realtime":
                        HbProcess.PriorityClass = ProcessPriorityClass.RealTime;
                        break;
                    case "High":
                        HbProcess.PriorityClass = ProcessPriorityClass.High;
                        break;
                    case "Above Normal":
                        HbProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                        break;
                    case "Normal":
                        HbProcess.PriorityClass = ProcessPriorityClass.Normal;
                        break;
                    case "Low":
                        HbProcess.PriorityClass = ProcessPriorityClass.Idle;
                        break;
                    default:
                        HbProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                        break;
                }

                // Set the class items
                this.processID = HbProcess.Id;
                this.processHandle = HbProcess.Handle;
            }
            catch (Exception exc)
            {
                Console.WriteLine(exc);
            }
        }

        /// <summary>
        /// Add the CLI Query to the Log File.
        /// </summary>
        /// <param name="encJob">
        /// The Encode Job Object
        /// </param>
        protected void AddCLIQueryToLog(Job encJob)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logPath = Path.Combine(logDir, "last_encode_log.txt");

                var reader = new StreamReader(File.Open(logPath, FileMode.Open, FileAccess.Read, FileShare.Read));
                string log = reader.ReadToEnd();
                reader.Close();

                var writer = new StreamWriter(File.Create(logPath));

                writer.WriteLine("### CLI Query: " + encJob.Query);
                writer.WriteLine("### User Query: " + encJob.CustomQuery);
                writer.WriteLine("#########################################");
                writer.WriteLine(log);
                writer.Flush();
                writer.Close();
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination">
        /// The Destination File Path
        /// </param>
        protected void CopyLog(string destination)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string tempLogFile = Path.Combine(logDir, "last_encode_log.txt");

                string encodeDestinationPath = Path.GetDirectoryName(destination);
                string destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " +
                                       DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + ".txt";

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                    Directory.CreateDirectory(logDir);

                // Copy the Log to HandBrakes log folder in the users applciation data folder.
                File.Copy(tempLogFile, Path.Combine(logDir, encodeLogFile));

                // Save a copy of the log file in the same location as the enocde.
                if (Settings.Default.saveLogWithVideo)
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(Settings.Default.saveLogPath))
                    if (Settings.Default.saveLogPath != String.Empty && Settings.Default.saveLogToSpecifiedPath)
                        File.Copy(tempLogFile, Path.Combine(Settings.Default.saveLogPath, encodeLogFile));
            }
            catch (Exception exc)
            {
                Main.ShowExceptiowWindow("Unable to make a copy of the log file", exc.ToString());
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        /// <param name="n">
        /// The object.
        /// </param>
        private void ReadFile(object n)
        {
            lock (logBuffer)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, "last_encode_log.txt");
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

                    // Put the Query and User Generated Query Flag on the log.
                    if (logFilePosition == 0 && job.Query != null)
                    {
                        logBuffer.AppendLine("### CLI Query: " + job.Query);
                        logBuffer.AppendLine("### User Query: " + job.CustomQuery);
                        logBuffer.AppendLine("#########################################");
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
        /// Recieve the Standard Error information and process it
        /// </summary>
        /// <param name="sender">
        /// The Sender Object
        /// </param>
        /// <param name="e">
        /// DataReceived EventArgs
        /// </param>
        private void HbProcErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (!String.IsNullOrEmpty(e.Data))
            {
                lock (logBuffer)
                    logBuffer.AppendLine(e.Data);
            }
        }

        /// <summary>
        /// Standard Input Data Recieved from the CLI
        /// </summary>
        /// <param name="sender">
        /// The Sender Object
        /// </param>
        /// <param name="e">
        /// DataReceived EventArgs
        /// </param>
        private void HbProcOutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (!String.IsNullOrEmpty(e.Data))
            {
                lock (logBuffer)
                    logBuffer.AppendLine(e.Data);
            }
        }

        /// <summary>
        /// Encode Started
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void Encode_EncodeStarted(object sender, EventArgs e)
        {
            Thread monitor = new Thread(EncodeMonitor);
            monitor.Start();
        }

        /// <summary>
        /// Monitor the Job
        /// </summary>
        private void EncodeMonitor()
        {
            try
            {
                Parser encode = new Parser(HbProcess.StandardOutput.BaseStream);
                encode.OnEncodeProgress += EncodeOnEncodeProgress;
                while (!encode.EndOfStream)
                    encode.ReadEncodeStatus();

               // Main.ShowExceptiowWindow("Encode Monitor Stopped", "Stopped");
            }
            catch (Exception exc)
            {
                Main.ShowExceptiowWindow("An Unknown Error has occured", exc.ToString());
            }
        }

        /// <summary>
        /// Displays the Encode status in the GUI
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="currentTask">The current task</param>
        /// <param name="taskCount">Number of tasks</param>
        /// <param name="percentComplete">Percent complete</param>
        /// <param name="currentFps">Current encode speed in fps</param>
        /// <param name="avg">Avg encode speed</param>
        /// <param name="timeRemaining">Time Left</param>
        private void EncodeOnEncodeProgress(object sender, int currentTask, int taskCount, float percentComplete, float currentFps, float avg, TimeSpan timeRemaining)
        {
            EncodeProgressEventArgs eventArgs = new EncodeProgressEventArgs
                {
                    AverageFrameRate = avg,
                    CurrentFrameRate = currentFps,
                    EstimatedTimeLeft = timeRemaining,
                    PercentComplete = percentComplete,
                    Task = currentTask,
                    TaskCount = taskCount
                };

            if (this.EncodeStatusChanged != null)
                this.EncodeStatusChanged(this, eventArgs);
        }
    }
}