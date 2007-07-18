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


namespace Handbrake
{
    public partial class frmReadDVD : Form
    {
        private string inputFile;
        private frmMain mainWindow;
        private frmDvdInfo dvdInfo;
        private Parsing.DVD thisDvd;
        private Process hbProc;
        private delegate void UpdateUIHandler();

        public frmReadDVD(string inputFile, frmMain parent, frmDvdInfo dvdInfoWindow)
        {
            InitializeComponent();
            this.inputFile = inputFile;
            this.mainWindow = parent;
            this.dvdInfo = dvdInfoWindow;
            Parsing.Parser.OnScanProgress += Parser_OnScanProgress;
        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            btn_ok.Enabled = false;
            lbl_pressOk.Visible = false;
            scanProgress.Value = 0;
            scanProgress.Visible = true;
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

            this.Close();
        }

        private void startProc(object state)
        {
            string query = "-i " + '"' + inputFile + '"' + " -t0";
          
            Functions.CLI process = new Functions.CLI();
            hbProc = process.runCli(this, query, true, true, false, true);

            Parsing.Parser readData = new Parsing.Parser(hbProc.StandardError.BaseStream);
            //hbProc.WaitForExit();
            hbProc.Close();

            // Setup the parser
            thisDvd = Parsing.DVD.Parse(readData);

            updateUIElements();
        }

        private void Parser_OnScanProgress(object Sender, int CurrentTitle, int TitleCount)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Parsing.ScanProgressEventHandler(Parser_OnScanProgress), new object[] { Sender, CurrentTitle, TitleCount });
                return;
            }
            this.scanProgress.Value = Convert.ToInt32(Convert.ToDouble(CurrentTitle) / Convert.ToDouble(TitleCount) * 100) + 1;
        }

    }
}