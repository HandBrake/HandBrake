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
    using System.Text.Json;
    using System.Text.RegularExpressions;
    using System.Windows;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;

    using IQueueService = Services.Queue.Interfaces.IQueueService;

    public class QueueRecoveryHelper
    {
        public static string QueueFileName = "hb_queue";

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
                // Check for any Corrupted Backup Files and try recover them
                RecoverFromBackupFailure();

                // Now check for all available recovery files. (There may be more than 1 for multi-instance support)
                string tempPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
                DirectoryInfo info = new DirectoryInfo(tempPath);
                IEnumerable<FileInfo> foundFiles = info.GetFiles("*.json").Where(f => f.Name.StartsWith(QueueFileName));
                var queueFiles = GetFilesExcludingActiveProcesses(foundFiles, filterQueueFiles);

                if (!queueFiles.Any())
                {
                    return queueFiles;
                }        

                List<string> removeFiles = new List<string>();
                List<string> acceptedFiles = new List<string>();

                foreach (string file in queueFiles)
                {
                    try
                    {
                        using (StreamReader stream = new StreamReader(file))
                        {
                            List<QueueTask> list = list = JsonSerializer.Deserialize<List<QueueTask>>(stream.ReadToEnd(), JsonSettings.Options);
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

                CleanupFiles(removeFiles, false);

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
        /// <param name="queueFilter">
        /// The queue Filter.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool RecoverQueue(IQueueService encodeQueue, IErrorService errorService, bool silentRecovery, List<string> queueFilter)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            List<string> queueFiles = CheckQueueRecovery(queueFilter);
            MessageBoxResult result = MessageBoxResult.None;
            if (!silentRecovery)
            {
                if (queueFiles.Count == 1)
                {
                    result =
                        errorService.ShowMessageBox(
                            Properties.Resources.Queue_RecoverQueueQuestionSingular,
                            Properties.Resources.Queue_RecoveryPossible,
                            MessageBoxButton.YesNo,
                            MessageBoxImage.Question);
                }
                else if (queueFiles.Count > 1)
                {
                    result =
                        errorService.ShowMessageBox(
                            Properties.Resources.Queue_RecoverQueueQuestionPlural,
                            Properties.Resources.Queue_RecoveryPossible,
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
                    CleanupFiles(new List<string> { file }, false);                   
                }

                return isRecovered;
            }

            CleanupFiles(queueFiles, true);
            return false;
        }

        public static bool ArchivesExist()
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            DirectoryInfo info = new DirectoryInfo(appDataPath);
            IEnumerable<FileInfo> foundFiles = info.GetFiles("*.archive").Where(f => f.Name.StartsWith(QueueFileName));

            return foundFiles.Any();
        }

        private static void RecoverFromBackupFailure()
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            DirectoryInfo info = new DirectoryInfo(appDataPath);
            IEnumerable<FileInfo> foundFiles = info.GetFiles("*.last");

            foreach (FileInfo file in foundFiles)
            {
                string corruptedFile = file.FullName.Replace(".last", string.Empty);
                if (File.Exists(corruptedFile))
                {
                    File.Delete(corruptedFile);
                }

                File.Move(file.FullName, corruptedFile);
            }
        }

        private static List<string> GetFilesExcludingActiveProcesses(IEnumerable<FileInfo> foundFiles, List<string> filterQueueFiles)
        {
            List<string> queueFiles = new List<string>();

            // Remove any files where we have an active instance.
            foreach (FileInfo file in foundFiles)
            {
                string fileProcessId = file.Name.Replace(QueueFileName, string.Empty).Replace(".json", string.Empty);
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

        private static void CleanupFiles(List<string> removeFiles, bool archive)
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());

            // Cleanup old/unused queue files for now.
            foreach (string file in removeFiles)
            {
                Match m = Regex.Match(file, @"([0-9]+).json");
                if (m.Success)
                {
                    int processId = int.Parse(m.Groups[1].ToString());
                    if (GeneralUtilities.IsPidACurrentHandBrakeInstance(processId))
                    {
                        continue;
                    }
                }

                string fullPath = Path.Combine(appDataPath, file);

                if (archive)
                {
                    File.Move(fullPath, fullPath + ".archive");
                }
                else
                {
                    File.Delete(fullPath);
                }      
            }

            TidyArchiveFiles();
        }

        /// <summary>
        /// Tidy up archive files older than 7 days.
        /// Gives the user an opportunity to recover a queue file they accidentally chose not to import. 
        /// </summary>
        private static void TidyArchiveFiles()
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            DirectoryInfo info = new DirectoryInfo(appDataPath);
            IEnumerable<FileInfo> foundFiles = info.GetFiles("*.archive").Where(f => f.Name.StartsWith(QueueFileName));

            DateTime lastWeek = DateTime.Now.AddDays(-7);

            foreach (FileInfo file in foundFiles)
            {
                if (file.CreationTime < lastWeek)
                {
                    string fullPath = Path.Combine(appDataPath, file.Name);
                    File.Delete(fullPath);
                }
            }
        }

        public static void ResetArchives()
        {
            string appDataPath = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            DirectoryInfo info = new DirectoryInfo(appDataPath);
            IEnumerable<FileInfo> foundFiles = info.GetFiles("*.archive").Where(f => f.Name.StartsWith(QueueFileName));
            foreach (FileInfo file in foundFiles)
            {
                string fullPath = Path.Combine(appDataPath, file.Name);
                File.Move(fullPath, fullPath.Replace(".archive", string.Empty));
            }
        }
    }
}
