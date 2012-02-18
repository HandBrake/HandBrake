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
    using System.IO;
    using System.Linq;
    using System.Windows;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Services.Interfaces;

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
        /// <returns>
        /// True if there is a queue to recover.
        /// </returns>
        public static List<string> CheckQueueRecovery()
        {
            try
            {
                XmlSerializer Ser = new XmlSerializer(typeof(List<QueueTask>));
                string tempPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
                List<string> queueFiles = new List<string>();
                List<string> removeFiles = new List<string>();

                DirectoryInfo info = new DirectoryInfo(tempPath);
                IEnumerable<FileInfo> logFiles = info.GetFiles("*.xml").Where(f => f.Name.StartsWith("hb_queue_recovery"));
                foreach (FileInfo file in logFiles)
                {
                    using (FileStream strm = new FileStream(file.FullName, FileMode.Open, FileAccess.Read))
                    {
                        List<QueueTask> list = Ser.Deserialize(strm) as List<QueueTask>;
                        if (list != null && list.Count == 0)
                        {
                            removeFiles.Add(file.FullName);
                        }

                        if (list != null && list.Count != 0)
                        {
                            List<QueueTask> tasks = list.Where(l => l.Status != QueueItemStatus.Completed).ToList();
                            if (tasks.Count != 0)
                            {
                                queueFiles.Add(file.Name);
                            }
                        }
                    }
                }

                // Cleanup old/unused queue files for now.
                if (!GeneralUtilities.IsMultiInstance)
                {
                    foreach (string file in removeFiles)
                    {
                        File.Delete(file);
                    }
                }

                return queueFiles;
            }
            catch (Exception exc)
            {
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
        public static void RecoverQueue(IQueueProcessor encodeQueue, IErrorService errorService)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            List<string> queueFiles = CheckQueueRecovery();
            MessageBoxResult result = MessageBoxResult.None;
            if (queueFiles.Count == 1)
            {
                result = errorService.ShowMessageBox(
                        "HandBrake has detected unfinished items on the queue from the last time the application was launched. Would you like to recover these?",
                        "Queue Recovery Possible", MessageBoxButton.YesNo, MessageBoxImage.Question);
            }
            else if (queueFiles.Count > 1)
            {
                result = MessageBox.Show(
                        "HandBrake has detected multiple unfinished queue files. These will be from multiple instances of HandBrake running. Would you like to recover all unfinished jobs?",
                        "Queue Recovery Possible", MessageBoxButton.YesNo, MessageBoxImage.Question);
            }

            if (result == MessageBoxResult.Yes)
            {
                foreach (string file in queueFiles)
                {
                    encodeQueue.QueueManager.RestoreQueue(appDataPath + file); // Start Recovery
                }
            }
            else
            {
                if (GeneralUtilities.IsMultiInstance) return; // Don't tamper with the files if we are multi instance

                foreach (string file in queueFiles)
                {
                    if (File.Exists(Path.Combine(appDataPath, file)))
                        File.Delete(Path.Combine(appDataPath, file));
                }
            }
        }
    }
}
