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
    public partial class frmDvdInfo : Form
    {
        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during the scan.
        /// </summary>
        public frmDvdInfo()
        {
            InitializeComponent();
            this.rtf_dvdInfo.Text = string.Empty;     
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
             this.Hide();
        }

        private void frmDvdInfo_Load(object sender, EventArgs e)
        {
            this.rtf_dvdInfo.Text = string.Empty;
            readFile();
        }

        private void readFile()
        {
            try
            {
                string appPath = Application.StartupPath.ToString();
                appPath = appPath + "\\";
                StreamReader sr = new StreamReader(appPath + "dvdinfo.dat");

                string line = sr.ReadLine();

                while (line != null)
                {
                    this.rtf_dvdInfo.AppendText(line + System.Environment.NewLine);
                    line = sr.ReadLine();
                }
                sr.Close();
            }
            catch (Exception)
            {
                // Don't do anything
            }
        }
    }
}