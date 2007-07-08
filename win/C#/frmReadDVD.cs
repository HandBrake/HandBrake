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

        public void scan(string filename)
        {
            string query = "-i " + '"' + filename + '"' + " -t0";
            System.Diagnostics.Process hbProc = new System.Diagnostics.Process();
            hbProc.StartInfo.FileName = "hbcli.exe";
            hbProc.StartInfo.RedirectStandardOutput = true;
            hbProc.StartInfo.RedirectStandardError = true;
            hbProc.StartInfo.Arguments = query;
            hbProc.StartInfo.UseShellExecute = false;
            hbProc.Start();
            System.IO.StreamReader errorReader = new System.IO.StreamReader(new System.IO.BufferedStream(hbProc.StandardError.BaseStream));
            //rtf_dvdInfo.AppendText(errorReader.ReadToEnd());
            hbProc.WaitForExit();
            hbProc.Close();

            String DvdData = errorReader.ReadToEnd();
            DvdData = DvdData + "-- end --";

            String[] DvdDataArr = DvdData.Split('\n');
            int DvdDataSize = DvdDataArr.Length -1;
            String line = "";
            int counter = 0;

            //
            // Some varbiles used for parseing HandBrakes output
            //

            // DVD info stroage varibles
            string titleData  = "";
            string duationData  = "";
            string sizeData  = "";
            string cropdata  = "";
            string chatperData  = "";
            string audioData  = "";
            string subtitleData  = "";

            string fullTitleData  = "";

            // Position Pointers
            bool chapterPointer  = false;
            bool audioPointer = false;
            bool subtitlePointer = false;

            // Error handling varibles
            bool titleError = false;
            bool readError = false;
            
            while (counter <= DvdDataSize)
            {
                line = DvdDataArr[counter];
                counter++;
                 
                // Get all the 1 liner data and set chaper potiner to true when done
                if (line.Contains("exited.")){
                    subtitlePointer = false;
                    fullTitleData = titleData.Trim() + " ~ " + duationData.Trim() + " ~ " + sizeData.Trim() + " ~ " + cropdata.Trim() + " ~ " + chatperData.Trim() + " ~ " + audioData.Trim() + " ~ " + subtitleData.Trim();
                    add(fullTitleData);
                    counter++;
                }else if (line.Contains("+ title")){
                    if (titleData != "") {
                        subtitlePointer = true;
                        fullTitleData = titleData.Trim() + " ~ " + duationData.Trim() + " ~ " + sizeData.Trim() + " ~ " + cropdata.Trim() + " ~ " + chatperData.Trim() + " ~ " + audioData.Trim() + " ~ " + subtitleData.Trim();
                        add(fullTitleData);
                        counter = counter + 1;
                    }
                    titleData = line;
                }else if (line.Contains("+ duration")) {
                    duationData = line;
                }else if (line.Contains("+ size")) {
                    sizeData = line;
                }else if (line.Contains("+ autocrop")) {
                    cropdata = line;
                }else if (line.Contains("+ chapters")) {
                    chatperData = line;
                    chapterPointer = true;
                }

                // Get all the chapter information in 1 varible
                if (chapterPointer == true)
                {
                    if (!line.Contains("+ audio"))
                    {
                        chapterPointer = false;
                        audioPointer = true;
                        audioData = line;
                   }
                    else 
                    {
                        if (!chatperData.Equals(line))
                        {
                            chatperData = chatperData + " & " + line.Trim();
                        }
                    }
                }

                 // Get all the audio channel information in 1 varible
                if (audioPointer == true)
                { 
                    if (line.Contains("+ subtitle"))
                    {
                        audioPointer = false;
                        subtitlePointer = true;
                        subtitleData = line;
                    } 
                    else 
                    {
                        if (!audioData.Equals(line))
                        {
                            audioData = audioData + " & " + line.Trim();
                        }
                    }
                 }

                 //Get all the subtitle data into 1 varible
                 if (subtitlePointer == true)
                 {
                            if (line.Contains("+ subtitle")) 
                            {
                                subtitleData = line;
                            } else 
                            {
                                if (!subtitleData.Equals(line))
                                {
                                    subtitleData = subtitleData + " & " + line.Trim();
                                }
                            }
                  }

                  // Handle some of Handbrakes Error outputs if they occur.
                  if (line.Contains("No title"))
                  {
                    titleError = true;
                  }

                  if (line.Contains("***")) 
                  {
                    readError = true;
                  }

                 // Display error messages for errors detected above.
                 if (readError == true)
                 {
                     MessageBox.Show("Some DVD Title information may be missing however you may still be able to select your required title for encoding!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                 }

                 if (titleError == true)
                 {
                     MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                 }

            }
        }

        public void add(string titleData)
        {
            string[] titleInfo = new string[10];
            string[] str = new string[1];

            string title;
            string t ="";
            string d ="";

            titleInfo = titleData.Split('~');
            try
            {
                t = titleInfo[0].Trim().Substring(8).Replace(":", ""); // Title
                d = titleInfo[1].Trim().Substring(12); //Duration
            } catch(Exception){
                MessageBox.Show("Incomplete DVD data found. Please copy the data on the View DVD Information tab and report this error.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }

            //Now lets add the info to the main form dropdowns
           frmMain form = (frmMain)frmMain.ActiveForm;
           title = t + " " + " " + d + " ";
           form.drp_dvdtitle.Items.Add(title);
        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            start(inputFile);
        }

       
    }
}