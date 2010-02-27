/*  Encode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */


namespace Handbrake.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using Functions;
    using Model;
    using Properties;
    using Timer = System.Threading.Timer;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode
    {
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
        /// Fires when a new CLI Job starts
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI job finishes.
        /// </summary>
        public event EventHandler EncodeEnded;

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        public Process HbProcess { get; set; }

        /// <summary>
        /// Gets or sets The Process Handle
        /// </summary>
        public IntPtr ProcessHandle { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether HandBrakeCLI.exe is running
        /// </summary>
        public bool IsEncoding { get; set; }

        /// <summary>
        /// Gets or sets the Process ID
        /// </summary>
        public int ProcessID { get; set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (logBuffer != null)
                    return logBuffer.ToString();

                return string.Empty;
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
            Job job = new Job {Query = query};
            this.Run(job);
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

            IsEncoding = false;
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyClose()
        {
            if ((int)this.ProcessHandle == 0)
                return;

            // Allow the CLI to exit cleanly
            Win32.SetForegroundWindow((int)this.ProcessHandle);
            SendKeys.Send("^C");
            SendKeys.Flush();

            // HbProcess.StandardInput.AutoFlush = true;
            // HbProcess.StandardInput.WriteLine("^C");

            IsEncoding = false;
        }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="encJob">
        /// The enc Job.
        /// </param>
        protected void Run(Job encJob)
        {
            this.job = encJob;
            try
            {
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());

                IsEncoding = true;

                ResetLogReader();

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath =
                    Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs",
                        "last_encode_log.txt");
                string strCmdLine = String.Format(@" /C """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, encJob.Query, logPath);
                var cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                if (Settings.Default.enocdeStatusInGui)
                {
                    cliStart.RedirectStandardOutput = true;
                    cliStart.UseShellExecute = false;
                    if (!Settings.Default.showCliForInGuiEncodeStatus)
                        cliStart.CreateNoWindow = true;
                }
                if (Settings.Default.cli_minimized)
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;

                Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                this.HbProcess = Process.Start(cliStart);
                this.ProcessID = Main.GetCliProcess(before);


                if (this.HbProcess != null)
                    this.ProcessHandle = this.HbProcess.MainWindowHandle; // Set the process Handle

                // Start the Log Monitor
                windowTimer = new Timer(new TimerCallback(ReadFile), null, 1000, 1000);

                // Set the process Priority
                Process hbCliProcess = null;
                if (this.ProcessID != -1)
                    hbCliProcess = Process.GetProcessById(this.ProcessID);

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
            }
            catch (Exception exc)
            {
                MessageBox.Show(
                    "It would appear that HandBrakeCLI has not started correctly. You should take a look at the Activity log as it may indicate the reason why.\n\nDetailed Error Information: error occured in runCli()\n\n" +
                    exc,
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
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
                this.ProcessID = HbProcess.Id;
                this.ProcessHandle = HbProcess.Handle;
            }
            catch (Exception exc)
            {
                Console.WriteLine(exc);
            }
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        protected void Finish()
        {
            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());

            IsEncoding = false;

            if (!IsEncoding)
            {
                windowTimer.Dispose();
                ReadFile(null);
            }

            // Growl
            if (Settings.Default.growlQueue)
                GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");

            // Do something whent he encode ends.
            switch (Settings.Default.CompletionOption)
            {
                case "Shutdown":
                    Process.Start("Shutdown", "-s -t 60");
                    break;
                case "Log Off":
                    Win32.ExitWindowsEx(0, 0);
                    break;
                case "Suspend":
                    Application.SetSuspendState(PowerState.Suspend, true, true);
                    break;
                case "Hibernate":
                    Application.SetSuspendState(PowerState.Hibernate, true, true);
                    break;
                case "Lock System":
                    Win32.LockWorkStation();
                    break;
                case "Quit HandBrake":
                    Application.Exit();
                    break;
                default:
                    break;
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
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
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
                MessageBox.Show(
                    "Something went a bit wrong trying to copy your log file.\nError Information:\n\n" + exc,
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
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
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
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
                    if (logFilePosition == 0)
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
    }
}