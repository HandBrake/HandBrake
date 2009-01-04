using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;

using QTOControlLib;
using QTOLibrary;

namespace Handbrake
{
    public partial class frmPreviewAX : Form
    {

        Handbrake.QueryGenerator hb_common_func = new Handbrake.QueryGenerator();
        Functions.Encode process = new Functions.Encode();

        String currently_playing = "";
        frmMain mainWindow;
        private Process hbProc;

        public frmPreviewAX(frmMain mw)
        {
            InitializeComponent();
            this.mainWindow = mw;
            cb_preview.SelectedIndex = 0;
            cb_duration.SelectedIndex = 1;
        }

        private void btn_play_Click(object sender, EventArgs e)
        {
            if (mainWindow.text_destination.Text != "")
                currently_playing = mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm");

            OpenMovie(currently_playing);

            this.Width = QTControl.Width + 5;
            this.Height = QTControl.Height + 90;
        }

        private void OpenMovie(string url)
        {
            try
            {
                QTControl.URL = url;
                QTControl.Show();
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
            }
        }
        #endregion

    }
}