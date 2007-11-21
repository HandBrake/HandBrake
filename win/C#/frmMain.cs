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
using System.Globalization;
using System.Text.RegularExpressions;


namespace Handbrake
{
    public partial class frmMain : Form
    {
        // -------------------------------------------------------------- 
        // Applicaiton Startup Stuff
        // --------------------------------------------------------------

        #region Application Startup

        // Some stuff that needs to be Initialized. 
        private Process hbProc;
        private Parsing.DVD thisDVD;
        private frmQueue queueWindow = new frmQueue();

        // The main window beings here...
        public frmMain()
        {
            // Load the splash screen in this thread
            Form splash = new frmSplashScreen();
            splash.Show();

            //Create a label that can be updated from the parent thread.
            Label lblStatus = new Label();

            //Size the label
            lblStatus.Size = new Size(250, 20);

            //Position the label
            lblStatus.Location = new Point(10, 280);

            //Put the label on the splash form
            splash.Controls.Add(lblStatus);

            //Fire a thread to wait for 2 seconds.  The splash screen will exit when the time expires
            Thread timer = new Thread(splashTimer);
            timer.Start();

            InitializeComponent();

            // show the form, but leave disabled until preloading is complete
            this.Enabled = false;

            // Show the main form
            this.Show();

            // Forces frmMain to draw
            Application.DoEvents();

            // Set the Version number lable to the corect version.
            Version.Text = "Version " + Properties.Settings.Default.CliVersion;

            // update the status
            if (Properties.Settings.Default.updateStatus == "Checked")
            {
                lblStatus.Text = "Checking for updates ...";
                // redraw the splash screen
                Application.DoEvents();
                // Run the update checker.
                updateCheck();
                Thread.Sleep(200);
            }

            // Update the presets
            if (Properties.Settings.Default.updatePresets == "Checked")
            {
                lblStatus.Text = "Updaing Presets ...";
                Application.DoEvents();
                updatePresets();
                Thread.Sleep(200);
            }

            // Now load the users default if required. (Will overide the above setting)
            if (Properties.Settings.Default.defaultSettings == "Checked")
            {
                lblStatus.Text = "Loading User Default Settings...";
                Application.DoEvents();
                loadNormalPreset();
                loadUserDefaults();
                Thread.Sleep(100);
            }

            // Enable or disable tooltips
            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                lblStatus.Text = "Loading Tooltips ...";
                Application.DoEvents();
                ToolTip.Active = true;
                Thread.Sleep(100);
            }

            // Hide the preset bar if required.
            hidePresetBar();

            //Finished Loading
            lblStatus.Text = "Loading Complete!";
            Application.DoEvents();
            Thread.Sleep(200);

            // Wait until splash screen is done
            while (timer.IsAlive)
            { Thread.Sleep(100); }

            //Close the splash screen
            splash.Close();
            splash.Dispose();

            // Turn the interface back to the user
            this.Enabled = true;
        }

        private void splashTimer(object sender)
        {
            Thread.Sleep(1000);  //sit for 1 seconds then exit
        }

        private void showSplash(object sender)
        {
            // Display splash screen for 1.5 Seconds
            Form splash = new frmSplashScreen();
            splash.Show();
            Thread.Sleep(1500);
            splash.Close(); // Then close.
        }

        private void loadNormalPreset()
        {
            ListViewItem item = listview_presets.FindItemWithText("Normal");

            if (item != null)
            {
                //listview_presets.SelectedItems.Clear();
                item.Selected = true;
            }
        }

        private void loadUserDefaults()
        {
            string userDefaults = Properties.Settings.Default.defaultUserSettings;
            try
            {
                // Some things that need to be done to reset some gui components:
                CheckPixelRatio.CheckState = CheckState.Unchecked;

                // Send the query from the file to the Query Parser class
                Functions.QueryParser presetQuery = Functions.QueryParser.Parse(userDefaults);

                // Now load the preset
                presetLoader(presetQuery, "User Defaults ");
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to load profile.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                MessageBox.Show(exc.ToString());
            }
        }

        private void hidePresetBar()
        {
            if (Properties.Settings.Default.hidePresets == "Checked")
            {
                mnu_showPresets.Visible = true;
                this.Width = 591;
            }

        }

        #endregion

        // -------------------------------------------------------------- 
        // The main Menu bar.
        // -------------------------------------------------------------- 

