using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;


namespace Handbrake
{

    public partial class frmMain : Form
    {
        // -------------------------------------------------------------- 
        // Applicaiton Startup Stuff
        // --------------------------------------------------------------

        #region Application Startup

        private Process hbProc;
        private Parsing.DVD thisDVD;        
        private frmQueue queueWindow = new frmQueue();  

        public frmMain()
        {

            // Load the splash screen on another thread. Once completed wait for 1 second.
            ThreadPool.QueueUserWorkItem(showSplash); 
            Thread.Sleep(1000);

            InitializeComponent();

            // This is a quick Hack fix for the cross-thread problem with frmDvdIndo ************************
            //dvdInfoWindow.Show();
            //dvdInfoWindow.Hide();
            // **********************************************************************************************

            // Set the Version number lable to the corect version.
            Version.Text = "Version " + Properties.Settings.Default.CliVersion;

            // Run the update checker.
            updateCheck();

            // Now load the users default if required. (Will overide the above setting)
            loadUserDefaults();

            // Enable or disable tooltips
            tooltip();

            // Hide the presets part of the window
            this.Width = 590;

            showPresets();

            /*
             * This code can be used for storing preset and preset name information in future versions.
             * Please ignore it for the moment.
            // Create and initialize a new StringCollection.
            StringCollection myCol = new StringCollection();
            // Add a range of elements from an array to the end of the StringCollection.
            String[] myArr = new String[] { "RED", "orange", "yellow", "RED", "green", "blue", "RED", "indigo", "violet", "RED" };
            myCol.AddRange(myArr);
            Properties.Settings.Default.BuiltInPresets = myCol;
            MessageBox.Show(Properties.Settings.Default.BuiltInPresets.ToString());
            */
        }

        private void showSplash(object sender)
        {
            // Display splash screen for 1.5 Seconds
            Form splash = new frmSplashScreen();
            splash.Show();
            Thread.Sleep(1500);  
            splash.Close(); // Then close.
        }

        private void loadUserDefaults()
        { 
            try
            {
                // Load the users default settings or if the user has not got this option enabled, load the normal preset.
                if (Properties.Settings.Default.defaultSettings == "Checked")
                {
                    // Source
                    text_source.Text = Properties.Settings.Default.DVDSource;
                    drp_dvdtitle.Text = Properties.Settings.Default.DVDTitle;
                    drop_chapterStart.Text = Properties.Settings.Default.ChapterStart;
                    drop_chapterFinish.Text = Properties.Settings.Default.ChapterFinish;

                    // Destination
                    text_destination.Text = Properties.Settings.Default.VideoDest;
                    drp_videoEncoder.Text = Properties.Settings.Default.VideoEncoder;
                    drp_audioCodec.Text = Properties.Settings.Default.AudioEncoder;
                    text_width.Text = Properties.Settings.Default.Width;
                    text_height.Text = Properties.Settings.Default.Height;

                    // Picture Settings Tab
                    drp_crop.Text = Properties.Settings.Default.CroppingOption;
                    text_top.Text = Properties.Settings.Default.CropTop;
                    text_bottom.Text = Properties.Settings.Default.CropBottom;
                    text_left.Text = Properties.Settings.Default.CropLeft;
                    text_right.Text = Properties.Settings.Default.CropRight;
                    drp_subtitle.Text = Properties.Settings.Default.Subtitles;

                    // Video Settings Tab
                    text_bitrate.Text = Properties.Settings.Default.VideoBitrate;
                    text_filesize.Text = Properties.Settings.Default.VideoFilesize;
                    slider_videoQuality.Value = Properties.Settings.Default.VideoQuality;

                    if (Properties.Settings.Default.TwoPass == "Checked")
                    {
                        check_2PassEncode.CheckState = CheckState.Checked;
                    }

                    drp_deInterlace_option.Text = Properties.Settings.Default.DeInterlace;
                    drp_deNoise.Text = Properties.Settings.Default.denoise;

                    if (Properties.Settings.Default.detelecine == "Checked")
                    {
                        check_detelecine.CheckState = CheckState.Checked;
                    }

                    if (Properties.Settings.Default.detelecine == "Checked")
                    {
                        check_deblock.CheckState = CheckState.Checked;
                    }


                    if (Properties.Settings.Default.Grayscale == "Checked")
                    {
                        check_grayscale.CheckState = CheckState.Checked;
                    }

                    drp_videoFramerate.Text = Properties.Settings.Default.Framerate;

                    if (Properties.Settings.Default.PixelRatio == "Checked")
                    {
                        CheckPixelRatio.CheckState = CheckState.Checked;
                    }
                    if (Properties.Settings.Default.turboFirstPass == "Checked")
                    {
                        check_turbo.CheckState = CheckState.Checked;
                    }
                    if (Properties.Settings.Default.largeFile == "Checked")
                    {
                        check_largeFile.CheckState = CheckState.Checked;
                    }

                    if (Properties.Settings.Default.chapterMarker == "Checked")
                    {
                        Check_ChapterMarkers.CheckState = CheckState.Checked;
                    }
                   
                    // Audio Settings Tab
                    drp_audioBitrate.Text = Properties.Settings.Default.AudioBitrate;
                    drp_audioSampleRate.Text = Properties.Settings.Default.AudioSampleRate;
                    drp_audioChannels.Text = Properties.Settings.Default.AudioChannels;

                    // H264 Tab
                    if (Properties.Settings.Default.CRF == "Checked")
                    {
                        CheckCRF.CheckState = CheckState.Checked;
                    }
                    rtf_h264advanced.Text = Properties.Settings.Default.H264;

                    groupBox_output.Text = "Output Settings (Preset: " + Properties.Settings.Default.selectedPreset + ")";
                }
                else
                {
                    // Load the default preset on lauch
                    ListBox_Presets.SelectedItem = "Normal";
                }
            }
            catch (Exception)
            {
                // No real need to alert the user. Try/Catch only in just incase there is a problem reading the settings xml file.
            }
        }

        private Boolean updateCheck()
        {
            try
            {
                if (Properties.Settings.Default.updateStatus == "Checked")
                {
                    String updateFile = Properties.Settings.Default.updateFile;
                    WebClient client = new WebClient();
                    String data = client.DownloadString(updateFile);
                    String[] versionData = data.Split('\n');

                    int verdata = int.Parse(versionData[0].Replace(".", ""));
                    int vergui = int.Parse(Properties.Settings.Default.GuiVersion.Replace(".", ""));
                    int verd1 = int.Parse(versionData[1].Replace(".", ""));
                    int cliversion = int.Parse(Properties.Settings.Default.CliVersion.Replace(".", ""));

                    Boolean update = ((verdata > vergui) || (verd1 > cliversion));
   
                    lbl_update.Visible = update;

                    return update;   
                }
                else
                {
                    return false;
                }
            }
            catch (Exception)
            {
                // Silently ignore the error
                return false;
            }
        }

        private void tooltip()
        {
            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                ToolTip.Active = true;
            }
        }

        private void showPresets()
        {
            if (Properties.Settings.Default.showPresets == "Checked")
            {
                btn_presets.Visible = false;
                this.Width = 881;
            }

        }

