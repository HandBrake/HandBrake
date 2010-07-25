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

    using HandBrake.ApplicationServices.Services.Interfaces;

    using Model;
    using Timer = System.Threading.Timer;

    /// <summary>
    /// The Activity Log Window
    /// </summary>
    public partial class frmActivityWindow : Form
    {
        /* Private Variables */

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
        private IQueue encode;

        /// <summary>
        /// The Scan Object
        /// </summary>
        private IScan scan;

        /// <summary>
        /// The Type of log that the window is currently dealing with
        /// </summary>
        private ActivityLogMode mode;

        /* Constructor */

        /// <summary>
        /// Initializes a new instance of the <see cref="frmActivityWindow"/> class.
        /// </summary>
        /// <param name="encode">
        /// The encode.
        /// </param>
        /// <param name="scan">
        /// The scan.
        /// </param>
        public frmActivityWindow(IQueue encode, IScan scan)
        {
            InitializeComponent();

            this.encode = encode;
            this.scan = scan;
            this.position = 0;

            // Listen for Scan and Encode Starting Events
            scan.ScanStared += scan_ScanStared;
            encode.EncodeStarted += encode_EncodeStarted;
        }

        /* Delegates */

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

        /// <summary>
        /// Set mode callback
        /// </summary>
        /// <param name="setMode">
        /// The set mode.
        /// </param>
        private delegate void SetModeCallback(ActivityLogMode setMode);

        /* Private Methods */

        /// <summary>
        /// Set the window to scan mode
        /// </summary>
        /// <param name="setMode">
        /// The set Mode.
        /// </param>
        private void SetMode(ActivityLogMode setMode)
        {
            if (IsHandleCreated)
            {
                if (rtf_actLog.InvokeRequired)
                {
                    IAsyncResult invoked = BeginInvoke(new SetModeCallback(SetMode), new object[] {setMode});
                    EndInvoke(invoked);
                }
                else
                {
                    Reset();
                    this.mode = setMode;

                    Array values = Enum.GetValues(typeof(ActivityLogMode));
                    Properties.Settings.Default.ActivityWindowLastMode = (int) values.GetValue(Convert.ToInt32(setMode));
                    Properties.Settings.Default.Save();

                    this.Text = mode == ActivityLogMode.Scan
                                    ? "Activity Window (Scan Log)"
                                    : "Activity Window (Encode Log)";

                    if (mode == ActivityLogMode.Scan)
                    {
                        scan.ScanCompleted += stopWindowRefresh;
                        encode.EncodeEnded -= stopWindowRefresh;
                    }
                    else
                    {
                        scan.ScanCompleted -= stopWindowRefresh;
                        encode.EncodeEnded += stopWindowRefresh;
                    }

                    // Start a fresh window timer
                    windowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
                }
            }
        }

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
            ActivityLogMode activitLogMode = (ActivityLogMode) Enum.ToObject(typeof(ActivityLogMode), Properties.Settings.Default.ActivityWindowLastMode);
            SetMode(activitLogMode);        
        }

        /// <summary>
        /// Set the Log window to encode mode when an encode starts.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void encode_EncodeStarted(object sender, EventArgs e)
        {
            SetMode(ActivityLogMode.Encode);
        }

        /// <summary>
        /// Set the log widow to scan mode when a scan starts
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void scan_ScanStared(object sender, EventArgs e)
        {
            SetMode(ActivityLogMode.Scan);
        }

        /// <summary>
        /// Stop refreshing the window when no scanning or encoding is happening.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void stopWindowRefresh(object sender, EventArgs e)
        {
            windowTimer.Dispose();
            Reset();
            LogMonitor(null);
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

                    // Stop the refresh process if log has finished.
                    if (text.ToString().Contains("HandBrake has Exited"))
                    {
                        windowTimer.Dispose();
                    }
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
        /// Reset Everything
        /// </summary>
        private void Reset()
        {
            if (windowTimer != null)
                windowTimer.Dispose();
            position = 0;
            ClearWindowText();
            windowTimer = new Timer(new TimerCallback(LogMonitor), null, 1000, 1000);
        }

        /* Menus and Buttons */

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

        /* Overrides */

        /// <summary>
        /// override onclosing
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnClosing(CancelEventArgs e)
        {
            scan.ScanStared -= scan_ScanStared;
            encode.EncodeStarted -= encode_EncodeStarted;

            scan.ScanCompleted -= stopWindowRefresh;
            encode.EncodeEnded -= stopWindowRefresh;

            windowTimer.Dispose();
            e.Cancel = true;
            this.Dispose();
            base.OnClosing(e);
        }
    }
}