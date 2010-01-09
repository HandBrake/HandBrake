/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.ComponentModel;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;
using Handbrake.Functions;
using Timer = System.Threading.Timer;

namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {
        private delegate void SetTextCallback(StringBuilder text);
        private delegate void SetTextClearCallback();
        private int Position;
        private string LastMode;
        private string CurrentMode;
        private Timer WindowTimer;
 
        public frmActivityWindow(string mode)
        {
            InitializeComponent();

            Position = 0;
            if (mode == "scan")
                SetScanMode();
            else
                SetEncodeMode();
        }

        private void NewActivityWindow_Load(object sender, EventArgs e)
        {
            WindowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
        }

        private void LogMonitor(object n)
        {
            if (SetLogFile != LastMode) Reset();

            // Perform the window update
            switch (SetLogFile)
            {
                case "last_scan_log.txt":
                    AppendWindowText(ReadFile("last_scan_log.txt"));
                    LastMode = "last_scan_log.txt";
                    break;
                case "last_encode_log.txt":
                    AppendWindowText(ReadFile("last_encode_log.txt"));
                    LastMode = "last_encode_log.txt";
                    break;
            }
        }
        private StringBuilder ReadFile(string file)
        {
            StringBuilder appendText = new StringBuilder();
            lock (this)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, file);
                string logFile2 = Path.Combine(logDir, "tmp_appReadable_log.txt");

                try
                {
                    // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                    if (File.Exists(logFile2))
                        File.Delete(logFile2);

                    // Copy the log file.
                    if (File.Exists(logFile))
                        File.Copy(logFile, logFile2, true);
                    else
                    {
                        appendText.AppendFormat("Waiting for the log file to be generated ...\n");
                        Position = 0;
                        ClearWindowText();
                        PrintLogHeader();
                        return appendText;
                    }

                    // Start the Reader
                    // Only use text which continues on from the last read line
                    StreamReader sr = new StreamReader(logFile2);
                    string line;
                    int i = 1;
                    while ((line = sr.ReadLine()) != null)
                    {
                        if (i > Position)
                        {
                            appendText.AppendLine(line);
                            Position++;
                        }
                        i++;
                    }
                    sr.Close();
                    sr.Dispose();

                }
                catch (Exception exc)
                {
                    Reset();
                    appendText = new StringBuilder();
                    appendText.AppendFormat(
                        "\nThe Log file is currently in use. Waiting for the log file to become accessible ...\n");
                }
            }
            return appendText;
        }
        private void AppendWindowText(StringBuilder text)
        {
            try
            {
                if (IsHandleCreated)
                {
                    if (rtf_actLog.InvokeRequired)
                    {
                        IAsyncResult invoked = BeginInvoke(new SetTextCallback(AppendWindowText), new object[] { text });
                        EndInvoke(invoked);
                    }
                    else
                        lock (rtf_actLog)
                            rtf_actLog.AppendText(text.ToString());
                }
            }
            catch (Exception)
            {
                return;
            }
        }
        private void ClearWindowText()
        {
            try
            {
                if (IsHandleCreated)
                {
                    if (rtf_actLog.InvokeRequired)
                    {
                        IAsyncResult invoked = BeginInvoke(new SetTextClearCallback(ClearWindowText));
                        EndInvoke(invoked);
                    }
                    else
                        lock(rtf_actLog)
                            rtf_actLog.Clear();
                }
            }
            catch (Exception)
            {
                return;
            }
        }
        private void PrintLogHeader()
        {
            try
            {
                if (IsHandleCreated)
                {
                    if (rtf_actLog.InvokeRequired)
                    {
                        IAsyncResult invoked = BeginInvoke(new SetTextClearCallback(PrintLogHeader));
                        EndInvoke(invoked);
                    }
                    else
                    {
                        lock (rtf_actLog)
                        {
                            // Print the log header. This function will be re-implimented later. Do not delete.
                            rtf_actLog.AppendText(String.Format("### Windows GUI {1} {0} \n",
                                                                Properties.Settings.Default.hb_build,
                                                                Properties.Settings.Default.hb_version));
                            rtf_actLog.AppendText(String.Format("### Running: {0} \n###\n", Environment.OSVersion));
                            rtf_actLog.AppendText(String.Format("### CPU: {0} \n", SystemInfo.GetCpuCount));
                            rtf_actLog.AppendText(String.Format("### Ram: {0} MB \n", SystemInfo.TotalPhysicalMemory));
                            rtf_actLog.AppendText(String.Format("### Screen: {0}x{1} \n",
                                                                SystemInfo.ScreenBounds.Bounds.Width,
                                                                SystemInfo.ScreenBounds.Bounds.Height));
                            rtf_actLog.AppendText(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
                            rtf_actLog.AppendText(String.Format("### Install Dir: {0} \n", Application.StartupPath));
                            rtf_actLog.AppendText(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
                            rtf_actLog.AppendText("#########################################\n\n");
                        }
                    }
                }
            }
            catch (Exception)
            {
                return;
            }

        }
        private void Reset()
        {
            if (WindowTimer != null)
                WindowTimer.Dispose();
            Position = 0;
            ClearWindowText();
            PrintLogHeader(); 
            WindowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
        }

        #region Public

        public string SetLogFile
        {
            get { return string.IsNullOrEmpty(CurrentMode) ? "" : CurrentMode; }
            set { CurrentMode = value; }
        }
        public void SetScanMode()
        {
            Reset();
            SetLogFile = "last_scan_log.txt";
            this.Text = "Activity Window (Scan Log)";
        }
        public void SetEncodeMode()
        {
            Reset();
            SetLogFile = "last_encode_log.txt";
            this.Text = "Activity Window (Enocde Log)";
        }

        #endregion

        #region User Interface
        private void mnu_copy_log_Click(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_actLog.SelectedText != "" ? rtf_actLog.SelectedText : rtf_actLog.Text, true);
        }
        private void mnu_openLogFolder_Click(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            System.Diagnostics.Process prc = new System.Diagnostics.Process
                                                 {
                                                     StartInfo =
                                                         {
                                                             FileName = windir + @"\explorer.exe",
                                                             Arguments = logDir
                                                         }
                                                 };
            prc.Start();
        }
        private void btn_copy_Click(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_actLog.SelectedText != "" ? rtf_actLog.SelectedText : rtf_actLog.Text, true);
        }
        private void btn_scan_log_Click(object sender, EventArgs e)
        {
            SetScanMode();
        }
        private void btn_encode_log_Click(object sender, EventArgs e)
        {
            SetEncodeMode();
        }
        #endregion

        protected override void OnClosing(CancelEventArgs e)
        {
            WindowTimer.Dispose();
            e.Cancel = true;
            this.Dispose();
            base.OnClosing(e);
        }
    }
}