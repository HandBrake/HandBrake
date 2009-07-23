/*  QueueHandler.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Serialization;
using Handbrake.Functions;

namespace Handbrake.EncodeQueue
{
    public class EncodeAndQueueHandler
    {
        private static XmlSerializer serializer = new XmlSerializer(typeof(List<Job>));
        private List<Job> queue = new List<Job>();
        private int nextJobId;

        #region Event Handlers
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
        #endregion

        #region Queue
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
        /// Gets the current state of the encode queue.
        /// </summary>
        public ReadOnlyCollection<Job> CurrentQueue
        {
            get { return queue.AsReadOnly(); }
        }

        /// <summary>
        /// Gets the number of items in the queue.
        /// </summary>
        public int Count
        {
            get { return queue.Count; }
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

        /// <summary>
        /// Gets the last encode that was processed.
        /// </summary>
        /// <returns></returns> 
        public Job LastEncode { get; set; }

        /// <summary>
        /// Request Pause
        /// </summary>
        public Boolean PauseRequested { get; private set; }

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
            closeCLI();
        }

        private void startProcess(object state)
        {
            // Run through each item on the queue
            while (this.Count != 0)
            {
                string query = GetNextJob().Query;
                WriteQueueStateToFile("hb_queue_recovery.xml"); // Update the queue recovery file

                runCli(query);

                if (NewJobStarted != null)
                    NewJobStarted(this, new EventArgs());

                hbProcess.WaitForExit();

                addCLIQueryToLog(query);
                copyLog(LastEncode.Destination);

                hbProcess.Close();
                hbProcess.Dispose();

                isEncoding = false;

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
            afterEncodeAction();
        }

        #endregion

        #region CLI and Log Handling
        public Process hbProcess { get; set; }
        public int processID { get; set; }
        public IntPtr processHandle { get; set; }
        public String currentQuery { get; set; }
        public Boolean isEncoding { get; set; }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="query">The CLI Query</param>
        public void runCli(string query)
        {
            try
            {
                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs", "last_encode_log.txt");
                string strCmdLine = String.Format(@" /C """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, query, logPath);
                ProcessStartInfo cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                if (Properties.Settings.Default.enocdeStatusInGui)
                {
                    cliStart.RedirectStandardOutput = true;
                    cliStart.UseShellExecute = false;
                }
                if (Properties.Settings.Default.cli_minimized)
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;

                Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                hbProcess = Process.Start(cliStart);
                processID = Main.getCliProcess(before);
                isEncoding = true;
                currentQuery = query;
                if (hbProcess != null)
                    processHandle = hbProcess.MainWindowHandle; // Set the process Handle

                // Set the process Priority
                Process hbCliProcess = null;
                if (processID != -1)
                    hbCliProcess = Process.GetProcessById(processID);

                if (hbCliProcess != null)
                    switch (Properties.Settings.Default.processPriority)
                    {
                        case "Realtime":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.RealTime;
                            break;
                        case "High":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.High;
                            break;
                        case "Above Normal":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                            break;
                        case "Normal":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.Normal;
                            break;
                        case "Low":
                            hbCliProcess.PriorityClass = ProcessPriorityClass.Idle;
                            break;
                        default:
                            hbCliProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                            break;
                    }
            }
            catch (Exception exc)
            {
                MessageBox.Show("It would appear that HandBrakeCLI has not started correctly. You should take a look at the Activity log as it may indicate the reason why.\n\n   Detailed Error Information: error occured in runCli()\n\n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void closeCLI()
        {
            hbProcess.Kill();
            isEncoding = false;
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        public void afterEncodeAction()
        {
            isEncoding = false;
            currentQuery = String.Empty;
            // Do something whent he encode ends.
            switch (Properties.Settings.Default.CompletionOption)
            {
                case "Shutdown":
                    Process.Start("Shutdown", "-s -t 60");
                    break;
                case "Log Off":
                    Win32.ExitWindowsEx(0, 0);
                    break;
                case "Suspend":
                    Application.SetSuspendState(PowerState.Suspend, true, true);
                    break;
                case "Hibernate":
                    Application.SetSuspendState(PowerState.Hibernate, true, true);
                    break;
                case "Lock System":
                    Win32.LockWorkStation();
                    break;
                case "Quit HandBrake":
                    Application.Exit();
                    break;
                default:
                    break;
            }
        }

        /// <summar>
        /// Append the CLI query to the start of the log file.
        /// </summary>
        /// <param name="query"></param>
        public void addCLIQueryToLog(string query)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string logPath = Path.Combine(logDir, "last_encode_log.txt");

            StreamReader reader = new StreamReader(File.Open(logPath, FileMode.Open, FileAccess.Read, FileShare.Read));
            String log = reader.ReadToEnd();
            reader.Close();

            StreamWriter writer = new StreamWriter(File.Create(logPath));

            writer.Write("### CLI Query: " + query + "\n\n");
            writer.Write("#########################################\n\n");
            writer.WriteLine(log);
            writer.Flush();
            writer.Close();
        }

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination"></param>
        public void copyLog(string destination)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string tempLogFile = Path.Combine(logDir, "last_encode_log.txt");

                string encodeDestinationPath = Path.GetDirectoryName(destination);
                String destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " + DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + ".txt";

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                    Directory.CreateDirectory(logDir);

                // Copy the Log to HandBrakes log folder in the users applciation data folder.
                File.Copy(tempLogFile, Path.Combine(logDir, encodeLogFile));

                // Save a copy of the log file in the same location as the enocde.
                if (Properties.Settings.Default.saveLogWithVideo)
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(Properties.Settings.Default.saveLogPath))
                    if (Properties.Settings.Default.saveLogPath != String.Empty && Properties.Settings.Default.saveLogToSpecifiedPath)
                        File.Copy(tempLogFile, Path.Combine(Properties.Settings.Default.saveLogPath, encodeLogFile));
            }
            catch (Exception exc)
            {
                MessageBox.Show("Something went a bit wrong trying to copy your log file.\nError Information:\n\n" + exc, "Error",
                                MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        #endregion
    }
}