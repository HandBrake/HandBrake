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
        private delegate void UpdateUIHandler();

        public frmReadDVD(string inputFile, frmMain parent, frmDvdInfo dvdInfoWindow)
        {
            InitializeComponent();
            this.inputFile = inputFile;
            this.mainWindow = parent;
            this.dvdInfo = dvdInfoWindow;
            startScan();

        }

        private void startScan()
        {
            try
            {
                lbl_status.Visible = true;
                ThreadPool.QueueUserWorkItem(startProc);
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmReadDVD.cs - startScan " + exc.ToString());
            }
        }

        private void updateUIElements()
        {
            try
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
                mainWindow.drp_dvdtitle.Text = "Automatic";
                mainWindow.drop_chapterFinish.Text = "Auto";
                mainWindow.drop_chapterStart.Text = "Auto";

                this.Close();
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmReadDVD.cs - updateUIElements " + exc.ToString());
            }
        }

        Functions.CLI process = new Functions.CLI();

        private void startProc(object state)
        {
            try
            {
                string appPath = Application.StartupPath.ToString() + "\\";
                string strCmdLine = "cmd /c " + '"' + '"' + appPath + "HandBrakeCLI.exe" + '"' + " -i " + '"' + inputFile + '"' + " -t0 -v >" + '"' + appPath + "dvdinfo.dat" + '"' + " 2>&1" + '"';
                Process hbproc = Process.Start("CMD.exe", strCmdLine);
                hbproc.WaitForExit();
                hbproc.Dispose();
                hbproc.Close();


                StreamReader sr = new StreamReader(appPath + "dvdinfo.dat");
                thisDvd = Parsing.DVD.Parse(sr);
                sr.Close();

                updateUIElements();
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmReadDVD.cs - startProc " + exc.ToString());
            }

        }
    }
}