/*  QueueHandler.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using System.Xml.Serialization;
using System.Threading;
using System.Diagnostics;

namespace Handbrake.Queue
{
    public class QueueHandler
    {
        private static XmlSerializer ser = new XmlSerializer(typeof(List<QueueItem>));
        List<QueueItem> queue = new List<QueueItem>();
        int id; // Unique identifer number for each job
        private QueueItem lastItem;

        #region Queue Handling
        public List<QueueItem> getQueue()
        {
            return queue;
        }

        /// <summary>
        /// Get's the next CLI query for encoding
        /// </summary>
        /// <returns>String</returns>
        public string getNextItemForEncoding()
        {
            QueueItem job = queue[0];
            String query = job.Query;
            lastItem = job;
            remove(0);    // Remove the item which we are about to pass out.
            return query;
        }

        /// <summary>
        /// Get the last query that was returned by getNextItemForEncoding()
        /// </summary>
        /// <returns></returns>
        public QueueItem getLastQueryItem()
        {
            return lastItem;
        }

        /// <summary>
        /// Add's a new item to the queue
        /// </summary>
        /// <param name="query">String</param>
        /// <param name="source"></param>
        /// <param name="destination"></param>
        public void add(string query, string source, string destination)
        {
            QueueItem newJob = new QueueItem();
            newJob.Id = id;
            newJob.Query = query;
            newJob.Source = source;
            newJob.Destination = destination;
            id++;

            // Adds the job to the queue
            queue.Add(newJob);
        }

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">Index</param>
        /// <returns>Bolean true if successful</returns>
        public Boolean remove(int index)
        {
            queue.RemoveAt(index);
            return true;
        }

        /// <summary>
        /// Returns how many items are in the queue
        /// </summary>
        /// <returns>Int</returns>
        public int count()
        {
            return queue.Count;
        }

        /// <summary>
        /// Move an item with an index x, up in the queue
        /// </summary>
        /// <param name="index">Int</param>
        public void moveUp(int index)
        {
            if (index > 0)
            {
                QueueItem item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index - 1), item);
            }
        }

        /// <summary>
        /// Move an item with an index x, down in the queue
        /// </summary>
        /// <param name="index">Int</param>
        public void moveDown(int index)
        {
            if (index < queue.Count - 1)
            {
                QueueItem item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index + 1), item);
            }
        }

        /// <summary>
        /// Writes the current queue to disk. hb_queue_recovery.xml
        /// This function is called after getNextItemForEncoding()
        /// </summary>
        public void write2disk(string file)
        {
            string tempPath;
            if (file == "hb_queue_recovery.xml")
                tempPath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml");
            else
                tempPath = file;

            try
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
                {
                    ser.Serialize(strm, queue);
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
        /// Writes the current queue to disk to the location specified in file
        /// </summary>
        /// <param name="file"></param>
        public void writeBatchScript(string file)
        {
            string queries = "";
            foreach (QueueItem queue_item in queue)
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
                    StreamWriter line = new StreamWriter(file);
                    line.WriteLine(strCmdLine);
                    line.Close();

                    MessageBox.Show("Your batch script has been sucessfully saved.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
                catch (Exception)
                {
                    MessageBox.Show("Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }

            }
        }

        /// <summary>
        /// Recover the queue from hb_queue_recovery.xml
        /// </summary>
        public void recoverQueue(string file)
        {
            string tempPath;
            if (file == "hb_queue_recovery.xml")
                tempPath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml");
            else
                tempPath = file;

            if (File.Exists(tempPath))
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<QueueItem> list = ser.Deserialize(strm) as List<QueueItem>;

                        if (list != null)
                            foreach (QueueItem item in list)
                                queue.Add(item);

                        if (file != "hb_queue_recovery.xml")
                            write2disk("hb_queue_recovery.xml");
                    }
                }
            }
        }
        #endregion

        //------------------------------------------------------------------------
        Functions.Encode encodeHandler = new Functions.Encode();
        private Boolean started = false;
        private Boolean paused;
        private Boolean encoding;

        #region Encoding

        public Boolean isEncodeStarted
        {
            get { return started; }
        }
        public Boolean isPaused
        {
            get { return paused; }
        }
        public Boolean isEncoding
        {
            get { return encoding; }
        }

        public void startEncode()
        {
            Thread theQueue;
            if (this.count() != 0)
            {
                if (paused)
                    paused = false;
                else
                {
                    paused = false;
                    try
                    {
                        theQueue = new Thread(startProc) {IsBackground = true};
                        theQueue.Start();
                    }
                    catch (Exception exc)
                    {
                        MessageBox.Show(exc.ToString());
                    }
                }
            }
        }
        public void pauseEncode()
        {
            paused = true;
            EncodePaused(null);
        }

        private void startProc(object state)
        {
            Process hbProc;
            try
            {
                // Run through each item on the queue
                while (this.count() != 0)
                {
                    string query = getNextItemForEncoding();
                    write2disk("hb_queue_recovery.xml"); // Update the queue recovery file

                    EncodeStarted(null);
                    hbProc = encodeHandler.runCli(this, query);
                    hbProc.WaitForExit();

                    encodeHandler.addCLIQueryToLog(query);
                    encodeHandler.copyLog(query, getLastQueryItem().Destination);

                    hbProc.Close();
                    hbProc.Dispose();
                    EncodeFinished(null);

                    while (paused) // Need to find a better way of doing this.
                    {
                        Thread.Sleep(10000);
                    }
                }
                EncodeQueueFinished(null);

                // After the encode is done, we may want to shutdown, suspend etc.
                encodeHandler.afterEncodeAction();
            }
            catch (Exception exc)
            {
                throw new Exception(exc.ToString());
            }
        }
        #endregion

        #region Events
        public event EventHandler OnEncodeStart;
        public event EventHandler OnPaused;
        public event EventHandler OnEncodeEnded;
        public event EventHandler OnQueueFinished;

        // Invoke the Changed event; called whenever encodestatus changes:
        protected virtual void EncodeStarted(EventArgs e)
        {
            if (OnEncodeStart != null)
                OnEncodeStart(this, e);

            encoding = true;
        }
        protected virtual void EncodePaused(EventArgs e)
        {
            if (OnPaused != null)
                OnPaused(this, e);
        }
        protected virtual void EncodeFinished(EventArgs e)
        {
            if (OnEncodeEnded != null)
                OnEncodeEnded(this, e);

            encoding = false;
        }
        protected virtual void EncodeQueueFinished(EventArgs e)
        {
            if (OnQueueFinished != null)
                OnQueueFinished(this, e);
        }
        #endregion

    }
}
