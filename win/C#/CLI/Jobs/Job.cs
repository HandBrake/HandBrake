using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace Handbrake.CLI.Jobs
{
    public class Job
    {
        private string m_exec;

        private Process m_proc;
        /// <summary>
        /// The base process associated with this job
        /// </summary>
        public Process Process
        {
            get
            {
                return this.m_proc;
            }
        }

        public Job()
        {
        }
    }
}
