/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.ComponentModel;
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
        private delegate void setTextCallback(string text);
        private String _readFile;
        private readonly EncodeAndQueueHandler _encodeQueue;
        private int _position;  // Position in the arraylist reached by the current log output in the rtf box.
        private readonly frmMain _mainWin;
        private Boolean _lastUpdate;
        private Boolean fileNotFoundQuickFix;

        public frmActivityWindow(string file, EncodeAndQueueHandler eh, frmMain mw)
        {
            InitializeComponent();

            _encodeQueue = eh;
            _mainWin = mw;

            fileNotFoundQuickFix = false;

            if (file == "last_scan_log.txt")
                SetLogView(true);
            else
                SetLogView(false);

            // Start a new thread which will montior and keep the log window up to date if required/
            try
            {
                Thread monitor = new Thread(AutoUpdate) { IsBackground = true };
                monitor.Start();
            }
            catch (Exception exc)
            {
                MessageBox.Show("startLogThread(): Exception: \n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Set the view which the Log window displays.
        /// Scan = true;
        /// Encode = false;
        /// </summary>
        /// <param name="scan">Boolean. Scan = true</param>
        public void SetLogView(Boolean scan)
        {
            // Reset
            _position = 0;
            rtf_actLog.Text = String.Empty;

            // Print the log header
            rtf_actLog.AppendText(String.Format("### Windows GUI {1} {0} \n", Properties.Settings.Default.hb_build, Properties.Settings.Default.hb_version));
            rtf_actLog.AppendText(String.Format("### Running: {0} \n###\n", Environment.OSVersion));
            rtf_actLog.AppendText(String.Format("### CPU: {0} \n", getCpuCount()));
            rtf_actLog.AppendText(String.Format("### Ram: {0} MB \n", TotalPhysicalMemory()));
            rtf_actLog.AppendText(String.Format("### Screen: {0}x{1} \n", screenBounds().Bounds.Width, screenBounds().Bounds.Height));
            rtf_actLog.AppendText(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
            rtf_actLog.AppendText(String.Format("### Install Dir: {0} \n", Application.StartupPath));
            rtf_actLog.AppendText(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
            rtf_actLog.AppendText("#########################################\n\n");
            if ((!_encodeQueue.LastEncode.IsEmpty) && _encodeQueue.LastEncode.Query != String.Empty)
            {
                rtf_actLog.AppendText("### CLI Query: " + _encodeQueue.LastEncode.Query + "\n\n");
                rtf_actLog.AppendText("#########################################\n\n");
            }

            // Seutp the log file
            if (scan)
            {
                txt_log.Text = "Scan Log";
                _readFile = "last_scan_log.txt";
            }
            else
            {
                _readFile = "last_encode_log.txt";
                txt_log.Text = "Encode Log";
            }
            _lastUpdate = false;
        }

        private void AutoUpdate(object state)
        {
            try
            {
                _lastUpdate = false;
                UpdateTextFromThread();
                while (true)
                {
                    if (IsHandleCreated)
                    {
                        if (_encodeQueue.isEncoding || _mainWin.isScanning)
                            UpdateTextFromThread();
                        else
                        {
                            // The encode may just have stoped, so, refresh the log one more time before restarting it.
                            if (_lastUpdate == false)
                                UpdateTextFromThread();

                            _lastUpdate = true; // Prevents the log window from being updated when there is no encode going.
                            _position = 0; // There is no encoding, so reset the log position counter to 0 so it can be reused
                        }
                    }
                    Thread.Sleep(1000);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("AutoUpdate(): Exception: \n" + exc);
            }
        }
        private void UpdateTextFromThread()
        {
            try
            {
                String info = ReadFile();
                if (info.Contains("has exited"))
                    info += "\n ############ End of Log ############## \n";

                SetText(info);
            }
            catch (Exception exc)
            {
                MessageBox.Show("UpdateTextFromThread(): Exception: \n" + exc);
            }
        }
        private void SetText(string text)
        {
            try
            {
                if (IsHandleCreated)
                {
                    if (rtf_actLog.InvokeRequired)
                    {
                        IAsyncResult invoked = BeginInvoke(new setTextCallback(SetText), new object[] { text });
                        EndInvoke(invoked);
                    }
                    else
                        rtf_actLog.AppendText(text);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("SetText(): Exception: \n" + exc);
            }
        }
        private String ReadFile()
        {
            String appendText = String.Empty;
            try
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, _readFile);
                string logFile2 = Path.Combine(logDir, "tmp_appReadable_log.txt");

                // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                if (File.Exists(logFile2))
                    File.Delete(logFile2);

                // Copy the log file.
                if (File.Exists(logFile))
                    File.Copy(logFile, logFile2, true);
                else
                {
                    if (fileNotFoundQuickFix)
                        return "";
                    fileNotFoundQuickFix = true;
                    return "\n\n\nERROR: The log file could not be found. \nMaybe you cleared your system's tempory folder or maybe you just havn't run an encode yet. \nTried to find the log file in: " + logFile;
                }
                   
                StreamReader sr = new StreamReader(logFile2);
                string line;
                int i = 1;
                while ((line = sr.ReadLine()) != null)
                {
                    if (i > _position)
                    {
                        appendText += line + Environment.NewLine;
                        _position++;
                    }
                    i++;
                }
                sr.Close();
                sr.Dispose();

                return appendText;
            }
            catch (Exception exc)
            {
                return "Error in ReadFile() \n Unable to read the log file.\n You may have to restart HandBrake. Will try reading the file again in a few seconds... \n  Error Information: \n\n" + exc;
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }

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
            SetLogView(true);
        }
        private void btn_encode_log_Click(object sender, EventArgs e)
        {
            SetLogView(false);
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

    }
}