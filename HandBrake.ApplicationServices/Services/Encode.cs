/*  Encode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Base;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode : EncodeBase, IEncode
    {
        #region Private Variables

        /// <summary>
        /// Gets The Process Handle
        /// </summary>
        private IntPtr processHandle;

        /// <summary>
        /// Gets the Process ID
        /// </summary>
        private int processId;

        /// <summary>
        /// The Start time of the current Encode;
        /// </summary>
        private DateTime startTime;

        /// <summary>
        /// The Current Task
        /// </summary>
        private QueueTask currentTask;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="Encode"/> class.
        /// </summary>
        public Encode()
        {
            this.EncodeStarted += this.EncodeEncodeStarted;
            GrowlCommunicator.Register();
        }

        #region Properties

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        protected Process HbProcess { get; set; }

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
                this.currentTask = encodeQueueTask;

                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encodeing.");
                }

                this.IsEncoding = true;

                if (enableLogging)
                {
                    try
                    {
                        this.SetupLogging(currentTask);
                    }
                    catch (Exception)
                    {
                        this.IsEncoding = false;
                        throw;
                    }
                }

                if (Properties.Settings.Default.PreventSleep)
                {
                    Win32.PreventSleep();
                }

                // Make sure the path exists, attempt to create it if it doesn't
                this.VerifyEncodeDestinationPath(currentTask);

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                ProcessStartInfo cliStart = new ProcessStartInfo(handbrakeCLIPath, currentTask.Query)
                {
                    RedirectStandardOutput = true,
                    RedirectStandardError = enableLogging ? true : false,
                    UseShellExecute = false,
                    CreateNoWindow = !Properties.Settings.Default.ShowCLI ? true : false
                };

                this.HbProcess = new Process { StartInfo = cliStart };

                this.HbProcess.Start();

                this.startTime = DateTime.Now;

                if (enableLogging)
                {
                    this.HbProcess.ErrorDataReceived += this.HbProcErrorDataReceived;
                    this.HbProcess.BeginErrorReadLine();
                }

                this.processId = this.HbProcess.Id;
                this.processHandle = this.HbProcess.Handle;

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
                this.Invoke_encodeStarted(EventArgs.Empty);
            }
            catch (Exception exc)
            {
                TimeSpan time = DateTime.Now.Subtract(this.currentTask.StartTime);
                this.Invoke_encodeCompleted(
                    new EncodeCompletedEventArgs(
                        false, exc, "An Error occured when trying to encode this source. "));
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
        public override void Stop(Exception exc)
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

            this.Invoke_encodeCompleted(
                exc == null
                    ? new EncodeCompletedEventArgs(true, null, string.Empty)
                    : new EncodeCompletedEventArgs(false, exc, "An Unknown Error has occured when trying to Stop this encode."));
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
            this.IsEncoding = false;
            if (this.WindowsSeven.IsWindowsSeven)
            {
                this.WindowsSeven.SetTaskBarProgressToNoProgress();
            }

            if (Properties.Settings.Default.PreventSleep)
            {
                Win32.AllowSleep();
            }

            try
            {
                // This is just a quick hack to ensure that we are done processing the logging data.
                // Logging data comes in after the exit event has processed sometimes. We should really impliment ISyncronizingInvoke
                // and set the SyncObject on the process. I think this may resolve this properly.
                // For now, just wait 2.5 seconds to let any trailing log messages come in and be processed.
                Thread.Sleep(2500);
                this.HbProcess.CancelErrorRead();
                this.ShutdownFileWriter();
            }
            catch (Exception exc)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }

            this.currentTask.Status = QueueItemStatus.Completed;
            this.Invoke_encodeCompleted(new EncodeCompletedEventArgs(true, null, string.Empty));
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
                this.ProcessLogMessage(e.Data);
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
            Thread monitor = new Thread(this.EncodeMonitor);
            monitor.Start();
        }

        /// <summary>
        /// Monitor the QueueTask
        /// </summary>
        private void EncodeMonitor()
        {
            try
            {
                Parser encode = new Parser(this.HbProcess.StandardOutput.BaseStream);
                encode.OnEncodeProgress += this.EncodeOnEncodeProgress;
                while (!encode.EndOfStream)
                {
                    encode.ReadEncodeStatus();
                }
            }
            catch (Exception)
            {
                this.EncodeOnEncodeProgress(null, 0, 0, 0, 0, 0, "Unknown, status not available..");
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
                TaskCount = taskCount,
                ElapsedTime = DateTime.Now - this.startTime,
            };

            this.Invoke_encodeStatusChanged(eventArgs);

            if (this.WindowsSeven.IsWindowsSeven)
            {
                int percent;
                int.TryParse(Math.Round(percentComplete).ToString(), out percent);

                this.WindowsSeven.SetTaskBarProgress(percent);
            }
        }

        #endregion
    }
}