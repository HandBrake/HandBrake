/*  CLI.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Diagnostics;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;

namespace Handbrake.Functions
{
    public class Encode
    {
        // DLL Imports
        [DllImport("user32.dll")]
        private static extern void LockWorkStation();
        [DllImport("user32.dll")]
        private static extern int ExitWindowsEx(int uFlags, int dwReason);

        // Declarations
        Process hbProc = new Process();

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="query">The CLI Query</param>
        public Process runCli(string query)
        {
            try
            {
                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");

                string strCmdLine = String.Format(@" CMD /c """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, query, logPath);

                ProcessStartInfo cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);
                if (Properties.Settings.Default.cli_minimized == "Checked")
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;
                hbProc = Process.Start(cliStart);
                isEncoding = true;
                currentQuery = query;

                // Set the process Priority 
                if (hbProc != null)
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
            catch (Exception exc)
            {
                MessageBox.Show("An error occured in runCli()\n Error Information: \n\n" + exc);
            }
            return hbProc;
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
        /// Append the CLI query to the start of the log file.
        /// </summary>
        /// <param name="query"></param>
        public void addCLIQueryToLog(string query)
        {
            string logPath = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");

            StreamReader reader = new StreamReader(File.Open(logPath, FileMode.Open, FileAccess.Read));
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
            // The user may wish to do something with the log.
            if (Properties.Settings.Default.saveLog == "Checked")
            {
                string logPath = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");

                if (Properties.Settings.Default.saveLogWithVideo == "Checked")
                {
                    string[] destName = destination.Split('\\');
                    string destinationFile = "";
                    for (int i = 0; i < destName.Length - 1; i++)
                    {
                        destinationFile += destName[i] + "\\";
                    }

                    destinationFile += DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + " " + destName[destName.Length - 1] + ".txt";

                    File.Copy(logPath, destinationFile);
                }
                else if (Properties.Settings.Default.saveLogPath != String.Empty)
                {
                    string[] destName = destination.Split('\\');
                    string dest = destName[destName.Length - 1];
                    string filename = DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + " " + dest + ".txt";
                    string useDefinedLogPath = Path.Combine(Properties.Settings.Default.saveLogPath, filename);

                    File.Copy(logPath, useDefinedLogPath);
                }
            }
        }

        /// <summary>
        /// Returns whether HandBrake is currently encoding or not.
        /// </summary>
        public Boolean isEncoding { get; set; }

        /// <summary>
        /// Returns the currently encoding query string
        /// </summary>
        public String currentQuery { get; set; }

    }
}
