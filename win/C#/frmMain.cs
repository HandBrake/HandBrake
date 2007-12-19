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
                Thread updateCheckThread = new Thread(startupUpdateCheck);
                updateCheckThread.Start();
                Thread.Sleep(100);
            }

            // Update the presets
            if (Properties.Settings.Default.updatePresets == "Checked")
            {
                lblStatus.Text = "Updaing Presets ...";
                Application.DoEvents();
                grabCLIPresets();
                Thread.Sleep(200);
            }

            // Load the presets for the preset box
            updatePresets();

            // Now load the users default if required. (Will overide the above setting)
            if (Properties.Settings.Default.defaultSettings == "Checked")
            {
                lblStatus.Text = "Loading User Default Settings...";
                Application.DoEvents();
                loadUserDefaults();
                Thread.Sleep(100);
            }
            else
            {
                loadNormalPreset();
            }

            // Enable or disable tooltips
            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                lblStatus.Text = "Loading Tooltips ...";
                Application.DoEvents();
                ToolTip.Active = true;
                Thread.Sleep(100);
            }

            // Make sure a default cropping option is selected. Not sure why this is not defaulting like the other checkdowns,
            // Can fix it later.
            drp_crop.SelectedItem = "Automatic";

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

        private delegate void updateStatusChanger();
        private void startupUpdateCheck()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new updateStatusChanger(startupUpdateCheck));
                    return;
                }

                Boolean update = updateCheck();
                if (update == true)
                {
                    lbl_update.Visible = true;

                    frmUpdater updateWindow = new frmUpdater();
                    updateWindow.Show();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
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
                MessageBox.Show("Unable to load user Default Settings. \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
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

        private void mnu_showCommand_Click(object sender, EventArgs e)
        {

            Form query = new frmQuery(GenerateTheQuery());
            query.ShowDialog();
        }

        #endregion

        #region Presets Menu
        Boolean presetStatus;

        private void mnu_presetReset_Click(object sender, EventArgs e)
        {
            treeView_presets.Nodes.Clear();
            grabCLIPresets();
            updatePresets();
            MessageBox.Show("Presets have been updated", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            loadNormalPreset();
        }

        #endregion

        #region Help Menu

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
                lbl_update.Visible = true;
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
        // Buttons on the main Window and items that require actions
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
            }
            else
            {
                ISO_Open.ShowDialog();
                filename = ISO_Open.FileName;
            }

            if (filename != "")
            {
                Form frmRD = new frmReadDVD(filename, this, dvdInfoWindow);
                text_source.Text = filename;
                frmRD.ShowDialog();
            }
            else
                text_source.Text = "Click 'Browse' to continue";

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

        #endregion

        #region frmMain Actions

        private void drp_dvdtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Reset some values on the form
            lbl_Aspect.Text = "Select a Title";
            lbl_RecomendedCrop.Text = "Select a Title";
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();

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
                drp_track1Audio.Items.Clear();
                drp_track1Audio.Items.Add("Automatic");
                drp_track1Audio.Items.Add("None");
                drp_track1Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                if (drp_track1Audio.Items.Count > 0)
                {
                    drp_track1Audio.Text = drp_track1Audio.Items[0].ToString();
                }
                drp_track2Audio.Items.Clear();
                drp_track2Audio.Items.Add("None");
                drp_track2Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                if (drp_track2Audio.Items.Count > 0)
                {
                    drp_track2Audio.Text = drp_track2Audio.Items[0].ToString();
                }


                // Populate the Subtitles dropdown
                drp_subtitle.Items.Clear();
                drp_subtitle.Items.Add("None");
                drp_subtitle.Items.Add("Autoselect");
                drp_subtitle.Items.AddRange(selectedTitle.Subtitles.ToArray());
                if (drp_subtitle.Items.Count > 0)
                {
                    drp_subtitle.Text = drp_subtitle.Items[0].ToString();
                }
            }

            // Run the Autonaming function
            autoName();
            chapterNaming();
        }

        private void drop_chapterStart_SelectedIndexChanged(object sender, EventArgs e)
        {
            drop_chapterStart.BackColor = Color.White;
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
            // Run the Autonaming function
            autoName();
            chapterNaming();

        }

        private void drop_chapterFinish_SelectedIndexChanged(object sender, EventArgs e)
        {
            drop_chapterFinish.BackColor = Color.White;
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

            // Run the Autonaming function
            autoName();
            chapterNaming();
        }

        private void text_bitrate_TextChanged(object sender, EventArgs e)
        {
            text_filesize.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
        }

        private void text_filesize_TextChanged(object sender, EventArgs e)
        {
            text_bitrate.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
        }

        private void slider_videoQuality_Scroll(object sender, EventArgs e)
        {
            SliderValue.Text = slider_videoQuality.Value.ToString() + "%";
            text_bitrate.Text = "";
            text_filesize.Text = "";
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
            if ((string)drp_crop.SelectedItem == "Custom")
            {
                text_left.Enabled = true;
                text_right.Enabled = true;
                text_top.Enabled = true;
                text_bottom.Enabled = true;
                text_left.Text = "0";
                text_right.Text = "0";
                text_top.Text = "0";
                text_bottom.Text = "0";
            }

            if ((string)drp_crop.SelectedItem == "Automatic")
            {
                text_left.Enabled = false;
                text_right.Enabled = false;
                text_top.Enabled = false;
                text_bottom.Enabled = false;


                if (lbl_RecomendedCrop.Text != "Select a Title")
                {
                    string[] temp = new string[4];
                    temp = lbl_RecomendedCrop.Text.Split('/');
                    text_left.Text = temp[2];
                    text_right.Text = temp[3];
                    text_top.Text = temp[0];
                    text_bottom.Text = temp[1];
                }
                else
                {
                    text_left.Text = "";
                    text_right.Text = "";
                    text_top.Text = "";
                    text_bottom.Text = "";
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

        private void check_vfr_CheckedChanged(object sender, EventArgs e)
        {
            if (check_vfr.CheckState == CheckState.Checked)
            {
                check_detelecine.Enabled = false;
                check_detelecine.CheckState = CheckState.Checked;
                drp_videoFramerate.Enabled = false;
                drp_videoFramerate.SelectedItem = "29.97";
                lbl_vfr.Visible = true;
            }
            else
            {
                check_detelecine.Enabled = true;
                drp_videoFramerate.Enabled = true;
                drp_videoFramerate.SelectedItem = "Automatic";
                lbl_vfr.Visible = false;
            }
        }

        private void CheckPixelRatio_CheckedChanged(object sender, EventArgs e)
        {
            text_width.Text = "";
            text_height.Text = "";
            text_width.BackColor = Color.White;
            text_height.BackColor = Color.White;
            CheckPixelRatio.BackColor = TabPage1.BackColor;

            if (CheckPixelRatio.Checked)
            {
                check_lAnamorphic.Enabled = false;
                check_lAnamorphic.Checked = false;
            }
            else
                check_lAnamorphic.Enabled = true;
        }

        private void check_lAnamorphic_CheckedChanged(object sender, EventArgs e)
        {
            if (check_lAnamorphic.Checked)
            {
                CheckPixelRatio.Enabled = false;
                CheckPixelRatio.Checked = false;
            }
            else
                CheckPixelRatio.Enabled = true;
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

        private void check_iPodAtom_CheckedChanged(object sender, EventArgs e)
        {
            if (!text_destination.Text.Contains(".mp4"))
            {
                lbl_ipodAtom.Visible = true;
                check_iPodAtom.CheckState = CheckState.Unchecked;
            }
            else
            {
                lbl_ipodAtom.Visible = false;
            }
        }

        private void check_optimiseMP4_CheckedChanged(object sender, EventArgs e)
        {
            if (!text_destination.Text.Contains(".mp4"))
            {
                check_optimiseMP4.BackColor = Color.LightCoral;
                check_optimiseMP4.CheckState = CheckState.Unchecked;
            }
            else
            {
                check_optimiseMP4.BackColor = Color.Transparent;
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
            if ((drp_audioCodec.Text == "AAC") && (drp_audioMixDown.Text == "6 Channel Discrete"))
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

        private void slider_drc_Scroll(object sender, EventArgs e)
        {
            double value = slider_drc.Value / 10.0;
            value++;

            lbl_drc.Text = value.ToString();
        }

        private void drp_subtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_subtitle.Text.Contains("None"))
            {
                check_forced.Enabled = false;
                check_forced.Checked = false;
            }
            else
                check_forced.Enabled = true;
        }

        private void Check_ChapterMarkers_CheckedChanged(object sender, EventArgs e)
        {
            if (Check_ChapterMarkers.Checked)
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".mp4", ".m4v");
                text_destination.Text = destination;
                data_chpt.Rows.Clear();
                data_chpt.Enabled = true;
                chapterNaming();
            }
            else
            {
                string destination = text_destination.Text;
                destination = destination.Replace(".m4v", ".mp4");
                text_destination.Text = destination;
                data_chpt.Rows.Clear();
                data_chpt.Enabled = false;
            }
        }

        private void drp_videoEncoder_SelectedIndexChanged(object sender, EventArgs e)
        {
            //Turn off some options which are H.264 only when the user selects a non h.264 encoder
            if (!drp_videoEncoder.Text.Contains("H.264"))
            {
                check_turbo.CheckState = CheckState.Unchecked;
                check_turbo.Enabled = false;
                h264Tab.Enabled = false;
                rtf_h264advanced.Text = "";
                check_iPodAtom.Enabled = false;
                check_iPodAtom.Checked = false;
                lbl_ipodAtom.Visible = false;
                check_optimiseMP4.Enabled = false;
            }
            else
            {
                if (check_2PassEncode.CheckState == CheckState.Checked)
                {
                    check_turbo.Enabled = true;
                }
                h264Tab.Enabled = true;
                check_iPodAtom.Enabled = true;
                lbl_ipodAtom.Visible = false;
                check_optimiseMP4.Enabled = true;
            }

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

            TreeNode preset_treeview = new TreeNode();

            foreach (string preset in presets)
            {
                preset_treeview = new TreeNode(preset);

                // Now Fill Out List View with Items
                treeView_presets.Nodes.Add(preset_treeview);
            }
        }

        private void grabCLIPresets()
        {
            string appPath = Application.StartupPath.ToString() + "\\";
            string strCmdLine = "cmd /c " + '"' + '"' + appPath + "HandBrakeCLI.exe" + '"' + " --preset-list >" + '"' + appPath + "presets.dat" + '"' + " 2>&1" + '"';
            Process hbproc = Process.Start("CMD.exe", strCmdLine);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();
        }

        // Function to select the default preset.
        private void loadNormalPreset()
        {

            int normal = 0;
            foreach (TreeNode treenode in treeView_presets.Nodes)
            {
                if (treenode.ToString().Equals("TreeNode: Normal"))
                    normal = treenode.Index;
            }

            TreeNode np = treeView_presets.Nodes[normal];

            treeView_presets.SelectedNode = np;



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
        private void treeView_presets_AfterSelect(object sender, TreeViewEventArgs e)
        {
            string selectedPreset = null;
            selectedPreset = treeView_presets.SelectedNode.Text;

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

        #region Encodeing and Queue

        Functions.CLI process = new Functions.CLI();
        private delegate void UpdateUIHandler();

        [DllImport("user32.dll")]
        public static extern void LockWorkStation();
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        private void btn_queue_Click(object sender, EventArgs e)
        {

            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                string query = GenerateTheQuery();

                queueWindow.list_queue.Items.Add(query);
                queueWindow.Show();
            }
        }

        private void showQueue()
        {
            queueWindow.Show();
        }

        private void btn_encode_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                string query = GenerateTheQuery();

                ThreadPool.QueueUserWorkItem(procMonitor, query);
                lbl_encode.Visible = true;
                lbl_encode.Text = "Encoding in Progress";
            }
        }

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

        private void setEncodeLabel()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new UpdateUIHandler(setEncodeLabel));
                return;
            }
            lbl_encode.Text = "Encoding Finished";
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

            if ((source != "") || (source != "Click 'Browse' to continue"))
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

            if (destination != "")
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
            string cropOut = "";
            string deInterlace_Option = drp_deInterlace_option.Text;
            string deinterlace = "";
            string grayscale = "";
            string pixelRatio = "";
            string vfr = "";
            string deblock = "";
            string detelecine = "";
            string lanamorphic = "";



            if (cropSetting == "Automatic")
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

            switch (deInterlace_Option)
            {
                case "None":
                    deinterlace = "";
                    break;
                case "Fast":
                    deinterlace = " --deinterlace=fast";
                    break;
                case "Slow":
                    deinterlace = " --deinterlace=slow";
                    break;
                case "Slower":
                    deinterlace = " --deinterlace=slower";
                    break;
                case "Slowest":
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

            if (check_deblock.Checked)
                deblock = " --deblock";

            if (check_detelecine.Checked)
                detelecine = " --detelecine";

            if (check_vfr.Checked)
                vfr = " -V ";

            if (check_lAnamorphic.Checked)
                lanamorphic = " -P ";

            string queryPictureSettings = cropOut + deinterlace + deblock + detelecine + vfr + grayscale + pixelRatio + lanamorphic;
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
            string denoise = "";
            string ipodAtom = "";
            string optimizeMP4 = "";

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

            if (check_iPodAtom.Checked)
                ipodAtom = " -I ";

            if (check_optimiseMP4.Checked)
                optimizeMP4 = " -O ";


            string queryVideoSettings = videoBitrate + videoFilesize + vidQSetting + twoPassEncoding + videoFramerate + turboH264 + ipodAtom + optimizeMP4 + largeFile + denoise;
            #endregion

            // Audio Settings Tab
            #region Audio Settings Tab

            string audioBitrate = drp_audioBitrate.Text;
            string audioSampleRate = drp_audioSampleRate.Text;
            string track1 = drp_track1Audio.Text;
            string track2 = drp_track2Audio.Text;
            string audioChannels = "";
            string Mixdown = drp_audioMixDown.Text;
            string SixChannelAudio = "";
            string subtitles = drp_subtitle.Text;
            string subScan = "";
            string forced = "";
            string drc = "";

            if (audioBitrate != "")
                audioBitrate = " -B " + audioBitrate;

            if (audioSampleRate != "")
                audioSampleRate = " -R " + audioSampleRate;

            // Audio Track 1
            if (track1 == "Automatic")
                audioChannels = "";
            else if (track1 == "")
                audioChannels = "";
            else if (track1 == "None")
                audioChannels = " -a none";
            else
            {
                string[] tempSub;
                tempSub = track1.Split(' ');
                audioChannels = " -a " + tempSub[0];
            }

            // Audio Track 2
            if (audioChannels != "")
            {
                if ((track2 != "") && (track2 != "None"))
                {
                    string[] tempSub;
                    tempSub = track2.Split(' ');
                    audioChannels = audioChannels + "," + tempSub[0];
                }
            }
            else
            {
                if ((track2 != "") && (track2 != "None"))
                {
                    string[] tempSub;
                    tempSub = track2.Split(' ');
                    audioChannels = " -a " + tempSub[0];
                }
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

            if (subtitles == "None")
                subtitles = "";
            else if (subtitles == "")
                subtitles = "";
            else if (subtitles == "Autoselect")
            {
                subScan = " -U ";
                subtitles = "";
            }
            else
            {
                string[] tempSub;
                tempSub = subtitles.Split(' ');
                subtitles = " -s " + tempSub[0];
            }

            if (check_forced.Checked)
                forced = " -F ";

            //Dynamic Range Compression (expects a float but a double is used for ease)
            double value = slider_drc.Value / 10.0;
            value++;
            drc = " -D " + value;

            string queryAudioSettings = audioBitrate + audioSampleRate + drc + audioChannels + SixChannelAudio + subScan + subtitles + forced;
            #endregion

            // Chapter Markers Tab
            #region Chapter Markers

            string ChapterMarkers = "";

            if (Check_ChapterMarkers.Checked)
            {
                Boolean saveCSV = chapterCSVSave();
                if (saveCSV == false)
                {
                    MessageBox.Show("Unable to save Chapter Makrers file! \n Chapter marker names will NOT be saved in your encode \n\n", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    ChapterMarkers = " -m ";
                }
                else
                {
                    string path = Application.StartupPath.ToString();
                    path = "\"" + path + "\\chapters.csv\" ";

                    ChapterMarkers = " --markers=" + path;
                }
            }

            string chapter_markers = ChapterMarkers;
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

            return querySource + queryDestination + queryPictureSettings + queryVideoSettings + h264Settings + queryAudioSettings + ChapterMarkers + queryAdvancedSettings + verbose;
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
                text_width.Text = presetQuery.Width.ToString();
            else
            {
                text_width.Text = "";
                text_width.BackColor = Color.White;
            }

            if (presetQuery.Height != 0)
                text_height.Text = presetQuery.Height.ToString();
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
                check_detelecine.CheckState = CheckState.Checked;
            else
                check_detelecine.CheckState = CheckState.Unchecked;


            if (presetQuery.DeBlock == true)
                check_deblock.CheckState = CheckState.Checked;
            else
                check_deblock.CheckState = CheckState.Unchecked;

            if (presetQuery.ChapterMarkers == true)
                Check_ChapterMarkers.CheckState = CheckState.Checked;
            else
                Check_ChapterMarkers.CheckState = CheckState.Unchecked;

            if (presetQuery.Anamorphic == true)
                CheckPixelRatio.CheckState = CheckState.Checked;
            else
                CheckPixelRatio.CheckState = CheckState.Unchecked;

            if (presetQuery.LooseAnamorphic == true)
                check_lAnamorphic.CheckState = CheckState.Checked;
            else
                check_lAnamorphic.CheckState = CheckState.Unchecked;

            if (presetQuery.VFR == true)
                check_vfr.CheckState = CheckState.Checked;
            else
                check_vfr.CheckState = CheckState.Unchecked;
            #endregion

            // Video Settings Tab
            #region video
            text_bitrate.Text = presetQuery.AverageVideoBitrate;
            text_filesize.Text = presetQuery.VideoTargetSize;
            slider_videoQuality.Value = presetQuery.VideoQuality;
            if (slider_videoQuality.Value != 0)
            {
                int ql = presetQuery.VideoQuality;
                SliderValue.Text = ql.ToString() + "%";
            }

            if (presetQuery.TwoPass == true)
                check_2PassEncode.CheckState = CheckState.Checked;
            else
                check_2PassEncode.CheckState = CheckState.Unchecked;

            if (presetQuery.Grayscale == true)
                check_grayscale.CheckState = CheckState.Checked;
            else
                check_grayscale.CheckState = CheckState.Unchecked;

            drp_videoFramerate.Text = presetQuery.VideoFramerate;

            if (presetQuery.TurboFirstPass == true)
                check_turbo.CheckState = CheckState.Checked;
            else
                check_turbo.CheckState = CheckState.Unchecked;

            if (presetQuery.LargeMP4 == true)
                check_largeFile.CheckState = CheckState.Checked;
            else
                check_largeFile.CheckState = CheckState.Unchecked;

            if (presetQuery.IpodAtom == true)
                check_iPodAtom.CheckState = CheckState.Checked;
            else
                check_iPodAtom.CheckState = CheckState.Unchecked;

            if (presetQuery.OptimizeMP4 == true)
                check_optimiseMP4.CheckState = CheckState.Checked;
            else
                check_optimiseMP4.CheckState = CheckState.Unchecked;

            #endregion

            // Audio Settings Tab
            #region Audio
            drp_audioBitrate.Text = presetQuery.AudioBitrate;
            drp_audioSampleRate.Text = presetQuery.AudioSampleBitrate;
            drp_track1Audio.Text = presetQuery.AudioTrack1;
            drp_track2Audio.Text = presetQuery.AudioTrack2;
            drp_audioMixDown.Text = presetQuery.AudioTrackMix;
            drp_subtitle.Text = presetQuery.Subtitles;

            if (presetQuery.ForcedSubtitles == true)
            {
                check_forced.CheckState = CheckState.Checked;
                check_forced.Enabled = true;
            }
            else
                check_forced.CheckState = CheckState.Unchecked;

            // Dynamic Range Compression (Should be a float but we use double for ease)
            double value = presetQuery.DRC;
            if (value > 0)
                value = value - 10;
            slider_drc.Value = int.Parse(value.ToString());

            double actualValue = presetQuery.DRC / 10;
            lbl_drc.Text = actualValue.ToString();


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
                    return update;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
                return false;
            }
        }

        public void setStreamReader(Parsing.DVD dvd)
        {
            this.thisDVD = dvd;
        }

        public void autoName()
        {
            if (Properties.Settings.Default.autoNaming == "Checked")
            {
                if (drp_dvdtitle.Text != "Automatic")
                {
                    string source = text_source.Text;
                    string[] sourceName = source.Split('\\');
                    source = sourceName[sourceName.Length - 1].Replace(".iso", "").Replace(".mpg", "").Replace(".ts", "").Replace(".ps", "");

                    string title = drp_dvdtitle.Text;
                    string[] titlesplit = title.Split(' ');
                    title = titlesplit[0];

                    string cs = drop_chapterStart.Text;
                    string cf = drop_chapterFinish.Text;

                    if (title == "Automatic")
                        title = "";
                    if (cs == "Auto")
                        cs = "";
                    if (cf == "Auto")
                        cf = "";

                    string dash = "";
                    if (cf != "Auto")
                        dash = "-";

                    if (!text_destination.Text.Contains("\\"))
                    {
                        string filePath = "";
                        if (Properties.Settings.Default.autoNamePath.Trim() != "")
                            filePath = Properties.Settings.Default.autoNamePath + "\\";
                        text_destination.Text = filePath + source + "_T" + title + "_C" + cs + dash + cf + ".mp4";
                    }
                    else
                    {
                        string dest = text_destination.Text;

                        string[] destName = dest.Split('\\');


                        string[] extension = dest.Split('.');
                        string ext = extension[extension.Length - 1];

                        destName[destName.Length - 1] = source + "_T" + title + "_C" + cs + dash + cf + "." + ext;

                        string fullDest = "";
                        foreach (string part in destName)
                        {
                            if (fullDest != "")
                                fullDest = fullDest + "\\" + part;
                            else
                                fullDest = fullDest + part;
                        }

                        text_destination.Text = fullDest;
                    }
                }
            }
        }

        private void chapterNaming()
        {
            try
            {
                data_chpt.Rows.Clear();
                int i = 0;
                int rowCount = 0;
                if (drop_chapterFinish.Text != "Auto")
                    rowCount = int.Parse(drop_chapterFinish.Text);
                while (i < rowCount)
                {
                    DataGridViewRow row = new DataGridViewRow();

                    data_chpt.Rows.Insert(i, row);
                    data_chpt.Rows[i].Cells[0].Value = (i + 1);
                    data_chpt.Rows[i].Cells[1].Value = "Chapter" + (i + 1);
                    i++;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("chapterNaming() Error has occured: \n" + exc.ToString());
            }
        }

        private Boolean chapterCSVSave()
        {
            try
            {
                string appPath = Application.StartupPath.ToString();
                appPath = appPath + "\\";

                string path = appPath + "chapters.csv";

                StringBuilder csv = new StringBuilder();

                foreach (DataGridViewRow row in data_chpt.Rows)
                {
                    csv.Append(row.Cells[0].Value.ToString());
                    csv.Append(",");
                    csv.Append(row.Cells[1].Value.ToString());
                    csv.Append("\n");

                }
                StreamWriter file = new StreamWriter(path);
                file.Write(csv.ToString());
                file.Close();
                file.Dispose();
                return true;

            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return false;
            }
        }

        #endregion

        // This is the END of the road ------------------------------------------------------------------------------
    }
}