/*  Encode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.EncodeQueue
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Forms;
    using Functions;
    using Properties;

    /// <summary>
    /// Class which handles the CLI
    /// </summary>
    public class Encode
    {
        /// <summary>
        /// Fires when a new CLI Job starts
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI job finishes.
        /// </summary>
        public event EventHandler EncodeEnded;

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        public Process HbProcess { get; set; }

        /// <summary>
        /// Gets or sets The Process Handle
        /// </summary>
        public IntPtr ProcessHandle { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether HandBrakeCLI.exe is running
        /// </summary>
        public bool IsEncoding { get; set; }

        /// <summary>
        /// Gets or sets the Process ID
        /// </summary>
        private int ProcessID { get; set; }

        /// <summary>
        /// Create a preview sample video
        /// </summary>
        /// <param name="query">
        /// The CLI Query
        /// </param>
        public void CreatePreviewSample(string query)
        {
            this.Run(query);
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            if (this.HbProcess != null)
                this.HbProcess.Kill();

            Process[] list = Process.GetProcessesByName("HandBrakeCLI");
            foreach (Process process in list)
                process.Kill();

            this.IsEncoding = false;

            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyClose()
        {
            if ((int) this.ProcessHandle == 0)
                return;

            // Allow the CLI to exit cleanly
            Win32.SetForegroundWindow((int) this.ProcessHandle);
            SendKeys.Send("^C");

            // HbProcess.StandardInput.AutoFlush = true;
            // HbProcess.StandardInput.WriteLine("^C");
        }

        /// <summary>
        /// Execute a HandBrakeCLI process.
        /// </summary>
        /// <param name="query">
        /// The CLI Query
        /// </param>
        protected void Run(string query)
        {
            try
            {
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());

                this.IsEncoding = true;

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logPath =
                    Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs", 
                        "last_encode_log.txt");
                string strCmdLine = String.Format(@" /C """"{0}"" {1} 2>""{2}"" """, handbrakeCLIPath, query, logPath);
                var cliStart = new ProcessStartInfo("CMD.exe", strCmdLine);

                if (Settings.Default.enocdeStatusInGui)
                {
                    cliStart.RedirectStandardOutput = true;
                    cliStart.UseShellExecute = false;
                    if (!Settings.Default.showCliForInGuiEncodeStatus)
                        cliStart.CreateNoWindow = true;
                }
                if (Settings.Default.cli_minimized)
                    cliStart.WindowStyle = ProcessWindowStyle.Minimized;

                Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                this.HbProcess = Process.Start(cliStart);
                this.ProcessID = Main.GetCliProcess(before);

                if (this.HbProcess != null)
                    this.ProcessHandle = this.HbProcess.MainWindowHandle; // Set the process Handle

                // Set the process Priority
                Process hbCliProcess = null;
                if (this.ProcessID != -1)
                    hbCliProcess = Process.GetProcessById(this.ProcessID);

                if (hbCliProcess != null)
                    switch (Settings.Default.processPriority)
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
                MessageBox.Show(
                    "It would appear that HandBrakeCLI has not started correctly. You should take a look at the Activity log as it may indicate the reason why.\n\nDetailed Error Information: error occured in runCli()\n\n" +
                    exc, 
                    "Error", 
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Function to run the CLI directly rather than via CMD
        /// TODO: Code to handle the Log data has yet to be written.
        /// TODO: Code to handle the % / ETA info has to be written.
        /// </summary>
        /// <param name="query">
        /// The query.
        /// </param>
        protected void DirectRun(string query)
        {
            try
            {
                if (this.EncodeStarted != null)
                    this.EncodeStarted(this, new EventArgs());

                this.IsEncoding = true;

                // Setup the job
                string handbrakeCLIPath = Path.Combine(Environment.CurrentDirectory, "HandBrakeCLI.exe");
                var hbProc = new Process
                                 {
                                     StartInfo =
                                         {
                                             FileName = handbrakeCLIPath, 
                                             Arguments = query, 
                                             UseShellExecute = false, 
                                             RedirectStandardOutput = true, 
                                             RedirectStandardError = true, 
                                             RedirectStandardInput = true, 
                                             CreateNoWindow = false, 
                                             WindowStyle = ProcessWindowStyle.Minimized
                                         }
                                 };

                // Setup the redirects
                hbProc.ErrorDataReceived += new DataReceivedEventHandler(HbProcErrorDataReceived);
                hbProc.OutputDataReceived += new DataReceivedEventHandler(HbProcOutputDataReceived);

                // Start the process
                hbProc.Start();

                // Set the Process Priority
                switch (Settings.Default.processPriority)
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

                // Set the class items
                this.HbProcess = hbProc;
                this.ProcessID = hbProc.Id;
                this.ProcessHandle = hbProc.Handle;
            }
            catch (Exception exc)
            {
                Console.WriteLine(exc);
            }
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        protected void Finish()
        {
            if (this.EncodeEnded != null)
                this.EncodeEnded(this, new EventArgs());

            this.IsEncoding = false;

            // Growl
            if (Settings.Default.growlQueue)
                GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");

            // Do something whent he encode ends.
            switch (Settings.Default.CompletionOption)
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
        /// <param name="encJob">
        /// The Encode Job Object
        /// </param>
        protected void AddCLIQueryToLog(Job encJob)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logPath = Path.Combine(logDir, "last_encode_log.txt");

                var reader =
                    new StreamReader(File.Open(logPath, FileMode.Open, FileAccess.Read, FileShare.Read));
                string log = reader.ReadToEnd();
                reader.Close();

                var writer = new StreamWriter(File.Create(logPath));

                writer.Write("### CLI Query: " + encJob.Query + "\n\n");
                writer.Write("### User Query: " + encJob.CustomQuery + "\n\n");
                writer.Write("#########################################\n\n");
                writer.WriteLine(log);
                writer.Flush();
                writer.Close();
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Save a copy of the log to the users desired location or a default location
        /// if this feature is enabled in options.
        /// </summary>
        /// <param name="destination">
        /// The Destination File Path
        /// </param>
        protected void CopyLog(string destination)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string tempLogFile = Path.Combine(logDir, "last_encode_log.txt");

                string encodeDestinationPath = Path.GetDirectoryName(destination);
                string destinationFile = Path.GetFileName(destination);
                string encodeLogFile = destinationFile + " " +
                                       DateTime.Now.ToString().Replace("/", "-").Replace(":", "-") + ".txt";

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                    Directory.CreateDirectory(logDir);

                // Copy the Log to HandBrakes log folder in the users applciation data folder.
                File.Copy(tempLogFile, Path.Combine(logDir, encodeLogFile));

                // Save a copy of the log file in the same location as the enocde.
                if (Settings.Default.saveLogWithVideo)
                    File.Copy(tempLogFile, Path.Combine(encodeDestinationPath, encodeLogFile));

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(Settings.Default.saveLogPath))
                    if (Settings.Default.saveLogPath != String.Empty && Settings.Default.saveLogToSpecifiedPath)
                        File.Copy(tempLogFile, Path.Combine(Settings.Default.saveLogPath, encodeLogFile));
            }
            catch (Exception exc)
            {
                MessageBox.Show(
                    "Something went a bit wrong trying to copy your log file.\nError Information:\n\n" + exc, 
                    "Error", 
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Recieve the Standard Error information and process it
        /// </summary>
        /// <param name="sender">
        /// The Sender Object
        /// </param>
        /// <param name="e">
        /// DataReceived EventArgs
        /// </param>
        private static void HbProcErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            // TODO: Recieve the Log data and process it
            throw new NotImplementedException();
        }

        /// <summary>
        /// Standard Input Data Recieved from the CLI
        /// </summary>
        /// <param name="sender">
        /// The Sender Object
        /// </param>
        /// <param name="e">
        /// DataReceived EventArgs
        /// </param>
        private static void HbProcOutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            // TODO: Recieve the %, ETA, FPS etc and process it
            throw new NotImplementedException();
        }
    }
}