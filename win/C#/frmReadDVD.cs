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
        private int cancel = 0;

        public frmReadDVD(string inputFile, frmMain parent, frmDvdInfo dvdInfoWindow)
        {
            InitializeComponent();
            this.inputFile = inputFile;
            this.mainWindow = parent;
            this.dvdInfo = dvdInfoWindow;
        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            btn_ok.Enabled = false;
            btn_skip.Visible = true;
            lbl_pressOk.Visible = false;
            lbl_progress.Text = "0%";
            lbl_progress.Visible = true;
            lbl_status.Visible = true;
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

        Functions.CLI process = new Functions.CLI();
        private void startProc(object state)
        {
            //string query = "-i " + '"' + inputFile + '"' + " -t0";
            // hbProc = process.runCli(this, query, true, true, false, true);

            //*********************************************************************************************************************************************
            /*
             * Quick and Dirty hack to get around the stderr crashes of hbcli. Lets try feeding brians parser text straight from a text file.
             */
            string appPath = Application.StartupPath.ToString();
            appPath = appPath + "\\";
            string strCmdLine = "cmd /c " + '"' + '"' + appPath + "\\hbcli.exe" + '"' +  " -i" + '"' + inputFile + '"' + " -t0 >" + '"'+ appPath + "\\dvdinfo.dat" + '"' + " 2>&1" + '"';
            Process hbproc = Process.Start("CMD.exe", strCmdLine);
            hbproc.WaitForExit();


            StreamReader sr = new StreamReader(appPath + "dvdinfo.dat");
    
            thisDvd = Parsing.DVD.Parse(sr);

            sr.Close();
            Console.ReadLine();
            updateUIElements();
            //*********************************************************************************************************************************************

            /*
             * This has been temporily disabled due to the stderr issue
             * 
             * 
            Parsing.Parser readData = new Parsing.Parser(hbProc.StandardError.BaseStream);
            readData.OnScanProgress += Parser_OnScanProgress;
            readData.OnReadLine += dvdInfo.HandleParsedData;
            readData.OnReadToEnd += dvdInfo.HandleParsedData;

            // Setup the parser
            

            if (cancel != 1)
            {
                updateUIElements();
                process.killCLI();
                process.closeCLI();
            }
            */
        }

        private void Parser_OnScanProgress(object Sender, int CurrentTitle, int TitleCount)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Parsing.ScanProgressEventHandler(Parser_OnScanProgress), new object[] { Sender, CurrentTitle, TitleCount });
                return;
            }
            int progress = Convert.ToInt32(Convert.ToDouble(CurrentTitle) / Convert.ToDouble(TitleCount) * 100) + 1;
            if (progress > 100)
            {
                progress = 100;
            }
            this.lbl_progress.Text = progress.ToString() + "%";
        }

        private void btn_skip_Click(object sender, EventArgs e)
        {
            //process.killCLI();
            this.Close();
            cancel = 1;  
        }

    }
}