        #region File Menu
        private void mnu_open_Click(object sender, EventArgs e)
        {
            string filename;
            File_Open.ShowDialog();
            filename = File_Open.FileName;

            if (filename != "")
            {
                try
                {
                    // Some things that need to be done to reset some gui components:
                    CheckPixelRatio.CheckState = CheckState.Unchecked;

                    //---------------------------
                    // Get the Preset
                    // ---------------------------

                    // Create StreamReader & open file
                    StreamReader line = new StreamReader(filename);

                    // Send the query from the file to the Query Parser class
                    Functions.QueryParser presetQuery = Functions.QueryParser.Parse(line.ReadLine());

                    // Now load the preset
                    presetLoader(presetQuery, filename);

                    // Close the stream
                    line.Close();
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Unable to load profile.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    MessageBox.Show(exc.ToString());
                }
            }
        }

        private void mnu_save_Click(object sender, EventArgs e)
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

                    // Generate and write the query string to the file
                    String query = GenerateTheQuery();
                    line.WriteLine(query);

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
        Boolean presetStatus;

        private void mnu_showPresets_Click(object sender, EventArgs e)
        {
            if (presetStatus == false)
            {
                this.Width = 881;
                presetStatus = true;
                mnu_showPresets.Text = "Hide Presets";
            }
            else
            {
                this.Width = 590;
                presetStatus = false;
                mnu_showPresets.Text = "Show Presets";
            }
        }

