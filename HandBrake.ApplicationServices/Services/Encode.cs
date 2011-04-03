/*  Encode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode : IEncode
    {
        #region Private Variables

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private IUserSettingService userSettingService = new UserSettingService();

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
        private int processId;

        /// <summary>
        /// Windows 7 API Pack wrapper
        /// </summary>
        private Win7 windowsSeven = new Win7();

        /// <summary>
        /// A Lock for the filewriter
        /// </summary>
        static readonly object fileWriterLock = new object();

        /// <summary>
        /// The Log File Header
        /// </summary>
        StringBuilder header = GeneralUtilities.CreateCliLogHeader(null);

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="Encode"/> class.
        /// </summary>
        public Encode()
        {
            this.EncodeStarted += this.EncodeEncodeStarted;
            this.logBuffer = new StringBuilder();
            GrowlCommunicator.Register();
        }

        #region Delegates and Event Handlers

        /// <summary>
        /// Fires when a new CLI QueueTask starts
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI QueueTask finishes.
        /// </summary>
        public event EncodeCompletedStatus EncodeCompleted;

        /// <summary>
        /// Encode process has progressed
        /// </summary>
        public event EncodeProgessStatus EncodeStatusChanged;
        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        protected Process HbProcess { get; set; }

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
                return string.IsNullOrEmpty(this.logBuffer.ToString()) ? header + "No log data available..." : header + this.logBuffer.ToString();
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="encodeQueueTask">
        /// The encodeQueueTask.
        /// </param>
        /// <param name="enableLogging">
        /// Enable Logging. When Disabled we onlt parse Standard Ouput for progress info. Standard Error log data is ignored.
        /// </param>
        public void Start(QueueTask encodeQueueTask, bool enableLogging)
        {
            try
            {
                QueueTask queueTask = encodeQueueTask;

                if (queueTask == null)
                {
                    throw new ArgumentNullException("QueueTask was null");
                }

                if (IsEncoding)
                {
                    throw new Exception("HandBrake is already encodeing.");
                }

                IsEncoding = true;

                if (enableLogging)
                {
                    try
                    {
                        SetupLogging(queueTask);
                    }
                    catch (Exception)
                    {
                        IsEncoding = false;
                        throw;
                    }
                }

                if (Properties.Settings.Default.PreventSleep)
                {
                    Win32.PreventSleep();
                }

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                ProcessStartInfo cliStart = new ProcessStartInfo(handbrakeCLIPath, queueTask.Query)
                {
                    RedirectStandardOutput = true,
                    RedirectStandardError = enableLogging ? true : false,
                    UseShellExecute = false,
                    CreateNoWindow = !Properties.Settings.Default.ShowCLI ? true : false
                };

                this.HbProcess = new Process { StartInfo = cliStart };

                this.HbProcess.Start();

                if (enableLogging)
                {
                    this.HbProcess.ErrorDataReceived += HbProcErrorDataReceived;
                    this.HbProcess.BeginErrorReadLine();
                }

                this.processId = HbProcess.Id;
                this.processHandle = HbProcess.Handle;

                // Set the process Priority
                if (this.processId != -1)
                {
                    this.HbProcess.EnableRaisingEvents = true;
                    this.HbProcess.Exited += this.HbProcessExited;
                }

                // Set the Process Priority
                switch (Properties.Settings.Default.ProcessPriority)
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
                if (this.EncodeCompleted != null)
                    this.EncodeCompleted(this, new EncodeCompletedEventArgs(false, exc, "An Error has occured in EncodeService.Run()"));
            }
        }

        /// <summary>
        /// Stop the Encode
        /// </summary>
        public void Stop()
        {
            this.Stop(null);
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        /// <param name="exc">
        /// The Exception that has occured.
        /// This will get bubbled up through the EncodeCompletedEventArgs
        /// </param>
        public void Stop(Exception exc)
        {
            try
            {
                if (this.HbProcess != null && !this.HbProcess.HasExited)
                {
                    this.HbProcess.Kill();
                }
            }
            catch (Exception)
            {
                // No need to report anything to the user. If it fails, it's probably already stopped.
            }


            if (exc == null)
            {
                if (this.EncodeCompleted != null)
                    this.EncodeCompleted(this, new EncodeCompletedEventArgs(true, null, string.Empty));
            }
            else
            {
                if (this.EncodeCompleted != null)
                    this.EncodeCompleted(this, new EncodeCompletedEventArgs(false, exc, "An Error has occured."));
            }
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyStop()
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

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination">
        /// The Destination File Path
        /// </param>
        public void ProcessLogs(string destination)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string tempLogFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", GeneralUtilities.GetInstanceCount));

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
                if (Properties.Settings.Default.SaveLogWithVideo)
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(Properties.Settings.Default.SaveLogCopyDirectory) && Properties.Settings.Default.SaveLogToCopyDirectory)
                    File.Copy(tempLogFile, Path.Combine(Properties.Settings.Default.SaveLogCopyDirectory, encodeLogFile));
            }
            catch (Exception exc)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }
        }

        #endregion

        #region Private Helper Methods

        /// <summary>
        /// The HandBrakeCLI process has exited.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void HbProcessExited(object sender, EventArgs e)
        {
            IsEncoding = false;
            if (windowsSeven.IsWindowsSeven)
            {
                windowsSeven.SetTaskBarProgressToNoProgress();
            }

            if (Properties.Settings.Default.PreventSleep)
            {
                Win32.AllowSleep();
            }

            try
            {
                lock (fileWriterLock)
                {
                    // This is just a quick hack to ensure that we are done processing the logging data.
                    // Logging data comes in after the exit event has processed sometimes. We should really impliment ISyncronizingInvoke
                    // and set the SyncObject on the process. I think this may resolve this properly.
                    // For now, just wait 2.5 seconds to let any trailing log messages come in and be processed.
                    Thread.Sleep(2500);

                    this.HbProcess.CancelErrorRead();

                    if (fileWriter != null)
                    {
                        fileWriter.Close();
                        fileWriter.Dispose();
                    }

                    fileWriter = null;
                }
            }
            catch (Exception exc)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }

            if (this.EncodeCompleted != null)
                this.EncodeCompleted(this, new EncodeCompletedEventArgs(true, null, string.Empty));
        }

        /// <summary>
        /// Setup the logging.
        /// </summary>
        /// <param name="encodeQueueTask">
        /// The encode QueueTask.
        /// </param>
        private void SetupLogging(QueueTask encodeQueueTask)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string logFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", GeneralUtilities.GetInstanceCount));
            string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", GeneralUtilities.GetInstanceCount));

            try
            {
                logBuffer = new StringBuilder();

                // Clear the current Encode Logs
                if (File.Exists(logFile)) File.Delete(logFile);
                if (File.Exists(logFile2)) File.Delete(logFile2);

                fileWriter = new StreamWriter(logFile) { AutoFlush = true };
                fileWriter.WriteLine(GeneralUtilities.CreateCliLogHeader(encodeQueueTask));
            }
            catch (Exception)
            {
                if (fileWriter != null)
                {
                    fileWriter.Close();
                    fileWriter.Dispose();
                }
                throw;
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

                    lock (fileWriterLock)
                    {
                        if (fileWriter != null && fileWriter.BaseStream.CanWrite)
                        {
                            fileWriter.WriteLine(e.Data);

                            // If the logging grows past 100MB, kill the encode and stop.
                            if (fileWriter.BaseStream.Length > 100000000)
                            {
                                this.Stop(
                                    new Exception(
                                        "The encode has been stopped. The log file has grown to over 100MB which indicates a serious problem has occured with the encode." +
                                        "Please check the encode log for an indication of what the problem is."));
                            }
                        }
                    }
                }
                catch (Exception exc)
                {
                    // Do Nothing.
                }
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
        private void EncodeEncodeStarted(object sender, EventArgs e)
        {
            Thread monitor = new Thread(EncodeMonitor);
            monitor.Start();
        }

        /// <summary>
        /// Monitor the QueueTask
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
                EstimatedTimeLeft = Converters.EncodeToTimespan(timeRemaining),
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