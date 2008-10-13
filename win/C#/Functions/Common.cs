/*  Common.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using System.Drawing;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace Handbrake.Functions
{
    class Common
    {

        /// <summary>
        /// This function takes in a Query which has been parsed by QueryParser and
        /// set's all the GUI widgets correctly.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <param name="presetQuery">The Parsed CLI Query</param>
        /// <param name="name">Name of the preset</param>
        public void presetLoader(frmMain mainWindow, Functions.QueryParser presetQuery, string name)
        {
            // ---------------------------
            // Setup the GUI
            // ---------------------------

            // Source tab
            #region source
            // Reset some vaules to stock first to prevent errors.
            mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            // Now load all the new settings onto the main window
            mainWindow.drp_dvdtitle.Text = "Automatic";
            mainWindow.drop_chapterStart.Text = "Auto";
            mainWindow.drop_chapterFinish.Text = "Auto";

            if (presetQuery.Format != null)
            {
                string destination = mainWindow.text_destination.Text;
                destination = destination.Replace(".mp4", "." + presetQuery.Format);
                destination = destination.Replace(".m4v", "." + presetQuery.Format);
                destination = destination.Replace(".avi", "." + presetQuery.Format);
                destination = destination.Replace(".mkv", "." + presetQuery.Format);
                destination = destination.Replace(".ogm", "." + presetQuery.Format);
                mainWindow.text_destination.Text = destination;
            }

            #endregion

            // Destination tab
            #region destination

            mainWindow.drp_videoEncoder.Text = presetQuery.VideoEncoder;

            if (presetQuery.Format != null)
            {
                if (presetQuery.Format == "mp4")
                    mainWindow.drop_format.SelectedIndex = 0;
                else if (presetQuery.Format == "m4v")
                    mainWindow.drop_format.SelectedIndex = 1;
                else if (presetQuery.Format == "mkv")
                    mainWindow.drop_format.SelectedIndex = 2;
                else if (presetQuery.Format == "avi")
                    mainWindow.drop_format.SelectedIndex = 3;
                else if (presetQuery.Format == "ogm")
                    mainWindow.drop_format.SelectedIndex = 4;
            }

            if (presetQuery.IpodAtom == true)
                mainWindow.check_iPodAtom.CheckState = CheckState.Checked;
            else
                mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            if (presetQuery.OptimizeMP4 == true)
                mainWindow.check_optimiseMP4.CheckState = CheckState.Checked;
            else
                mainWindow.check_optimiseMP4.CheckState = CheckState.Unchecked;

            #endregion

            // Picture Settings Tab
            #region Picture
            mainWindow.check_autoCrop.Checked = true;
            mainWindow.drp_deInterlace_option.Text = presetQuery.DeInterlace;
            mainWindow.drp_deNoise.Text = presetQuery.DeNoise;

            if (presetQuery.Decomb == true)
                mainWindow.check_decomb.CheckState = CheckState.Checked;
            else
                mainWindow.check_decomb.CheckState = CheckState.Unchecked;

            if (presetQuery.DeTelecine == true)
                mainWindow.check_detelecine.CheckState = CheckState.Checked;
            else
                mainWindow.check_detelecine.CheckState = CheckState.Unchecked;

            if (presetQuery.DeBlock != 0)
            {
                mainWindow.slider_deblock.Value = presetQuery.DeBlock;
                mainWindow.lbl_deblockVal.Text = presetQuery.DeBlock.ToString();
            }
            else
            {
                mainWindow.slider_deblock.Value = 4;
                mainWindow.lbl_deblockVal.Text = "Off";
            }

            if (presetQuery.Anamorphic == true)
                mainWindow.drp_anamorphic.SelectedIndex = 1;
            else
                mainWindow.drp_anamorphic.SelectedIndex = 0;

            if (presetQuery.LooseAnamorphic == true)
                mainWindow.drp_anamorphic.SelectedIndex = 2;
            else
            {
                if (presetQuery.Anamorphic != true)
                    mainWindow.drp_anamorphic.SelectedIndex = 0;
            }

            if (presetQuery.Width != 0)
                mainWindow.text_width.Text = presetQuery.Width.ToString();
            else
            {
                mainWindow.text_width.Text = "";
            }

            if (presetQuery.Height != 0)
                mainWindow.text_height.Text = presetQuery.Height.ToString();
            else
            {
                mainWindow.text_height.Text = "";
            }

            #endregion

            // Video Settings Tab
            #region video
            mainWindow.text_bitrate.Text = presetQuery.AverageVideoBitrate;
            mainWindow.text_filesize.Text = presetQuery.VideoTargetSize;
            mainWindow.slider_videoQuality.Value = presetQuery.VideoQuality;
            if (mainWindow.slider_videoQuality.Value != 0)
            {
                int ql = presetQuery.VideoQuality;
                mainWindow.SliderValue.Text = ql.ToString() + "%";
            }

            if (presetQuery.TwoPass == true)
                mainWindow.check_2PassEncode.CheckState = CheckState.Checked;
            else
                mainWindow.check_2PassEncode.CheckState = CheckState.Unchecked;

            if (presetQuery.Grayscale == true)
                mainWindow.check_grayscale.CheckState = CheckState.Checked;
            else
                mainWindow.check_grayscale.CheckState = CheckState.Unchecked;

            mainWindow.drp_videoFramerate.Text = presetQuery.VideoFramerate;

            if (presetQuery.TurboFirstPass == true)
                mainWindow.check_turbo.CheckState = CheckState.Checked;
            else
                mainWindow.check_turbo.CheckState = CheckState.Unchecked;

            if (presetQuery.LargeMP4 == true)
                mainWindow.check_largeFile.CheckState = CheckState.Checked;
            else
            {
                mainWindow.check_largeFile.CheckState = CheckState.Unchecked;
                mainWindow.check_largeFile.BackColor = Color.Transparent;
            }
            #endregion

            // Chapter Markers Tab
            #region Chapter Markers

            if (presetQuery.ChapterMarkers == true)
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Checked;
            else
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Unchecked;

            #endregion

            // Audio Settings Tab
            #region Audio

            // Handle Track 1
            mainWindow.drp_track1Audio.Text = "Automatic";

            // Handle Track 2
            if (presetQuery.AudioEncoder2 != null)  // Fix for loading in built in presets. Where 2 encoders but no tracks in the preset.
            {
                mainWindow.drp_track2Audio.Enabled = true;
                mainWindow.drp_audsr_2.Enabled = true;
                mainWindow.drp_audmix_2.Enabled = true;
                mainWindow.drp_audenc_2.Enabled = true;
                mainWindow.drp_audbit_2.Enabled = true;
                mainWindow.drp_audsr_2.Text = "48";
                mainWindow.drp_track2Audio.Text = "Automatic";
            }
            else if (presetQuery.AudioTrack2 == "None")
            {
                mainWindow.drp_track2Audio.Text = "None";
                mainWindow.drp_track2Audio.SelectedIndex = 0;
                mainWindow.drp_audsr_2.Enabled = false;
                mainWindow.drp_audmix_2.Enabled = false;
                mainWindow.drp_audenc_2.Enabled = false;
                mainWindow.drp_audbit_2.Enabled = false;
            }
            else
            {
                mainWindow.drp_track2Audio.Text = presetQuery.AudioTrack2;
                mainWindow.drp_audsr_2.Enabled = true;
                mainWindow.drp_audmix_2.Enabled = true;
                mainWindow.drp_audenc_2.Enabled = true;
                mainWindow.drp_audbit_2.Enabled = true;
            }

            // Handle Track 3
            if (presetQuery.AudioTrack3 == "None")
            {
                mainWindow.drp_track3Audio.SelectedIndex = 0;
                mainWindow.drp_audsr_3.Enabled = false;
                mainWindow.drp_audmix_3.Enabled = false;
                mainWindow.drp_audenc_3.Enabled = false;
                mainWindow.drp_audbit_3.Enabled = false;
                mainWindow.trackBar3.Enabled = false;

                mainWindow.drp_track3Audio.Text = "None";
                mainWindow.drp_audsr_3.Text = "";
                mainWindow.drp_audmix_3.Text = "Automatic";
                mainWindow.drp_audenc_3.Text = "";
                mainWindow.drp_audbit_3.Text = "";
                mainWindow.trackBar3.Value = 0;

            }
            else
            {
                mainWindow.drp_track3Audio.Text = presetQuery.AudioTrack3;
                mainWindow.drp_audsr_3.Enabled = true;
                mainWindow.drp_audmix_3.Enabled = true;
                mainWindow.drp_audenc_3.Enabled = true;
                mainWindow.drp_audbit_3.Enabled = true;
                mainWindow.trackBar3.Enabled = true;
            }

            // Handle Track 4
            if (presetQuery.AudioTrack4 == "None")
            {
                mainWindow.drp_track4Audio.SelectedIndex = 0;
                mainWindow.drp_audsr_4.Enabled = false;
                mainWindow.drp_audmix_4.Enabled = false;
                mainWindow.drp_audenc_4.Enabled = false;
                mainWindow.drp_audbit_4.Enabled = false;
                mainWindow.trackBar4.Enabled = false;

                mainWindow.drp_track4Audio.Text = "None";
                mainWindow.drp_audsr_4.Text = "";
                mainWindow.drp_audmix_4.Text = "Automatic";
                mainWindow.drp_audenc_4.Text = "";
                mainWindow.drp_audbit_4.Text = "";
                mainWindow.trackBar4.Value = 0;
            }
            else
            {
                mainWindow.drp_track4Audio.Text = presetQuery.AudioTrack4;
                mainWindow.drp_audsr_4.Enabled = true;
                mainWindow.drp_audmix_4.Enabled = true;
                mainWindow.drp_audenc_4.Enabled = true;
                mainWindow.drp_audbit_4.Enabled = true;
                mainWindow.trackBar4.Enabled = true;
            }

            // Now lets start setting stuff
            if (presetQuery.AudioEncoder1 != null)
                mainWindow.drp_audenc_1.Text = presetQuery.AudioEncoder1;
            mainWindow.drp_audenc_2.Text = presetQuery.AudioEncoder2;
            mainWindow.drp_audenc_3.Text = presetQuery.AudioEncoder3;
            mainWindow.drp_audenc_4.Text = presetQuery.AudioEncoder4;

            mainWindow.drp_audmix_1.Text = presetQuery.AudioTrackMix1;
            mainWindow.drp_audmix_2.Text = presetQuery.AudioTrackMix2;
            mainWindow.drp_audmix_3.Text = presetQuery.AudioTrackMix3;
            mainWindow.drp_audmix_4.Text = presetQuery.AudioTrackMix4;

            if (presetQuery.AudioBitrate1 != null)
                mainWindow.drp_audbit_1.Text = presetQuery.AudioBitrate1;
            mainWindow.drp_audbit_2.Text = presetQuery.AudioBitrate2;
            mainWindow.drp_audbit_3.Text = presetQuery.AudioBitrate4;
            mainWindow.drp_audbit_3.Text = presetQuery.AudioBitrate4;

            if (presetQuery.AudioSamplerate1 != null)
                mainWindow.drp_audsr_1.Text = presetQuery.AudioSamplerate1;
            mainWindow.drp_audsr_2.Text = presetQuery.AudioSamplerate2;
            mainWindow.drp_audsr_3.Text = presetQuery.AudioSamplerate3;
            mainWindow.drp_audsr_4.Text = presetQuery.AudioSamplerate4;

            // Dynamic Range Compression (Should be a float but we use double for ease)
            double value = 0;
            double actualValue = 0;

            value = presetQuery.DRC1;
            if (value > 0)
                value = value - 10;
            mainWindow.trackBar1.Value = int.Parse(value.ToString());
            actualValue = presetQuery.DRC1 / 10;
            mainWindow.lbl_drc1.Text = actualValue.ToString();

            value = presetQuery.DRC2;
            if (value > 0)
                value = value - 10;
            mainWindow.trackBar2.Value = int.Parse(value.ToString());
            actualValue = presetQuery.DRC2 / 10;
            mainWindow.lbl_drc2.Text = actualValue.ToString();

            value = presetQuery.DRC3;
            if (value > 0)
                value = value - 10;
            mainWindow.trackBar3.Value = int.Parse(value.ToString());
            actualValue = presetQuery.DRC3 / 10;
            mainWindow.lbl_drc3.Text = actualValue.ToString();

            value = presetQuery.DRC4;
            if (value > 0)
                value = value - 10;
            mainWindow.trackBar4.Value = int.Parse(value.ToString());
            actualValue = presetQuery.DRC4 / 10;
            mainWindow.lbl_drc4.Text = actualValue.ToString();


            // Subtitle Stuff
            mainWindow.drp_subtitle.Text = presetQuery.Subtitles;

            if (presetQuery.ForcedSubtitles == true)
            {
                mainWindow.check_forced.CheckState = CheckState.Checked;
                mainWindow.check_forced.Enabled = true;
            }
            else
                mainWindow.check_forced.CheckState = CheckState.Unchecked;


            #endregion

            // H264 Tab & Preset Name
            #region other
            mainWindow.rtf_x264Query.Text = presetQuery.H264Query;

            // Set the preset name
            mainWindow.groupBox_output.Text = "Output Settings (Preset: " + name + ")";
            #endregion
        }

        /// <summary>
        /// Select the longest title in the DVD title dropdown menu on frmMain
        /// </summary>
        public void selectLongestTitle(frmMain mainWindow)
        {
            int current_largest = 0;
            Handbrake.Parsing.Title title2Select;

            // Check if there are titles in the DVD title dropdown menu and make sure, it's not just "Automatic"
            if (mainWindow.drp_dvdtitle.Items[0].ToString() != "Automatic")
                title2Select = (Handbrake.Parsing.Title)mainWindow.drp_dvdtitle.Items[0];
            else
                title2Select = null;

            // So, If there are titles in the DVD Title dropdown menu, lets select the longest.
            if (title2Select != null)
            {
                foreach (Handbrake.Parsing.Title x in mainWindow.drp_dvdtitle.Items)
                {
                    string title = x.ToString();
                    if (title != "Automatic")
                    {
                        string[] y = title.Split(' ');
                        string time = y[1].Replace("(", "").Replace(")", "");
                        string[] z = time.Split(':');

                        int hours = int.Parse(z[0]) * 60 * 60;
                        int minutes = int.Parse(z[1]) * 60;
                        int seconds = int.Parse(z[2]);
                        int total_sec = hours + minutes + seconds;

                        if (current_largest == 0)
                        {
                            current_largest = hours + minutes + seconds;
                            title2Select = x;
                        }
                        else
                        {
                            if (total_sec > current_largest)
                            {
                                current_largest = total_sec;
                                title2Select = x;
                            }
                        }
                    }
                }

                // Now set the longest title in the gui.
                mainWindow.drp_dvdtitle.SelectedItem = title2Select;
            }
        }

        /// <summary>
        /// Set's up the DataGridView on the Chapters tab (frmMain)
        /// </summary>
        /// <param name="mainWindow"></param>
        public void chapterNaming(frmMain mainWindow)
        {
            try
            {
                mainWindow.data_chpt.Rows.Clear();
                int i = 0;
                int rowCount = 0;
                int start = 0;
                int finish = 0;
                if (mainWindow.drop_chapterFinish.Text != "Auto")
                    finish = int.Parse(mainWindow.drop_chapterFinish.Text);

                if (mainWindow.drop_chapterStart.Text != "Auto")
                    start = int.Parse(mainWindow.drop_chapterStart.Text);

                rowCount = finish - (start - 1);

                while (i < rowCount)
                {
                    DataGridViewRow row = new DataGridViewRow();

                    mainWindow.data_chpt.Rows.Insert(i, row);
                    mainWindow.data_chpt.Rows[i].Cells[0].Value = (i + 1);
                    mainWindow.data_chpt.Rows[i].Cells[1].Value = "Chapter " + (i + 1);
                    i++;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("chapterNaming() Error has occured: \n" + exc.ToString());
            }
        }

        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        /// <param name="mainWindow"></param>
        public void autoName(frmMain mainWindow)
        {
            if (Properties.Settings.Default.autoNaming == "Checked")
            {
                if (mainWindow.drp_dvdtitle.Text != "Automatic")
                {
                    // Todo: This code is a tad messy. Clean it up at some point.
                    // Get the Source Name
                    string source = mainWindow.text_source.Text;
                    string[] sourceName = source.Split('\\');
                    source = sourceName[sourceName.Length - 1].Replace(".iso", "").Replace(".mpg", "").Replace(".ts", "").Replace(".ps", "");

                    // Get the Selected Title Number
                    string title = mainWindow.drp_dvdtitle.Text;
                    string[] titlesplit = title.Split(' ');
                    title = titlesplit[0];

                    // Get the Chapter Start and Chapter End Numbers
                    string cs = mainWindow.drop_chapterStart.Text;
                    string cf = mainWindow.drop_chapterFinish.Text;

                    // Just incase the above are set to their default Automatic values, set the varible to ""
                    if (title == "Automatic")
                        title = "";
                    if (cs == "Auto")
                        cs = "";
                    if (cf == "Auto")
                        cf = "";

                    // If both CS and CF are populated, set the dash varible
                    string dash = "";
                    if (cf != "Auto")
                        dash = "-";

                    // Get the destination filename.
                    string destination_filename = "";
                    if (Properties.Settings.Default.autoNameFormat != "")
                    {
                        destination_filename = Properties.Settings.Default.autoNameFormat;
                        destination_filename = destination_filename.Replace("{source}", source).Replace("{title}", title).Replace("{chapters}", cs + dash + cf);
                    }
                    else
                        destination_filename = source + "_T" + title + "_C" + cs + dash + cf;

                    // If the text box is blank
                    if (!mainWindow.text_destination.Text.Contains("\\"))
                    {
                        string filePath = "";
                        if (Properties.Settings.Default.autoNamePath.Trim() != "")
                        {
                            if (Properties.Settings.Default.autoNamePath.Trim() != "Click 'Browse' to set the default location")
                                filePath = Properties.Settings.Default.autoNamePath + "\\";
                        }

                        if (mainWindow.drop_format.SelectedIndex == 0)
                            mainWindow.text_destination.Text = filePath + destination_filename + ".mp4";
                        else if (mainWindow.drop_format.SelectedIndex == 1)
                            mainWindow.text_destination.Text = filePath + destination_filename + ".m4v";
                        else if (mainWindow.drop_format.SelectedIndex == 2)
                            mainWindow.text_destination.Text = filePath + destination_filename + ".mkv";
                        else if (mainWindow.drop_format.SelectedIndex == 3)
                            mainWindow.text_destination.Text = filePath + destination_filename + ".avi";
                        else if (mainWindow.drop_format.SelectedIndex == 4)
                            mainWindow.text_destination.Text = filePath + destination_filename + ".ogm";
                    }
                    else // If the text box already has a path and file
                    {
                        string dest = mainWindow.text_destination.Text;
                        string[] destName = dest.Split('\\');
                        string[] extension = dest.Split('.');
                        string ext = extension[extension.Length - 1];

                        destName[destName.Length - 1] = destination_filename + "." + ext;

                        string fullDest = "";
                        foreach (string part in destName)
                        {
                            if (fullDest != "")
                                fullDest = fullDest + "\\" + part;
                            else
                                fullDest = fullDest + part;
                        }

                        mainWindow.text_destination.Text = fullDest;
                    }
                }
            }
        }

        /// <summary>
        /// Checks for updates and returns true if an update is available.
        /// </summary>
        /// <param name="debug">Turns on debug mode. Don't use on program startup</param>
        /// <returns>Boolean True = Update available</returns>
        public Boolean updateCheck(Boolean debug)
        {
            try
            {
                Functions.AppcastReader rssRead = new Functions.AppcastReader();
                rssRead.getInfo(); // Initializes the class.
                string build = rssRead.build();

                int latest = int.Parse(build);
                int current = Properties.Settings.Default.hb_build;
                int skip = Properties.Settings.Default.skipversion;

                if (latest == skip)
                    return false;
                else
                {
                    Boolean update = (latest > current);
                    return update;
                }
            }
            catch (Exception exc)
            {
                if (debug == true)
                    MessageBox.Show("Unable to check for updates, Please try again later. \n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }
        }

        /// <summary>
        /// Get's HandBrakes version data from the CLI.
        /// </summary>
        /// <returns>Arraylist of Version Data. 0 = hb_version 1 = hb_build</returns>
        public ArrayList getCliVersionData()
        {
            ArrayList cliVersionData = new ArrayList();
            // 0 = SVN Build / Version
            // 1 = Build Date

            Process cliProcess = new Process();
            ProcessStartInfo handBrakeCLI = new ProcessStartInfo("HandBrakeCLI.exe", " -u");
            handBrakeCLI.UseShellExecute = false;
            handBrakeCLI.RedirectStandardError = true;
            handBrakeCLI.RedirectStandardOutput = true;
            handBrakeCLI.CreateNoWindow = true;
            cliProcess.StartInfo = handBrakeCLI;
            cliProcess.Start();

            // Retrieve standard output and report back to parent thread until the process is complete
            String line;
            TextReader stdOutput = cliProcess.StandardError;

            while (!cliProcess.HasExited)
            {
                line = stdOutput.ReadLine();
                Match m = Regex.Match(line, @"HandBrake [0-9\.]*svn[0-9]*[M]* \([0-9]*\)");
                if (m.Success != false)
                {
                    string data = line.Replace("(", "").Replace(")", "").Replace("HandBrake ", "");
                    string[] arr = data.Split(' ');
                    cliVersionData.Add(arr[0]);
                    cliVersionData.Add(arr[1]);
                    return cliVersionData;
                }
            }
            return null;
        }

        /// <summary>
        /// Check if the queue recovery file contains records.
        /// If it does, it means the last queue did not complete before HandBrake closed.
        /// So, return a boolean if true. 
        /// </summary>
        public Boolean check_queue_recovery()
        {
            try
            {
                string tempPath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.dat");
                using (StreamReader reader = new StreamReader(tempPath))
                {
                    string queue_item = reader.ReadLine();
                    if (queue_item == null)
                    {
                        reader.Close();
                        reader.Dispose();
                        return false;
                    }
                    else // There exists an item in the recovery queue file, so try and recovr it.
                    {
                        reader.Close();
                        reader.Dispose();
                        return true;
                    }
                }
            }
            catch (Exception)
            {
                // Keep quiet about the error.
                return false;
            }
        }

    }
}