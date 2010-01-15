using System;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using Handbrake.EncodeQueue;
using Handbrake.Functions;
using QTOControlLib;
using QTOLibrary;

namespace Handbrake
{
    public partial class frmPreview : Form
    {
        readonly QueryGenerator HbCommonFunc = new QueryGenerator();
        readonly Queue Process = new Queue();
        private delegate void UpdateUIHandler();
        String CurrentlyPlaying = "";
        readonly frmMain MainWindow;
        private Thread Player;
        private readonly Boolean NoQT;

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
        }

        #region Encode Sample
        private void btn_playVLC_Click(object sender, EventArgs e)
        {
            lbl_status.Visible = true;
            try
            {
                if (!NoQT)
                    QTControl.URL = "";

                if (File.Exists(CurrentlyPlaying))
                    File.Delete(CurrentlyPlaying);
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            btn_playQT.Enabled = false;
            btn_playVLC.Enabled = false;
            lbl_status.Text = "Encoding Sample for (VLC) ...";
            int duration;
            int.TryParse(cb_duration.Text, out duration);
            String query = HbCommonFunc.GenerateCLIQuery(MainWindow, 3, duration, cb_preview.Text);
            ThreadPool.QueueUserWorkItem(ProcMonitor, query);
        }
        private void btn_playQT_Click(object sender, EventArgs e)
        {
            if (NoQT)
            {
                MessageBox.Show(this, "It would appear QuickTime 7 is not installed or not accessible. Please (re)install QuickTime.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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
                lbl_status.Visible = true;
                try
                {
                    QTControl.URL = "";
                    if (File.Exists(CurrentlyPlaying))
                        File.Delete(CurrentlyPlaying);
                }
                catch (Exception)
                {
                    MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                btn_playQT.Enabled = false;
                btn_playVLC.Enabled = false;
                lbl_status.Text = "Encoding Sample for (QT) ...";
                int duration;
                int.TryParse(cb_duration.Text, out duration);
                String query = HbCommonFunc.GenerateCLIQuery(MainWindow, 3, duration, cb_preview.Text);

                ThreadPool.QueueUserWorkItem(ProcMonitor, query);
            }
        }
        private void ProcMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (Process.HbProcess != null)
                MessageBox.Show(this, "Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                Process.CreatePreviewSampe((string)state);
                if (Process.HbProcess != null)
                {
                    Process.HbProcess.WaitForExit();
                    Process.HbProcess = null;
                }
                EncodeCompleted();
            }
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
                if (!NoQT)
                    btn_playQT.Enabled = true;
                btn_playVLC.Enabled = true;

                // Decide which Player to use.
                String playerSelection = lbl_status.Text.Contains("QT") ? "QT" : "VLC";

                lbl_status.Text = "Loading Clip ...";

                // Get the sample filename
                if (MainWindow.text_destination.Text != "")
                    CurrentlyPlaying = MainWindow.text_destination.Text.Replace(".mp4", "_sample.mp4").Replace(".m4v", "_sample.m4v").Replace(".mkv", "_sample.mkv"); ;

                // Play back in QT or VLC
                if (playerSelection == "QT")
                    Play();
                else
                    PlayVLC();

                lbl_status.Text = "";
            }
            catch (Exception exc)
            {
                MessageBox.Show(this, "frmPreview.cs EncodeCompleted " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        #endregion

        #region Playback

        /// <summary>
        /// Play the video back in the QuickTime control
        /// </summary>
        private void Play()
        {
            Player = new Thread(OpenMovie) { IsBackground = true };
            Player.Start();
            lbl_status.Visible = false;
        }

        /// <summary>
        /// Play the video back in an external VLC Player
        /// </summary>
        private void PlayVLC()
        {
            // Launch VLC and Play video.
            if (CurrentlyPlaying != "")
            {
                if (File.Exists(CurrentlyPlaying))
                {
                    // Attempt to find VLC if it doesn't exist in the default set location.
                    string vlcPath;
                    
                    if (8 == IntPtr.Size || (!String.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432"))))
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
                    else
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles");
      
                    vlcPath = vlcPath != null ? vlcPath + @"\VideoLAN\VLC\vlc.exe" : @"C:\Program Files (x86)\VideoLAN\VLC\vlc.exe";
                    
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
                        String args = "\"" + CurrentlyPlaying + "\"";
                        ProcessStartInfo vlc = new ProcessStartInfo(Properties.Settings.Default.VLC_Path, args);
                        System.Diagnostics.Process.Start(vlc);
                        lbl_status.Text = "VLC will now launch.";
                    }

                }
                else
                    MessageBox.Show(this, "Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.", "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            lbl_status.Visible = false;
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
                MessageBox.Show(this, "Unable to open movie:\n\nError Code: " + ex.ErrorCode.ToString("X") + "\nQT Error code : " + qtu.QTErrorFromErrorCode(ex.ErrorCode), "QT", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "Unable to open movie:\n\n" + ex, "QT", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        #endregion
    }
}
