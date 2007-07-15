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
    public partial class frmReadDVD : Form
    {
        private string inputFile;
        private frmMain mainWindow;
        private frmDvdInfo dvdInfo;
        private Parsing.DVD thisDvd;
        private delegate void UpdateUIHandler();

        public frmReadDVD(string inputFile, frmMain parent, frmDvdInfo dvdInfoWindow)
        {
            InitializeComponent();
            this.inputFile = inputFile;
            this.mainWindow = parent;
            this.dvdInfo = dvdInfoWindow;
        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            lbl_status.Visible = true;
            btn_ok.Enabled = false;
            lbl_pressOk.Visible = false;
            // throw cli call and parsing on it's own thread
            ThreadPool.QueueUserWorkItem(startProc);
        }
     
        private void updateUIElements()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new UpdateUIHandler(updateUIElements));
                return;
            }
            // Now pass this streamreader to frmMain so that it can be used there.
            mainWindow.setStreamReader(thisDvd);

            mainWindow.drp_dvdtitle.Items.Clear();
            mainWindow.drp_dvdtitle.Items.AddRange(thisDvd.Titles.ToArray());

            // Just a quick test to see if data can be pushed to frmDvdInfo.
            // What needs to happen here is the plaintext outout needs to be sent to the frmDvdInfo.
            dvdInfo.rtf_dvdInfo.Text = thisDvd.ToString();

            this.Close();
        }

        private void startProc(object state)
        {
            string query = "-i " + '"' + inputFile + '"' + " -t0";
            System.Diagnostics.Process hbProc = new System.Diagnostics.Process();
            hbProc.StartInfo.FileName = "hbcli.exe";
            hbProc.StartInfo.RedirectStandardOutput = true;
            hbProc.StartInfo.RedirectStandardError = true;
            hbProc.StartInfo.Arguments = query;
            hbProc.StartInfo.UseShellExecute = false;
            hbProc.StartInfo.CreateNoWindow = true;

            hbProc.Start();
            Parsing.Parser readData = new Parsing.Parser(hbProc.StandardError.BaseStream);
            hbProc.WaitForExit();
            hbProc.Close();

            // Setup the parser
            thisDvd = Parsing.DVD.Parse(readData);

            updateUIElements();
        }

    }
}