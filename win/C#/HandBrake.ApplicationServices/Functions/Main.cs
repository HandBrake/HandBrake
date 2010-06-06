/*  Main.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;

    /// <summary>
    /// Useful functions which various screens can use.
    /// </summary>
    public static class Main
    {
        /// <summary>
        /// Get the Process ID of HandBrakeCLI for the current instance.
        /// </summary>
        /// <param name="before">List of processes before the new process was started</param>
        /// <returns>Int - Process ID</returns>
        public static int GetCliProcess(Process[] before)
        {
            // This is a bit of a cludge. Maybe someone has a better idea on how to impliment this.
            // Since we used CMD to start HandBrakeCLI, we don't get the process ID from hbProc.
            // Instead we take the processes before and after, and get the ID of HandBrakeCLI.exe
            // avoiding any previous instances of HandBrakeCLI.exe in before.
            // Kill the current process.

            DateTime startTime = DateTime.Now;
            TimeSpan duration;

            Process[] hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
            while (hbProcesses.Length == 0)
            {
                hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
                duration = DateTime.Now - startTime;
                if (duration.Seconds > 5 && hbProcesses.Length == 0)
                    // Make sure we don't wait forever if the process doesn't start
                    return -1;
            }

            Process hbProcess = null;
            foreach (Process process in hbProcesses)
            {
                bool found = false;
                // Check if the current CLI instance was running before we started the current one
                foreach (Process bprocess in before)
                {
                    if (process.Id == bprocess.Id)
                        found = true;
                }

                // If it wasn't running before, we found the process we want.
                if (!found)
                {
                    hbProcess = process;
                    break;
                }
            }
            if (hbProcess != null)
                return hbProcess.Id;

            return -1;
        }

        /// <summary>
        /// Show the Exception Window
        /// </summary>
        /// <param name="shortError">
        /// The short error.
        /// </param>
        /// <param name="longError">
        /// The long error.
        /// </param>
        public static void ShowExceptiowWindow(string shortError, string longError)
        {
            frmExceptionWindow exceptionWindow = new frmExceptionWindow();
            exceptionWindow.Setup(shortError, longError);
            exceptionWindow.Show();
        }
    }
}