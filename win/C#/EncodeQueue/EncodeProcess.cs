/*  QueueItem.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Diagnostics;

namespace Handbrake.EncodeQueue
{
    public class EncodeProcess
    {
        /// <summary>
        /// The CMD.exe process that runs HandBrakeCLI.exe
        /// </summary>
        public Process hbProcProcess { get; set; }

        /// <summary>
        /// Returns whether HandBrake is currently encoding or not.
        /// </summary>
        public Boolean isEncoding { get; set; }

        /// <summary>
        /// Returns the currently encoding query string
        /// </summary>
        public String currentQuery { get; set; }

        /// <summary>
        /// Get or set the process ID of HandBrakeCLI.exe
        /// </summary>
        public int processID { get; set; }

        /// <summary>
        /// Get or Set the Process Handle
        /// </summary>
        public int processHandle { get; set; }
    }
}