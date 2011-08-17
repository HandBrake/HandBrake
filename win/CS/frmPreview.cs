/*  frmPreview.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using MessageBox = System.Windows.Forms.MessageBox;

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
        /// The encode queue
        /// </summary>
        private readonly IEncode encodeQueue = new Encode();

        /// <summary>
        /// What is currently playing
        /// </summary>
        private string currentlyPlaying = string.Empty;

        /// <summary>
        /// The User Setting Service.
        /// </summary>
        private static readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;

        /// <summary>
        /// Update UI Delegate
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private delegate void UpdateUiHandler(object sender, EventArgs e);

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="frmPreview"/> class.
        /// </summary>
        /// <param name="mw">
        /// The mw.
        /// </param>
        public frmPreview(frmMain mw)
        {
            InitializeComponent();
            this.mainWindow = mw;

            startPoint.SelectedIndex = 0;
            endPoint.SelectedIndex = 1;

            startPoint.Items.Clear();
            for (int i = 1; i <= UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount); i++)
            {
                startPoint.Items.Add(i.ToString());
            }

            startPoint.SelectedIndex = 0;

            encodeQueue.EncodeStarted += this.EncodeQueueEncodeStarted;
            encodeQueue.EncodeCompleted += this.EncodeQueueEncodeEnded;

            defaultPlayer.Checked = UserSettingService.GetUserSetting<bool>(UserSettingConstants.DefaultPlayer);
        }

        #region Event Handlers
        /// <summary>
        /// The encode has started
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueueEncodeStarted(object sender, EventArgs e)
        {
            encodeQueue.EncodeStatusChanged += this.EncodeQueueEncodeStatusChanged;
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
        private void EncodeQueueEncodeEnded(object sender, EventArgs e)
        {
            encodeQueue.EncodeStatusChanged -= this.EncodeQueueEncodeStatusChanged;

            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUiHandler(EncodeQueueEncodeEnded), new[] { sender, e });
                    return;
                }

                btn_play.Enabled = true;
                this.Text = this.Text.Replace(" (Encoding)", string.Empty);
                progressBar.Value = 0;
                lbl_progress.Text = "0.00%";

                // Get the sample filename
                if (this.mainWindow.text_destination.Text != string.Empty)
                    this.currentlyPlaying =
                        this.mainWindow.text_destination.Text.Replace(".mp4", "_sample.mp4").Replace(".m4v", "_sample.m4v").
                            Replace(".mkv", "_sample.mkv");

                this.Play();
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
        private void EncodeQueueEncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new EncodeProgessStatus(this.EncodeQueueEncodeStatusChanged), new[] { sender, e });
                return;
            }

            lbl_progress.Text = e.PercentComplete + "%";
            progressBar.Value = (int)Math.Round(e.PercentComplete);
        }

        private void DefaultPlayerCheckedChanged(object sender, EventArgs e)
        {
            UserSettingService.SetUserSetting(UserSettingConstants.DefaultPlayer, defaultPlayer.Checked);
        }
        #endregion

        #region Encode Sample

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
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);

                return;
            }

            encodeQueue.Start((QueueTask)state, false);
        }

        #endregion

        #region Playback

        private void btn_play_Click(object sender, EventArgs e)
        {
            try
            {
                btn_play.Enabled = false;
                if (File.Exists(this.currentlyPlaying))
                    File.Delete(this.currentlyPlaying);
            }
            catch (Exception)
            {
                btn_play.Enabled = true;
                MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.",
                                "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            this.Text += " (Encoding)";
            int duration;
            int.TryParse(endPoint.Text, out duration);
            string query = QueryGenerator.GeneratePreviewQuery(this.mainWindow, duration, startPoint.Text);
            QueueTask task = new QueueTask(query) { Destination = this.mainWindow.text_destination.Text };
            ThreadPool.QueueUserWorkItem(this.CreatePreview, task);
        }

        /// <summary>
        /// Play the video back in an external VLC Player
        /// </summary>
        private void Play()
        {
            // Launch VLC and Play video.
            if (this.currentlyPlaying != string.Empty)
            {
                if (File.Exists(this.currentlyPlaying))
                {
                    string args = "\"" + this.currentlyPlaying + "\"";

                    if (defaultPlayer.Checked)
                    {
                        Process.Start(args);
                    }
                    else
                    {

                        // Attempt to find VLC if it doesn't exist in the default set location.
                        string vlcPath;

                        if (8 == IntPtr.Size ||
                            (!String.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432")))) vlcPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
                        else vlcPath = Environment.GetEnvironmentVariable("ProgramFiles");


                        if (!File.Exists(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path)))
                        {
                            if (File.Exists(vlcPath))
                            {
                                UserSettingService.SetUserSetting(UserSettingConstants.VLC_Path, "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe");
                                Properties.Settings.Default.Save(); // Save this new path if it does
                            }
                            else
                            {
                                MessageBox.Show(
                                    this,
                                    "Unable to detect VLC Player. \nPlease make sure VLC is installed and the directory specified in HandBrake's options is correct. (See: \"Tools Menu > Options > Picture Tab\") ",
                                    "VLC",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Warning);
                            }
                        }

                        if (File.Exists(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path)))
                        {
                            ProcessStartInfo vlc = new ProcessStartInfo(UserSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path), args);
                            Process.Start(vlc);
                        }
                    }
                }
                else
                    MessageBox.Show(this,
                                    "Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.",
                                    "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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
            encodeQueue.EncodeStarted -= this.EncodeQueueEncodeStarted;
            encodeQueue.EncodeCompleted -= this.EncodeQueueEncodeEnded;
            base.OnClosing(e);
        }
    }
}