        #endregion

        // -------------------------------------------------------------- 
        // The main Menu bar.
        // -------------------------------------------------------------- 

        #region File Menu

        private void mnu_exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        #endregion

        #region Tools Menu

        private void mnu_encode_Click(object sender, EventArgs e)
        {
            showQueue();
        }
   
        private void mnu_viewDVDdata_Click(object sender, EventArgs e)
        {
            frmDvdInfo dvdInfoWindow = new frmDvdInfo();
            dvdInfoWindow.Show();
        }

        private void mnu_options_Click(object sender, EventArgs e)
        {
            Form Options = new frmOptions();
            Options.ShowDialog();
        }

        #endregion

        #region Presets Menu

        private void mnu_presetReset_Click(object sender, EventArgs e)
        {
            ListBox_Presets.Items.Clear();
            ListBox_Presets.Items.Add("Animation");
            ListBox_Presets.Items.Add("AppleTV");
            ListBox_Presets.Items.Add("Bedlam");
            ListBox_Presets.Items.Add("Blind");
            ListBox_Presets.Items.Add("Broke");
            ListBox_Presets.Items.Add("Classic");
            ListBox_Presets.Items.Add("Constant Quality Rate");
            ListBox_Presets.Items.Add("Deux Six Quatre");
            ListBox_Presets.Items.Add("Film");
            ListBox_Presets.Items.Add("iPhone");
            ListBox_Presets.Items.Add("iPod High-Rez");
            ListBox_Presets.Items.Add("iPod Low-Rez");
            ListBox_Presets.Items.Add("Normal");
            ListBox_Presets.Items.Add("PS3");
            ListBox_Presets.Items.Add("PSP");
            ListBox_Presets.Items.Add("QuickTime");
            ListBox_Presets.Items.Add("Television");

            if (presetStatus == false)
            {
                this.Width = 881;
                presetStatus = true;
                btn_presets.Text = "Hide Presets";
            }
        }

        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            ListBox_Presets.SelectedItem = "Normal";
            if (presetStatus == false)
            {
                this.Width = 881;
                presetStatus = true;
                btn_presets.Text = "Hide Presets";
            }
        }

        #endregion

        #region Help Menu


        private void mnu_quickStart_Click(object sender, EventArgs e)
        {
            Form QuickStart = new frmQuickStart();
            QuickStart.ShowDialog();
        }

        private void mnu_wiki_Click(object sender, EventArgs e)
        {
           Process.Start("http://handbrake.m0k.org/trac");
        }

        private void mnu_faq_Click(object sender, EventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/trac/wiki/WindowsGuiFaq");
        }

        private void mnu_onlineDocs_Click(object sender, EventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/?page_id=11");
        }

        private void mnu_homepage_Click(object sender, EventArgs e)
        {
           Process.Start("http://handbrake.m0k.org");
        }

        private void mnu_forum_Click(object sender, EventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/forum");
        }

        private void mnu_UpdateCheck_Click(object sender, EventArgs e)
        {
            Boolean update = updateCheck();
            if (update == true)
            {
                MessageBox.Show("There is a new update available. Please visit http://handbrake.m0k.org for details!", "Update Check", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                MessageBox.Show("There are no new updates at this time.", "Update Check", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private void mnu_about_Click(object sender, EventArgs e)
        {
			Form About = new frmAbout();
            About.ShowDialog();
        }

        #endregion

        // -------------------------------------------------------------- 
        // Buttons on the main Window
        // --------------------------------------------------------------

        #region Buttons

        private void btn_Browse_Click(object sender, EventArgs e)
        {
            String filename = ""; 
	 	    text_source.Text = "";
            frmDvdInfo dvdInfoWindow = new frmDvdInfo();          	 
	 	    if (RadioDVD.Checked) 
	 	    { 
	 	        DVD_Open.ShowDialog(); 
	 	        filename = DVD_Open.SelectedPath; 
	 	        if (filename != "") 
	 	        { 
	 	            Form frmRD = new frmReadDVD(filename, this, dvdInfoWindow); 
	 	            text_source.Text = filename; 
	 	            frmRD.ShowDialog(); 
	 	        } 
	 	     } 
	 	     else 
	 	     { 
	 	        ISO_Open.ShowDialog(); 
	 	        filename = ISO_Open.FileName; 
	 	        if (filename != "") 
	 	        { 
	 	            Form frmRD = new frmReadDVD(filename, this, dvdInfoWindow); 
	 	            text_source.Text = filename; 
	 	            frmRD.ShowDialog(); 
	 	        } 
	 	     }   
	 	 
	 	     // Check if there was titles in the dvd title dropdown 
	 	     if (filename == "") 
	 	     { 
	 	        text_source.Text = "Click 'Browse' to continue"; 
	 	     } 
	 	    
             // If there are no titles in the dropdown menu then the scan has obviously failed. Display an error message explaining to the user.
	 	     if (drp_dvdtitle.Items.Count == 0) 
	 	     { 
	 	        MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source. Please refer to the FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand); 
	 	     }  	        
        }

        private void btn_destBrowse_Click(object sender, EventArgs e)
        {
            DVD_Save.ShowDialog();
            text_destination.Text = DVD_Save.FileName;

            if (Check_ChapterMarkers.Checked)
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".mp4", ".m4v");
                text_destination.Text = destination;
            }
        }

        private void btn_h264Clear_Click(object sender, EventArgs e)
        {
            rtf_h264advanced.Text = "";
        }

        private void GenerateQuery_Click(object sender, EventArgs e)
        {
            String query = GenerateTheQuery();
            QueryEditorText.Text = query;
        }

        private void btn_ClearQuery_Click(object sender, EventArgs e)
        {
            QueryEditorText.Text = "";
        }

        private void btn_queue_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                string query;
                if (QueryEditorText.Text == "")
                {
                    query = GenerateTheQuery();
                }
                else
                {
                    query = QueryEditorText.Text;
                }
                queueWindow.list_queue.Items.Add(query);
                queueWindow.Show();
            } 
        }

        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (QueryEditorText.Text != "")
                Clipboard.SetText(QueryEditorText.Text, TextDataFormat.Text);
        }

        private void showQueue()
        {
            queueWindow.Show();
        }

        #endregion

        // -------------------------------------------------------------- 
        // Main Window Preset System
        // --------------------------------------------------------------

        #region Preset System

        // Varibles
        private Boolean presetStatus = false;

        // Buttons
        private void btn_presets_Click(object sender, EventArgs e)
        {
            if (presetStatus == false)
            {
                this.Width = 881;
                presetStatus = true;
                btn_presets.Text = "Hide Presets";
            }
            else
            {
                this.Width = 590;
                presetStatus = false;
                btn_presets.Text = "Show Presets";
            }

        }

        private void btn_addPreset_Click(object sender, EventArgs e)
        {
            string filename;
            File_Open.ShowDialog();
            filename = File_Open.FileName;
            if (filename != "")
            {
                try
                {
                    // Create StreamReader & open file
                    StreamReader line = new StreamReader(filename);
                    string temporyLine; // Used for reading the line into a varible before processing on the checkState items below.

                    // Read in the data and set the correct GUI component with the setting.
                    text_source.Text = line.ReadLine();
                    drp_dvdtitle.Text = line.ReadLine();
                    drop_chapterStart.Text = line.ReadLine();
                    drop_chapterFinish.Text = line.ReadLine();
                    text_destination.Text = line.ReadLine();
                    drp_videoEncoder.Text = line.ReadLine();
                    drp_audioCodec.Text = line.ReadLine();
                    text_width.Text = line.ReadLine();
                    text_height.Text = line.ReadLine();
                    text_top.Text = line.ReadLine();
                    text_bottom.Text = line.ReadLine();
                    text_left.Text = line.ReadLine();
                    text_right.Text = line.ReadLine();
                    drp_subtitle.Text = line.ReadLine();
                    text_bitrate.Text = line.ReadLine();
                    text_filesize.Text = line.ReadLine();
                    slider_videoQuality.Value = int.Parse(line.ReadLine());

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        check_2PassEncode.CheckState = CheckState.Checked;
                    }

                    drp_deInterlace_option.Text = line.ReadLine();

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        check_grayscale.CheckState = CheckState.Checked;
                    }

                    drp_videoFramerate.Text = line.ReadLine();

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        Check_ChapterMarkers.CheckState = CheckState.Checked;
                    }

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        CheckPixelRatio.CheckState = CheckState.Checked;
                    }

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        check_turbo.CheckState = CheckState.Checked;
                    }

                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        check_largeFile.CheckState = CheckState.Checked;
                    }

                    drp_audioBitrate.Text = line.ReadLine();
                    drp_audioSampleRate.Text = line.ReadLine();
                    drp_audioChannels.Text = line.ReadLine();
                    drp_audioMixDown.Text = line.ReadLine();

                    // Advanced H264 Options
                    temporyLine = line.ReadLine();
                    if (temporyLine == "Checked")
                    {
                        CheckCRF.CheckState = CheckState.Checked;
                    }
                    rtf_h264advanced.Text = line.ReadLine();

                    // Close the stream
                    line.Close();


                    // Fix for SliderValue not appearing when Opening saved file
                    SliderValue.Text = slider_videoQuality.Value + "%";

                }
                catch (Exception)
                {
                    MessageBox.Show("Unable to load profile.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }
            }
        }

        private void btn_removePreset_Click(object sender, EventArgs e)
        {
            string filename;
            File_Save.ShowDialog();
            filename = File_Save.FileName;
            if (filename != "")
            {
                try
                {
                    // Create a StreamWriter and open the file
                    StreamWriter line = new StreamWriter(filename);

                    //Source
                    line.WriteLine(text_source.Text);
                    line.WriteLine(drp_dvdtitle.Text);
                    line.WriteLine(drop_chapterStart.Text);
                    line.WriteLine(drop_chapterFinish.Text);
                    //Destination
                    line.WriteLine(text_destination.Text);
                    line.WriteLine(drp_videoEncoder.Text);
                    line.WriteLine(drp_audioCodec.Text);
                    line.WriteLine(text_width.Text);
                    line.WriteLine(text_height.Text);
                    //Picture Settings Tab
                    line.WriteLine(text_top.Text);
                    line.WriteLine(text_bottom.Text);
                    line.WriteLine(text_left.Text);
                    line.WriteLine(text_right.Text);
                    line.WriteLine(drp_subtitle.Text);
                    //Video Settings Tab
                    line.WriteLine(text_bitrate.Text);
                    line.WriteLine(text_filesize.Text);
                    line.WriteLine(slider_videoQuality.Value.ToString());
                    line.WriteLine(check_2PassEncode.CheckState.ToString());
                    line.WriteLine(drp_deInterlace_option.Text);
                    line.WriteLine(check_grayscale.CheckState.ToString());
                    line.WriteLine(drp_videoFramerate.Text);
                    line.WriteLine(Check_ChapterMarkers.CheckState.ToString());
                    line.WriteLine(CheckPixelRatio.CheckState.ToString());
                    line.WriteLine(check_turbo.CheckState.ToString());
                    line.WriteLine(check_largeFile.CheckState.ToString());
                    //Audio Settings Tab
                    line.WriteLine(drp_audioBitrate.Text);
                    line.WriteLine(drp_audioSampleRate.Text);
                    line.WriteLine(drp_audioChannels.Text);
                    line.WriteLine(drp_audioMixDown.Text);
                    //H264 Tab
                    line.WriteLine(CheckCRF.CheckState.ToString());
                    line.WriteLine(rtf_h264advanced.Text);
                    // close the stream
                    line.Close();
                    MessageBox.Show("Your profile has been sucessfully saved.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
                catch (Exception)
                {
                    MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }

            }
        }

        private void btn_setDefault_Click(object sender, EventArgs e)
        {
            //Source
            Properties.Settings.Default.DVDSource = text_source.Text;
            Properties.Settings.Default.DVDTitle = drp_dvdtitle.Text;
            Properties.Settings.Default.ChapterStart = drop_chapterStart.Text;
            Properties.Settings.Default.ChapterFinish = drop_chapterFinish.Text;
            //Destination
            Properties.Settings.Default.VideoDest = text_destination.Text;
            Properties.Settings.Default.VideoEncoder = drp_videoEncoder.Text;
            Properties.Settings.Default.AudioEncoder = drp_audioCodec.Text;
            Properties.Settings.Default.Width = text_width.Text;
            Properties.Settings.Default.Height = text_height.Text;
            //Picture Settings Tab
            Properties.Settings.Default.CroppingOption = drp_crop.Text;
            Properties.Settings.Default.CropTop = text_top.Text;
            Properties.Settings.Default.CropBottom = text_bottom.Text;
            Properties.Settings.Default.CropLeft = text_left.Text;
            Properties.Settings.Default.CropRight = text_right.Text;
            Properties.Settings.Default.Subtitles = drp_subtitle.Text;
            //Video Settings Tab
            Properties.Settings.Default.VideoBitrate = text_bitrate.Text;
            Properties.Settings.Default.VideoFilesize = text_filesize.Text;
            Properties.Settings.Default.VideoQuality = slider_videoQuality.Value;
            Properties.Settings.Default.TwoPass = check_2PassEncode.CheckState.ToString();
            Properties.Settings.Default.DeInterlace = drp_deInterlace_option.Text;
            Properties.Settings.Default.Grayscale = check_grayscale.CheckState.ToString();
            Properties.Settings.Default.Framerate = drp_videoFramerate.Text;
            Properties.Settings.Default.PixelRatio = CheckPixelRatio.CheckState.ToString();
            Properties.Settings.Default.turboFirstPass = check_turbo.CheckState.ToString();
            Properties.Settings.Default.largeFile = check_largeFile.CheckState.ToString();
            Properties.Settings.Default.detelecine = check_detelecine.CheckState.ToString();
            Properties.Settings.Default.denoise = drp_deNoise.Text;
            Properties.Settings.Default.deblock = check_deblock.CheckState.ToString();
            Properties.Settings.Default.chapterMarker = Check_ChapterMarkers.CheckState.ToString();
            //Audio Settings Tab
            Properties.Settings.Default.AudioBitrate = drp_audioBitrate.Text;
            Properties.Settings.Default.AudioSampleRate = drp_audioSampleRate.Text;
            Properties.Settings.Default.AudioChannels = drp_audioChannels.Text;
            //H264 Tab
            Properties.Settings.Default.CRF = CheckCRF.CheckState.ToString();
            Properties.Settings.Default.H264 = rtf_h264advanced.Text;
            try
            {
                Properties.Settings.Default.selectedPreset = ListBox_Presets.SelectedItem.ToString();
            }
            catch (Exception exc)
            {
                // If the user has not selected an item, then an exception may be thrown. Catch and ignore.
            }
            Properties.Settings.Default.Save();
        }

        // Preset Seleciton
        private void ListBox_Presets_SelectedIndexChanged(object sender, EventArgs e)
        {
            string selectedPreset = null;
            if (ListBox_Presets.SelectedItem != null)
            {
                selectedPreset = ListBox_Presets.SelectedItem.ToString();
            }
            else
            {
                selectedPreset = "";
            }

            switch (selectedPreset)
            {
                case "Animation":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "1000", "", 0, "0%", "160", CheckState.Checked, "48", "ref=5:mixed-refs:bframes=6:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=5:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2", "Origional (Fast)", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Apple Animation)");
                    setMkv();
                    break;
                case "AppleTV":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "2500", "", 0, "0%", "160", CheckState.Checked, "48", "bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=2", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: Apple TV)");
                    break;
                case "Bedlam":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "1800", "", 0, "0%", "160", CheckState.Checked, "48", "ref=16:mixed-refs:bframes=6:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:me-range=64:analyse=all:8x8dct:trellis=2:no-fast-pskip:no-dct-decimate:filter=-2,-1", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Bedlam)");
                    setMkv();
                    break;
                case "Blind":
                    setGuiSetttings(CheckState.Unchecked, "512", "", "H.264", "512", "", 0, "0%", "128", CheckState.Checked, "48", "", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: Blind)");
                    break;
                case "Broke":
                    setGuiSetttings(CheckState.Unchecked, "640", "", "H.264", "", "695", 0, "0%", "128", CheckState.Checked, "48", "ref=3:mixed-refs:bframes=6:bime:weightb:b-rdo:b-pyramid::direct=auto:me=umh:subme=6:trellis=1:analyse=all:8x8dct:no-fast-pskip", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Broke)");
                    break;
                case "Classic":
                    setGuiSetttings(CheckState.Unchecked, "", "", "H.264", "1000", "", 0, "0%", "160", CheckState.Unchecked, "48", "", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: Classic)");
                    break;
                case "Constant Quality Rate":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "", "", 64, "64%", "160", CheckState.Checked, "48", "ref=3:mixed-refs:bframes=3:b-pyramid:b-rdo:bime:weightb:filter=-2,-1:subme=6:trellis=1:analyse=all:8x8dct:me=umh", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AC3", "Output Settings (Preset: CQR)");
                    setMkv();
                    break;
                case "Deux Six Quatre":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "1600", "", 0, "0%", "160", CheckState.Checked, "48", "ref=5:mixed-refs:bframes=3:bime:weightb:b-rdo:b-pyramid:me=umh:subme=7:trellis=1:analyse=all:8x8dct:no-fast-pskip", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AC3", "Output Settings (Preset: DSQ)");
                    setMkv();
                    break;
                case "Film":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "2000", "", 0, "0%", "160", CheckState.Checked, "48", "ref=3:mixed-refs:bframes=3:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:no-fast-pskip", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AC3", "Output Settings (Preset: Film)");
                    setMkv();
                    break;
                case "iPhone / iPod Touch":
                    setGuiSetttings(CheckState.Unchecked, "480", "", "H.264 (iPod)", "960", "", 0, "0%", "128", CheckState.Checked, "48", "cabac=0:ref=1:analyse=all:me=umh:subme=6:no-fast-pskip=1:trellis=1", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: iPhone)");
                    break;
                case "iPod High-Rez":
                    setGuiSetttings(CheckState.Unchecked, "640", "", "H.264 (iPod)", "1500", "", 0, "0%", "160", CheckState.Checked, "48", "keyint=300:keyint-min=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: iPod High Rez)");
                    break;
                case "iPod Low-Rez":
                    setGuiSetttings(CheckState.Unchecked, "320", "", "H.264 (iPod)", "700", "", 0, "0%", "160", CheckState.Checked, "48", "keyint=300:keyint-min=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: iPod Low Rez)");
                    break;
                case "Normal":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "1500", "", 0, "0%", "160", CheckState.Checked, "48", "ref=2:bframes=2:subme=5:me=umh", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Normal)");
                    break;
                case "PS3":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "2500", "", 0, "0%", "160", CheckState.Checked, "48", "level=41:subme=5:me=umh", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: PS3)");
                    break;
                case "PSP":
                    setGuiSetttings(CheckState.Unchecked, "368", "208", "Mpeg 4", "1024", "", 0, "0%", "160", CheckState.Unchecked, "48", "", "None", CheckState.Unchecked, "No Crop", CheckState.Unchecked, "AAC", "Output Settings (Preset: PSP)");
                    break;
                case "QuickTime":
                    setGuiSetttings(CheckState.Checked, "", "", "H.264", "2000", "", 0, "0%", "160", CheckState.Checked, "48", "ref=3:mixed-refs:bframes=3:bime:weightb:b-rdo:direct-auto:me=umh:subme=5:analyse=all:8x8dct:trellis=1:no-fast-pskip", "None", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Quicktime)");
                    break;
                case "Television":
                    setGuiSetttings(CheckState.Unchecked, "", "", "H.264", "1300", "", 0, "0%", "160", CheckState.Checked, "48", "ref=3:mixed-refs:bframes=6:bime:weightb:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip", "Origional (Fast)", CheckState.Checked, "No Crop", CheckState.Checked, "AAC", "Output Settings (Preset: Television)");
                    setMkv();
                    break;
                default:
                    break;
            }
        }

        // Functions - It's a bit dirty but i'll sort this out later. Simply done to reduce the amount of code above.
        private void setGuiSetttings(CheckState anamorphic, string width, string height, string vencoder, string bitrate, string filesize, int quality, string qpercent, string audioBit, CheckState chpt, string audioSample, string h264, string deinterlace, CheckState twopass, string crop, CheckState turbo, string audioCodec, string preset)
        {
            CheckPixelRatio.CheckState = anamorphic;
            text_width.Text = width;
            text_height.Text = height;
            drp_videoEncoder.Text = vencoder;
            text_bitrate.Text = bitrate;
            text_filesize.Text = filesize;
            slider_videoQuality.Value = quality;
            SliderValue.Text = qpercent;
            drp_audioBitrate.Text = audioBit;
            Check_ChapterMarkers.CheckState = chpt;
            drp_audioSampleRate.Text = audioSample;
            rtf_h264advanced.Text = h264;
            drp_deInterlace_option.Text = deinterlace;
            check_2PassEncode.CheckState = twopass;
            drp_crop.Text = crop;
            check_turbo.CheckState = turbo;
            drp_audioCodec.Text = audioCodec;

            groupBox_output.Text = preset;
        }

        private void setMkv()
        {
            // Set file extension to MKV
            string destination = text_destination.Text;
            destination = destination.Replace(".mp4", ".mkv");
            destination = destination.Replace(".avi", ".mkv");
            destination = destination.Replace(".m4v", ".mkv");
            destination = destination.Replace(".ogm", ".mkv");
            text_destination.Text = destination;
        }

        #endregion

        //---------------------------------------------------
        // Encode / Cancel Buttons
        // Encode Progress Text Handler
        //---------------------------------------------------

        #region Encode/CLI

        Functions.CLI process = new Functions.CLI();

        private void btn_encode_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                btn_eCancel.Enabled = true;
                String query = "";
                if (QueryEditorText.Text == "")
                {
                    query = GenerateTheQuery();
                }
                else
                {
                    query = QueryEditorText.Text;
                }

                ThreadPool.QueueUserWorkItem(procMonitor, query);
                lbl_encode.Visible = true;
                lbl_encode.Text = "Encoding in Progress";
            }
        }

        private void btn_eCancel_Click(object sender, EventArgs e)
        {
            process.killCLI();
            process.setNull();
            lbl_encode.Text = "Encoding Canceled";
        }

        [DllImport("user32.dll")]
        public static extern void LockWorkStation();
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason); 

        private void procMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (hbProc != null)
            {
                MessageBox.Show("Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                hbProc = process.runCli(this, (string)state, false, false, false, false);
                hbProc.WaitForExit();

                try
                {
                    /*
                    //*****************************************************************************************
                    // BUG!
                    // When the below code is used and standard error is set to true, hbcli is outputing a
                    // video stream which has mild corruption issues every few seconds.
                    // Maybe an issue with the Parser cauing the CLI to hickup/pause?
                    //*****************************************************************************************

                    
                    Parsing.Parser encode = new Parsing.Parser(hbProc.StandardOutput.BaseStream);
                    encode.OnEncodeProgress += encode_OnEncodeProgress;
                    while (!encode.EndOfStream)
                    {
                        encode.ReadLine();
                    }

                    hbProc.WaitForExit();
                    process.closeCLI();
                     */
                    
                }
                catch (Exception exc)
                {
                    // Do nothing
                    MessageBox.Show(exc.ToString());
                }


                setEncodeLabel();
                hbProc = null;

                // Do something whent he encode ends.
                switch (Properties.Settings.Default.CompletionOption)
                {
                    case "Shutdown":
                        System.Diagnostics.Process.Start("Shutdown", "-s -t 60");
                        break;
                    case "Log Off":
                        ExitWindowsEx(0, 0); 
                        break;
                    case "Suspend":
                        Application.SetSuspendState(PowerState.Suspend, true, true);
                        break;
                    case "Hibernate":
                        Application.SetSuspendState(PowerState.Hibernate, true, true);
                        break;
                    case "Lock System":
                        LockWorkStation();
                        break;
                    case "Quit HandBrake":
                        Application.Exit();
                        break;
                    default:
                        break;
                }
            }
        }

        private delegate void UpdateUIHandler();

        private void setEncodeLabel()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new UpdateUIHandler(setEncodeLabel));
                return;
            }
            lbl_encode.Text = "Encoding Finished";
        }

        private void encode_OnEncodeProgress(object Sender, int CurrentTask, int TaskCount, float PercentComplete, float CurrentFps, float AverageFps, TimeSpan TimeRemaining)
        {
            
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Parsing.EncodeProgressEventHandler(encode_OnEncodeProgress),
                    new object[] { Sender, CurrentTask, TaskCount, PercentComplete, CurrentFps, AverageFps, TimeRemaining });
                return;
            }
            lbl_encode.Text = string.Format("Encode Progress: {0}%,       FPS: {1},       Avg FPS: {2},       Time Remaining: {3} ", PercentComplete, CurrentFps, AverageFps, TimeRemaining);
        }

        #endregion

        //---------------------------------------------------
        //  Items that require actions on frmMain
        //---------------------------------------------------

        #region frmMain Actions

        private void drop_chapterStart_SelectedIndexChanged(object sender, EventArgs e)
        {
            drop_chapterStart.BackColor = Color.White;
            QueryEditorText.Text = "";
            if ((drop_chapterFinish.Text != "Auto") && (drop_chapterStart.Text != "Auto"))
            {
                try
                {
                    int chapterFinish = int.Parse(drop_chapterFinish.Text);
                    int chapterStart = int.Parse(drop_chapterStart.Text);

                    if (chapterFinish < chapterStart)
                    {
                        drop_chapterStart.BackColor = Color.LightCoral;
                    }
                }
                catch (Exception)
                {
                    drop_chapterStart.BackColor = Color.LightCoral;
                }
            }

            
        }

        private void drop_chapterFinish_SelectedIndexChanged(object sender, EventArgs e)
        {
            drop_chapterFinish.BackColor = Color.White;
            QueryEditorText.Text = "";
            if ((drop_chapterFinish.Text != "Auto") && (drop_chapterStart.Text != "Auto"))
            {
                try
                {
                    int chapterFinish = int.Parse(drop_chapterFinish.Text);
                    int chapterStart = int.Parse(drop_chapterStart.Text);

                    if (chapterFinish < chapterStart)
                    {
                        drop_chapterFinish.BackColor = Color.LightCoral;
                    }
                }
                catch (Exception)
                {
                    drop_chapterFinish.BackColor = Color.LightCoral;
                }
            }
        }

        private void text_bitrate_TextChanged(object sender, EventArgs e)
        {
            text_filesize.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
            CheckCRF.CheckState = CheckState.Unchecked;
            CheckCRF.Enabled = false;
        }

        private void text_filesize_TextChanged(object sender, EventArgs e)
        {
            text_bitrate.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
            CheckCRF.CheckState = CheckState.Unchecked;
            CheckCRF.Enabled = false;
        }

        private void slider_videoQuality_Scroll(object sender, EventArgs e)
        {
            SliderValue.Text = slider_videoQuality.Value.ToString() + "%";
            text_bitrate.Text = "";
            text_filesize.Text = "";
            CheckCRF.Enabled = true;
        }

        private void label_h264_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/trac/wiki/x264Options");
        }

        private void text_width_TextChanged(object sender, EventArgs e)
        {
            try
            {
                if (CheckPixelRatio.Checked)
                {
                    text_width.Text = "";
                    text_width.BackColor = Color.LightCoral;
                    CheckPixelRatio.BackColor = Color.LightCoral;
                    lbl_anamorphicError.Visible = true;
                }
                else
                {
                    if ((int.Parse(text_width.Text) % 16) != 0)
                    {
                        text_width.BackColor = Color.LightCoral;
                    }
                    else
                    {
                        text_width.BackColor = Color.LightGreen;
                    }
                }

                if (lbl_Aspect.Text != "Select a Title")
                {
                    double height = int.Parse(text_width.Text) / double.Parse(lbl_Aspect.Text);
                    double mod16 = height % 16;
                    height = height - mod16;

                    if (text_width.Text == "")
                    {
                        text_height.Text = "";
                        text_width.BackColor = Color.White;
                    }
                    else
                    {
                        text_height.Text = height.ToString();
                    }
                }                   
            }
            catch (Exception)
            {
                // No need to throw an error here.
                // Note on non english systems, this will throw an error because of double.Parse(lbl_Aspect.Text); not working.
            }
               
          
        }

        private void text_height_TextChanged(object sender, EventArgs e)
        {
            try
            {
                if (CheckPixelRatio.Checked)
                    {
                        text_height.Text = "";
                        text_height.BackColor = Color.LightCoral;
                        CheckPixelRatio.BackColor = Color.LightCoral;
                        lbl_anamorphicError.Visible = true;
                }
                else
                {
                    if ((int.Parse(text_height.Text) % 16) != 0)
                    {
                            text_height.BackColor = Color.LightCoral;
                    }
                    else
                    {
                            text_height.BackColor = Color.LightGreen;
                    }
                }

            } catch(Exception){
                // No need to alert the user.
            }
        }

        private void drp_crop_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((string)drp_crop.SelectedItem == "Manual")
            {
            text_left.Enabled = true;
            text_right.Enabled = true;
            text_top.Enabled = true;
            text_bottom.Enabled = true;
            }

            if ((string)drp_crop.SelectedItem == "Auto Crop")
            {
                text_left.Enabled = false;
                text_right.Enabled = false;
                text_top.Enabled = false;
                text_bottom.Enabled = false;
                text_left.Text = "";
                text_right.Text = "";
                text_top.Text = "";
                text_bottom.Text = "";

                if (lbl_RecomendedCrop.Text != "Select a Title")
                {
                    string[] temp = new string[4];
                    temp = lbl_RecomendedCrop.Text.Split('/');
                    text_left.Text = temp[2];
                    text_right.Text = temp[3];
                    text_top.Text = temp[0];
                    text_bottom.Text = temp[1];
                }
            }

            if ((string)drp_crop.SelectedItem == "No Crop")
            {
                text_left.Enabled = false;
                text_right.Enabled = false;
                text_top.Enabled = false;
                text_bottom.Enabled = false;
                text_left.Text = "0";
                text_right.Text = "0";
                text_top.Text = "0";
                text_bottom.Text = "0";

            }
        }
        
        private void CheckPixelRatio_CheckedChanged(object sender, EventArgs e)
        {
            text_width.Text = "";
            text_height.Text = "";
            text_width.BackColor = Color.White;
            text_height.BackColor = Color.White;
            CheckPixelRatio.BackColor = frmMain.DefaultBackColor;
            lbl_anamorphicError.Visible = false;
        }

        private void check_2PassEncode_CheckedChanged(object sender, EventArgs e)
        {
            if (check_2PassEncode.CheckState.ToString() == "Checked")
            {
                check_turbo.Enabled = true;
            }
            else
            {
                check_turbo.Enabled = false;
                check_turbo.CheckState = CheckState.Unchecked;
            }
        }

        private void check_largeFile_CheckedChanged(object sender, EventArgs e)
        {
            if (!text_destination.Text.Contains(".mp4"))
            {
                lbl_largeMp4Warning.Text = "Warning: Only mp4 files are supported";
                lbl_largeMp4Warning.ForeColor = Color.Red;
                check_largeFile.CheckState = CheckState.Unchecked;
            }
            else
            {
                lbl_largeMp4Warning.Text = "Warning: Breaks iPod, @TV, PS3 compatibility.";
                lbl_largeMp4Warning.ForeColor = Color.Black;
            }
        }

        private void drp_dvdtitle_Click(object sender, EventArgs e)
        {
            if (drp_dvdtitle.Items.Count == 0)
            {
                MessageBox.Show("There are no titles to select. Please scan the DVD by clicking the 'browse' button above before trying to select a title.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
            }
        }

        private void drp_audioCodec_SelectedIndexChanged(object sender, EventArgs e)
        {

            //CLI Audio mixdown Names: mono stereo dpl1 dpl2 6ch

            drp_audioMixDown.Items.Clear();

            if (drp_audioCodec.Text == "AAC")
            {
                drp_audioMixDown.Items.Clear();
                drp_audioMixDown.Items.Add("Mono");
                drp_audioMixDown.Items.Add("Stereo");
                drp_audioMixDown.Items.Add("Dolby Surround");
                drp_audioMixDown.Items.Add("Dolby Pro Logic II");
                drp_audioMixDown.Items.Add("6 Channel Discrete");
                
                drp_audioBitrate.Items.Clear();
                drp_audioBitrate.Items.Add("32");
                drp_audioBitrate.Items.Add("40");
                drp_audioBitrate.Items.Add("48");
                drp_audioBitrate.Items.Add("56");
                drp_audioBitrate.Items.Add("64");
                drp_audioBitrate.Items.Add("80");
                drp_audioBitrate.Items.Add("86");
                drp_audioBitrate.Items.Add("112");
                drp_audioBitrate.Items.Add("128");
                drp_audioBitrate.Items.Add("160");

            }
            else
            {
                drp_audioMixDown.Items.Clear();
                drp_audioMixDown.Items.Add("Stereo");
                drp_audioMixDown.Items.Add("Dolby Surround");
                drp_audioMixDown.Items.Add("Dolby Pro Logic II");

                drp_audioBitrate.Items.Clear();
                drp_audioBitrate.Items.Add("32");
                drp_audioBitrate.Items.Add("40");
                drp_audioBitrate.Items.Add("48");
                drp_audioBitrate.Items.Add("56");
                drp_audioBitrate.Items.Add("64");
                drp_audioBitrate.Items.Add("80");
                drp_audioBitrate.Items.Add("86");
                drp_audioBitrate.Items.Add("112");
                drp_audioBitrate.Items.Add("128");
                drp_audioBitrate.Items.Add("160");
                drp_audioBitrate.Items.Add("192");
                drp_audioBitrate.Items.Add("224");
                drp_audioBitrate.Items.Add("256");
                drp_audioBitrate.Items.Add("320");
            }
        }

        private void drp_audioMixDown_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_audioCodec.Text == "AAC")
            {
                if (drp_audioMixDown.Text == "6 Channel Discrete")
                {

                    drp_audioBitrate.Items.Clear();
                    drp_audioBitrate.Items.Add("32");
                    drp_audioBitrate.Items.Add("40");
                    drp_audioBitrate.Items.Add("48");
                    drp_audioBitrate.Items.Add("56");
                    drp_audioBitrate.Items.Add("64");
                    drp_audioBitrate.Items.Add("80");
                    drp_audioBitrate.Items.Add("86");
                    drp_audioBitrate.Items.Add("112");
                    drp_audioBitrate.Items.Add("128");
                    drp_audioBitrate.Items.Add("160");
                    drp_audioBitrate.Items.Add("192");
                    drp_audioBitrate.Items.Add("224");
                    drp_audioBitrate.Items.Add("256");
                    drp_audioBitrate.Items.Add("320");
                    drp_audioBitrate.Items.Add("384");
                }
            }
        }

        private void Check_ChapterMarkers_CheckedChanged(object sender, EventArgs e)
        {
            if (Check_ChapterMarkers.Checked)
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".mp4", ".m4v");
                text_destination.Text = destination;
            }
            else
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".m4v", ".mp4");
                text_destination.Text = destination;
            }
        }



        private void drp_videoEncoder_SelectedIndexChanged(object sender, EventArgs e)
        {
            //Turn off some options which are H.264 only when the user selects a non h.264 encoder
            if (!drp_videoEncoder.Text.Contains("H.264"))
            {
                check_turbo.CheckState = CheckState.Unchecked;
                CheckCRF.CheckState = CheckState.Unchecked;
                CheckCRF.Enabled = false;
                check_turbo.Enabled = false;
                h264Tab.Enabled = false;
                rtf_h264advanced.Text = "";
            }
            else
            {
                CheckCRF.Enabled = true;
                check_turbo.Enabled = true;
                h264Tab.Enabled = true;
            }

        }

        public void setStreamReader(Parsing.DVD dvd)
        {
            this.thisDVD = dvd;
        }

        private void drp_dvdtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Reset some values on the form
            lbl_Aspect.Text = "Select a Title";
            lbl_RecomendedCrop.Text = "Select a Title";
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();
            QueryEditorText.Text = "";

            // If the dropdown is set to automatic nothing else needs to be done.
            // Otheriwse if its not, title data has to be loased from parsing.
            if (drp_dvdtitle.Text != "Automatic")
            {
                Parsing.Title selectedTitle = drp_dvdtitle.SelectedItem as Parsing.Title;

                // Set the Aspect Ratio
                lbl_Aspect.Text = selectedTitle.AspectRatio.ToString();

                // Set the Recommended Cropping values
                lbl_RecomendedCrop.Text = string.Format("{0}/{1}/{2}/{3}", selectedTitle.AutoCropDimensions[0], selectedTitle.AutoCropDimensions[1], selectedTitle.AutoCropDimensions[2], selectedTitle.AutoCropDimensions[3]);
                
                // Populate the Start chapter Dropdown
                drop_chapterStart.Items.Clear();
                drop_chapterStart.Items.AddRange(selectedTitle.Chapters.ToArray());
                if (drop_chapterStart.Items.Count > 0)
                {
                    drop_chapterStart.Text = drop_chapterStart.Items[0].ToString();
                }

                // Populate the Final Chapter Dropdown
                drop_chapterFinish.Items.Clear();
                drop_chapterFinish.Items.AddRange(selectedTitle.Chapters.ToArray());
                if (drop_chapterFinish.Items.Count > 0)
                {
                    drop_chapterFinish.Text = drop_chapterFinish.Items[drop_chapterFinish.Items.Count - 1].ToString();
                }

                // Populate the Audio Channels Dropdown
                drp_audioChannels.Items.Clear();
                drp_audioChannels.Items.Add("Automatic");
                drp_audioChannels.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                if (drp_audioChannels.Items.Count > 0)
                {
                    drp_audioChannels.Text = drp_audioChannels.Items[0].ToString();
                }

                // Populate the Subtitles dropdown
                drp_subtitle.Items.Clear();
                drp_subtitle.Items.Add("None");
                drp_subtitle.Items.AddRange(selectedTitle.Subtitles.ToArray());
                if (drp_subtitle.Items.Count > 0)
                {
                    drp_subtitle.Text = drp_subtitle.Items[0].ToString();
                }
            }
        }

        #endregion

        //---------------------------------------------------
        //  Some Functions
        //  - Query Generation
        //---------------------------------------------------

        #region Program Functions

        public string GenerateTheQuery()
        {
            string source = text_source.Text;
            string dvdTitle = drp_dvdtitle.Text;
            string chapterStart = drop_chapterStart.Text;
            string chapterFinish = drop_chapterFinish.Text;
            int totalChapters = drop_chapterFinish.Items.Count - 1;
            string dvdChapter = "";

            source = " -i " + '"' + source+ '"';

            if (dvdTitle ==  "Automatic")
                dvdTitle = "";
            else
            {
                string[] titleInfo = dvdTitle.Split(' ');
                dvdTitle = " -t "+ titleInfo[0];
            }

            if (chapterFinish.Equals("Auto") && chapterStart.Equals("Auto"))
                dvdChapter = "";
            else if (chapterFinish == chapterStart)
                dvdChapter = " -c " + chapterStart;
            else
                dvdChapter = " -c " + chapterStart + "-" + chapterFinish;

            string querySource = source+ dvdTitle+ dvdChapter;
            // ----------------------------------------------------------------------

            // Destination

            string destination = text_destination.Text;
            string videoEncoder = drp_videoEncoder.Text;
            string audioEncoder = drp_audioCodec.Text;
            string width = text_width.Text;
            string height = text_height.Text;

            if (destination ==  "")
                MessageBox.Show("No destination has been selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
                destination = " -o " + '"' + destination + '"'; //'"'+ 


            switch (videoEncoder)
            {
                case "Mpeg 4":
                    videoEncoder = " -e ffmpeg";
                    break;
                case "Xvid":
                    videoEncoder = " -e xvid";
                    break;
                case "H.264":
                    videoEncoder = " -e x264";
                    break;
                case "H.264 Baseline 1.3":
                    videoEncoder = " -e x264b13";
                    break;
                case "H.264 (iPod)":
                    videoEncoder = " -e x264b30";
                    break;
                default:
                    videoEncoder = " -e x264";
                    break;
            }
           
            switch (audioEncoder)
            {
                case "AAC":
                    audioEncoder = " -E faac";
                    break;
                case "MP3":
                    audioEncoder = " -E lame";
                    break;
                case "Vorbis":
                    audioEncoder = " -E vorbis";
                    break;
                case "AC3":
                    audioEncoder = " -E ac3";
                    break;
                default:
                    audioEncoder = " -E faac";
                    break;
            }

            if (width !=  "")
                width = " -w "+ width;


            if (height == "Auto")
            {
                height = "";
            }
            else if (height != "")
            {
                height = " -l " + height;
            }
            

            string queryDestination = destination+ videoEncoder+ audioEncoder+ width+ height;
            // ----------------------------------------------------------------------

            // Picture Settings Tab

            string cropSetting = drp_crop.Text;
            string cropTop = text_top.Text;
            string cropBottom = text_bottom.Text;
            string cropLeft = text_left.Text;
            string cropRight = text_right.Text;
            string subtitles = drp_subtitle.Text;
            string cropOut = "";
            string deInterlace_Option = drp_deInterlace_option.Text;
            string deinterlace = "";
            string grayscale = "";
            string pixelRatio = "";
            string ChapterMarkers = "";
            // Returns Crop Query

            if (cropSetting == "Auto Crop")
                cropOut = "";
            else if (cropSetting == "No Crop")
                cropOut = " --crop 0:0:0:0 ";
            else
            {
                if (text_top.Text == "")
                    cropTop = "0";
                if (text_bottom.Text == "")
                    cropBottom = "0";
                if (text_left.Text == "")
                    cropLeft = "0";
                if (text_right.Text == "")
                    cropRight = "0";

                cropOut = " --crop " + cropTop + ":" + cropBottom + ":" + cropLeft + ":" + cropRight;
            }

            if (subtitles ==  "None")
                subtitles = "";
            else if (subtitles ==  "")
                subtitles = "";
            else
            {
                string[] tempSub;
                tempSub = subtitles.Split(' ');
                subtitles = " -s "+ tempSub[0];
            }

            switch (deInterlace_Option)
            {
                case "None":
                    deinterlace = "";
                    break;
                case "Original (Fast)":
                    deinterlace = " --deinterlace=" + '"' + "-1" + '"';
                    break;
                case "yadif (Slow)":
                    deinterlace = " --deinterlace=" + '"' + "0" + '"';
                    break;
                case "yadif + mcdeint (Slower)":
                    deinterlace = " --deinterlace=" + '"' + "2:-1:1" + '"';
                    break;
                case "yadif + mcdeint (Slowest)":
                    deinterlace = " --deinterlace=" + '"' + "1:-1:1" + '"';
                    break;
                default:
                    deinterlace = "";
                    break;
            }

            if (check_grayscale.Checked)
                grayscale = " -g ";

            if (CheckPixelRatio.Checked)
                pixelRatio = " -p ";

            if (Check_ChapterMarkers.Checked)
                ChapterMarkers = " -m ";

            string queryPictureSettings = cropOut + subtitles + deinterlace + grayscale + pixelRatio + ChapterMarkers;
            // ----------------------------------------------------------------------

            // Video Settings Tab

            string videoBitrate = text_bitrate.Text;
            string videoFilesize = text_filesize.Text;
            double videoQuality = slider_videoQuality.Value;
            string vidQSetting = "";
            string twoPassEncoding = "";
            string videoFramerate = drp_videoFramerate.Text;
            string turboH264 = "";
            string largeFile = "";
            string deblock = "";
            string detelecine = "";
            string denoise = "";
            string CRF = CheckCRF.CheckState.ToString();

            if (CRF == "Checked")
                CRF = " -Q ";
            else
                CRF = "";

            if (videoBitrate !=  "")
                videoBitrate = " -b "+ videoBitrate;

            if (videoFilesize !=  "")
                videoFilesize = " -S "+ videoFilesize;

            // Video Quality Setting

            if ((videoQuality ==  0))
                vidQSetting = "";
            else
            {
                videoQuality = videoQuality / 100;
                if (videoQuality ==  1)
                {
                    vidQSetting = "1.0";
                }

                vidQSetting = " -q " + videoQuality.ToString();
            }

            if (check_2PassEncode.Checked)
                twoPassEncoding = " -2 ";

            if (videoFramerate ==  "Automatic")
                videoFramerate = "";
            else
                videoFramerate = " -r "+ videoFramerate;

            if (check_turbo.Checked)
                turboH264 = " -T ";

            if (check_largeFile.Checked)
                largeFile = " -4 ";

            if (check_deblock.Checked)
                deblock = " --deblock";

            if (check_detelecine.Checked)
                detelecine = " --detelecine";

            switch (drp_deNoise.Text)
            {
                case "None":
                    denoise = "";
                    break;
                case "Weak":
                    denoise = " --denoise=2:1:2:3";
                    break;
                case "Medium":
                    denoise = " --denoise=3:2:2:3";
                    break;
                case "Strong":
                    denoise = " --denoise=7:7:5:5";
                    break;
                default:
                    denoise = "";
                    break;
            }

            string queryVideoSettings = videoBitrate + videoFilesize + vidQSetting + CRF + twoPassEncoding + videoFramerate + turboH264 + largeFile + deblock + detelecine + denoise;
            // ----------------------------------------------------------------------

            // Audio Settings Tab

            string audioBitrate = drp_audioBitrate.Text;
            string audioSampleRate = drp_audioSampleRate.Text;
            string audioChannels = drp_audioChannels.Text;
            string Mixdown = drp_audioMixDown.Text;
            string SixChannelAudio = "";

            if (audioBitrate !=  "")
                audioBitrate = " -B "+ audioBitrate;

            if (audioSampleRate !=  "")
                audioSampleRate = " -R "+ audioSampleRate;

            if (audioChannels ==  "Automatic")
                audioChannels = "";
            else if (audioChannels ==  "")
                audioChannels = "";
            else
            {
                string[] tempSub;
                tempSub = audioChannels.Split(' ');
                audioChannels = " -a "+ tempSub[0];
            }

            switch (Mixdown)
            {
                case "Automatic":
                    Mixdown = "";
                    break;
                case "Mono":
                    Mixdown = "mono";
                    break;
                case "Stereo":
                    Mixdown = "stereo";
                    break;
                case "Dolby Surround":
                    Mixdown = "dpl1";
                    break;
                case "Dolby Pro Logic II":
                    Mixdown = "dpl2";
                    break;
                case "6 Channel Discrete":
                    Mixdown = "6ch";
                    break;
                default:
                    Mixdown = "";
                    break;
            }

            if (Mixdown !=  "")
                SixChannelAudio = " -6 "+ Mixdown;
            else
                SixChannelAudio = "";

            string queryAudioSettings = audioBitrate+ audioSampleRate+ audioChannels+ SixChannelAudio;
            // ----------------------------------------------------------------------

            //  H.264 Tab

            
            string h264Advanced = rtf_h264advanced.Text;
            
            if ((h264Advanced ==  ""))
                h264Advanced = "";
            else
                h264Advanced = " -x "+ h264Advanced;
    

            string h264Settings = h264Advanced;
            // ----------------------------------------------------------------------

            // Processors (Program Settings)

            string processors = Properties.Settings.Default.Processors;
            //  Number of Processors Handler

            if (processors ==  "Automatic")
                processors = "";
            else
                processors = " -C "+ processors+ " ";


            string queryAdvancedSettings = processors;
            // ----------------------------------------------------------------------

            //  Verbose option (Program Settings)

            string verbose = "";
            if (Properties.Settings.Default.verbose ==  "Checked")
                verbose = " -v ";

            // ----------------------------------------------------------------------

            return querySource+ queryDestination+ queryPictureSettings+ queryVideoSettings+ h264Settings+ queryAudioSettings+ queryAdvancedSettings+ verbose;
        }

        #endregion



        private Functions.QueryParser thisQuery;
        private void button1_Click(object sender, EventArgs e)
        {
            String query = "";
            if (QueryEditorText.Text == "")
            {
                query = GenerateTheQuery();
            }
            else
            {
                query = QueryEditorText.Text;
            }
            thisQuery = Functions.QueryParser.Parse(query);
            MessageBox.Show(thisQuery.DeTelecine.ToString());
        }




        // This is the END of the road ------------------------------------------------------------------------------
    }
}
