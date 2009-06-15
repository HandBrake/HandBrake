/*  QueryGenerator.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using System.Collections.Generic;

namespace Handbrake.Functions
{
    class QueryGenerator
    {
        /// <summary>
        /// Generates a CLI query based on the GUI widgets.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns>The CLI String</returns>
        public string generateTheQuery(frmMain mainWindow)
        {
            // Source tab
            string query = "";

            if ((mainWindow.text_source.Text != "") && (mainWindow.text_source.Text.Trim() != "Click 'Source' to continue"))
                query = " -i " + '"' + mainWindow.text_source.Text + '"';

            if (mainWindow.drp_dvdtitle.Text != "Automatic")
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            if (Properties.Settings.Default.dvdnav == "Checked")
                if (mainWindow.drop_angle.Items.Count != 0)
                    query += " --angle " + mainWindow.drop_angle.SelectedItem;

            if (mainWindow.drop_chapterFinish.Text == mainWindow.drop_chapterStart.Text && mainWindow.drop_chapterStart.Text != "Auto")
                query += " -c " + mainWindow.drop_chapterStart.Text;
            else if (mainWindow.drop_chapterStart.Text == "Auto" && mainWindow.drop_chapterFinish.Text != "Auto")
                query += " -c " + "0-" + mainWindow.drop_chapterFinish.Text;
            else if (mainWindow.drop_chapterStart.Text != "Auto" && mainWindow.drop_chapterFinish.Text != "Auto")
                query += " -c " + mainWindow.drop_chapterStart.Text + "-" + mainWindow.drop_chapterFinish.Text;

            // Destination tab
            if (mainWindow.text_destination.Text != "")
                query += " -o " + '"' + mainWindow.text_destination.Text + '"';

            query += generateTabbedComponentsQuery(mainWindow);
            return query;
        }

        /// <summary>
        /// Generates a CLI query for the preview function.
        /// This basically forces a shortened version of the encdode.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <param name="duration">Duration</param>
        /// <param name="preview">Start at preview</param>
        /// <returns>Returns a CLI query String.</returns>
        public string generatePreviewQuery(frmMain mainWindow, string duration, string preview)
        {
            int seconds;
            int.TryParse(duration, out seconds);

            // Source tab
            string query = "";

            if ((mainWindow.text_source.Text != "") && (mainWindow.text_source.Text.Trim() != "Click 'Source' to continue"))
                query = " -i " + '"' + mainWindow.text_source.Text + '"';

            if (mainWindow.drp_dvdtitle.Text != "Automatic")
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            if (mainWindow.drop_angle.SelectedIndex != 0)
                query += " --angle " + mainWindow.drop_angle.SelectedItem;

            query += " --start-at-preview " + preview;
            query += " --stop-at duration:" + duration + " ";

            // Destination tab
            if (mainWindow.text_destination.Text != "")
                query += " -o " + '"' + mainWindow.text_destination.Text.Replace(".m", "_sample.m") + '"';

            query += generateTabbedComponentsQuery(mainWindow);
            return query;
        }

        /// <summary>
        /// Generates part of the CLI query, for the tabbed components only.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns></returns>
        public static string generateTabbedComponentsQuery(frmMain mainWindow)
        {
            string query = "";

            #region Output Settings Box
            query += " -f " + mainWindow.drop_format.Text.ToLower().Replace(" file", "");

            // These are output settings features
            if (mainWindow.check_largeFile.Checked)
                query += " -4 ";

            if (mainWindow.check_iPodAtom.Checked)
                query += " -I ";

            if (mainWindow.check_optimiseMP4.Checked)
                query += " -O ";
            #endregion

            #region Picture Settings Tab

            // Use MaxWidth for built-in presets and width for user settings.
            if (mainWindow.pictureSettings.maxWidth == 0)
            {

                if (mainWindow.pictureSettings.text_width.Text != "")
                    if (mainWindow.pictureSettings.drp_anamorphic.SelectedIndex != 1) // Prevent usage for strict anamorphic
                        query += " -w " + mainWindow.pictureSettings.text_width.Text;
            }
            else
            {
                if (mainWindow.pictureSettings.text_width.Text != "")
                    query += " -X " + mainWindow.pictureSettings.text_width.Text;
            }

            // Use MaxHeight for built-in presets and height for user settings.
            if (mainWindow.pictureSettings.maxHeight == 0)
            {
                if (mainWindow.pictureSettings.text_height.Text != "0")
                    if (mainWindow.pictureSettings.text_height.Text != "")
                        if (mainWindow.pictureSettings.drp_anamorphic.SelectedIndex == 0 || mainWindow.pictureSettings.drp_anamorphic.SelectedIndex == 3) // Prevent usage for strict anamorphic
                            query += " -l " + mainWindow.pictureSettings.text_height.Text;
            }
            else
            {
                if (mainWindow.pictureSettings.text_height.Text != "")
                    query += " -Y " + mainWindow.pictureSettings.text_height.Text;
            }

            string cropTop = mainWindow.pictureSettings.crop_top.Text;
            string cropBottom = mainWindow.pictureSettings.crop_bottom.Text;
            string cropLeft = mainWindow.pictureSettings.crop_left.Text;
            string cropRight = mainWindow.pictureSettings.crop_right.Text;

            if (mainWindow.pictureSettings.check_customCrop.Checked)
            {
                if (mainWindow.pictureSettings.crop_top.Text == string.Empty)
                    cropTop = "0";
                if (mainWindow.pictureSettings.crop_bottom.Text == string.Empty)
                    cropBottom = "0";
                if (mainWindow.pictureSettings.crop_left.Text == string.Empty)
                    cropLeft = "0";
                if (mainWindow.pictureSettings.crop_right.Text == string.Empty)
                    cropRight = "0";

                query += " --crop " + cropTop + ":" + cropBottom + ":" + cropLeft + ":" + cropRight;
            }

            switch (mainWindow.pictureSettings.drp_anamorphic.SelectedIndex)
            {
                case 1:
                    query += " --strict-anamorphic ";
                    break;
                case 2:
                    query += " --loose-anamorphic ";
                    break;
                case 3:
                    query += " --custom-anamorphic ";
                    query += " --display-width " + mainWindow.pictureSettings.txt_displayWidth.Text + " ";
                    if (mainWindow.pictureSettings.check_KeepAR.Checked)
                        query += " --keep-display-aspect ";
                    if (mainWindow.pictureSettings.txt_parWidth.Text != "" && mainWindow.pictureSettings.txt_parHeight.Text != "")
                        query += " --pixel-aspect " + mainWindow.pictureSettings.txt_parWidth.Text + ":" + mainWindow.pictureSettings.txt_parHeight.Text + " ";
                    break;
            }
            #endregion

            #region Filters
            query += mainWindow.ctl_detelecine.getCLIQuery;
            query += mainWindow.ctl_decomb.getCLIQuery;
            query += mainWindow.ctl_deinterlace.getCLIQuery;
            query += mainWindow.ctl_denoise.getCLIQuery;

            if (mainWindow.slider_deblock.Value != 4)
                query += " --deblock=" + mainWindow.slider_deblock.Value;
            #endregion

            #region Video Settings Tab

            switch (mainWindow.drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    query += " -e ffmpeg";
                    break;
                case "H.264 (x264)":
                    query += " -e x264";
                    break;
                case "VP3 (Theora)":
                    query += " -e theora";
                    break;
                default:
                    query += " -e x264";
                    break;
            }

            if (mainWindow.check_grayscale.Checked)
                query += " -g ";

            // Video Settings
            if (mainWindow.radio_avgBitrate.Checked)
                query += " -b " + mainWindow.text_bitrate.Text;

            if (mainWindow.radio_targetFilesize.Checked)
                query += " -S " + mainWindow.text_filesize.Text;

            // Video Quality Setting
            if (mainWindow.radio_cq.Checked)
            {
                double value;
                switch (mainWindow.drp_videoEncoder.Text)
                {
                    case "MPEG-4 (FFmpeg)":
                        value = 31 - (mainWindow.slider_videoQuality.Value - 1);
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                    case "H.264 (x264)":
                        double divided;
                        CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
                        double.TryParse(Properties.Settings.Default.x264cqstep,
                                        NumberStyles.Number,
                                        culture,
                                        out divided);
                        value = 51 - mainWindow.slider_videoQuality.Value * divided;
                        value = Math.Round(value, 2);
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                    case "VP3 (Theora)":
                        value = mainWindow.slider_videoQuality.Value;
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                }
            }

            if (mainWindow.check_2PassEncode.Checked)
                query += " -2 ";

            if (mainWindow.check_turbo.Checked)
                query += " -T ";

            if (mainWindow.drp_videoFramerate.Text != "Same as source")
                query += " -r " + mainWindow.drp_videoFramerate.Text;
            #endregion

            #region Audio Settings Tab

            ListView audioTracks = mainWindow.audioPanel.getAudioPanel();
            List<string> tracks = new List<string>();
            List<string> codecs = new List<string>();
            List<string> mixdowns = new List<string>();
            List<string> samplerates = new List<string>();
            List<string> bitrates = new List<string>();
            List<string> drcs = new List<string>();

            // No Audio
            if (audioTracks.Items.Count == 0)
                query += " -a none ";

            // Gather information about each audio track and store them in the declared lists.
            foreach (ListViewItem row in audioTracks.Items)
            {
                // Audio Track (-a)
                if (row.Text == "Automatic")
                    tracks.Add("1");
                else if (row.Text != "None")
                {
                    string[] tempSub = row.SubItems[1].Text.Split(' ');
                    tracks.Add(tempSub[0]);
                }

                // Audio Codec (-E)
                if (row.SubItems[1].Text != String.Empty)
                    codecs.Add(getAudioEncoder(row.SubItems[2].Text));

                // Audio Mixdown (-6)
                if (row.SubItems[2].Text != String.Empty)
                    mixdowns.Add(getMixDown(row.SubItems[3].Text));

                // Sample Rate (-R)
                if (row.SubItems[3].Text != String.Empty)
                    samplerates.Add(row.SubItems[4].Text.Replace("Auto", "Auto"));

                // Audio Bitrate (-B)
                if (row.SubItems[4].Text != String.Empty)
                    bitrates.Add(row.SubItems[5].Text.Replace("Auto", "auto"));

                // DRC (-D)
                if (row.SubItems[5].Text != String.Empty)
                    drcs.Add(row.SubItems[6].Text);
            }

            // Audio Track (-a)
            String audioItems = "";
            Boolean firstLoop = true;

            foreach (String item in tracks)
            {
                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -a " + audioItems;
            firstLoop = true; audioItems = ""; // Reset for another pass.

            // Audio Codec (-E)
            foreach (String item in codecs)
            {

                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -E " + audioItems;
            firstLoop = true; audioItems = ""; // Reset for another pass.

            // Audio Mixdown (-6)
            foreach (String item in mixdowns)
            {
                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -6 " + audioItems;
            firstLoop = true; audioItems = ""; // Reset for another pass.

            // Sample Rate (-R)
            foreach (String item in samplerates)
            {
                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -R " + audioItems;
            firstLoop = true; audioItems = ""; // Reset for another pass.

            // Audio Bitrate (-B)
            foreach (String item in bitrates)
            {
                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -B " + audioItems;
            firstLoop = true; audioItems = ""; // Reset for another pass.

            // DRC (-D)
            foreach (String item in drcs)
            {
                if (firstLoop)
                {
                    audioItems = item; firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -D " + audioItems;

            #endregion

            #region Subtitles Tab

            if (mainWindow.Subtitles.lv_subList.Items.Count != 0) // If we have subtitle tracks
            {
                // Find --subtitle <string>
                query += " --subtitle ";
                String subtitleTracks = "";
                String itemToAdd;
                foreach (ListViewItem item in mainWindow.Subtitles.lv_subList.Items)
                {
                    if (item.SubItems[1].Text.Contains("Foreign Audio Search"))
                        itemToAdd = "scan";
                    else
                    {
                        string[] tempSub = item.SubItems[1].Text.Split(' ');
                        itemToAdd = tempSub[0];
                    }

                    subtitleTracks += subtitleTracks == "" ? itemToAdd : "," + itemToAdd;
                }
                query += subtitleTracks;


                // Find --subtitle-forced
                String forcedTrack = "";
                foreach (ListViewItem item in mainWindow.Subtitles.lv_subList.Items)
                {
                    itemToAdd = "";
                    string[] tempSub = item.SubItems[1].Text.Split(' ');
                    string trackID = tempSub[0];

                    if (item.SubItems[2].Text == "Yes")
                        itemToAdd = trackID;

                    if (itemToAdd != "")
                        forcedTrack += forcedTrack == "" ? itemToAdd : "," + itemToAdd;
                }
                if (forcedTrack != "")
                    query += " --subtitle-forced " + forcedTrack;


                // Find --subtitle-burn and --subtitle-default
                String burned = "";
                String defaultTrack = "";
                foreach (ListViewItem item in mainWindow.Subtitles.lv_subList.Items)
                {
                    string[] tempSub = item.SubItems[1].Text.Split(' ');
                    string trackID = tempSub[0];

                    if (trackID.Trim() == "Foreign")
                        trackID = "scan";

                    if (item.SubItems[3].Text == "Yes") // Burned
                        burned = trackID;

                    if (item.SubItems[4].Text == "Yes") // Burned
                        defaultTrack = trackID;

                }
                if (burned != "")
                    query += " --subtitle-burn " + burned;

                if (defaultTrack != "")
                    query += " --subtitle-default " + defaultTrack;

            }

            #endregion

            #region Chapter Markers

            // Attach Source name and dvd title to the start of the chapters.csv filename.
            // This is for the queue. It allows different chapter name files for each title.
            string[] destName = mainWindow.text_destination.Text.Split('\\');
            string dest_name = destName[destName.Length - 1];
            dest_name = dest_name.Replace("\"", "");
            dest_name = dest_name.Replace(".mp4", "").Replace(".m4v", "").Replace(".mkv", "");

            string source_title = mainWindow.drp_dvdtitle.Text;
            string[] titlesplit = source_title.Split(' ');
            source_title = titlesplit[0];

            if (mainWindow.Check_ChapterMarkers.Checked)
            {
                if (dest_name.Trim() != String.Empty)
                {
                    string path = source_title != "Automatic"
                                      ? Path.Combine(Path.GetTempPath(), dest_name + "-" + source_title + "-chapters.csv")
                                      : Path.Combine(Path.GetTempPath(), dest_name + "-chapters.csv");

                    if (chapterCSVSave(mainWindow, path) == false)
                        query += " -m ";
                    else
                        query += " --markers=" + "\"" + path + "\"";
                }
                else
                    query += " -m";
            }
            #endregion

            #region  H264 Tab
            if (mainWindow.x264Panel.x264Query != "")
                query += " -x " + mainWindow.x264Panel.x264Query;
            #endregion

            #region Processors / Other
            string processors = Properties.Settings.Default.Processors;
            if (processors != "Automatic")
                query += " -C " + processors + " ";

            query += " -v " + Properties.Settings.Default.verboseLevel;

            if (Properties.Settings.Default.dvdnav == "Checked")
                query += " --dvdnav";
            #endregion

            return query;
        }

        private static string getMixDown(string selectedAudio)
        {
            switch (selectedAudio)
            {
                case "Automatic":
                    return "auto";
                case "Mono":
                    return "mono";
                case "Stereo":
                    return "stereo";
                case "Dolby Surround":
                    return "dpl1";
                case "Dolby Pro Logic II":
                    return "dpl2";
                case "6 Channel Discrete":
                    return "6ch";
                default:
                    return "auto";
            }
        }
        private static string getAudioEncoder(string selectedEncoder)
        {
            switch (selectedEncoder)
            {
                case "AAC (faac)":
                    return "faac";
                case "MP3 (lame)":
                    return "lame";
                case "Vorbis (vorbis)":
                    return "vorbis";
                case "AC3 Passthru":
                    return "ac3";
                case "DTS Passthru":
                    return "dts";
                default:
                    return "";
            }
        }
        private static Boolean chapterCSVSave(frmMain mainWindow, string filePathName)
        {
            try
            {
                string csv = "";

                foreach (DataGridViewRow row in mainWindow.data_chpt.Rows)
                {
                    csv += row.Cells[0].Value.ToString();
                    csv += ",";
                    csv += row.Cells[1].Value.ToString();
                    csv += Environment.NewLine;
                }
                StreamWriter file = new StreamWriter(filePathName);
                file.Write(csv);
                file.Close();
                file.Dispose();
                return true;

            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to save Chapter Makrers file! \nChapter marker names will NOT be saved in your encode \n\n" + exc, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return false;
            }
        }
    }
}