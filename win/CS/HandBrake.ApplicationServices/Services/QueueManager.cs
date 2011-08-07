/*  QueueManager.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Linq;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using EventArgs = System.EventArgs;

    /// <summary>
    /// The Queue Manager.
    /// Thread Safe.
    /// </summary>
    public class QueueManager : IQueueManager
    {
        /*
         * TODO
         * - Rewrite the batch script generator. 
         * - QueueTask, switch everything to use the Task property, which is a model of all settings.
         */

        #region Private Variables

        /// <summary>
        /// A Lock object to maintain thread safety
        /// </summary>
        static readonly object QueueLock = new object();

        /// <summary>
        /// The Queue of Job objects
        /// </summary>
        private readonly List<QueueTask> queue = new List<QueueTask>();

        /// <summary>
        /// HandBrakes Queue file with a place holder for an extra string.
        /// </summary>
        private readonly string queueFile;

        /// <summary>
        /// The ID of the job last added
        /// </summary>
        private int lastJobId;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueManager"/> class.
        /// </summary>
        /// <param name="instanceId">
        /// The instance Id.
        /// </param>
        public QueueManager(int instanceId)
        {
            // If this is the first instance, just use the main queue file, otherwise add the instance id to the filename.
            this.queueFile = instanceId == 0 ? "hb_queue_recovery.xml" : string.Format("hb_queue_recovery{0}.xml", instanceId);
        }

        #region Events
        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        public event EventHandler QueueChanged;

        /// <summary>
        /// Invoke the Queue Changed Event
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        private void InvokeQueueChanged(EventArgs e)
        {
            try
            {
                this.BackupQueue(string.Empty);
            }
            catch (Exception)
            {
                // Do Nothing.
            }

            EventHandler handler = this.QueueChanged;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets or sets Last Processed Job.
        /// This is set when the job is poped of the queue by GetNextJobForProcessing();
        /// </summary>
        public QueueTask LastProcessedJob { get; set; }

        /// <summary>
        /// Gets the number of jobs in the queue;
        /// </summary>
        public int Count
        {
            get
            {
                return this.queue.Where(item => item.Status == QueueItemStatus.Waiting).Count();
            }
        }

        /// <summary>
        /// Gets The current queue.
        /// </summary>
        public ReadOnlyCollection<QueueTask> Queue
        {
            get
            {
                return this.queue.AsReadOnly();
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a job to the Queue. 
        /// This method is Thread Safe.
        /// </summary>
        /// <param name="job">
        /// The encode Job object.
        /// </param>
        public void Add(QueueTask job)
        {
            lock (QueueLock)
            {
                // Tag the job with an ID
                job.Id = lastJobId++;
                queue.Add(job);
                InvokeQueueChanged(EventArgs.Empty);
            }
        }

        /// <summary>
        /// Remove a job from the Queue.
        /// This method is Thread Safe
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void Remove(QueueTask job)
        {
            lock (QueueLock)
            {
                queue.Remove(job);
                InvokeQueueChanged(EventArgs.Empty);
            }
        }

        /// <summary>
        /// Reset a Queued Item from Error or Completed to Waiting
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void ResetJobStatusToWaiting(QueueTask job)
        {
            if (job.Status != QueueItemStatus.Error && job.Status != QueueItemStatus.Completed)
            {
                throw new GeneralApplicationException("Job Error", "Unable to reset job status as it is not in an Error or Completed state", null);
            }

            job.Status = QueueItemStatus.Waiting;
        }

        /// <summary>
        /// Clear down the Queue´s completed items
        /// </summary>
        public void ClearCompleted()
        {
            List<QueueTask> deleteList = this.queue.Where(task => task.Status == QueueItemStatus.Completed).ToList();
            foreach (QueueTask item in deleteList)
            {
                this.queue.Remove(item);
            }
            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Get the first job on the queue for processing.
        /// This also removes the job from the Queue and sets the LastProcessedJob
        /// </summary>
        /// <returns>
        /// An encode Job object.
        /// </returns>
        public QueueTask GetNextJobForProcessing()
        {
            if (this.queue.Count > 0)
            {
                QueueTask job = this.queue.FirstOrDefault(q => q.Status == QueueItemStatus.Waiting);
                if (job != null)
                {
                    job.Status = QueueItemStatus.InProgress;
                    job.StartTime = DateTime.Now;
                    this.LastProcessedJob = job;
                    InvokeQueueChanged(EventArgs.Empty);
                }

                return job;
            }

            return null;
        }

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveUp(int index)
        {
            if (index > 0)
            {
                QueueTask item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index - 1), item);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveDown(int index)
        {
            if (index < this.queue.Count - 1)
            {
                QueueTask item = this.queue[index];

                this.queue.RemoveAt(index);
                this.queue.Insert((index + 1), item);
            }

            this.InvokeQueueChanged(EventArgs.Empty);
        }

        /// <summary>
        /// Backup any changes to the queue file
        /// </summary>
        /// <param name="exportPath">
        /// If this is not null or empty, this will be used instead of the standard backup location.
        /// </param>
        public void BackupQueue(string exportPath)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = !string.IsNullOrEmpty(exportPath) ? exportPath : appDataPath + string.Format(this.queueFile, string.Empty);


            using (FileStream strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
            {
                List<QueueTask> tasks = queue.Where(item => item.Status != QueueItemStatus.Completed).ToList();
                XmlSerializer serializer = new XmlSerializer(typeof(List<QueueTask>));
                serializer.Serialize(strm, tasks);
                strm.Close();
                strm.Dispose();
            }
        }

        /// <summary>
        /// Restore a Queue from file or from the queue backup file.
        /// </summary>
        /// <param name="importPath">
        /// The import path. String.Empty or null will result in the default file being loaded.
        /// </param>
        public void RestoreQueue(string importPath)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = !string.IsNullOrEmpty(importPath) ? importPath : (appDataPath + string.Format(this.queueFile, string.Empty));

            if (File.Exists(tempPath))
            {
                using (FileStream strm = new FileStream((!string.IsNullOrEmpty(importPath) ? importPath : tempPath), FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(List<QueueTask>));

                        List<QueueTask> list = serializer.Deserialize(strm) as List<QueueTask>;

                        if (list != null)
                            foreach (QueueTask item in list)
                            {
                                if (item.Status != QueueItemStatus.Completed)
                                {
                                    // Reset InProgress/Error to Waiting so it can be processed
                                    if (item.Status == QueueItemStatus.InProgress)
                                    {
                                        item.Status = QueueItemStatus.Waiting;
                                    }

                                    this.queue.Add(item);
                                }
                            }

                        this.InvokeQueueChanged(EventArgs.Empty);
                    }
                }
            }
        }

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">The destination of the encode.</param>
        /// <returns>Whether or not the supplied destination is already in the queue.</returns>
        public bool CheckForDestinationPathDuplicates(string destination)
        {
            return this.queue.Any(checkItem => checkItem.Destination.Contains(destination.Replace("\\\\", "\\")));
        }

        /// <summary>
        /// Writes the current state of the queue in the form of a batch (.bat) file.
        /// </summary>
        /// <param name="file">
        /// The location of the file to write the batch file to.
        /// </param>
        /// <returns>
        /// The write batch script to file.
        /// </returns>
        public bool WriteBatchScriptToFile(string file)
        {
            string queries = string.Empty;
            foreach (QueueTask queueItem in this.queue)
            {
                string qItem = queueItem.Query;
                string fullQuery = '"' + Application.StartupPath + "\\HandBrakeCLI.exe" + '"' + qItem;

                if (queries == string.Empty)
                    queries = queries + fullQuery;
                else
                    queries = queries + " && " + fullQuery;
            }
            string strCmdLine = queries;

            if (file != string.Empty)
            {
                try
                {
                    // Create a StreamWriter and open the file, Write the batch file query to the file and 
                    // Close the stream
                    using (StreamWriter line = new StreamWriter(file))
                    {
                        line.WriteLine(strCmdLine);
                    }

                    return true;
                }
                catch (Exception exc)
                {
                    throw new Exception("Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", exc);
                }
            }
            return false;
        }

        #endregion
    }
}
