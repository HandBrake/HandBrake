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

    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using EncodeCompletedEventArgs = EventArgs.EncodeCompletedEventArgs;
    using EncodeCompletedStatus = Interfaces.EncodeCompletedStatus;
    using EncodeProgessStatus = Interfaces.EncodeProgessStatus;
    using EncodeProgressEventArgs = EventArgs.EncodeProgressEventArgs;
    using EncodeTask = Model.EncodeTask;
    using GeneralApplicationException = Exceptions.GeneralApplicationException;
    using ILog = Logging.Interfaces.ILog;

    public class EncodeBase
    {
        protected ILog encodeLogService;

        private readonly IUserSettingService userSettingService;
        
        public EncodeBase(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
        }

        public event EventHandler EncodeStarted;

        public event EncodeCompletedStatus EncodeCompleted;

        public event EncodeProgessStatus EncodeStatusChanged;

        public bool IsEncoding { get; protected set; }

        public void InvokeEncodeStatusChanged(EncodeProgressEventArgs e)
        {
            EncodeProgessStatus handler = this.EncodeStatusChanged;
            handler?.Invoke(this, e);
        }

        public void InvokeEncodeCompleted(EncodeCompletedEventArgs e)
        {
            EncodeCompletedStatus handler = this.EncodeCompleted;
            handler?.Invoke(this, e);
        }

        public void InvokeEncodeStarted(System.EventArgs e)
        {
            EventHandler handler = this.EncodeStarted;
            handler?.Invoke(this, e);
        }

        public string ProcessLogs(string destination)
        {
            try
            {
                string logDir = DirectoryUtilities.GetLogDirectory();
                string encodeDestinationPath = Path.GetDirectoryName(destination);
                string destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " + DateTime.Now.ToString(CultureInfo.InvariantCulture).Replace("/", "-").Replace(":", "-") + ".txt";
                string logContent = this.encodeLogService.GetFullLog();

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                {
                    Directory.CreateDirectory(logDir);
                }

                // Copy the Log to HandBrakes log folder in the users applciation data folder.
                this.WriteFile(logContent, Path.Combine(logDir, encodeLogFile));

                // Save a copy of the log file in the same location as the enocde.
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo))
                {
                    this.WriteFile(logContent, Path.Combine(encodeDestinationPath, encodeLogFile));
                }

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory)) && this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory))
                {
                    this.WriteFile(logContent, Path.Combine(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory), encodeLogFile));
                }

                return Path.Combine(logDir, encodeLogFile);
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrant user interaction, but it should be logged
            }

            return null;
        }

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

        private void WriteFile(string content, string fileName)
        {
            try
            {
                using (StreamWriter fileWriter = new StreamWriter(fileName) { AutoFlush = true })
                {
                    fileWriter.Write(content);
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }
    }
}