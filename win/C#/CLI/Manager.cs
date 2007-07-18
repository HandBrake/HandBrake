using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace Handbrake.CLI
{
    /// <summary>
    /// Delegate to handle pointers to event subscriptions regarding CLI jobs
    /// </summary>
    /// <param name="Sender">The object which raised the event using this delegate</param>
    /// <param name="Job">The job which caused this delegate to fire</param>
    public delegate void JobStatusHandler(Jobs.Job Job);

    /// <summary>
    /// still workin on this
    /// </summary>
    class Manager
    {
        private static Queue<Process> m_processQueue = new Queue<Process>();

        /// <summary>
        /// Raised upon a job being completed
        /// </summary>
        public static event JobStatusHandler OnJobCompleted;

        /// <summary>
        /// Raised upon a new job starting
        /// </summary>
        public static event JobStatusHandler OnJobStarted;

        /// <summary>
        /// Raised upon any noteable progress during a job
        /// </summary>
        public static event JobStatusHandler OnJobProgress;

        /// <summary>
        /// Used for queueing up a job to be processed later
        /// </summary>
        /// <param name="job">The job to be processed later</param>
        public static void EnqueueJob(Jobs.Job job)
        {
            //TODO: create new Process object from passed 
            m_processQueue.Enqueue(CreateProcess(job));
        }

        /// <summary>
        /// Starts the job queue
        /// </summary>
        public static void StartJobs()
        {
            while (m_processQueue.Count > 0)
            {
                Process proc = m_processQueue.Dequeue();
                proc.Start();
                proc.Close();
            }
        }

        /// <summary>
        /// Creates a new Process object from a Job object
        /// </summary>
        /// <param name="job">The Job object to create a process from</param>
        /// <returns>A Process object based on the requirements of the Job passed</returns>
        private static Process CreateProcess(Jobs.Job job)
        {
            Process hbProc = new Process();
            hbProc.StartInfo.FileName = "hbcli.exe";
            hbProc.StartInfo.Arguments = job.ToString();
            hbProc.StartInfo.RedirectStandardOutput = true;
            hbProc.StartInfo.RedirectStandardError = true;
            hbProc.StartInfo.UseShellExecute = false;
            hbProc.StartInfo.CreateNoWindow = true;

            // Set the process Priority
            switch (Properties.Settings.Default.processPriority)
            {
                case "Realtime":
                    hbProc.PriorityClass = ProcessPriorityClass.RealTime;
                    break;
                case "High":
                    hbProc.PriorityClass = ProcessPriorityClass.High;
                    break;
                case "Above Normal":
                    hbProc.PriorityClass = ProcessPriorityClass.AboveNormal;
                    break;
                case "Normal":
                    hbProc.PriorityClass = ProcessPriorityClass.Normal;
                    break;
                case "Low":
                    hbProc.PriorityClass = ProcessPriorityClass.Idle;
                    break;
                default:
                    hbProc.PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
            }

            return hbProc;
        }
    }
}
