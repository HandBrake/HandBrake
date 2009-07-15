/*  QueueHandler.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Serialization;

namespace Handbrake.EncodeQueue
{
    /// <summary>
    /// Provides a handler for encoding jobs and a queue of those jobs.
    /// </summary>
    public class QueueHandler
    {
        public Encode encodeHandler = new Encode();
        private static XmlSerializer serializer = new XmlSerializer(typeof(List<Job>));
        private List<Job> queue = new List<Job>();
        private int nextJobId;

        /// <summary>
        /// Gets the number of items in the queue.
        /// </summary>
        public int Count
        {
            get { return queue.Count; }
        }

        /// <summary>
        /// Gets the last encode that was processed.
        /// </summary>
        /// <returns></returns>
        public Job LastEncode { get; set; }

        /// <summary>
        /// Gets the current state of the encode queue.
        /// </summary>
        public ReadOnlyCollection<Job> CurrentQueue
        {
            get { return queue.AsReadOnly(); }
        }

        /// <summary>
        /// Fires when an encode job has been started.
        /// </summary>
        public event EventHandler NewJobStarted;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        public event EventHandler QueuePauseRequested;

        /// <summary>
        /// Fires when an encode job has been completed.
        /// </summary>
        public event EventHandler CurrentJobCompleted;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        public event EventHandler QueueCompleted;

        #region Queue Handling

        /// <summary>
        /// Gets and removes the next job in the queue.
        /// </summary>
        /// <returns>The job that was removed from the queue.</returns>
        private Job GetNextJob()
        {
            Job job = queue[0];
            LastEncode = job;
            RemoveJob(0); // Remove the item which we are about to pass out.

            WriteQueueStateToFile("hb_queue_recovery.xml");

            return job;
        }

        /// <summary>
        /// Adds an item to the queue.
        /// </summary>
        /// <param name="query">The query that will be passed to the HandBrake CLI.</param>
        /// <param name="source">The location of the source video.</param>
        /// <param name="destination">The location where the encoded video will be.</param>
        public void AddJob(string query, string source, string destination)
        {
            Job newJob = new Job { Id = nextJobId++, Query = query, Source = source, Destination = destination };

            queue.Add(newJob);
            WriteQueueStateToFile("hb_queue_recovery.xml");
        }

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void RemoveJob(int index)
        {
            queue.RemoveAt(index);
            WriteQueueStateToFile("hb_queue_recovery.xml");
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
            if (index < queue.Count - 1)
            {
                Job item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index + 1), item);
            }

            WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file
        }

        /// <summary>
        /// Writes the current state of the queue to a file.
        /// </summary>
        /// <param name="file">The location of the file to write the queue to.</param>
        public void WriteQueueStateToFile(string file)
        {
            string tempPath = file == "hb_queue_recovery.xml" ? Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml") : file;

            try
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
                {
                    serializer.Serialize(strm, queue);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception)
            {
                // Any Errors will be out of diskspace/permissions problems. 
                // Don't report them as they'll annoy the user.
            }
        }

        /// <summary>
        /// Writes the current state of the queue in the form of a batch (.bat) file.
        /// </summary>
        /// <param name="file">The location of the file to write the batch file to.</param>
        public void WriteBatchScriptToFile(string file)
        {
            string queries = "";
            foreach (Job queue_item in queue)
            {
                string q_item = queue_item.Query;
                string fullQuery = '"' + Application.StartupPath + "\\HandBrakeCLI.exe" + '"' + q_item;

                if (queries == string.Empty)
                    queries = queries + fullQuery;
                else
                    queries = queries + " && " + fullQuery;
            }
            string strCmdLine = queries;

            if (file != "")
            {
                try
                {
                    // Create a StreamWriter and open the file, Write the batch file query to the file and 
                    // Close the stream
                    using (StreamWriter line = new StreamWriter(file))
                    {
                        line.WriteLine(strCmdLine);
                    }

                    MessageBox.Show("Your batch script has been sucessfully saved.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
                catch (Exception)
                {
                    MessageBox.Show("Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }

            }
        }

        /// <summary>
        /// Reads a serialized XML file that represents a queue of encoding jobs.
        /// </summary>
        /// <param name="file">The location of the file to read the queue from.</param>
        public void LoadQueueFromFile(string file)
        {
            string tempPath = file == "hb_queue_recovery.xml" ? Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml") : file;

            if (File.Exists(tempPath))
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Job> list = serializer.Deserialize(strm) as List<Job>;

                        if (list != null)
                            foreach (Job item in list)
                                queue.Add(item);

                        if (file != "hb_queue_recovery.xml")
                            WriteQueueStateToFile("hb_queue_recovery.xml");
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
            foreach (Job checkItem in queue)
            {
                if (checkItem.Destination.Contains(destination.Replace("\\\\", "\\")))
                    return true;
            }

            return false;
        }

        #endregion

        #region Encoding

        public bool PauseRequested { get; private set; }
        public bool IsEncoding { get; private set; }

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        public void StartEncodeQueue()
        { 
            if (this.Count != 0)
            {
                if (PauseRequested)
                    PauseRequested = false;
                else
                {
                    PauseRequested = false;
                    try
                    {
                        Thread theQueue = new Thread(startProcess) { IsBackground = true };
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
        public void RequestPause()
        {
            PauseRequested = true;

            if (QueuePauseRequested != null)
                QueuePauseRequested(this, new EventArgs());
        }

        /// <summary>
        /// Stops the current job.
        /// </summary>
        public void EndEncodeJob()
        {
            encodeHandler.closeCLI();
        }

        private void startProcess(object state)
        {
            // Run through each item on the queue
            while (this.Count != 0)
            {
                string query = GetNextJob().Query;
                WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file

                encodeHandler.runCli(query);

                if (NewJobStarted != null)
                    NewJobStarted(this, new EventArgs());

                encodeHandler.hbProcess.WaitForExit();

                encodeHandler.addCLIQueryToLog(query);
                encodeHandler.copyLog(LastEncode.Destination);

                encodeHandler.hbProcess.Close();
                encodeHandler.hbProcess.Dispose();

                if (CurrentJobCompleted != null)
                    CurrentJobCompleted(this, new EventArgs());

                while (PauseRequested) // Need to find a better way of doing this.
                {
                    Thread.Sleep(5000);
                }
            }

            if (QueueCompleted != null)
                QueueCompleted(this, new EventArgs());

            // After the encode is done, we may want to shutdown, suspend etc.
            encodeHandler.afterEncodeAction();
        }

        #endregion
    }
}