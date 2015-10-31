// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Base Class for the Encode Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;

    using HandBrake.ApplicationServices.Model;

    using HandBrakeWPF.Utilities;

    using EncodeCompletedEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeCompletedEventArgs;
    using EncodeCompletedStatus = HandBrakeWPF.Services.Encode.Interfaces.EncodeCompletedStatus;
    using EncodeProgessStatus = HandBrakeWPF.Services.Encode.Interfaces.EncodeProgessStatus;
    using EncodeProgressEventArgs = HandBrakeWPF.Services.Encode.EventArgs.EncodeProgressEventArgs;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;

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
            this.header = GeneralUtilities.CreateLogHeader();
            this.LogIndex = 0;
        }

        #region Events

        /// <summary>
        /// Fires when a new QueueTask starts
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a QueueTask finishes.
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
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination">
        /// The Destination File Path
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        public void ProcessLogs(string destination, bool isPreview, HBConfiguration configuration)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";

                string tempLogFile = Path.Combine(logDir, isPreview ? $"preview_encode_log{GeneralUtilities.ProcessId}.txt" : string.Format("last_encode_log{0}.txt", GeneralUtilities.ProcessId));

                string encodeDestinationPath = Path.GetDirectoryName(destination);
                string destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " +
                                       DateTime.Now.ToString(CultureInfo.InvariantCulture).Replace("/", "-").Replace(":", "-") + ".txt";

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
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrent user interaction, but it should be logged
            }
        }

        /// <summary>
        /// Setup the logging.
        /// </summary>
        protected void SetupLogging(bool isPreviewEncode)
        {
            this.ShutdownFileWriter();
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string logFile = Path.Combine(logDir, isPreviewEncode ? $"preview_last_encode_log{GeneralUtilities.ProcessId}.txt" : $"last_encode_log{GeneralUtilities.ProcessId}.txt");

            try
            {
                this.logBuffer = new StringBuilder();

                this.logBuffer.AppendLine();

                // Clear the current Encode Logs)
                if (File.Exists(logFile))
                {
                    File.Delete(logFile);
                }

                lock (FileWriterLock)
                {
                    this.fileWriter = new StreamWriter(logFile) { AutoFlush = true };
                    this.fileWriter.WriteLine(this.header);
                    this.fileWriter.WriteLine();
                }
            }
            catch (Exception)
            {
                if (this.fileWriter != null)
                {
                    lock (FileWriterLock)
                    {
                        this.fileWriter.Flush();
                        this.fileWriter.Close();
                        this.fileWriter.Dispose();
                    }                
                }

                throw;
            }
        }

        /// <summary>
        /// The service log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ServiceLogMessage(string message)
        {
            this.ProcessLogMessage(string.Format("# {0}", message));
        }

        /// <summary>
        /// Process an Incomming Log Message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        protected void ProcessLogMessage(string message)
        {
            if (!string.IsNullOrEmpty(message))
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
                catch (Exception exc)
                {
                    Debug.WriteLine(exc); // This exception doesn't warrent user interaction, but it should be logged
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
                        this.fileWriter.Flush();
                        this.fileWriter.Close();
                        this.fileWriter.Dispose();
                    }

                    this.fileWriter = null;
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrent user interaction, but it should be logged
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
        protected void VerifyEncodeDestinationPath(EncodeTask task)
        {
            // Make sure the path exists, attempt to create it if it doesn't
            try
            {
                string path = Directory.GetParent(task.Destination).ToString();
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