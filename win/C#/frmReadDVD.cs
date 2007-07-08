using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmReadDVD : Form
    {

        string inputFile;

        public frmReadDVD(string inputFile)
        {
            InitializeComponent();
            this.inputFile = inputFile;
        }

        private void frmReadDVD_Load(object sender, EventArgs e)
        {
            //start(inputFile);
        }

        public void start(string filename)
        {
            MessageBox.Show(filename);
            string query = "-i " + '"' + filename + '"' + " -t0";
            System.Diagnostics.Process hbProc = new System.Diagnostics.Process();
            hbProc.StartInfo.FileName = "hbcli.exe";
            hbProc.StartInfo.RedirectStandardOutput = true;
            hbProc.StartInfo.RedirectStandardError = true;
            //hbProc.StartInfo.StandardOutputEncoding = System.Text.Encoding.UTF8;
            //hbProc.StartInfo.StandardErrorEncoding = System.Text.Encoding.UTF8;
            hbProc.StartInfo.Arguments = query;
            hbProc.StartInfo.UseShellExecute = false;
            hbProc.Start();

            rtf_dvdInfo.Text = "-- Start --";
            while (hbProc.StandardOutput.BaseStream.CanRead && !hbProc.HasExited)
            {
                rtf_dvdInfo.Text = rtf_dvdInfo.Text + "\n" + hbProc.StandardOutput.ReadLine();
                rtf_dvdInfo.Text = rtf_dvdInfo.Text + "\n" + hbProc.StandardError.ReadLine();
                MessageBox.Show("Test");
            }

            rtf_dvdInfo.Text = rtf_dvdInfo.Text + "\n" + "-- End --";
        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            start(inputFile);
        }

       
    }
}