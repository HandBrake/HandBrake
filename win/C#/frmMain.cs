/*  frmMain.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

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
        private delegate void updateStatusChanger();
        Functions.Common hb_common_func = new Functions.Common();

        public frmMain()
        {
            // Load the splash screen in this thread
            Form splash = new frmSplashScreen();
            splash.Show();

            //Create a label that can be updated from the parent thread.
            Label lblStatus = new Label();
            lblStatus.Size = new Size(250, 20);
            lblStatus.Location = new Point(10, 280);
            splash.Controls.Add(lblStatus);

            //Fire a thread to wait for 2 seconds.  The splash screen will exit when the time expires
            Thread timer = new Thread(splashTimer);
            timer.Start();

            InitializeComponent();

            // show the form, but leave disabled until preloading is complete then show the main form
            this.Enabled = false;

            this.Show();

            // Forces frmMain to draw
            Application.DoEvents();

            // update the status
            if (Properties.Settings.Default.updateStatus == "Checked")
            {
                lblStatus.Text = "Checking for updates ...";
                Application.DoEvents();
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
                loadNormalPreset();

            // Enable or disable tooltips
            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                lblStatus.Text = "Loading Tooltips ...";
                Application.DoEvents();
                ToolTip.Active = true;
                Thread.Sleep(100);
            }

            // Set some defaults for the dropdown menus. Just incase the normal or user presets dont load.
            drp_crop.SelectedIndex = 0;
            drp_videoEncoder.SelectedIndex = 2;
            drp_audioCodec.SelectedIndex = 0;

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
    
        private void startupUpdateCheck()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new updateStatusChanger(startupUpdateCheck));
                    return;
                }
                
                Boolean update = hb_common_func.updateCheck();
                if (update == true)
                {
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
            Form splash = new frmSplashScreen();
            splash.Show();
        }

        private void loadUserDefaults()
        {
            string userDefaults = Properties.Settings.Default.defaultUserSettings;
            try
            {
                // Some things that need to be done to reset some gui components:
                CheckPixelRatio.CheckState = CheckState.Unchecked;

                // Send the query from the file to the Query Parser class Then load the preset
                Functions.QueryParser presetQuery = Functions.QueryParser.Parse(userDefaults);
                hb_common_func.presetLoader(this, presetQuery, "User Defaults ");
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to load user Default Settings. \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        #endregion

        #region Set Varible Function
        public void setStreamReader(Parsing.DVD dvd)
        {
            this.thisDVD = dvd;
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
                    // Create StreamReader & open file
                    StreamReader line = new StreamReader(filename);

                    // Send the query from the file to the Query Parser class then load the preset
                    Functions.QueryParser presetQuery = Functions.QueryParser.Parse(line.ReadLine());
                    hb_common_func.presetLoader(this, presetQuery, filename);

                    // Close the stream
                    line.Close();
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Unable to load profile. \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
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
                    String query = hb_common_func.GenerateTheQuery(this);
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
            Boolean update = hb_common_func.updateCheck();
            if (update == true)
            {
                frmUpdater updateWindow = new frmUpdater();
                updateWindow.Show();
            }
            else
                MessageBox.Show("There are no new updates at this time.", "Update Check", MessageBoxButtons.OK, MessageBoxIcon.Information);
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
                MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source. Please refer to the FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
        }

        private void btn_destBrowse_Click(object sender, EventArgs e)
        {
            // This removes the file extension from the filename box on the save file dialog.
            // It's daft but some users don't realise that typing an extension overrides the dropdown extension selected.
            DVD_Save.FileName = DVD_Save.FileName.Replace(".mp4", "").Replace(".m4v", "").Replace(".mkv", "").Replace(".ogm", "").Replace(".avi", "");

            // Show the dialog and set the main form file path
            DVD_Save.ShowDialog();
            text_destination.Text = DVD_Save.FileName;

            // Quicktime requires .m4v file for chapter markers to work. If checked, change the extension to .m4v (mp4 and m4v are the same thing)
            if (Check_ChapterMarkers.Checked)
                text_destination.Text = text_destination.Text.Replace(".mp4", ".m4v");
        }

        private void btn_h264Clear_Click(object sender, EventArgs e)
        {
            rtf_h264advanced.Text = "";
        }

        private void btn_ActivityWindow_Click(object sender, EventArgs e)
        {
            Form ActivityWindow = new frmActivityWindow();
            ActivityWindow.ShowDialog();
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
                    drop_chapterStart.Text = drop_chapterStart.Items[0].ToString();

                // Populate the Final Chapter Dropdown
                drop_chapterFinish.Items.Clear();
                drop_chapterFinish.Items.AddRange(selectedTitle.Chapters.ToArray());
                if (drop_chapterFinish.Items.Count > 0)
                    drop_chapterFinish.Text = drop_chapterFinish.Items[drop_chapterFinish.Items.Count - 1].ToString();

                // Populate the Audio Channels Dropdown
                drp_track1Audio.Items.Clear();
                drp_track1Audio.Items.Add("Automatic");
                drp_track1Audio.Items.Add("None");
                drp_track1Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                if (drp_track1Audio.Items.Count > 0)
                    drp_track1Audio.Text = drp_track1Audio.Items[0].ToString();

                drp_track2Audio.Items.Clear();
                drp_track2Audio.Items.Add("None");
                drp_track2Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                if (drp_track2Audio.Items.Count > 0)
                    drp_track2Audio.Text = drp_track2Audio.Items[0].ToString();

                // Populate the Subtitles dropdown
                drp_subtitle.Items.Clear();
                drp_subtitle.Items.Add("None");
                drp_subtitle.Items.Add("Autoselect");
                drp_subtitle.Items.AddRange(selectedTitle.Subtitles.ToArray());
                if (drp_subtitle.Items.Count > 0)
                    drp_subtitle.Text = drp_subtitle.Items[0].ToString();

            }

            // Run the autoName & chapterNaming functions
            hb_common_func.autoName(this);
            hb_common_func.chapterNaming(this);
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
                        drop_chapterStart.BackColor = Color.LightCoral;
                }
                catch (Exception)
                {
                    drop_chapterStart.BackColor = Color.LightCoral;
                }
            }
            // Run the Autonaming function
            hb_common_func.autoName(this);
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
                        drop_chapterFinish.BackColor = Color.LightCoral;
                }
                catch (Exception)
                {
                    drop_chapterFinish.BackColor = Color.LightCoral;
                }
            }

            // Run the Autonaming function
            hb_common_func.autoName(this);
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
                        text_width.BackColor = Color.LightCoral;
                    else
                        text_width.BackColor = Color.LightGreen;
                }

                if ((lbl_Aspect.Text != "Select a Title") && (drp_crop.SelectedIndex == 2))
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
                        text_height.Text = height.ToString();
                }
            }
            catch (Exception)
            {
                // No need to throw an error here.
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
                        text_height.BackColor = Color.LightCoral;
                    else
                        text_height.BackColor = Color.LightGreen;
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
                text_height.BackColor = Color.LightGray;
                text_width.BackColor = Color.LightGray;
                text_height.Enabled = false;
                text_width.Enabled = false;
            }
            else
            {
                check_lAnamorphic.Enabled = true;
                text_height.BackColor = Color.White;
                text_width.BackColor = Color.White;
                text_height.Enabled = true;
                text_width.Enabled = true;
            }
        }

        private void check_lAnamorphic_CheckedChanged(object sender, EventArgs e)
        {
            if (check_lAnamorphic.Checked)
            {
                CheckPixelRatio.Enabled = false;
                CheckPixelRatio.Checked = false;
                text_height.Text = "";
                text_height.Enabled = false;
                text_height.BackColor = Color.LightGray;
            }
            else
            {
                CheckPixelRatio.Enabled = true;
                text_height.Enabled = true;
                text_height.BackColor = Color.White;
            }
        }

        private void check_2PassEncode_CheckedChanged(object sender, EventArgs e)
        {
            if (check_2PassEncode.CheckState.ToString() == "Checked")
            {
                if (drp_videoEncoder.Text.Contains("H.264"))
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

        private void check_iPodAtom_CheckedChanged(object sender, EventArgs e)
        {
            if (!text_destination.Text.Contains(".mp4"))
            {
                lbl_ipodAtom.Visible = true;
                check_iPodAtom.CheckState = CheckState.Unchecked;
            }
            else
                lbl_ipodAtom.Visible = false;
        }

        private void check_optimiseMP4_CheckedChanged(object sender, EventArgs e)
        {
            if (!text_destination.Text.Contains(".mp4"))
            {
                check_optimiseMP4.BackColor = Color.LightCoral;
                check_optimiseMP4.CheckState = CheckState.Unchecked;
            }
            else
                check_optimiseMP4.BackColor = Color.Transparent;
        }

        private void drp_dvdtitle_Click(object sender, EventArgs e)
        {
            if (drp_dvdtitle.Items.Count == 0)
                MessageBox.Show("There are no titles to select. Please scan the DVD by clicking the 'browse' button above before trying to select a title.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
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
                hb_common_func.chapterNaming(this);
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
                check_lAnamorphic.Enabled = false;
                check_lAnamorphic.Checked = false;
            }
            else
            {
                if (check_2PassEncode.CheckState == CheckState.Checked)
                    check_turbo.Enabled = true;

                h264Tab.Enabled = true;
                check_iPodAtom.Enabled = true;
                lbl_ipodAtom.Visible = false;
                check_optimiseMP4.Enabled = true;
                check_lAnamorphic.Enabled = true;
            }

        }

        #endregion

        #region Query Editor Tab

        private void btn_clear_Click(object sender, EventArgs e)
        {
            rtf_query.Clear();
        }

        private void btn_generate_Query_Click(object sender, EventArgs e)
        {
            rtf_query.Text = hb_common_func.GenerateTheQuery(this);
        }

        private void btn_copy2C_Click(object sender, EventArgs e)
        {
            if (rtf_query.Text != "")
                Clipboard.SetText(rtf_query.Text, TextDataFormat.Text);
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
            string appPath = Application.StartupPath.ToString() + "\\presets.dat";
            if (File.Exists(appPath))
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
        }

        // Buttons
        private void btn_setDefault_Click(object sender, EventArgs e)
        {
            String query = hb_common_func.GenerateTheQuery(this);
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
                string appPath = Application.StartupPath.ToString() + "\\presets.dat";
                if (File.Exists(appPath))
                {
                    StreamReader presetInput = new StreamReader(appPath);
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
                                hb_common_func.presetLoader(this, presetQuery, selectedPreset);
                            }
                        }
                        else
                            presetInput.ReadLine();
                    }
                }
                else
                {
                    MessageBox.Show("Unable to load the presets.dat file. Please select \"Update Built-in Presets\" from the Presets Menu \nMake sure you are running the program in Admin mode if running on Vista. See Windows FAQ for details!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void btn_start_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                String query;
                if (rtf_query.Text != "")
                    query = rtf_query.Text;
                else
                    query = hb_common_func.GenerateTheQuery(this);

                ThreadPool.QueueUserWorkItem(procMonitor, query);
                lbl_encode.Visible = true;
                lbl_encode.Text = "Encoding in Progress";
            }
        }

        private void btn_add2Queue_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Browse' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                String query;
                if (rtf_query.Text != "")
                    query = rtf_query.Text;
                else
                    query = hb_common_func.GenerateTheQuery(this);

                queueWindow.list_queue.Items.Add(query);
                queueWindow.Show();
            }
        }

        private void btn_showQueue_Click(object sender, EventArgs e)
        {
            showQueue();
        }

        private void showQueue()
        {
            queueWindow.Show();
        }

        private void procMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (hbProc != null)
                MessageBox.Show("Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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



        // This is the END of the road ------------------------------------------------------------------------------
    }
}