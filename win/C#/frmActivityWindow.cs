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
    using Model;
    using Services;
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
        /// A Timer for this window
        /// </summary>
        private Timer windowTimer;

        /// <summary>
        /// The Encode Object
        /// </summary>
        private Encode encode;

        /// <summary>
        /// The Scan Object
        /// </summary>
        private Scan scan;

        /// <summary>
        /// The Type of log that the window is currently dealing with
        /// </summary>
        private ActivityLogMode mode;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmActivityWindow"/> class.
        /// </summary>
        /// <param name="mode">
        /// The mode.
        /// </param>
        /// <param name="encode">
        /// The encode.
        /// </param>
        /// <param name="scan">
        /// The scan.
        /// </param>
        public frmActivityWindow(ActivityLogMode mode, Encode encode, Scan scan)
        {
            InitializeComponent();

            this.encode = encode;
            this.scan = scan;
            this.mode = mode;
            this.position = 0;
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
        /// Set the window to scan mode
        /// </summary>
        /// <param name="setMode">
        /// The set Mode.
        /// </param>
        public void SetMode(ActivityLogMode setMode)
        {
            Reset();
            this.mode = setMode;
            this.Text = mode == ActivityLogMode.Scan ? "Activity Window (Scan Log)" : "Activity Window (Enocde Log)";
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
            AppendWindowText(GetLog());
        }

        /// <summary>
        /// New Code for getting the Activity log from the Services rather than reading a file.
        /// </summary>
        /// <returns>
        /// The StringBuilder containing a log
        /// </returns>
        private StringBuilder GetLog()
        {
            StringBuilder appendText = new StringBuilder();
            
            if (this.mode == ActivityLogMode.Scan)
            {
                if (scan == null || scan.ActivityLog == string.Empty)
                {
                    appendText.AppendFormat("Waiting for the log to be generated ...\n");
                    position = 0;
                    ClearWindowText();
                    PrintLogHeader();
                    return appendText;
                }

                using (StringReader reader = new StringReader(scan.ActivityLog))
                {
                    LogReader(reader, appendText);
                }
            }
            else
            {
                if (encode == null || encode.ActivityLog == string.Empty)
                {
                    appendText.AppendFormat("Waiting for the log to be generated ...\n");
                    position = 0;
                    ClearWindowText();
                    PrintLogHeader();
                    return appendText;
                }

                using (StringReader reader = new StringReader(encode.ActivityLog))
                {
                    LogReader(reader, appendText);
                }
            }
            return appendText;
        }

        /// <summary>
        /// Reads the log data from a Scan or Encode object
        /// </summary>
        /// <param name="reader">
        /// The reader.
        /// </param>
        /// <param name="appendText">
        /// The append text.
        /// </param>
        private void LogReader(StringReader reader, StringBuilder appendText)
        {
            string line;
            int i = 1;
            while ((line = reader.ReadLine()) != null)
            {
                if (i > position)
                {
                    appendText.AppendLine(line);
                    position++;
                }
                i++;
            }
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

                            header.Append(String.Format("### Windows GUI {1} {0} \n", Properties.Settings.Default.hb_build, Properties.Settings.Default.hb_version));
                            header.Append(String.Format("### Running: {0} \n###\n", Environment.OSVersion));
                            header.Append(String.Format("### CPU: {0} \n", SystemInfo.GetCpuCount));
                            header.Append(String.Format("### Ram: {0} MB \n", SystemInfo.TotalPhysicalMemory));
                            header.Append(String.Format("### Screen: {0}x{1} \n", SystemInfo.ScreenBounds.Bounds.Width, SystemInfo.ScreenBounds.Bounds.Height));
                            header.Append(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
                            header.Append(String.Format("### Install Dir: {0} \n", Application.StartupPath));
                            header.Append(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
                            header.Append("#########################################\n\n");

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
            SetMode(ActivityLogMode.Scan);
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
            SetMode(ActivityLogMode.Encode);
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