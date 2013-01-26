// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Encode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Class which handles the CLI
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Windows.Forms;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Base;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode : EncodeBase, IEncode
    {
        #region Private Variables

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingService;

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

        /// <summary>
        /// Initializes a new instance of the <see cref="Encode"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public Encode(IUserSettingService userSettingService)
            : base(userSettingService)
        {
            this.userSettingService = userSettingService;  
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
        /// This should only be called from the UI thread.
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
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encodeing.");
                }

                this.IsEncoding = true;

                this.currentTask = encodeQueueTask;

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

                if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep))
                {
                    Win32.PreventSleep();
                }

                // Make sure the path exists, attempt to create it if it doesn't
                this.VerifyEncodeDestinationPath(currentTask);

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");

                // TODO tidy this code up, it's kinda messy.
                string query = this.currentTask.Task.IsPreviewEncode
                                   ? QueryGeneratorUtility.GeneratePreviewQuery(
                                       new EncodeTask(this.currentTask.Task),
                                       this.currentTask.Task.PreviewEncodeDuration,
                                       this.currentTask.Task.PreviewEncodeStartAt,
                                       userSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount),
                                       userSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity),
                                       userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav))
                                   : QueryGeneratorUtility.GenerateQuery(new EncodeTask(this.currentTask.Task), 
                                   userSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount),
                                   userSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity),
                                   userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav));

                ProcessStartInfo cliStart = new ProcessStartInfo(handbrakeCLIPath, query)
                {
                    RedirectStandardOutput = true,
                    RedirectStandardError = enableLogging,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                this.HbProcess = new Process { StartInfo = cliStart };

                this.HbProcess.Start();

                this.startTime = DateTime.Now;

                if (enableLogging)
                {
                    this.HbProcess.ErrorDataReceived += this.HbProcErrorDataReceived;
                    this.HbProcess.BeginErrorReadLine();
                }

                this.HbProcess.OutputDataReceived += HbProcess_OutputDataReceived;
                this.HbProcess.BeginOutputReadLine();

                this.processId = this.HbProcess.Id;

                // Set the process Priority
                if (this.processId != -1)
                {
                    this.HbProcess.EnableRaisingEvents = true;
                    this.HbProcess.Exited += this.HbProcessExited;
                }

                // Set the Process Priority
                switch (this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.ProcessPriority))
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
                this.InvokeEncodeStarted(EventArgs.Empty);
            }
            catch (Exception exc)
            {
                encodeQueueTask.Status = QueueItemStatus.Error;
                this.InvokeEncodeCompleted(
                    new EncodeCompletedEventArgs(
                        false, null, "An Error occured when trying to encode this source. "));
                throw;
            }
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
        private void HbProcessExited(object sender, EventArgs e)
        {
            HbProcess.WaitForExit();

            try
            {
                this.HbProcess.CancelErrorRead();
                this.HbProcess.CancelOutputRead();
                this.ShutdownFileWriter();
            }
            catch (Exception exc)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }

            Execute.OnUIThread(() =>
                {
                    if (this.WindowsSeven.IsWindowsSeven)
                    {
                        this.WindowsSeven.SetTaskBarProgressToNoProgress();
                    }

                    if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep))
                    {
                        Win32.AllowSleep();
                    }

                    this.currentTask.Status = QueueItemStatus.Completed;
                    this.IsEncoding = false;
                    this.InvokeEncodeCompleted(new EncodeCompletedEventArgs(true, null, string.Empty));
                });
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
                if (initShutdown && this.LogBuffer.Length < 25000000) 
                {
                    initShutdown = false; // Reset this flag.
                }

                if (this.LogBuffer.Length > 25000000 && !initShutdown) // Approx 23.8MB and make sure it's only printed once
                {
                    this.ProcessLogMessage("ERROR: Initiating automatic shutdown of encode process. The size of the log file inidcates that there is an error! ");
                    initShutdown = true;
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
                    Execute.OnUIThread(
                        () =>
                            {
                                if (!this.IsEncoding)
                                {
                                    // We can get events out of order since the CLI progress is monitored on a background thread.
                                    // So make sure we don't send a status update after an encode complete event.
                                    return;
                                }

                                this.InvokeEncodeStatusChanged(eventArgs);

                                if (this.WindowsSeven.IsWindowsSeven)
                                {
                                    int percent;
                                    int.TryParse(
                                        Math.Round(eventArgs.PercentComplete).ToString(CultureInfo.InvariantCulture),
                                        out percent);

                                    this.WindowsSeven.SetTaskBarProgress(percent);
                                }
                            });
                }
            }
        }

        #endregion
    }
}