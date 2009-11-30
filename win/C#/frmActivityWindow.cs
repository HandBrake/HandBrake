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
using Handbrake.EncodeQueue;
using Handbrake.Functions;
using Microsoft.Win32;


namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {
        private delegate void setTextCallback(StringBuilder text);
        private delegate void setTextClearCallback();
        private static int _position;
        private static string _lastMode;
        private static string _currentMode;
        private Thread monitor;
        private Boolean kilLThread;

        public frmActivityWindow(string mode)
        {
            if (mode == "scan")
                SetScanMode();
            else
                SetEncodeMode();

            InitializeComponent();
        }
        private void NewActivityWindow_Load(object sender, EventArgs e)
        {
            monitor = new Thread(LogMonitor);
            _position = 0;
            kilLThread = false;

            try
            {
                monitor.Start();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private void LogMonitor()
        {
            while (true)
            {
                if (!IsHandleCreated || kilLThread) // break out the thread if the window has been disposed.
                    break;

                // Perform a reset if require.
                // If we have switched to a different log file, we want to start from the beginning.
                if (SetLogFile != _lastMode)
                {
                    _position = 0;
                    ClearWindowText();
                    PrintLogHeader();
                }

                // Perform the window update
                switch (SetLogFile)
                {
                    case "last_scan_log.txt":
                        AppendWindowText(ReadFile("last_scan_log.txt"));
                        _lastMode = "last_scan_log.txt";
                        break;
                    case "last_encode_log.txt":
                        AppendWindowText(ReadFile("last_encode_log.txt"));
                        _lastMode = "last_encode_log.txt";
                        break;
                }

                try
                {
                    Thread.Sleep(1000);
                }
                catch (ThreadInterruptedException)
                {
                    // Do Nothnig.
                }

            }
        }
        private StringBuilder ReadFile(string file)
        {
            StringBuilder appendText = new StringBuilder();

            // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
            // we'll need to make a copy of it.
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
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
                    _position = 0;
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
                    if (i > _position)
                    {
                        appendText.AppendLine(line);
                        _position++;
                    }
                    i++;
                }
                sr.Close();
                sr.Dispose();

            }
            catch (Exception exc)
            {
                appendText.AppendFormat("\nERROR: The Log file could not be read. You may need to restart HandBrake! " + exc);
                _position = 0;
                ClearWindowText();
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
                        IAsyncResult invoked = BeginInvoke(new setTextCallback(AppendWindowText), new object[] { text });
                        EndInvoke(invoked);
                    }
                    else
                        rtf_actLog.AppendText(text.ToString());
                }
            } catch(ThreadInterruptedException)
            {
                // Do Nothing
            }
            catch (Exception exc)
            {
                MessageBox.Show("SetWindowText(): Exception: \n" + exc);
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
                        IAsyncResult invoked = BeginInvoke(new setTextClearCallback(ClearWindowText));
                        EndInvoke(invoked);
                    }
                    else
                        rtf_actLog.Clear();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("ClearWindowText(): Exception: \n" + exc);
            }
        }
        public void PrintLogHeader()
        {
            try
            {
                if (IsHandleCreated)
                {
                    if (rtf_actLog.InvokeRequired)
                    {
                        IAsyncResult invoked = BeginInvoke(new setTextClearCallback(PrintLogHeader));
                        EndInvoke(invoked);
                    }
                    else
                    {
                        // Print the log header. This function will be re-implimented later. Do not delete.
                        rtf_actLog.AppendText(String.Format("### Windows GUI {1} {0} \n", Properties.Settings.Default.hb_build, Properties.Settings.Default.hb_version));
                        rtf_actLog.AppendText(String.Format("### Running: {0} \n###\n", Environment.OSVersion));
                        rtf_actLog.AppendText(String.Format("### CPU: {0} \n", getCpuCount()));
                        rtf_actLog.AppendText(String.Format("### Ram: {0} MB \n", TotalPhysicalMemory()));
                        rtf_actLog.AppendText(String.Format("### Screen: {0}x{1} \n", screenBounds().Bounds.Width, screenBounds().Bounds.Height));
                        rtf_actLog.AppendText(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
                        rtf_actLog.AppendText(String.Format("### Install Dir: {0} \n", Application.StartupPath));
                        rtf_actLog.AppendText(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
                        rtf_actLog.AppendText("#########################################\n\n");
                    }
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("PrintLogHeader(): Exception: \n" + exc);
            }

        }

        #region Public

        public string SetLogFile
        {
            get { return string.IsNullOrEmpty(_currentMode) ? "" : _currentMode; }
            set { _currentMode = value; }
        }
        public void SetScanMode()
        {
            SetLogFile = "last_scan_log.txt";
            this.Text = "Activity Window (Scan Log)";
        }
        public void SetEncodeMode()
        {
            SetLogFile = "last_encode_log.txt";
            this.Text = "Activity Window (Enocde Log)";
        }

        #endregion

        #region User Interface
        private void mnu_copy_log_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.SelectedText != "")
                Clipboard.SetDataObject(rtf_actLog.SelectedText, true);
            else
                Clipboard.SetDataObject(rtf_actLog.Text, true);
        }
        private void mnu_openLogFolder_Click(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            System.Diagnostics.Process prc = new System.Diagnostics.Process();
            prc.StartInfo.FileName = windir + @"\explorer.exe";
            prc.StartInfo.Arguments = logDir;
            prc.Start();
        }
        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.SelectedText != "")
                Clipboard.SetDataObject(rtf_actLog.SelectedText, true);
            else
                Clipboard.SetDataObject(rtf_actLog.Text, true);
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

        #region System Information
        /// <summary>
        /// Returns the total physical ram in a system
        /// </summary>
        /// <returns></returns>
        public uint TotalPhysicalMemory()
        {
            Win32.MEMORYSTATUS memStatus = new Win32.MEMORYSTATUS();
            Win32.GlobalMemoryStatus(ref memStatus);

            uint MemoryInfo = memStatus.dwTotalPhys;
            MemoryInfo = MemoryInfo / 1024 / 1024;

            return MemoryInfo;
        }

        /// <summary>
        /// Get the number of CPU Cores
        /// </summary>
        /// <returns>Object</returns>
        public Object getCpuCount()
        {
            RegistryKey RegKey = Registry.LocalMachine;
            RegKey = RegKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
            return RegKey.GetValue("ProcessorNameString");
        }

        /// <summary>
        /// Get the System screen size information.
        /// </summary>
        /// <returns>System.Windows.Forms.Scree</returns>
        public Screen screenBounds()
        {
            return Screen.PrimaryScreen;
        }
        #endregion

        protected override void OnClosing(CancelEventArgs e)
        {
            kilLThread = true;
            monitor.Interrupt();
            monitor.Join();
            e.Cancel = true;
            this.Dispose();
            base.OnClosing(e);
        }
    }
}