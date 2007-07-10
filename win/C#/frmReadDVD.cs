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
    public partial class frmReadDVD : Form
    {

        string inputFile;
        frmMain mainWindow;

        public frmReadDVD(string inputFile, frmMain window)
        {
            InitializeComponent();
            this.inputFile = inputFile;
            this.mainWindow = window;
        }

        private void btn_ok_Click(object sender, EventArgs e)
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
            StreamReader readData = new StreamReader(new BufferedStream(hbProc.StandardError.BaseStream));
            hbProc.WaitForExit();
            hbProc.Close();

            // Setup the parser
            Parsing.DVD thisDvd = Parsing.DVD.Parse(readData);

            // Now pass this streamreader to frmMain so that it can be used there.
            mainWindow.setStreamReader(thisDvd);

            // Setup frmMain drp_dvdTitle with the title information in the form:  1 (02:34:11)
            int count = thisDvd.Titles.Count -1;
            int counter = 0;
            string title;

            while (count >= counter)
            {
                title = thisDvd.Titles[counter].TitleNumber.ToString() + " (" + thisDvd.Titles[counter].Duration.ToString() + ")";
                mainWindow.drp_dvdtitle.Items.Add(title);
                counter++;
            }
            this.Close();
        }


       
    }
}