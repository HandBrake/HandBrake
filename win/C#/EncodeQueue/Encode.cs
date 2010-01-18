/*  Encode.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using Handbrake.Functions;

namespace Handbrake.EncodeQueue
{
    public class Encode
    {
        public Process HbProcess { get; set; }
        public int ProcessID { get; set; }
        public IntPtr ProcessHandle { get; set; }
        public String CurrentQuery { get; set; }
        public Boolean IsEncoding { get; set; }

        public event EventHandler EncodeStarted;
        public event EventHandler EncodeEnded;

        /// <summary>
        /// Create a preview sample video
        /// </summary>
        /// <param name="query"></param>
        public void CreatePreviewSampe(string query)
        {
            Run(query);
        }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="query">The CLI Query</param>
        protected void Run(string query)
        {
            try
            {
                if (EncodeStarted != null)
                    EncodeStarted(this, new EventArgs());

                IsEncoding = true;

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs", "last_encode_log.txt");
                string strCmdLine = String.Format(@" /C """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, query, logPath);
                ProcessStartInfo cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                if (Properties.Settings.Default.enocdeStatusInGui)
                {
                    cliStart.RedirectStandardOutput = true;
                    cliStart.UseShellExecute = false;
                    if (!Properties.Settings.Default.showCliForInGuiEncodeStatus)
                        cliStart.CreateNoWindow = true;
                }
                if (Properties.Settings.Default.cli_minimized)
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;

                Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                HbProcess = Process.Start(cliStart);
                ProcessID = Main.GetCliProcess(before);
                CurrentQuery = query;
                if (HbProcess != null)
                    ProcessHandle = HbProcess.MainWindowHandle; // Set the process Handle

                // Set the process Priority
                Process hbCliProcess = null;
                if (ProcessID != -1)
                    hbCliProcess = Process.GetProcessById(ProcessID);

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
        public void Stop()
        {
            if (HbProcess != null)
                HbProcess.Kill();

            Process[] list = Process.GetProcessesByName("HandBrakeCLI");
            foreach (Process process in list)
                    process.Kill();

            IsEncoding = false;

            if (EncodeEnded != null)
                EncodeEnded(this, new EventArgs());
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        protected void Finish()
        {
            IsEncoding = false;
            CurrentQuery = String.Empty;

            //Growl
            if (Properties.Settings.Default.growlQueue)
                GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");

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

        /// <summary>
        /// Add the CLI Query to the Log File.
        /// </summary>
        /// <param name="encJob"></param>
        protected void AddCLIQueryToLog(Job encJob)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logPath = Path.Combine(logDir, "last_encode_log.txt");

                StreamReader reader =
                    new StreamReader(File.Open(logPath, FileMode.Open, FileAccess.Read, FileShare.Read));
                String log = reader.ReadToEnd();
                reader.Close();

                StreamWriter writer = new StreamWriter(File.Create(logPath));

                writer.Write("### CLI Query: " + encJob.Query + "\n\n");
                writer.Write("### User Query: " + encJob.CustomQuery + "\n\n");
                writer.Write("#########################################\n\n");
                writer.WriteLine(log);
                writer.Flush();
                writer.Close();
            } catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination"></param>
        protected void CopyLog(string destination)
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
    }
}