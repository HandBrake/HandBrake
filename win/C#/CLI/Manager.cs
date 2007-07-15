using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace Handbrake.CLI
{
    /// <summary>
    /// still workin on this
    /// </summary>
    class Manager
    {
        private static Queue<Process> m_processQueue = new Queue<Process>();

        public static void Enqueue(object s)
        {
            //TODO: create new Process object from passed 
        }
    }
}
