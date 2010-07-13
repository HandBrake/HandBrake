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

    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using QTOControlLib;
    using QTOLibrary;

    /// <summary>
    /// The Preview Window
    /// </summary>
    public partial class frmPreview : Form
    {
        #region Private Variables
        /// <summary>
        /// The Main Window
        /// </summary>
        private readonly frmMain mainWindow;

        /// <summary>
        /// True if QT is not installed
        /// </summary>
        private readonly bool noQT;

        /// <summary>
        /// The encode queue
        /// </summary>
        private readonly IQueue encodeQueue = new Queue();

        /// <summary>
        /// What is currently playing
        /// </summary>
        private string currentlyPlaying = string.Empty;

        /// <summary>
        /// Play With VLC tracker
        /// </summary>
        private bool playWithVLC;

        /// <summary>
        /// A Thread for the video player
        /// </summary>
        private Thread player;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmPreview"/> class.
        /// </summary>
        /// <param name="mw">
        /// The mw.
        /// </param>
        public frmPreview(frmMain mw)
        {
            try
            {
                InitializeComponent();
            }
            catch (Exception)
            {
                this.noQT = true;
            }
            this.mainWindow = mw;
            cb_preview.SelectedIndex = 0;
            cb_duration.SelectedIndex = 1;

            cb_preview.Items.Clear();
            for (int i = 1; i <= Properties.Settings.Default.previewScanCount; i++)
                cb_preview.Items.Add(i.ToString());
            cb_preview.SelectedIndex = 0;

            encodeQueue.EncodeStarted += this.EncodeQueue_EncodeStarted;
            encodeQueue.EncodeEnded += this.EncodeQueue_EncodeEnded;
        }

        /// <summary>
        /// Update UI Delegate
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private delegate void UpdateUIHandler(object sender, EventArgs e);

        /// <summary>
        /// The Open Movie Handler
        /// </summary>
        private delegate void OpenMovieHandler();

        #endregion

        /// <summary>
        /// The encode has started
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueue_EncodeStarted(object sender, EventArgs e)
        {
            encodeQueue.EncodeStatusChanged += this.EncodeQueue_EncodeStatusChanged;
        }

        /// <summary>
        /// The Enocde has ended
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueue_EncodeEnded(object sender, EventArgs e)
        {
            encodeQueue.EncodeStatusChanged -= this.EncodeQueue_EncodeStatusChanged;

            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUIHandler(EncodeQueue_EncodeEnded), new[] { sender, e });
                    return;
                }

                ProgressBarStatus.Visible = false;
                lbl_encodeStatus.Visible = false;

                if (!this.noQT)
                    btn_playQT.Enabled = true;
                btn_playVLC.Enabled = true;

                this.Text = this.Text.Replace(" (Encoding)", string.Empty);

                // Get the sample filename
                if (this.mainWindow.text_destination.Text != string.Empty)
                    this.currentlyPlaying =
                        this.mainWindow.text_destination.Text.Replace(".mp4", "_sample.mp4").Replace(".m4v", "_sample.m4v").
                            Replace(".mkv", "_sample.mkv");

                // Play back in QT or VLC
                if (!playWithVLC)
                    Play();
                else
                    PlayVLC();
            }
            catch (Exception exc)
            {
                Main.ShowExceptiowWindow("An Unexpected error has occured", exc.ToString());
            }
        }

        /// <summary>
        /// Encode status has changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueue_EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EncodeProgressEventArgs e)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Encode.EncodeProgessStatus(this.EncodeQueue_EncodeStatusChanged), new[] { sender, e });
                return;
            }

            lbl_encodeStatus.Text = e.PercentComplete + "%";
            ProgressBarStatus.Value = (int)Math.Round(e.PercentComplete);
        }

        #region Encode Sample

        /// <summary>
        /// Play with VLC
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void PlayVLC_Click(object sender, EventArgs e)
        {
            ProgressBarStatus.Visible = true;
            ProgressBarStatus.Value = 0;
            lbl_encodeStatus.Visible = true;
            playWithVLC = true;

            try
            {
                if (!this.noQT)
                    QTControl.URL = string.Empty;

                if (File.Exists(this.currentlyPlaying))
                    File.Delete(this.currentlyPlaying);
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
            string query = QueryGenerator.GeneratePreviewQuery(this.mainWindow, duration, cb_preview.Text);
            ThreadPool.QueueUserWorkItem(this.CreatePreview, query);
        }

        /// <summary>
        /// Encode and Play with QT
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void PlayQT_Click(object sender, EventArgs e)
        {
            playWithVLC = false;
            if (this.noQT)
            {
                MessageBox.Show(this,
                                "It would appear QuickTime 7 is not installed or not accessible. Please (re)install QuickTime.",
                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            if (this.mainWindow.text_destination.Text.Contains(".mkv"))
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
                    if (File.Exists(this.currentlyPlaying))
                        File.Delete(this.currentlyPlaying);
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
                string query = QueryGenerator.GeneratePreviewQuery(this.mainWindow, duration, cb_preview.Text);

                ThreadPool.QueueUserWorkItem(this.CreatePreview, query);
            }
        }

        /// <summary>
        /// Create the Preview.
        /// </summary>
        /// <param name="state">
        /// The state.
        /// </param>
        private void CreatePreview(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (encodeQueue.IsEncoding)
            {
                MessageBox.Show(
                    this,
                    "Handbrake is already encoding a video!",
                    "Status",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);

                return;
            }

            encodeQueue.CreatePreviewSample((string)state);
        }

        #endregion

        #region Playback

        /// <summary>
        /// Play the video back in the QuickTime control
        /// </summary>
        private void Play()
        {
            this.player = new Thread(OpenMovie) { IsBackground = true };
            this.player.Start();
        }

        /// <summary>
        /// Play the video back in an external VLC Player
        /// </summary>
        private void PlayVLC()
        {
            // Launch VLC and Play video.
            if (this.currentlyPlaying != string.Empty)
            {
                if (File.Exists(this.currentlyPlaying))
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
                        string args = "\"" + this.currentlyPlaying + "\"";
                        ProcessStartInfo vlc = new ProcessStartInfo(Properties.Settings.Default.VLC_Path, args);
                        Process.Start(vlc);
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
                    BeginInvoke(new OpenMovieHandler(OpenMovie));
                    return;
                }
                QTControl.URL = this.currentlyPlaying;
                QTControl.SetSizing(QTSizingModeEnum.qtControlFitsMovie, true);
                QTControl.URL = this.currentlyPlaying;
                QTControl.Show();

                this.ClientSize = QTControl.Size;
                this.Height += 25;
            }
            catch (COMException ex)
            {
                QTUtils qtu = new QTUtils();
                Main.ShowExceptiowWindow("Unable to open movie.", ex + Environment.NewLine + qtu.QTErrorFromErrorCode(ex.ErrorCode));
            }
            catch (Exception ex)
            {
                Main.ShowExceptiowWindow("Unable to open movie.", ex.ToString());
            }
        }

        #endregion

        /// <summary>
        /// Remove any subscribed events then close.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            encodeQueue.EncodeStarted -= this.EncodeQueue_EncodeStarted;
            encodeQueue.EncodeEnded -= this.EncodeQueue_EncodeEnded;
            base.OnClosing(e);
        }
    }
}