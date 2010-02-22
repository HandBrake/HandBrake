/*  frmActivityWindow.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using Functions;
    using Timer = System.Threading.Timer;

    /// <summary>
    /// The Activity Log Window
    /// </summary>
    public partial class frmActivityWindow : Form
    {
        /// <summary>
        /// The current position in the log file
        /// </summary>
        private int position;

        /// <summary>
        /// The previous mode
        /// </summary>
        private string lastMode;

        /// <summary>
        /// The current mode
        /// </summary>
        private string currentMode;

        /// <summary>
        /// A Timer for this window
        /// </summary>
        private Timer windowTimer;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmActivityWindow"/> class.
        /// </summary>
        /// <param name="mode">
        /// The mode.
        /// </param>
        public frmActivityWindow(string mode)
        {
            InitializeComponent();

            position = 0;
            if (mode == "scan")
                SetScanMode();
            else
                SetEncodeMode();
        }

        /// <summary>
        /// A callback function for updating the ui
        /// </summary>
        /// <param name="text">
        /// The text.
        /// </param>
        private delegate void SetTextCallback(StringBuilder text);

        /// <summary>
        /// Clear text callback
        /// </summary>
        private delegate void SetTextClearCallback();

        // Public

        /// <summary>
        /// Gets or sets SetLogFile.
        /// </summary>
        public string SetLogFile
        {
            get { return string.IsNullOrEmpty(currentMode) ? string.Empty : currentMode; }
            set { currentMode = value; }
        }

        /// <summary>
        /// Set the window to scan mode
        /// </summary>
        public void SetScanMode()
        {
            Reset();
            SetLogFile = "last_scan_log.txt";
            this.Text = "Activity Window (Scan Log)";
        }

        /// <summary>
        /// Set the window to encode mode
        /// </summary>
        public void SetEncodeMode()
        {
            Reset();
            SetLogFile = "last_encode_log.txt";
            this.Text = "Activity Window (Enocde Log)";
        }

        // Logging

        /// <summary>
        /// On Window load, start a new timer
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NewActivityWindow_Load(object sender, EventArgs e)
        {
            windowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
        }

        /// <summary>
        /// Append new text to the window
        /// </summary>
        /// <param name="n">
        /// The n.
        /// </param>
        private void LogMonitor(object n)
        {
            if (SetLogFile != lastMode) Reset();

            // Perform the window update
            switch (SetLogFile)
            {
                case "last_scan_log.txt":
                    AppendWindowText(ReadFile("last_scan_log.txt"));
                    lastMode = "last_scan_log.txt";
                    break;
                case "last_encode_log.txt":
                    AppendWindowText(ReadFile("last_encode_log.txt"));
                    lastMode = "last_encode_log.txt";
                    break;
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        /// <param name="file">
        /// The file.
        /// </param>
        /// <returns>
        /// A string builder containing the log data
        /// </returns>
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
                        position = 0;
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
                        if (i > position)
                        {
                            appendText.AppendLine(line);
                            position++;
                        }
                        i++;
                    }
                    sr.Close();
                    sr.Dispose();
                }
                catch (Exception)
                {
                    Reset();
                    appendText = new StringBuilder();
                    appendText.AppendLine("\nThe Log file is currently in use. Waiting for the log file to become accessible ...\n");
                }
            }
            return appendText;
        }

        /// <summary>
        /// Append text to the RTF box
        /// </summary>
        /// <param name="text">
        /// The text.
        /// </param>
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

        /// <summary>
        /// Clear the contents of the log window
        /// </summary>
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
                        lock (rtf_actLog)
                            rtf_actLog.Clear();
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Display the log header
        /// </summary>
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
                            StringBuilder header = new StringBuilder();

                            header.AppendLine(String.Format("### Windows GUI {1} {0} \n", Properties.Settings.Default.hb_build, Properties.Settings.Default.hb_version));
                            header.AppendLine(String.Format("### Running: {0} \n###\n", Environment.OSVersion));
                            header.AppendLine(String.Format("### CPU: {0} \n", SystemInfo.GetCpuCount));
                            header.AppendLine(String.Format("### Ram: {0} MB \n", SystemInfo.TotalPhysicalMemory));
                            header.AppendLine(String.Format("### Screen: {0}x{1} \n", SystemInfo.ScreenBounds.Bounds.Width, SystemInfo.ScreenBounds.Bounds.Height));
                            header.AppendLine(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
                            header.AppendLine(String.Format("### Install Dir: {0} \n", Application.StartupPath));
                            header.AppendLine(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
                            header.AppendLine("#########################################\n\n");

                            rtf_actLog.AppendText(header.ToString());
                        }
                    }
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Reset Everything
        /// </summary>
        private void Reset()
        {
            if (windowTimer != null)
                windowTimer.Dispose();
            position = 0;
            ClearWindowText();
            PrintLogHeader();
            windowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
        }

        // Menus and Buttons

        /// <summary>
        /// Copy log to clipboard
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuCopyLogClick(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_actLog.SelectedText != string.Empty ? rtf_actLog.SelectedText : rtf_actLog.Text, true);
        }

        /// <summary>
        /// Open the log folder
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuOpenLogFolderClick(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process
                              {
                                  StartInfo =
                                      {
                                          FileName = windir + @"\explorer.exe",
                                          Arguments = logDir
                                      }
                              };
            prc.Start();
        }

        /// <summary>
        /// Copy the log
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnCopyClick(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_actLog.SelectedText != string.Empty ? rtf_actLog.SelectedText : rtf_actLog.Text, true);
        }

        /// <summary>
        /// Set scan mode
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnScanLogClick(object sender, EventArgs e)
        {
            SetScanMode();
        }

        /// <summary>
        /// Set the encode mode
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnEncodeLogClick(object sender, EventArgs e)
        {
            SetEncodeMode();
        }

        // Overrides

        /// <summary>
        /// override onclosing
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnClosing(CancelEventArgs e)
        {
            windowTimer.Dispose();
            e.Cancel = true;
            this.Dispose();
            base.OnClosing(e);
        }
    }
}