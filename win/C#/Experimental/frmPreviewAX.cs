using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;


namespace Handbrake
{
    public partial class frmPreviewAX : Form
    {
        // !!!!!!!!!!!! WARNING !!!!!!!!!!!!
        // This code may cause a Blue Screen of Death if run.
        // This usually happens after the VLC active X control has been playing video for a short period
        // and the user tries to close the window. Calling vlc_player.dispose() also causes this I believe.
        // Patches to fix this would be very welcome!
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Handbrake.QueryGenerator hb_common_func = new Handbrake.QueryGenerator();
        Functions.Encode process = new Functions.Encode();

        String currently_playing = "";
        frmMain mainWindow;
        private Process hbProc;
        Boolean playing = false;
        int window_height;
        int window_width;


        public frmPreviewAX(frmMain mw, int w, int h)
        {
            InitializeComponent();

            this.Width = w;
            this.Height = h + statusStrip.Height + toolBar.Height;
            this.mainWindow = mw;

            resizeWindowUntilCorrect(w, h);

            vlc_player.Height = h;
            vlc_player.Width = w;
        }
        private void resizeWindowUntilCorrect(int w, int h)
        {
            // This needs fixed. It makes the window seriously slow to load.
            while (vlc_player.Width != w || vlc_player.Height != h)
            {
                if (vlc_player.Width < w)
                    this.Width++;

                if (vlc_player.Width > w)
                    this.Width--;

                if (vlc_player.Height < h)
                    this.Height++;

                if (vlc_player.Height > h)
                    this.Height--;
            }

            window_height = this.Height;
            window_width = this.Width;
        }
        private void btn_reset_Click(object sender, EventArgs e)
        {
            this.WindowState = FormWindowState.Normal;
            this.Width = window_width;
            this.Height = window_height;
        }

        // Playback Controls
        private void btn_play_Click(object sender, EventArgs e)
        {
            // Get the Destination of the sample video.
            currently_playing = "";
            if (mainWindow.text_destination.Text != "")
                currently_playing = mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm");

            if (currently_playing != "" && playing != true)
            {
                vlc_player.playlist.clear();
                vlc_player.playlist.add(currently_playing, null, null);
                vlc_player.playlist.play();
            }

            if (playing == false)
            {
                btn_play.Text = "Pause";
                btn_play.Image = Handbrake.Properties.Resources.Pause;
                playing = true;
            }
            else
            {
                vlc_player.playlist.togglePause();
                btn_play.Text = "Play";
                btn_play.Image = Handbrake.Properties.Resources.Play;
                playing = false;
            }
        }
        private void btn_stop_Click(object sender, EventArgs e)
        {
            if (vlc_player.playlist.isPlaying)
            {
                vlc_player.playlist.stop();
                vlc_player.playlist.clear();
                //vlc_player.Dispose(); // Causes a BlueScreen of Death!!!
                btn_play.Text = "Play";
                playing = false;
            }
        }


        // Encoding a Sample
        private void btn_encode_Click(object sender, EventArgs e)
        {
            String query;
            query = hb_common_func.GeneratePreviewQuery(mainWindow,"30");

            ThreadPool.QueueUserWorkItem(procMonitor, query);
        }
        private void procMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (hbProc != null)
                MessageBox.Show("Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                hbProc = process.runCli(this, (string)state);
                hbProc.WaitForExit();
                hbProc = null;
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {

           /* try
            {
                while (vlc_player.playlist.isPlaying)
                {
                    vlc_player.playlist.stop();
                    Thread.Sleep(100);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }


            //vlc_player.Dispose();
            while (vlc_player.IsDisposed == false)
            {
                Thread.Sleep(100);
            }
            vlc_player = null;
           */

            this.Dispose();
            this.Hide();
        }


    }
}
