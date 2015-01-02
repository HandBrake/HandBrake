// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Base Class for the Encode Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    /// <summary>
    /// A Base Class for the Encode Services.
    /// </summary>
    public class EncodeBase
    {
        #region Private Variables

        /// <summary>
        /// A Lock for the filewriter
        /// </summary>
        private static readonly object FileWriterLock = new object();

        /// <summary>
        /// The Log File Header
        /// </summary>
        private readonly StringBuilder header;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The Log file writer
        /// </summary>
        private StreamWriter fileWriter;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="EncodeBase"/> class.
        /// </summary>
        public EncodeBase()
        {
            this.logBuffer = new StringBuilder();
            this.header = GeneralUtilities.CreateCliLogHeader();
            this.LogIndex = 0;
        }

        #region Events

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
        /// Gets or sets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding { get; protected set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                string noLog = "There is no log information to display." + Environment.NewLine + Environment.NewLine
                 + "This window will only display logging information after you have started an encode." + Environment.NewLine
                 + Environment.NewLine + "You can find previous log files in the log directory or by clicking the 'Open Log Directory' button above.";
                
                return string.IsNullOrEmpty(this.logBuffer.ToString())
                           ? noLog
                           : this.header + this.logBuffer.ToString();
            }
        }

        /// <summary>
        /// Gets the log index.
        /// </summary>
        public int LogIndex { get; private set; }

        /// <summary>
        /// Gets LogBuffer.
        /// </summary>
        public StringBuilder LogBuffer
        {
            get
            {
                return this.logBuffer;
            }
        }

        #endregion

        #region Invoke Events

        /// <summary>
        /// Invoke the Encode Status Changed Event.
        /// </summary>
        /// <param name="e">
        /// The EncodeProgressEventArgs.
        /// </param>
        public void InvokeEncodeStatusChanged(EncodeProgressEventArgs e)
        {
            EncodeProgessStatus handler = this.EncodeStatusChanged;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        /// <summary>
        /// Invoke the Encode Completed Event
        /// </summary>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        public void InvokeEncodeCompleted(EncodeCompletedEventArgs e)
        {
            EncodeCompletedStatus handler = this.EncodeCompleted;
            if (handler != null)
            {
                handler(this, e);
            }

            this.LogIndex = 0; // Reset
        }

        /// <summary>
        /// Invoke the Encode Started Event
        /// </summary>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        public void InvokeEncodeStarted(System.EventArgs e)
        {
            EventHandler handler = this.EncodeStarted;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        #endregion

        #region Methods

        /// <summary>
        /// A Stop Method to be implemeneted.
        /// </summary>
        public virtual void Stop()
        {
            // Do Nothing
        }

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination">
        /// The Destination File Path
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        public void ProcessLogs(string destination, HBConfiguration configuration)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string tempLogFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", GeneralUtilities.ProcessId));

                string encodeDestinationPath = Path.GetDirectoryName(destination);
                string destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " +
                                       DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + ".txt";

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                {
                    Directory.CreateDirectory(logDir);
                }

                // Copy the Log to HandBrakes log folder in the users applciation data folder.
                File.Copy(tempLogFile, Path.Combine(logDir, encodeLogFile));

                // Save a copy of the log file in the same location as the enocde.
                if (configuration.SaveLogWithVideo)
                {
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));
                }

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(configuration.SaveLogCopyDirectory) && configuration.SaveLogToCopyDirectory)
                {
                    File.Copy(
                        tempLogFile, Path.Combine(configuration.SaveLogCopyDirectory, encodeLogFile));
                }
            }
            catch (Exception)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }
        }

        /// <summary>
        /// Pase the CLI status output (from standard output)
        /// </summary>
        /// <param name="encodeStatus">
        /// The encode Status.
        /// </param>
        /// <param name="startTime">
        /// The start Time.
        /// </param>
        /// <returns>
        /// The <see cref="EncodeProgressEventArgs"/>.
        /// </returns>
        public EncodeProgressEventArgs ReadEncodeStatus(string encodeStatus, DateTime startTime)
        {
            try
            {
                Match m = Regex.Match(
                    encodeStatus,
                    @"^Encoding: task ([0-9]*) of ([0-9]*), ([0-9]*\.[0-9]*) %( \(([0-9]*\.[0-9]*) fps, avg ([0-9]*\.[0-9]*) fps, ETA ([0-9]{2})h([0-9]{2})m([0-9]{2})s\))?");

                if (m.Success)
                {
                    int currentTask = int.Parse(m.Groups[1].Value);
                    int totalTasks = int.Parse(m.Groups[2].Value);
                    float percent = float.Parse(m.Groups[3].Value, CultureInfo.InvariantCulture);
                    float currentFps = m.Groups[5].Value == string.Empty
                                           ? 0.0F
                                           : float.Parse(m.Groups[5].Value, CultureInfo.InvariantCulture);
                    float avgFps = m.Groups[6].Value == string.Empty
                                       ? 0.0F
                                       : float.Parse(m.Groups[6].Value, CultureInfo.InvariantCulture);
                    string remaining = string.Empty;
                    if (m.Groups[7].Value != string.Empty)
                    {
                        remaining = m.Groups[7].Value + ":" + m.Groups[8].Value + ":" + m.Groups[9].Value;
                    }
                    if (string.IsNullOrEmpty(remaining))
                    {
                        remaining = "Calculating ...";
                    }

                    EncodeProgressEventArgs eventArgs = new EncodeProgressEventArgs
                                                            {
                                                                AverageFrameRate = avgFps,
                                                                CurrentFrameRate = currentFps,
                                                                EstimatedTimeLeft =
                                                                    Converters.EncodeToTimespan(
                                                                        remaining),
                                                                PercentComplete = percent,
                                                                Task = currentTask,
                                                                TaskCount = totalTasks,
                                                                ElapsedTime =
                                                                    DateTime.Now - startTime,
                                                            };

                    return eventArgs;
                }

                return null;
            }
            catch (Exception)
            {
                return null;
            }
        }

        /// <summary>
        /// Setup the logging.
        /// </summary>
        /// <param name="encodeQueueTask">
        /// The encode QueueTask.
        /// </param>
        protected void SetupLogging(QueueTask encodeQueueTask)
        {
            this.ShutdownFileWriter();
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string logFile = Path.Combine(logDir, string.Format("last_encode_log{0}.txt", GeneralUtilities.ProcessId));
            string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", GeneralUtilities.ProcessId));

            try
            {
                string query = QueryGeneratorUtility.GenerateQuery(new EncodeTask(encodeQueueTask.Task), encodeQueueTask.Configuration);
                this.logBuffer = new StringBuilder();
                this.logBuffer.AppendLine(String.Format("CLI Query: {0}", query));
                this.logBuffer.AppendLine();

                // Clear the current Encode Logs)
                if (File.Exists(logFile))
                {
                    File.Delete(logFile);
                }

                if (File.Exists(logFile2))
                {
                    File.Delete(logFile2);
                }

                this.fileWriter = new StreamWriter(logFile) { AutoFlush = true };
                this.fileWriter.WriteLine(this.header);
                this.fileWriter.WriteLine(string.Format("CLI Query: {0}", query));
                this.fileWriter.WriteLine();
            }
            catch (Exception)
            {
                if (this.fileWriter != null)
                {
                    this.fileWriter.Close();
                    this.fileWriter.Dispose();
                }

                throw;
            }
        }

        /// <summary>
        /// Process an Incomming Log Message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ProcessLogMessage(string message)
        {
            if (!String.IsNullOrEmpty(message))
            {
                try
                {
                    this.LogIndex = this.LogIndex + 1;

                    lock (this.LogBuffer)
                    {
                        this.LogBuffer.AppendLine(message);
                    }

                    lock (FileWriterLock)
                    {
                        if (this.fileWriter != null && this.fileWriter.BaseStream.CanWrite)
                        {
                            this.fileWriter.WriteLine(message);
                        }
                    }
                }
                catch (Exception)
                {
                    // Do Nothing.
                }
            }
        }

        /// <summary>
        /// Shutdown and Dispose of the File Writer.
        /// </summary>
        protected void ShutdownFileWriter()
        {
            try
            {
                lock (FileWriterLock)
                {
                    if (this.fileWriter != null)
                    {
                        this.fileWriter.Close();
                        this.fileWriter.Dispose();
                    }

                    this.fileWriter = null;
                }
            }
            catch (Exception)
            {
                // This exception doesn't warrent user interaction, but it should be logged (TODO)
            }
        }

        /// <summary>
        /// Verify the Encode Destination path exists and if not, create it.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <exception cref="Exception">
        /// If the creation fails, an exception is thrown.
        /// </exception>
        protected void VerifyEncodeDestinationPath(QueueTask task)
        {
            // Make sure the path exists, attempt to create it if it doesn't
            try
            {
                string path = Directory.GetParent(task.Task.Destination).ToString();
                if (!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    "Unable to create directory for the encoded output.", "Please verify that you have a valid path.", exc);
            }
        }

        #endregion
    }
}