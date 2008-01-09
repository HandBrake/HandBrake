/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;


namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {
        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during an encode.
        /// </summary>
        public frmActivityWindow()
        {
            InitializeComponent();
            this.rtf_actLog.Text = string.Empty;     
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
             this.Hide();
        }

        private void frmActivityWindow_Load(object sender, EventArgs e)
        {
            this.rtf_actLog.Text = string.Empty;
            readFile();
        }

        private void readFile()
        {
            try
            {
                string dvdInfoPath = Path.Combine(Path.GetTempPath(), "hb_encode_log.dat");
                FileStream f = System.IO.File.Open(dvdInfoPath, FileMode.Open, FileAccess.Read, FileShare.Read);

                StreamReader sr = new StreamReader(f);



                string line = sr.ReadLine();

                while (line != null)
                {
                    this.rtf_actLog.AppendText(line + System.Environment.NewLine);
                    line = sr.ReadLine();
                }
                sr.Close();
            }
            catch (Exception)
            {
                rtf_actLog.Clear();
                rtf_actLog.Text = "Please wait until the encode has finished to view the log.";
            }
        }

        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.Text != "")
                Clipboard.SetText(rtf_actLog.Text, TextDataFormat.Text);
        }

        private void btn_refresh_Click(object sender, EventArgs e)
        {
            rtf_actLog.Clear();
            readFile();
        }


    }
}