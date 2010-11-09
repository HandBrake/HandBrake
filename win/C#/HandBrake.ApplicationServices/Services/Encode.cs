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

    using HandBrake.Framework.Services;
    using HandBrake.Framework.Services.Interfaces;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode : IEncode
    {
        #region Private Variables

        /// <summary>
        /// The Error Service
        /// </summary>
        protected IErrorService errorService;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The Log file writer
        /// </summary>
        private StreamWriter fileWriter;

        /// <summary>
        /// Gets The Process Handle
        /// </summary>
        private IntPtr processHandle;

        /// <summary>
        /// Gets the Process ID
        /// </summary>
        private int processID;

        /// <summary>
        /// Windows 7 API Pack wrapper
        /// </summary>
        private Win7 windowsSeven = new Win7();

        #endregion

        /* Constructor */

        /// <summary>
        /// Initializes a new instance of the <see cref="Encode"/> class.
        /// </summary>
        public Encode()
        {
            this.EncodeStarted += Encode_EncodeStarted;
            GrowlCommunicator.Register();

            this.errorService = new ErrorService();
        }

        #region Delegates and Event Handlers

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
        #endregion

        /* Properties */

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        protected Process HbProcess { get; set; }

        private bool processKilled;

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (this.IsEncoding == false)
                {
                    try
                    {
                        ReadFile(); // Read the last log file back in if it exists
                    }
                    catch (Exception exc)
                    {
                        return exc.ToString();
                    }
                }

                return string.IsNullOrEmpty(this.logBuffer.ToString()) ? "No log data available..." : this.logBuffer.ToString();
            }
        }

        /* Public Methods */

        /// <summary>
        /// Create a preview sample video
        /// </summary>
        /// <param name="query">
        /// The CLI Query
        /// </param>
        public void CreatePreviewSample(string query)
        {
            this.Run(new Job { Query = query }, false);
        }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="encJob">
        /// The enc Job.
        /// </param>
        /// <param name="enableLogging">
        /// Enable Logging. When Disabled we onlt parse Standard Ouput for progress info. Standard Error log data is ignored.
        /// </param>
        protected void Run(Job encJob, bool enableLogging)
        {
            try
            {
                IsEncoding = true;

                if (enableLogging)
                    SetupLogging(encJob);

                if (Init.PreventSleep)
                {
                    Win32.PreventSleep();
                }

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                ProcessStartInfo cliStart = new ProcessStartInfo(handbrakeCLIPath, encJob.Query)
                {
                    RedirectStandardOutput = true,
                    RedirectStandardError = enableLogging ? true : false,
                    UseShellExecute = false,
                    CreateNoWindow = !Init.ShowCliForInGuiEncodeStatus ? true : false
                };

                this.HbProcess = Process.Start(cliStart);

                if (enableLogging)
                {
                    this.HbProcess.ErrorDataReceived += HbProcErrorDataReceived;
                    this.HbProcess.BeginErrorReadLine();
                }

                this.processID = HbProcess.Id;
                this.processHandle = HbProcess.Handle;

                // Set the process Priority
                if (this.processID != -1)
                {
                    this.HbProcess.EnableRaisingEvents = true;
                    this.HbProcess.Exited += HbProcess_Exited;
                }

                // Set the Process Priority
                switch (Init.ProcessPriority)
                {
                    case "Realtime":
                        this.HbProcess.PriorityClass = ProcessPriorityClass.RealTime;
                        break;
                    case "High":
                        this.HbProcess.PriorityClass = ProcessPriorityClass.High;
                        break;
                    case "Above Normal":
                        this.HbProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                        break;
                    case "Normal":
                        this.HbProcess.PriorityClass = ProcessPriorityClass.Normal;
                        break;
                    case "Low":
                        this.HbProcess.PriorityClass = ProcessPriorityClass.Idle;
                        break;
                    default:
                        this.HbProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                        break;
                }

                // Fire the Encode Started Event
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());
            }
            catch (Exception exc)
            {
                errorService.ShowError("It would appear that HandBrakeCLI has not started correctly." +
                "You should take a look at the Activity log as it may indicate the reason why.\n\nDetailed Error Information: error occured in runCli()",
                exc.ToString());
            }
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            try
            {
                if (this.HbProcess != null && !this.HbProcess.HasExited)
                {
                    processKilled = true;
                    this.HbProcess.Kill();
                }
            }
            catch (Exception exc)
            {
                errorService.ShowError("Unable to stop HandBrakeCLI. It may not be running.", exc.ToString());
            }

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

            /*/if (HbProcess != null)
            //{
            //    HbProcess.StandardInput.AutoFlush = true;
            //    HbProcess.StandardInput.WriteLine("^c^z");
            //}*/
        }

        /* Helpers */

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
                string tempLogFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", Init.InstanceId));

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
                if (Init.SaveLogWithVideo)
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(Init.SaveLogPath))
                    if (Init.SaveLogPath != String.Empty && Init.SaveLogToSpecifiedPath)
                        File.Copy(tempLogFile, Path.Combine(Init.SaveLogPath, encodeLogFile));
            }
            catch (Exception exc)
            {
                errorService.ShowError("Unable to make a copy of the log file", exc.ToString());
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
            if (HbProcess != null && HbProcess.HasExited && HbProcess.ExitCode != 0 && !processKilled)
            {
                errorService.ShowError("It appears that HandBrakeCLI has crashed. You can check the Activity Log for further information.", string.Format("Exit Code was: {0}", HbProcess.ExitCode));
            }

            IsEncoding = false;
            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());

            if (windowsSeven.IsWindowsSeven)
            {
                windowsSeven.SetTaskBarProgressToNoProgress();
            }

            if (Init.PreventSleep)
            {
                Win32.AllowSleep();
            }

            try
            {
                if (fileWriter != null)
                {
                    fileWriter.Close();
                    fileWriter.Dispose();
                }
            }
            catch (Exception exc)
            {
                errorService.ShowError("Unable to close the log file wrtier", exc.ToString());
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        private void ReadFile()
        {
            logBuffer = new StringBuilder();
            lock (logBuffer)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", Init.InstanceId));
                string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", Init.InstanceId));
                int logFilePosition = 0;

                try
                {
                    // Copy the log file.
                    if (File.Exists(logFile))
                        File.Copy(logFile, logFile2, true);
                    else
                        return;

                    // Start the Reader
                    // Only use text which continues on from the last read line
                    using (StreamReader sr = new StreamReader(logFile2))
                    {
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
                    }
                }
                catch (Exception exc)
                {
                    throw new Exception("Unable to read log file" + Environment.NewLine + exc);
                }
            }
        }

        /// <summary>
        /// Setup the logging.
        /// </summary>
        /// <param name="encodeJob">
        /// The encode Job.
        /// </param>
        private void SetupLogging(Job encodeJob)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string logFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", Init.InstanceId));
            string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", Init.InstanceId));

            try
            {
                logBuffer = new StringBuilder();

                // Clear the current Encode Logs
                if (File.Exists(logFile)) File.Delete(logFile);
                if (File.Exists(logFile2)) File.Delete(logFile2);

                fileWriter = new StreamWriter(logFile) { AutoFlush = true };

                fileWriter.WriteLine(Logging.CreateCliLogHeader(encodeJob));
                logBuffer.AppendLine(Logging.CreateCliLogHeader(encodeJob));
            }
            catch (Exception exc)
            {
                if (fileWriter != null)
                {
                    fileWriter.Close();
                    fileWriter.Dispose();
                }

                errorService.ShowError("Error", exc.ToString());
            }
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
                try
                {
                    lock (logBuffer)
                        logBuffer.AppendLine(e.Data);

                    if (fileWriter != null && fileWriter.BaseStream.CanWrite)
                    {
                        fileWriter.WriteLine(e.Data);

                        // If the logging grows past 100MB, kill the encode and stop.
                        if (fileWriter.BaseStream.Length > 100000000)
                        {
                            this.Stop();
                            errorService.ShowError("The encode has been stopped. The log file has grown to over 100MB which indicates a serious problem has occured with the encode.",
                                "Please check the encode log for an indication of what the problem is.");
                        }
                    }            
                }
                catch (Exception exc)
                {
                    // errorService.ShowError("Unable to write log data...", exc.ToString());
                }
            }
        }

        #region Encode Status from Standard Output

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
            }
            catch (Exception exc)
            {
                EncodeOnEncodeProgress(null, 0, 0, 0, 0, 0, "Unknown, status not available..");
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
        private void EncodeOnEncodeProgress(object sender, int currentTask, int taskCount, float percentComplete, float currentFps, float avg, string timeRemaining)
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

            if (windowsSeven.IsWindowsSeven)
            {
                int percent;
                int.TryParse(Math.Round(percentComplete).ToString(), out percent);

                windowsSeven.SetTaskBarProgress(percent);
            }
        }

        #endregion
    }
}