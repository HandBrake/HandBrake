/*  CLI.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Threading;
using System.Diagnostics;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;

namespace Handbrake.Functions
{
    public class CLI
    {       
        /// <summary>
        /// CLI output is based on en-US locale,
        /// we use this CultureInfo as IFormatProvider to *.Parse() calls
        /// </summary>
        static readonly public CultureInfo Culture = new CultureInfo("en-US", false);

        Process hbProc = new Process();

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="s"></param>
        /// <param name="query">The CLI Query</param>
        /// <param name="stderr">Rediect standard error</param>
        /// <param name="stdout">Redirect Standard output</param>
        /// <param name="useShellExec"> Use Shell Executable</param>
        /// <param name="noWindow">Display No Window</param>
        /// <returns>Returns a process</returns>
        public Process runCli(object s, string query)
        {
            try
            {
                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");

                string strCmdLine = String.Format(@"cmd /c """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, query, logPath);

                ProcessStartInfo cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                hbProc = Process.Start(cliStart);

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
            }
            catch
            {
                MessageBox.Show("Internal Software Error. Please Restart the Program");
            }
            return hbProc;
        }

        [DllImport("user32.dll")]
        public static extern void LockWorkStation();
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        public void afterEncodeAction()
        {
            // Do something whent he encode ends.
            switch (Properties.Settings.Default.CompletionOption)
            {
                case "Shutdown":
                    System.Diagnostics.Process.Start("Shutdown", "-s -t 60");
                    break;
                case "Log Off":
                    ExitWindowsEx(0, 0);
                    break;
                case "Suspend":
                    Application.SetSuspendState(PowerState.Suspend, true, true);
                    break;
                case "Hibernate":
                    Application.SetSuspendState(PowerState.Hibernate, true, true);
                    break;
                case "Lock System":
                    LockWorkStation();
                    break;
                case "Quit HandBrake":
                    Application.Exit();
                    break;
                default:
                    break;
            }
        }

        /// <summary>
        /// Update the presets.dat file with the latest version of HandBrak's presets from the CLI
        /// </summary>
        public void grabCLIPresets()
        {
            string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
            string presetsPath = Path.Combine(Application.StartupPath, "presets.dat");

            string strCmdLine = String.Format(@"cmd /c """"{0}"" --preset-list >""{1}"" 2>&1""", handbrakeCLIPath, presetsPath);

            ProcessStartInfo hbGetPresets = new ProcessStartInfo("CMD.exe", strCmdLine);
            hbGetPresets.WindowStyle = ProcessWindowStyle.Hidden;

            Process hbproc = Process.Start(hbGetPresets);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();
        }
    }
}
