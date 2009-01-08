using System;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;
using QTOLibrary;

namespace Handbrake
{
    public partial class frmPreview : Form
    {

        QueryGenerator hb_common_func = new QueryGenerator();
        Functions.Encode process = new Functions.Encode();
        private delegate void UpdateUIHandler();
        String currently_playing = "";
        frmMain mainWindow;
        private Process hbProc;
        private Thread player;

        public frmPreview(frmMain mw)
        {
            InitializeComponent();
            this.mainWindow = mw;
            cb_preview.SelectedIndex = 0;
            cb_duration.SelectedIndex = 1;
        }

        private void play()
        {
            player = new Thread(OpenMovie);
            player.IsBackground = true;
            player.Start();
        }

        [STAThread]
        private void OpenMovie()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUIHandler(OpenMovie));
                    return;
                }
                QTControl.URL = currently_playing;
                QTControl.Width = QTControl.Movie.Width;
                QTControl.Height = QTControl.Movie.Height;
                // The initial control size is 64,64. If we do not reload the clip here
                // it'll scale the video from 64,64. 
                // Unsure why as it correctly resizes the control to the movies actual size.
                QTControl.URL = currently_playing; 
                QTControl.SetScale(0);
                QTControl.Show();

                this.Width = QTControl.Width + 5;
                this.Height = QTControl.Height + 90;
            }
            catch (COMException ex)
            {
                QTUtils qtu = new QTUtils();
                MessageBox.Show("Unable to open movie:\n\nError Code: " + ex.ErrorCode.ToString("X") + "\nQT Error code : " + qtu.QTErrorFromErrorCode(ex.ErrorCode).ToString());
            }
            catch (Exception ex)
            {
                MessageBox.Show("Unable to open movie:\n\n" + ex.ToString());
            }
        }
 
        #region Encode Sample
        private void btn_encode_Click(object sender, EventArgs e)
        {
            btn_encode.Enabled = false;
            lbl_encode.Text = "Encoding Sample ...";
            String query = hb_common_func.GeneratePreviewQuery(mainWindow, cb_duration.Text, cb_preview.Text);
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
                encodeCompleted();
            }
        }
        private void encodeCompleted()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUIHandler(encodeCompleted));
                    return;
                }
                btn_encode.Enabled = true;
                lbl_encode.Text = "Loading Clip ...";

                if (mainWindow.text_destination.Text != "")
                    currently_playing = mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm");

                play();
                lbl_encode.Text = "";
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmPreview.cs encodeCompleted " + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        #endregion

    }
}