/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;


namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {
        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during an encode.
        /// </summary>
        /// 
        Thread monitorFile;
        public frmActivityWindow()
        {
            InitializeComponent();
            this.rtf_actLog.Text = string.Empty;
            monitorFile = new Thread(autoUpdate);

        }
        
        private void btn_close_Click(object sender, EventArgs e)
        {
            monitorFile.Abort();
            this.Hide();
        }

        private void frmActivityWindow_Load(object sender, EventArgs e)
        {
            this.rtf_actLog.Text = string.Empty;
            rtf_actLog.Text = readFile();
   
            monitorFile.Start();
        }

        private void autoUpdate(object state)
        {
            while (true)
            {
                updateTextFromThread();
                Thread.Sleep(3000);
            }
        }
        
        private delegate void UpdateUIHandler();
        private void updateTextFromThread()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new UpdateUIHandler(updateTextFromThread));
                return;
            }
            rtf_actLog.Text = readFile();
            this.rtf_actLog.SelectionStart = this.rtf_actLog.Text.Length -1;
            this.rtf_actLog.ScrollToCaret();

            if (rtf_actLog.Text.Contains("HandBrake has exited."))
                monitorFile.Abort();
        }

        private string readFile()
        {
            string log = "";
            try
            {
                // hb_encode_log.dat is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it,
                // we'll need to make a copy of it.
                string logFile = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");
                string logFile2 = Path.Combine(Path.GetTempPath(), "hb_encode_log_AppReadable.dat");

                // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                if (File.Exists(logFile2))
                    File.Delete(logFile2);

                // Copy the log file.
                File.Copy(logFile, logFile2);

                // Begin processing the log file.
                StreamReader sr = new StreamReader(logFile2);
                string line = sr.ReadLine();
                while (line != null)
                {
                    log = log + (line + System.Environment.NewLine);
                    line = sr.ReadLine();
                }
                sr.Close();
            }
            catch (Exception exc)
            {
                MessageBox.Show("An Error has occured! \n\n" + exc.ToString(), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }

            return log;
        }

        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.Text != "")
                Clipboard.SetText(rtf_actLog.Text, TextDataFormat.Rtf);
        }

        private void btn_refresh_Click(object sender, EventArgs e)
        {
            rtf_actLog.Clear();
            rtf_actLog.Text = readFile();
        }


    }
}