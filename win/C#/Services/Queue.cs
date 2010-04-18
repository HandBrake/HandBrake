/*  Queue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */


namespace Handbrake.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Threading;
    using System.Windows.Forms;
    using System.Xml.Serialization;
    using Functions;
    using Model;

    /// <summary>
    /// The HandBrake Queue
    /// </summary>
    public class Queue : Encode
    {
        /// <summary>
        /// An XML Serializer
        /// </summary>
        private static XmlSerializer serializer;

        /// <summary>
        /// The Queue Job List
        /// </summary>
        private readonly List<Job> queue = new List<Job>();

        /// <summary>
        /// The Next Job ID
        /// </summary>
        private int nextJobId;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        public event EventHandler QueuePauseRequested;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        public event EventHandler QueueCompleted;

        #region Queue

        /// <summary>
        /// Gets and removes the next job in the queue.
        /// </summary>
        /// <returns>The job that was removed from the queue.</returns>
        private Job GetNextJob()
        {
            Job job = this.queue[0];
            this.LastEncode = job;
            this.Remove(0); // Remove the item which we are about to pass out.

            this.WriteQueueStateToFile("hb_queue_recovery.xml");

            return job;
        }

        /// <summary>
        /// Gets the current state of the encode queue.
        /// </summary>
        public ReadOnlyCollection<Job> CurrentQueue
        {
            get { return this.queue.AsReadOnly(); }
        }

        /// <summary>
        /// Gets the number of items in the queue.
        /// </summary>
        public int Count
        {
            get { return this.queue.Count; }
        }

        /// <summary>
        /// Adds an item to the queue.
        /// </summary>
        /// <param name="query">
        /// The query that will be passed to the HandBrake CLI.
        /// </param>
        /// <param name="source">
        /// The location of the source video.
        /// </param>
        /// <param name="destination">
        /// The location where the encoded video will be.
        /// </param>
        /// <param name="customJob">
        /// Custom job
        /// </param>
        public void Add(string query, int title, string source, string destination, bool customJob)
        {
            Job newJob = new Job
                             {
                                 Id = this.nextJobId++,
                                 Title = title,
                                 Query = query, 
                                 Source = source, 
                                 Destination = destination, 
                                 CustomQuery = customJob
                             };

            this.queue.Add(newJob);
            this.WriteQueueStateToFile("hb_queue_recovery.xml");
        }

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void Remove(int index)
        {
            this.queue.RemoveAt(index);
            this.WriteQueueStateToFile("hb_queue_recovery.xml");
        }

        /// <summary>
        /// Retrieve a job from the queue
        /// </summary>
        /// <param name="index">the job id</param>
        /// <returns>A job for the given index or blank job object</returns>
        public Job GetJob(int index)
        {
            if (this.queue.Count >= (index + 1))
                return this.queue[index];

            return new Job();
        }

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveUp(int index)
        {
            if (index > 0)
            {
                Job item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index - 1), item);
            }

            WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file
        }

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveDown(int index)
        {
            if (index < this.queue.Count - 1)
            {
                Job item = this.queue[index];

                this.queue.RemoveAt(index);
                this.queue.Insert((index + 1), item);
            }

            this.WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file
        }

        /// <summary>
        /// Writes the current state of the queue to a file.
        /// </summary>
        /// <param name="file">The location of the file to write the queue to.</param>
        public void WriteQueueStateToFile(string file)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), 
                                              @"HandBrake\hb_queue_recovery.xml");
            string tempPath = file == "hb_queue_recovery.xml" ? appDataPath : file;

            try
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
                {
                    if (serializer == null)
                        serializer = new XmlSerializer(typeof (List<Job>));
                    serializer.Serialize(strm, queue);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Writes the current state of the queue in the form of a batch (.bat) file.
        /// </summary>
        /// <param name="file">The location of the file to write the batch file to.</param>
        public void WriteBatchScriptToFile(string file)
        {
            string queries = string.Empty;
            foreach (Job queueItem in this.queue)
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

                    MessageBox.Show("Your batch script has been sucessfully saved.", "Status", MessageBoxButtons.OK, 
                                    MessageBoxIcon.Asterisk);
                }
                catch (Exception)
                {
                    MessageBox.Show(
                        "Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", 
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }
            }
        }

        /// <summary>
        /// Reads a serialized XML file that represents a queue of encoding jobs.
        /// </summary>
        /// <param name="file">The location of the file to read the queue from.</param>
        public void LoadQueueFromFile(string file)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), 
                                              @"HandBrake\hb_queue_recovery.xml");
            string tempPath = file == "hb_queue_recovery.xml" ? appDataPath : file;

            if (File.Exists(tempPath))
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        if (serializer == null)
                            serializer = new XmlSerializer(typeof (List<Job>));

                        List<Job> list = serializer.Deserialize(strm) as List<Job>;

                        if (list != null)
                            foreach (Job item in list)
                                this.queue.Add(item);

                        if (file != "hb_queue_recovery.xml")
                            this.WriteQueueStateToFile("hb_queue_recovery.xml");
                    }
                }
            }
        }

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">The destination of the encode.</param>
        /// <returns>Whether or not the supplied destination is already in the queue.</returns>
        public bool CheckForDestinationDuplicate(string destination)
        {
            foreach (Job checkItem in this.queue)
            {
                if (checkItem.Destination.Contains(destination.Replace("\\\\", "\\")))
                    return true;
            }

            return false;
        }

        #endregion

        #region Encoding

        /// <summary>
        /// Gets or sets the last encode that was processed.
        /// </summary>
        /// <returns></returns> 
        public Job LastEncode { get; set; }

        /// <summary>
        /// Gets a value indicating whether Request Pause
        /// </summary>
        public bool PauseRequested { get; private set; }

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        public void Start()
        {
            if (this.Count != 0)
            {
                if (this.PauseRequested)
                    this.PauseRequested = false;
                else
                {
                    this.PauseRequested = false;
                    try
                    {
                        Thread theQueue = new Thread(this.StartQueue) {IsBackground = true};
                        theQueue.Start();
                    }
                    catch (Exception exc)
                    {
                        MessageBox.Show(exc.ToString());
                    }
                }
            }
        }

        /// <summary>
        /// Requests a pause of the encode queue.
        /// </summary>
        public void Pause()
        {
            this.PauseRequested = true;

            if (this.QueuePauseRequested != null)
                this.QueuePauseRequested(this, new EventArgs());
        }

        /// <summary>
        /// Run through all the jobs on the queue.
        /// </summary>
        /// <param name="state">Object State</param>
        private void StartQueue(object state)
        {
            // Run through each item on the queue
            while (this.Count != 0)
            {
                Job encJob = this.GetNextJob();
                this.WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file

                Run(encJob);

                if (HbProcess == null)
                {
                    return;
                }
                HbProcess.WaitForExit();

                AddCLIQueryToLog(encJob);
                this.CopyLog(this.LastEncode.Destination);

                HbProcess.Close();
                HbProcess.Dispose();

                IsEncoding = false;

                // Growl
                if (Properties.Settings.Default.growlEncode)
                    GrowlCommunicator.Notify("Encode Completed", 
                                             "Put down that cocktail...\nyour Handbrake encode is done.");

                while (this.PauseRequested) // Need to find a better way of doing this.
                {
                    Thread.Sleep(2000);
                }
            }
            this.LastEncode = new Job();

            if (this.QueueCompleted != null)
                this.QueueCompleted(this, new EventArgs());

            // After the encode is done, we may want to shutdown, suspend etc.
            Finish();
        }

        #endregion
    }
}