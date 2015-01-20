// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Class which handles the CLI
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class EncodeService : EncodeBase, IEncode
    {
        #region Private Variables

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

        /// <summary>
        /// The init shutdown.
        /// </summary>
        private bool initShutdown;

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        protected Process HbProcess { get; set; }

        /// <summary>
        /// Gets a value indicating whether can pause.
        /// </summary>
        public bool CanPause
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is pasued.
        /// </summary>
        public bool IsPasued { get; private set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// This should only be called from the UI thread.
        /// </summary>
        /// <param name="encodeQueueTask">
        /// The encodeQueueTask.
        /// </param>
        public void Start(QueueTask encodeQueueTask)
        {
            try
            {
                if (this.IsEncoding)
                {
                    throw new GeneralApplicationException("HandBrake is already encodeing.", "Please try again in a minute", null);
                }

                this.IsEncoding = true;
                this.currentTask = encodeQueueTask;

                try
                {
                    this.SetupLogging(this.currentTask, false);
                }
                catch (Exception)
                {
                    this.IsEncoding = false;
                    throw;
                }

                // Make sure the path exists, attempt to create it if it doesn't
                this.VerifyEncodeDestinationPath(this.currentTask);

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");

                // TODO tidy this code up, it's kinda messy.
                string query = this.currentTask.Task.IsPreviewEncode
                                   ? QueryGeneratorUtility.GeneratePreviewQuery(
                                       new EncodeTask(this.currentTask.Task),
                                       encodeQueueTask.Configuration,
                                       this.currentTask.Task.PreviewEncodeDuration.HasValue ? this.currentTask.Task.PreviewEncodeDuration.Value : 0,
                                       this.currentTask.Task.PreviewEncodeStartAt.HasValue ? this.currentTask.Task.PreviewEncodeStartAt.Value : 0)
                                   : QueryGeneratorUtility.GenerateQuery(new EncodeTask(this.currentTask.Task), encodeQueueTask.Configuration);

                ProcessStartInfo cliStart = new ProcessStartInfo(handbrakeCLIPath, query)
                {
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                this.HbProcess = new Process { StartInfo = cliStart };

                this.HbProcess.Start();

                this.startTime = DateTime.Now;

                this.HbProcess.ErrorDataReceived += this.HbProcErrorDataReceived;
                this.HbProcess.BeginErrorReadLine();

                this.HbProcess.OutputDataReceived += this.HbProcess_OutputDataReceived;
                this.HbProcess.BeginOutputReadLine();

                this.processId = this.HbProcess.Id;

                // Set the process Priority
                if (this.processId != -1)
                {
                    this.HbProcess.EnableRaisingEvents = true;
                    this.HbProcess.Exited += this.HbProcessExited;
                }

                // Set the Process Priority
                switch (encodeQueueTask.Configuration.ProcessPriority)
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
                this.InvokeEncodeStarted(System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                encodeQueueTask.Status = QueueItemStatus.Error;
                this.IsEncoding = false;
                this.InvokeEncodeCompleted(
                    new EncodeCompletedEventArgs(
                        false, exc, "An Error occured when trying to encode this source. ", this.currentTask.Task.Destination));
                throw;
            }
        }

        /// <summary>
        /// The pause.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// This feature is not available for CLI based encoding.
        /// </exception>
        public void Pause()
        {
            throw new NotImplementedException("This feature is not available for CLI based encoding.");
        }

        /// <summary>
        /// The resume.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// This feature is not available for CLI based encoding.
        /// </exception>
        public void Resume()
        {
            throw new NotImplementedException("This feature is not available for CLI based encoding.");
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public override void Stop()
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
        }

        /// <summary>
        /// Shutdown the service.
        /// </summary>
        public void Shutdown()
        {
            // Nothing to do.
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
        private void HbProcessExited(object sender, System.EventArgs e)
        {
            this.HbProcess.WaitForExit();

            try
            {
                this.HbProcess.CancelErrorRead();
                this.HbProcess.CancelOutputRead();
                this.ShutdownFileWriter();
            }
            catch (Exception)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }

            this.currentTask.Status = QueueItemStatus.Completed;
            this.IsEncoding = false;
            this.InvokeEncodeCompleted(new EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Task.Destination));
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
        /// <remarks>
        /// Worker Thread.
        /// </remarks>
        private void HbProcErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (!String.IsNullOrEmpty(e.Data))
            {
                if (this.initShutdown && this.LogBuffer.Length < 25000000)
                {
                    this.initShutdown = false; // Reset this flag.
                }

                if (this.LogBuffer.Length > 25000000 && !this.initShutdown) // Approx 23.8MB and make sure it's only printed once
                {
                    this.ProcessLogMessage("ERROR: Initiating automatic shutdown of encode process. The size of the log file indicates that there is an error! ");
                    this.initShutdown = true;
                    this.Stop();
                }

                this.ProcessLogMessage(e.Data);
            }
        }

        /// <summary>
        /// The hb process output data received.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void HbProcess_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (!String.IsNullOrEmpty(e.Data) && this.IsEncoding)
            {
                EncodeProgressEventArgs eventArgs = this.ReadEncodeStatus(e.Data, this.startTime);
                if (eventArgs != null)
                {
                    if (!this.IsEncoding)
                    {
                        // We can get events out of order since the CLI progress is monitored on a background thread.
                        // So make sure we don't send a status update after an encode complete event.
                        return;
                    }

                    this.InvokeEncodeStatusChanged(eventArgs);
                }
            }
        }

        #endregion
    }
}