        private void mnu_presetReset_Click(object sender, EventArgs e)
        {
            listview_presets.Items.Clear();
            updatePresets();
            MessageBox.Show("Presets have been updated", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            ListViewItem item = listview_presets.FindItemWithText("Normal");

            if (item != null)
            {
                listview_presets.SelectedItems.Clear();
                item.Selected = true;
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
                frmUpdater updateWindow = new frmUpdater();
                updateWindow.Show();
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
            // This removes the file extension from the filename box on the save file dialog.
            // It's daft but some users don't realise that typing an extension overrides the dropdown extension selected.
            // Should be a nicer way to do this.
            DVD_Save.FileName = DVD_Save.FileName.Replace(".mp4", "").Replace(".m4v", "").Replace(".mkv", "").Replace(".ogm", "").Replace(".avi", "");

            // Show the dialog and set the main form file path
            DVD_Save.ShowDialog();
            text_destination.Text = DVD_Save.FileName;

            // Quicktime requires .m4v file for chapter markers to work. If checked, change the extension to .m4v (mp4 and m4v are the same thing)
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

        // Import Current Presets
        private void updatePresets()
        {
            string[] presets = new string[17];
            presets[0] = "Animation";
            presets[1] = "AppleTV";
            presets[2] = "Bedlam";
            presets[3] = "Blind";
            presets[4] = "Broke";
            presets[5] = "Classic";
            presets[6] = "Constant Quality Rate";
            presets[7] = "Deux Six Quatre";
            presets[8] = "Film";
            presets[9] = "iPhone / iPod Touch";
            presets[10] = "iPod High-Rez";
            presets[11] = "iPod Low-Rez";
            presets[12] = "Normal";
            presets[13] = "PS3";
            presets[14] = "PSP";
            presets[15] = "QuickTime";
            presets[16] = "Television";

            ListViewItem preset_listview = new ListViewItem();
            string[] presetList = new string[1];

            foreach (string preset in presets)
            {
                presetList[0] = preset;
                preset_listview = new ListViewItem(presetList);

                // Now Fill Out List View with Items
                listview_presets.Items.Add(preset_listview);
            }

            string appPath = Application.StartupPath.ToString() + "\\";
            string strCmdLine = "cmd /c " + '"' + '"' + appPath + "HandBrakeCLI.exe" + '"' + " --preset-list >" + '"' + appPath + "presets.dat" + '"' + " 2>&1" + '"';
            Process hbproc = Process.Start("CMD.exe", strCmdLine);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();

        }

        // Buttons
        private void btn_setDefault_Click(object sender, EventArgs e)
        {
            String query = GenerateTheQuery();
            Properties.Settings.Default.defaultUserSettings = query;
            // Save the new default Settings
            Properties.Settings.Default.Save();
            MessageBox.Show("New default settings saved.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
        }

        // Preset Selection
        private void listview_presets_SelectedIndexChanged(object sender, EventArgs e)
        {

            string selectedPreset = null;
            ListView.SelectedListViewItemCollection name = null;
            name = listview_presets.SelectedItems;

            if (listview_presets.SelectedItems.Count != 0)
                selectedPreset = name[0].SubItems[0].Text;

            try
            {
                string appPath = Application.StartupPath.ToString() + "\\";
                StreamReader presetInput = new StreamReader(appPath + "presets.dat");

                while (!presetInput.EndOfStream)
                {
                    if ((char)presetInput.Peek() == '+')
                    {
                        string preset = presetInput.ReadLine().Replace("+ ", "");
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(preset);

                        if (selectedPreset == presetName[0])
                        {
                            // Need to disable anamorphic now, otherwise it may overide the width / height values later.
                            CheckPixelRatio.CheckState = CheckState.Unchecked;

                            // Send the query from the file to the Query Parser class
                            Functions.QueryParser presetQuery = Functions.QueryParser.Parse(preset);

                            // Now load the preset
                            presetLoader(presetQuery, selectedPreset);
                        }

                    }
                    else
                    {
                        presetInput.ReadLine();
                    }
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }


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
                        text_width.BackColor = Color.LightGreen;
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

            }
            catch (Exception)
            {
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
            CheckPixelRatio.BackColor = TabPage1.BackColor;
        }

        private void check_2PassEncode_CheckedChanged(object sender, EventArgs e)
        {
            if (check_2PassEncode.CheckState.ToString() == "Checked")
            {
                if (drp_videoEncoder.Text.Contains("H.264"))
                {
                    check_turbo.Enabled = true;
                }
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
                if (check_2PassEncode.CheckState.ToString() == "Checked")
                {
                    check_turbo.Enabled = true;
                }
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
            // Source tab
            #region source
            string source = text_source.Text;
            string dvdTitle = drp_dvdtitle.Text;
            string chapterStart = drop_chapterStart.Text;
            string chapterFinish = drop_chapterFinish.Text;
            int totalChapters = drop_chapterFinish.Items.Count - 1;
            string dvdChapter = "";

            source = " -i " + '"' + source + '"';

            if (dvdTitle == "Automatic")
                dvdTitle = "";
            else
            {
                string[] titleInfo = dvdTitle.Split(' ');
                dvdTitle = " -t " + titleInfo[0];
            }

            if (chapterFinish.Equals("Auto") && chapterStart.Equals("Auto"))
                dvdChapter = "";
            else if (chapterFinish == chapterStart)
                dvdChapter = " -c " + chapterStart;
            else
                dvdChapter = " -c " + chapterStart + "-" + chapterFinish;

            string querySource = source + dvdTitle + dvdChapter;
            #endregion

            // Destination tab
            #region Destination

            string destination = text_destination.Text;
            string videoEncoder = drp_videoEncoder.Text;
            string audioEncoder = drp_audioCodec.Text;
            string width = text_width.Text;
            string height = text_height.Text;

            if (destination == "")
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

            if (width != "")
                width = " -w " + width;


            if (height == "Auto")
            {
                height = "";
            }
            else if (height != "")
            {
                height = " -l " + height;
            }


            string queryDestination = destination + videoEncoder + audioEncoder + width + height;
            #endregion

            // Picture Settings Tab
            #region Picture Settings Tab

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

            if (subtitles == "None")
                subtitles = "";
            else if (subtitles == "")
                subtitles = "";
            else
            {
                string[] tempSub;
                tempSub = subtitles.Split(' ');
                subtitles = " -s " + tempSub[0];
            }

            switch (deInterlace_Option)
            {
                case "None":
                    deinterlace = "";
                    break;
                case "Original (Fast)":
                    deinterlace = " --deinterlace=fast";
                    break;
                case "yadif (Slow)":
                    deinterlace = " --deinterlace=slow";
                    break;
                case "yadif + mcdeint (Slower)":
                    deinterlace = " --deinterlace=slower";
                    break;
                case "yadif + mcdeint (Slowest)":
                    deinterlace = " --deinterlace=slowest";
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
            #endregion

            // Video Settings Tab
            #region Video Settings Tab

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

            if (videoBitrate != "")
                videoBitrate = " -b " + videoBitrate;

            if (videoFilesize != "")
                videoFilesize = " -S " + videoFilesize;

            // Video Quality Setting

            if ((videoQuality == 0))
                vidQSetting = "";
            else
            {
                videoQuality = videoQuality / 100;
                if (videoQuality == 1)
                {
                    vidQSetting = "1.0";
                }
                vidQSetting = " -q " + videoQuality.ToString(new CultureInfo("en-US"));
            }

            if (check_2PassEncode.Checked)
                twoPassEncoding = " -2 ";

            if (videoFramerate == "Automatic")
                videoFramerate = "";
            else
                videoFramerate = " -r " + videoFramerate;

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
                    denoise = " --denoise=weak";
                    break;
                case "Medium":
                    denoise = " --denoise=medium";
                    break;
                case "Strong":
                    denoise = " --denoise=strong";
                    break;
                default:
                    denoise = "";
                    break;
            }

            string queryVideoSettings = videoBitrate + videoFilesize + vidQSetting + CRF + twoPassEncoding + videoFramerate + turboH264 + largeFile + deblock + detelecine + denoise;
            #endregion

            // Audio Settings Tab
            #region Audio Settings Tab

            string audioBitrate = drp_audioBitrate.Text;
            string audioSampleRate = drp_audioSampleRate.Text;
            string audioChannels = drp_audioChannels.Text;
            string Mixdown = drp_audioMixDown.Text;
            string SixChannelAudio = "";

            if (audioBitrate != "")
                audioBitrate = " -B " + audioBitrate;

            if (audioSampleRate != "")
                audioSampleRate = " -R " + audioSampleRate;

            if (audioChannels == "Automatic")
                audioChannels = "";
            else if (audioChannels == "")
                audioChannels = "";
            else
            {
                string[] tempSub;
                tempSub = audioChannels.Split(' ');
                audioChannels = " -a " + tempSub[0];
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

            if (Mixdown != "")
                SixChannelAudio = " -6 " + Mixdown;
            else
                SixChannelAudio = "";

            string queryAudioSettings = audioBitrate + audioSampleRate + audioChannels + SixChannelAudio;
            #endregion

            // H264 Tab
            #region  H264 Tab

            string h264Advanced = rtf_h264advanced.Text;

            if ((h264Advanced == ""))
                h264Advanced = "";
            else
                h264Advanced = " -x " + h264Advanced;


            string h264Settings = h264Advanced;
            #endregion

            // Other
            #region Processors / Other

            string processors = Properties.Settings.Default.Processors;
            //  Number of Processors Handler

            if (processors == "Automatic")
                processors = "";
            else
                processors = " -C " + processors + " ";


            string queryAdvancedSettings = processors;

            string verbose = "";
            if (Properties.Settings.Default.verbose == "Checked")
                verbose = " -v ";
            #endregion

            return querySource + queryDestination + queryPictureSettings + queryVideoSettings + h264Settings + queryAudioSettings + queryAdvancedSettings + verbose;
        }

        private void presetLoader(Functions.QueryParser presetQuery, string name)
        {
            // ---------------------------
            // Setup the GUI
            // ---------------------------

            // Source tab
            #region source
            if (presetQuery.Source != "")
                text_source.Text = presetQuery.Source;

            if (presetQuery.DVDTitle != 0)
                drp_dvdtitle.Text = presetQuery.DVDTitle.ToString();

            if (presetQuery.DVDChapterStart != 0)
                drop_chapterStart.Text = presetQuery.DVDChapterStart.ToString();

            if (presetQuery.DVDChapterFinish != 0)
                drop_chapterFinish.Text = presetQuery.DVDChapterFinish.ToString();

            if (presetQuery.Format != "")
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".mp4", "." + presetQuery.Format);
                destination = destination.Replace(".m4v", "." + presetQuery.Format);
                destination = destination.Replace(".avi", "." + presetQuery.Format);
                destination = destination.Replace(".mkv", "." + presetQuery.Format);
                destination = destination.Replace(".ogm", "." + presetQuery.Format);
                text_destination.Text = destination;
            }

            #endregion

            // Destination tab
            #region destination

            if (presetQuery.Destination != "")
                text_destination.Text = presetQuery.Destination;

            drp_videoEncoder.Text = presetQuery.VideoEncoder;
            drp_audioCodec.Text = presetQuery.AudioEncoder;
            if (presetQuery.Width != 0)
            {
                text_width.Text = presetQuery.Width.ToString();

            }
            else
            {
                text_width.Text = "";
                text_width.BackColor = Color.White;
            }

            if (presetQuery.Height != 0)
            {
                text_height.Text = presetQuery.Height.ToString();
            }
            else
            {
                text_height.Text = "";
                text_height.BackColor = Color.White;
            }
            #endregion

            // Picture Settings Tab
            #region Picture
            drp_crop.Text = "Manual";
            text_top.Text = presetQuery.CropTop;
            text_bottom.Text = presetQuery.CropBottom;
            text_left.Text = presetQuery.CropLeft;
            text_right.Text = presetQuery.CropRight;

            drp_deInterlace_option.Text = presetQuery.DeInterlace;
            drp_deNoise.Text = presetQuery.DeNoise;

            if (presetQuery.DeTelecine == true)
            {
                check_detelecine.CheckState = CheckState.Checked;
            }
            else
            {
                check_detelecine.CheckState = CheckState.Unchecked;
            }


            if (presetQuery.DeBlock == true)
            {
                check_deblock.CheckState = CheckState.Checked;
            }
            else
            {
                check_deblock.CheckState = CheckState.Unchecked;
            }

            if (presetQuery.ChapterMarkers == true)
            {
                Check_ChapterMarkers.CheckState = CheckState.Checked;
            }
            else
            {
                Check_ChapterMarkers.CheckState = CheckState.Unchecked;
            }

            if (presetQuery.Anamorphic == true)
            {
                CheckPixelRatio.CheckState = CheckState.Checked;
            }
            else
            {
                CheckPixelRatio.CheckState = CheckState.Unchecked;
            }
            #endregion

            // Video Settings Tab
            #region video
            text_bitrate.Text = presetQuery.AverageVideoBitrate;
            text_filesize.Text = presetQuery.VideoTargetSize;
            slider_videoQuality.Value = presetQuery.VideoQuality;
            if (slider_videoQuality.Value != 0)
            {
                CheckCRF.Enabled = true;
                int ql = presetQuery.VideoQuality;
                SliderValue.Text = ql.ToString() + "%";
            }

            if (presetQuery.TwoPass == true)
            {
                check_2PassEncode.CheckState = CheckState.Checked;
            }
            else
            {
                check_2PassEncode.CheckState = CheckState.Unchecked;
            }

            if (presetQuery.Grayscale == true)
            {
                check_grayscale.CheckState = CheckState.Checked;
            }
            else
            {
                check_grayscale.CheckState = CheckState.Unchecked;
            }

            drp_videoFramerate.Text = presetQuery.VideoFramerate;

            if (presetQuery.TurboFirstPass == true)
            {
                check_turbo.CheckState = CheckState.Checked;
            }
            else
            {
                check_turbo.CheckState = CheckState.Unchecked;
            }

            if (presetQuery.LargeMP4 == true)
            {
                check_largeFile.CheckState = CheckState.Checked;
            }
            else
            {
                check_largeFile.CheckState = CheckState.Unchecked;
            }
            if (presetQuery.CRF == true)
            {
                CheckCRF.CheckState = CheckState.Checked;
            }
            else
            {
                CheckCRF.CheckState = CheckState.Unchecked;
            }
            #endregion

            // Audio Settings Tab
            #region audio
            drp_audioBitrate.Text = presetQuery.AudioBitrate;
            drp_audioSampleRate.Text = presetQuery.AudioSampleBitrate;
            drp_audioChannels.Text = presetQuery.AudioTrack1;
            drp_audioMixDown.Text = presetQuery.AudioTrackMix;
            drp_subtitle.Text = presetQuery.Subtitles;
            #endregion

            // H264 Tab & Preset Name
            #region other
            rtf_h264advanced.Text = presetQuery.H264Query;

            // Set the preset name
            groupBox_output.Text = "Output Settings (Preset: " + name + ")";
            #endregion
        }

        private Boolean updateCheck()
        {
            try
            {
                Functions.RssReader rssRead = new Functions.RssReader();
                string build = rssRead.build();

                int latest = int.Parse(build);
                int current = Properties.Settings.Default.build;
                int skip = Properties.Settings.Default.skipversion;

                if (latest == skip)
                {
                    return false;
                }
                else
                {
                    Boolean update = (latest > current);
                    if (update == true)
                        lbl_update.Visible = true;
                    return update;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
                return false;
            }
        }

        #endregion


        // This is the END of the road ------------------------------------------------------------------------------
    }
}