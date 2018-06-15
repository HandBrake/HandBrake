// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueRecoveryHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueRecoveryHelper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Windows;
    using System.Xml.Serialization;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;

    using IQueueProcessor = HandBrakeWPF.Services.Queue.Interfaces.IQueueProcessor;

    /// <summary>
    /// Queue Recovery Helper
    /// </summary>
    public class QueueRecoveryHelper
    {
        /// <summary>
        /// Check if the queue recovery file contains records.
        /// If it does, it means the last queue did not complete before HandBrake closed.
        /// So, return a boolean if true. 
        /// </summary>
        /// <param name="filterQueueFiles">
        /// The filter Queue Files.
        /// </param>
        /// <returns>
        /// True if there is a queue to recover.
        /// </returns>
        public static List<string> CheckQueueRecovery(List<string> filterQueueFiles)
        {
            try
            {
                string tempPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
                DirectoryInfo info = new DirectoryInfo(tempPath);
                IEnumerable<FileInfo> foundFiles = info.GetFiles("*.xml").Where(f => f.Name.StartsWith("hb_queue_recovery"));
                var queueFiles = GetFilesExcludingActiveProcesses(foundFiles, filterQueueFiles);

                if (!queueFiles.Any())
                {
                    return queueFiles;
                }        

                List<string> removeFiles = new List<string>();
                List<string> acceptedFiles = new List<string>();

                XmlSerializer ser = new XmlSerializer(typeof(List<QueueTask>));
                foreach (string file in queueFiles)
                {
                    try
                    {
                        using (FileStream strm = new FileStream(file, FileMode.Open, FileAccess.Read))
                        {
                            List<QueueTask> list = ser.Deserialize(strm) as List<QueueTask>;
                            if (list != null && list.Count == 0)
                            {
                                removeFiles.Add(file);
                            }

                            if (list != null && list.Count != 0)
                            {
                                List<QueueTask> tasks = list.Where(l => l.Status != QueueItemStatus.Completed).ToList();
                                if (tasks.Count != 0)
                                {
                                    acceptedFiles.Add(Path.GetFileName(file));
                                }
                                else
                                {
                                    removeFiles.Add(file);
                                }
                            }
                        }
                    }
                    catch (Exception exc)
                    {
                        Debug.WriteLine(exc);
                    }
                }

                CleanupFiles(removeFiles);

                return acceptedFiles;
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
                return new List<string>(); // Keep quiet about the error.
            }
        }

        /// <summary>
        /// Recover a queue from file.
        /// </summary>
        /// <param name="encodeQueue">
        /// The encode Queue.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        /// <param name="silentRecovery">
        /// The silent Recovery.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool RecoverQueue(IQueueProcessor encodeQueue, IErrorService errorService, bool silentRecovery, List<string> queueFilter)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());
            List<string> queueFiles = CheckQueueRecovery(queueFilter);
            MessageBoxResult result = MessageBoxResult.None;
            if (!silentRecovery)
            {
                if (queueFiles.Count == 1)
                {
                    result =
                        errorService.ShowMessageBox(
                            "HandBrake has detected unfinished items on the queue from the last time the application was launched. Would you like to recover these?",
                            "Queue Recovery Possible",
                            MessageBoxButton.YesNo,
                            MessageBoxImage.Question);
                }
                else if (queueFiles.Count > 1)
                {
                    result =
                        errorService.ShowMessageBox(
                            "HandBrake has detected multiple unfinished queue files. These will be from multiple instances of HandBrake running. Would you like to recover all unfinished jobs?",
                            "Queue Recovery Possible",
                            MessageBoxButton.YesNo,
                            MessageBoxImage.Question);
                }
            }
            else
            {
                result = MessageBoxResult.Yes;
            }

            if (result == MessageBoxResult.Yes)
            {
                bool isRecovered = false;
                foreach (string file in queueFiles)
                {
                    // Recover the Queue
                    encodeQueue.RestoreQueue(Path.Combine(appDataPath, file));
                    isRecovered = true;

                    // Cleanup
                    CleanupFiles(new List<string> { file });                   
                }

                return isRecovered;
            }

            CleanupFiles(queueFiles);
            return false;
        }

        private static List<string> GetFilesExcludingActiveProcesses(IEnumerable<FileInfo> foundFiles, List<string> filterQueueFiles)
        {
            List<string> queueFiles = new List<string>();

            // Remove any files where we have an active instnace.
            foreach (FileInfo file in foundFiles)
            {
                string fileProcessId = file.Name.Replace("hb_queue_recovery", string.Empty).Replace(".xml", string.Empty);
                int processId;
                if (!string.IsNullOrEmpty(fileProcessId) && int.TryParse(fileProcessId, out processId))
                {
                    if (!GeneralUtilities.IsPidACurrentHandBrakeInstance(processId))
                    {
                        if (filterQueueFiles != null && filterQueueFiles.Count > 0)
                        {
                            if (filterQueueFiles.Contains(processId.ToString()))
                            {
                                queueFiles.Add(file.FullName);
                            }
                        }
                        else
                        {
                            queueFiles.Add(file.FullName);
                        }                       
                    }
                }
            }

            return queueFiles;
        }

        private static void CleanupFiles(List<string> removeFiles)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly());

            // Cleanup old/unused queue files for now.
            foreach (string file in removeFiles)
            {
                Match m = Regex.Match(file, @"([0-9]+).xml");
                if (m.Success)
                {
                    int processId = int.Parse(m.Groups[1].ToString());
                    if (GeneralUtilities.IsPidACurrentHandBrakeInstance(processId))
                    {
                        continue;
                    }
                }

                string fullPath = Path.Combine(appDataPath, file);
                File.Delete(fullPath);
            }
        }
    }
}
