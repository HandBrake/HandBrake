using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.IO;

namespace Handbrake
{
    public partial class frmGenPreview : Form
    {
        private delegate void UpdateHandler();
        Handbrake.QueryGenerator queryGen = new Handbrake.QueryGenerator();
        Functions.Encode process = new Functions.Encode();
        Process hbProc;
        frmMain mainWindow;

        public frmGenPreview(frmMain mw)
        {
            InitializeComponent();
            this.mainWindow = mw;
            cb_duration.SelectedIndex = 0;
            cb_preview.SelectedIndex = 0;
        }

        private void btn_play_Click(object sender, EventArgs e)
        {
            String currently_playing;

            // Get the Destination of the sample video.
            currently_playing = "";
            if (mainWindow.text_destination.Text != "")
                currently_playing = mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm");

            // Launch VLC and play video.
            if (currently_playing != "")
            {
                if (File.Exists(currently_playing))
                {
                    if (File.Exists(Properties.Settings.Default.VLC_Path))
                    {
                        String args = "\"" + currently_playing + "\"";
                        ProcessStartInfo vlc = new ProcessStartInfo(Properties.Settings.Default.VLC_Path, args);
                        Process.Start(vlc);
                        lbl_status.Text = "VLC will now launch.";
                    }
                    else
                        MessageBox.Show("Unable to detect VLC Player. \nPlease make sure VLC is installed and the directory specified in the program options is correct.", "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                } else
                    MessageBox.Show("Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.", "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void btn_encode_Click(object sender, EventArgs e)
        {
            String query = queryGen.GeneratePreviewQuery(mainWindow, cb_duration.Text, cb_preview.Text);
            ThreadPool.QueueUserWorkItem(procMonitor, query);
        }
        private void procMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (hbProc != null)
                MessageBox.Show("Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                encodingMessage();
                hbProc = process.runCli(this, (string)state);
                hbProc.WaitForExit();
                hbProc = null;
                updateUIElements();
            }
        }
        // Update the UI now that the encode has finished.
        private void encodingMessage()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateHandler(encodingMessage));
                    return;
                }
                lbl_status.Text = "Encoding, Please wait ...";
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        // Update the UI now that the encode has finished.
        private void updateUIElements()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateHandler(updateUIElements));
                    return;
                }

                btn_play.Visible = true;
                toolStripSeparator1.Visible = true;
                lbl_status.Text = "Your sample is ready to play.";
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }


    }
}
