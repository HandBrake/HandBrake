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
using System.Diagnostics;
using System.Runtime.InteropServices;



namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {
        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during an encode.
        /// </summary>
        /// 
        Thread monitorFile;
        String read_file;
        public frmActivityWindow(string file)
        {
            InitializeComponent();
            this.rtf_actLog.Text = string.Empty;

            read_file = file;

            string logFile = Path.Combine(Path.GetTempPath(), read_file);
            if (File.Exists(logFile))
            {
                if (read_file == "dvdinfo.dat") // No need to refresh the window if we are viwing dvdinfo.dat
                    updateTextFromThread();
                else // however, we should refresh when reading the encode log file.
                {
                    monitorFile = new Thread(autoUpdate);
                    monitorFile.Start();
                }
            }
            else
                MessageBox.Show("The log file could not be found. Maybe you cleared your system's tempory folder or maybe you just havn't run an encode yet.", "Notice", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        // Update the Activity window every 5 seconds with the latest log data.
        private void autoUpdate(object state)
        {
            while (true)
            {
                updateTextFromThread();
                Thread.Sleep(5000);
            }
        }

        private delegate void UpdateUIHandler();
        private void updateTextFromThread()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUIHandler(updateTextFromThread));
                    return;
                }
                rtf_actLog.Text = readFile();
                this.rtf_actLog.SelectionStart = this.rtf_actLog.Text.Length - 1;
                this.rtf_actLog.ScrollToCaret();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private string readFile()
        {
            string log = "";
            try
            {
                // hb_encode_log.dat is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it,
                // we'll need to make a copy of it.
                string logFile = Path.Combine(Path.GetTempPath(), read_file);
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

        protected override void OnClosing(CancelEventArgs e)
        {
            if (monitorFile != null)
                monitorFile.Abort();
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }
    }
}