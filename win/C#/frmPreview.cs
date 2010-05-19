/*  frmPreview.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Windows.Forms;
    using Functions;
    using QTOControlLib;
    using QTOLibrary;
    using Services;
    using Parsing;

    public partial class frmPreview : Form
    {
        private string CurrentlyPlaying = string.Empty;
        private readonly frmMain MainWindow;
        private Thread Player;
        private readonly bool NoQT;
        private readonly Queue Process = new Queue();
        private delegate void UpdateUIHandler();
        private bool playWithVLC;

        public frmPreview(frmMain mw)
        {
            try
            {
                InitializeComponent();
            }
            catch (Exception)
            {
                NoQT = true;
            }
            this.MainWindow = mw;
            cb_preview.SelectedIndex = 0;
            cb_duration.SelectedIndex = 1;

            cb_preview.Items.Clear();
            for (int i = 1; i <= Properties.Settings.Default.previewScanCount; i++)
                cb_preview.Items.Add(i.ToString());
            cb_preview.SelectedIndex = 0;

            Process.EncodeStarted += new EventHandler(Process_EncodeStarted);
        }
        private void Process_EncodeStarted(object sender, EventArgs e)
        {
            Thread encodeMon = new Thread(EncodeMonitorThread);
            encodeMon.Start();
        }

        #region Encode Sample

        private void btn_playVLC_Click(object sender, EventArgs e)
        {
            ProgressBarStatus.Visible = true;
            ProgressBarStatus.Value = 0;
            lbl_encodeStatus.Visible = true;
            playWithVLC = true;

            try
            {
                if (!NoQT)
                    QTControl.URL = string.Empty;

                if (File.Exists(CurrentlyPlaying))
                    File.Delete(CurrentlyPlaying);
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.", 
                                "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            btn_playQT.Enabled = false;
            btn_playVLC.Enabled = false;
            this.Text += " (Encoding)";
            int duration;
            int.TryParse(cb_duration.Text, out duration);
            string query = QueryGenerator.GenerateCliQuery(MainWindow, 3, duration, cb_preview.Text);
            ThreadPool.QueueUserWorkItem(ProcMonitor, query);
        }

        private void btn_playQT_Click(object sender, EventArgs e)
        {
            playWithVLC = false;
            if (NoQT)
            {
                MessageBox.Show(this, 
                                "It would appear QuickTime 7 is not installed or not accessible. Please (re)install QuickTime.", 
                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            if (MainWindow.text_destination.Text.Contains(".mkv"))
            {
                MessageBox.Show(this, 
                                "The QuickTime Control does not support MKV files, It is recommended you use VLC option instead.", 
                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                ProgressBarStatus.Visible = true;
                ProgressBarStatus.Value = 0;
                lbl_encodeStatus.Visible = true;
                try
                {
                    QTControl.URL = string.Empty;
                    if (File.Exists(CurrentlyPlaying))
                        File.Delete(CurrentlyPlaying);
                }
                catch (Exception)
                {
                    MessageBox.Show(this, 
                                    "Unable to delete previous preview file. You may need to restart the application.", 
                                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                btn_playQT.Enabled = false;
                btn_playVLC.Enabled = false;
                this.Text += " (Encoding)";
                int duration;
                int.TryParse(cb_duration.Text, out duration);
                string query = QueryGenerator.GenerateCliQuery(MainWindow, 3, duration, cb_preview.Text);

                ThreadPool.QueueUserWorkItem(ProcMonitor, query);
            }
        }

        private void ProcMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (Process.HbProcess != null)
                MessageBox.Show(this, "Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, 
                                MessageBoxIcon.Warning);
            else
            {
                Process.CreatePreviewSample((string) state);

                if (Process.HbProcess != null)
                {
                    Process.HbProcess.WaitForExit();
                    Process.HbProcess = null;
                }
                EncodeCompleted();
            }
        }

        private void EncodeMonitorThread()
        {
            try
            {
                Parser encode = new Parser(Process.HbProcess.StandardOutput.BaseStream);
                encode.OnEncodeProgress += EncodeOnEncodeProgress;
                while (!encode.EndOfStream)
                    encode.ReadEncodeStatus();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void EncodeOnEncodeProgress(object Sender, int CurrentTask, int TaskCount, float PercentComplete, float CurrentFps, float AverageFps, TimeSpan TimeRemaining)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(
                    new EncodeProgressEventHandler(EncodeOnEncodeProgress),
                    new[] { Sender, CurrentTask, TaskCount, PercentComplete, CurrentFps, AverageFps, TimeRemaining });
                return;
            }
            lbl_encodeStatus.Text = PercentComplete + "%";
            ProgressBarStatus.Value = (int)Math.Round(PercentComplete);
        }


        private void EncodeCompleted()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateUIHandler(EncodeCompleted));
                    return;
                }

                ProgressBarStatus.Visible = false;
                lbl_encodeStatus.Visible = false;

                if (!NoQT)
                    btn_playQT.Enabled = true;
                btn_playVLC.Enabled = true;

                this.Text = this.Text.Replace(" (Encoding)", string.Empty);

                // Get the sample filename
                if (MainWindow.text_destination.Text != string.Empty)
                    CurrentlyPlaying =
                        MainWindow.text_destination.Text.Replace(".mp4", "_sample.mp4").Replace(".m4v", "_sample.m4v").
                            Replace(".mkv", "_sample.mkv");

                // Play back in QT or VLC
                if (!playWithVLC)
                    Play();
                else
                    PlayVLC();
            }
            catch (Exception exc)
            {
                MessageBox.Show(this, "frmPreview.cs EncodeCompleted " + exc, "Error", MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
            }
        }

        #endregion

        #region Playback

        /// <summary>
        /// Play the video back in the QuickTime control
        /// </summary>
        private void Play()
        {
            Player = new Thread(OpenMovie) {IsBackground = true};
            Player.Start();
        }

        /// <summary>
        /// Play the video back in an external VLC Player
        /// </summary>
        private void PlayVLC()
        {
            // Launch VLC and Play video.
            if (CurrentlyPlaying != string.Empty)
            {
                if (File.Exists(CurrentlyPlaying))
                {
                    // Attempt to find VLC if it doesn't exist in the default set location.
                    string vlcPath;

                    if (8 == IntPtr.Size ||
                        (!String.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432"))))
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
                    else
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles");

                    vlcPath = vlcPath != null
                                  ? vlcPath + @"\VideoLAN\VLC\vlc.exe"
                                  : @"C:\Program Files (x86)\VideoLAN\VLC\vlc.exe";

                    if (!File.Exists(Properties.Settings.Default.VLC_Path))
                    {
                        if (File.Exists(vlcPath))
                        {
                            Properties.Settings.Default.VLC_Path = "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe";
                            Properties.Settings.Default.Save(); // Save this new path if it does
                        }
                        else
                        {
                            MessageBox.Show(this, 
                                            "Unable to detect VLC Player. \nPlease make sure VLC is installed and the directory specified in HandBrake's options is correct. (See: \"Tools Menu > Options > Picture Tab\") ", 
                                            "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        }
                    }

                    if (File.Exists(Properties.Settings.Default.VLC_Path))
                    {
                        string args = "\"" + CurrentlyPlaying + "\"";
                        ProcessStartInfo vlc = new ProcessStartInfo(Properties.Settings.Default.VLC_Path, args);
                        System.Diagnostics.Process.Start(vlc);
                    }
                }
                else
                    MessageBox.Show(this, 
                                    "Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.", 
                                    "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        /// <summary>
        /// QT control - Open the file
        /// </summary>
        [STAThread]
        private void OpenMovie()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateUIHandler(OpenMovie));
                    return;
                }
                QTControl.URL = CurrentlyPlaying;
                QTControl.SetSizing(QTSizingModeEnum.qtControlFitsMovie, true);
                QTControl.URL = CurrentlyPlaying;
                QTControl.Show();

                this.ClientSize = QTControl.Size;
                this.Height += 25;
            }
            catch (COMException ex)
            {
                QTUtils qtu = new QTUtils();
                MessageBox.Show(this, 
                                "Unable to open movie:\n\nError Code: " + ex.ErrorCode.ToString("X") +
                                "\nQT Error code : " + qtu.QTErrorFromErrorCode(ex.ErrorCode), "QT", 
                                MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "Unable to open movie:\n\n" + ex, "QT", MessageBoxButtons.OK, 
                                MessageBoxIcon.Warning);
            }
        }

        #endregion

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            Process.EncodeStarted -= Process_EncodeStarted;
            base.OnClosing(e);
        }
    }
}