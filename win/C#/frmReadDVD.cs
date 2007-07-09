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
            hbProc.WaitForExit();
            hbProc.Close();

            //Parsing.DVD thisDvd = Parsing.DVD.Parse(errorReader);

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
                    add(fullTitleData, titleData, duationData);
                    counter++;
                }else if (line.Contains("+ title")){
                    if (titleData != "") {
                        subtitlePointer = true;
                        fullTitleData = titleData.Trim() + " ~ " + duationData.Trim() + " ~ " + sizeData.Trim() + " ~ " + cropdata.Trim() + " ~ " + chatperData.Trim() + " ~ " + audioData.Trim() + " ~ " + subtitleData.Trim();
                        add(fullTitleData, titleData, duationData);
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

        public void add(string fullTitleData, string titleData, string durationData)
        {

            try
            {
                string t = titleData.Trim().Substring(8).Replace(":", "");
                string d = durationData.Trim().Substring(12);

                // Lets store the captured full title data as a string in the programs settings file.
                // This can then be used by the DVD title dropdown menu to populate other fields.

                Properties.Settings.Default.FullDVDInfo = Properties.Settings.Default.FullDVDInfo + " \n " + fullTitleData;

                //Now lets add the info to the main form dropdowns
                frmMain form = (frmMain)frmMain.ActiveForm;
                string title = t + " " + " " + d + " ";
                form.drp_dvdtitle.Items.Add(title);
            }
            catch (Exception)
            {
                // Don't really need to do anything about it.
            }
        }


        private void btn_ok_Click(object sender, EventArgs e)
        {
            scan(inputFile);
        }

       
    }
}