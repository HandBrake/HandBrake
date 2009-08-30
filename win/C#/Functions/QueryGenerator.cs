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
        /// Generates a full CLI query for either encoding or previe encoeds if duration and preview are defined.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <param name="duration"></param>
        /// <param name="preview"></param>
        /// <returns></returns>
        public string GenerateCLIQuery(frmMain mainWindow, int duration, string preview)
        {
            string query = "";

            if (!string.IsNullOrEmpty(mainWindow.sourcePath))
                if (mainWindow.sourcePath.Trim() != "Select \"Source\" to continue")
                    query = " -i " + '"' + mainWindow.sourcePath + '"';

            if (mainWindow.drp_dvdtitle.Text != "")
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            if (Properties.Settings.Default.dvdnav)
                if (mainWindow.drop_angle.Items.Count != 0)
                    query += " --angle " + mainWindow.drop_angle.SelectedItem;


            if (duration != 0 && preview != null) // Preivew Query
            {
                query += " --start-at-preview " + preview;
                query += " --stop-at duration:" + duration + " ";

                if (mainWindow.text_destination.Text != "")
                    query += " -o " + '"' + mainWindow.text_destination.Text.Replace(".m", "_sample.m") + '"';
            }
            else // Non Preview Query
            {
                if (mainWindow.drop_chapterFinish.Text == mainWindow.drop_chapterStart.Text && mainWindow.drop_chapterStart.Text != "")
                    query += " -c " + mainWindow.drop_chapterStart.Text;
                else if (mainWindow.drop_chapterStart.Text == "Auto" && mainWindow.drop_chapterFinish.Text != "Auto")
                    query += " -c " + "0-" + mainWindow.drop_chapterFinish.Text;
                else if (mainWindow.drop_chapterStart.Text != "Auto" && mainWindow.drop_chapterFinish.Text != "Auto" && mainWindow.drop_chapterStart.Text != "")
                    query += " -c " + mainWindow.drop_chapterStart.Text + "-" + mainWindow.drop_chapterFinish.Text;

                if (mainWindow.text_destination.Text != "")
                    query += " -o " + '"' + mainWindow.text_destination.Text + '"';
            }

            query += GenerateTabbedComponentsQuery(mainWindow);

            return query;
        }

        /// <summary>
        /// Generates part of the CLI query, for the tabbed components only.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns></returns>
        public static string GenerateTabbedComponentsQuery(frmMain mainWindow)
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
            if (mainWindow.PictureSettings.PresetMaximumResolution.Width == 0)
            {
                if (mainWindow.PictureSettings.text_width.Value != 0)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex != 1) // Prevent usage for strict anamorphic
                        query += " -w " + mainWindow.PictureSettings.text_width.Text;
            }
            else
            {
                if (mainWindow.PictureSettings.text_width.Value != 0)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex != 1)
                        query += " -X " + mainWindow.PictureSettings.text_width.Text;
            }

            // Use MaxHeight for built-in presets and height for user settings.
            if (mainWindow.PictureSettings.PresetMaximumResolution.Height == 0)
            {
                if (mainWindow.PictureSettings.text_height.Value != 0)
                    if (mainWindow.PictureSettings.text_height.Text != "")
                        if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 0 || mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 3) // Prevent usage for strict anamorphic
                            query += " -l " + mainWindow.PictureSettings.text_height.Text;
            }
            else
            {
                if (mainWindow.PictureSettings.text_height.Value != 0)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 0 || mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 3)
                        query += " -Y " + mainWindow.PictureSettings.text_height.Text;
            }

            string cropTop = mainWindow.PictureSettings.crop_top.Text;
            string cropBottom = mainWindow.PictureSettings.crop_bottom.Text;
            string cropLeft = mainWindow.PictureSettings.crop_left.Text;
            string cropRight = mainWindow.PictureSettings.crop_right.Text;

            if (mainWindow.PictureSettings.check_customCrop.Checked)
            {
                if (mainWindow.PictureSettings.crop_top.Text == string.Empty)
                    cropTop = "0";
                if (mainWindow.PictureSettings.crop_bottom.Text == string.Empty)
                    cropBottom = "0";
                if (mainWindow.PictureSettings.crop_left.Text == string.Empty)
                    cropLeft = "0";
                if (mainWindow.PictureSettings.crop_right.Text == string.Empty)
                    cropRight = "0";

                query += " --crop " + cropTop + ":" + cropBottom + ":" + cropLeft + ":" + cropRight;
            }

            switch (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex)
            {
                case 1:
                    query += " --strict-anamorphic ";
                    break;
                case 2:
                    query += " --loose-anamorphic ";
                    break;
                case 3:
                    query += " --custom-anamorphic ";
                    query += " --display-width " + mainWindow.PictureSettings.updownDisplayWidth.Text + " ";
                    if (mainWindow.PictureSettings.check_KeepAR.Checked)
                        query += " --keep-display-aspect ";

                    if (!mainWindow.PictureSettings.check_KeepAR.Checked)
                        if (mainWindow.PictureSettings.updownParWidth.Text != "" && mainWindow.PictureSettings.updownParHeight.Text != "")
                            query += " --pixel-aspect " + mainWindow.PictureSettings.updownParWidth.Text + ":" + mainWindow.PictureSettings.updownParHeight.Text + " ";
                    break;
            }
            #endregion

            #region Filters
            query += mainWindow.Filters.getCLIQuery;
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

            // Video Settings
            if (mainWindow.radio_avgBitrate.Checked)
                query += " -b " + mainWindow.text_bitrate.Text;

            if (mainWindow.radio_targetFilesize.Checked)
                query += " -S " + mainWindow.text_filesize.Text;

            // Video Quality Setting
            if (mainWindow.radio_cq.Checked)
            {
                double cqStep = Properties.Settings.Default.x264cqstep;
                double value;
                switch (mainWindow.drp_videoEncoder.Text)
                {
                    case "MPEG-4 (FFmpeg)":
                        value = 31 - (mainWindow.slider_videoQuality.Value - 1);
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                    case "H.264 (x264)":
                        CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
                        value = 51 - mainWindow.slider_videoQuality.Value * cqStep;
                        value = Math.Round(value, 2);
                        query += " -q " + value.ToString(culture);
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

            ListView audioTracks = mainWindow.AudioSettings.GetAudioPanel();
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
                if (row.SubItems[1].Text == "Automatic")
                    tracks.Add("1");
                else if (row.Text != "None")
                {
                    string[] tempSub = row.SubItems[1].Text.Split(' ');
                    tracks.Add(tempSub[0]);
                }

                // Audio Codec (-E)
                if (row.SubItems[2].Text != String.Empty)
                    codecs.Add(getAudioEncoder(row.SubItems[2].Text));

                // Audio Mixdown (-6)
                if (row.SubItems[3].Text != String.Empty)
                    mixdowns.Add(getMixDown(row.SubItems[3].Text));

                // Sample Rate (-R)
                if (row.SubItems[4].Text != String.Empty)
                    samplerates.Add(row.SubItems[4].Text.Replace("Auto", "Auto"));

                // Audio Bitrate (-B)
                if (row.SubItems[5].Text != String.Empty)
                    bitrates.Add(row.SubItems[5].Text.Replace("Auto", "auto"));

                // DRC (-D)
                if (row.SubItems[6].Text != String.Empty)
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
                IDictionary<string, string> langMap = Main.mapLanguages();

                // BitMap and CC's
                string subtitleTracks = String.Empty;
                string subtitleForced = String.Empty;
                string subtitleBurn = String.Empty;
                string subtitleDefault = String.Empty;

                // SRT
                string srtFile = String.Empty;
                string srtCodeset = String.Empty;
                string srtOffset = String.Empty;
                string srtLang = String.Empty;
                string srtDefault = String.Empty;
                int srtCount = 0;

                foreach (ListViewItem item in mainWindow.Subtitles.lv_subList.Items)
                {
                    string itemToAdd, trackID;

                    if (item.SubItems.Count != 5) // We have an SRT file
                    {
                        srtCount++; // SRT track id.
                        string[] trackData = item.SubItems[1].Text.Split(',');
                        if (trackData != null)
                        {
                            string charCode = trackData[1].Replace("(", "").Replace(")", "").Trim();
                            string realLangCode = langMap[trackData[0].Trim()];

                            srtLang += srtLang == "" ? realLangCode : "," + realLangCode;
                            srtCodeset += srtCodeset == "" ? charCode : "," + charCode;
                        }
                        if (item.SubItems[4].Text == "Yes") // default
                            srtDefault = srtCount.ToString();

                        itemToAdd = item.SubItems[5].Text;
                        srtFile += srtFile == "" ? itemToAdd : "," + itemToAdd;

                        itemToAdd = item.SubItems[6].Text;
                        srtOffset += srtOffset == "" ? itemToAdd : "," + itemToAdd;
                    }
                    else // We have Bitmap or CC
                    {
                        string[] tempSub;

                        // Find --subtitle <string>
                        if (item.SubItems[1].Text.Contains("Foreign Audio Search"))
                            itemToAdd = "scan";
                        else
                        {
                            tempSub = item.SubItems[1].Text.Split(' ');
                            itemToAdd = tempSub[0];
                        }

                        subtitleTracks += subtitleTracks == "" ? itemToAdd : "," + itemToAdd;

                        // Find --subtitle-forced
                        itemToAdd = "";
                        tempSub = item.SubItems[1].Text.Split(' ');
                        trackID = tempSub[0];

                        if (item.SubItems[2].Text == "Yes")
                            itemToAdd = trackID;

                        if (itemToAdd != "")
                            subtitleForced += subtitleForced == "" ? itemToAdd : "," + itemToAdd;

                        // Find --subtitle-burn and --subtitle-default
                        tempSub = item.SubItems[1].Text.Split(' ');
                        trackID = tempSub[0];

                        if (trackID.Trim() == "Foreign")
                            trackID = "scan";

                        if (item.SubItems[3].Text == "Yes") // burn
                            subtitleBurn = trackID;

                        if (item.SubItems[4].Text == "Yes") // default
                            subtitleDefault = trackID;
                    }
                }

                // Build The CLI Subtitles Query
                if (subtitleTracks != "")
                {
                    query += " --subtitle " + subtitleTracks;

                    if (subtitleForced != "")
                        query += " --subtitle-forced " + subtitleForced;
                    if (subtitleBurn != "")
                        query += " --subtitle-burn " + subtitleBurn;
                    if (subtitleDefault != "")
                        query += " --subtitle-default " + subtitleDefault;
                }

                if (srtFile != "") // SRTs
                {
                    query += " --srt-file " + "\"" + srtFile + "\"";

                    if (srtCodeset != "")
                        query += " --srt-codeset " + srtCodeset;
                    if (srtOffset != "")
                        query += " --srt-offset " + srtOffset;
                    if (srtLang != "")
                        query += " --srt-lang " + srtLang;
                    if (srtDefault != "")
                        query += " --subtitle-default " + srtDefault;
                }

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

            if (mainWindow.Check_ChapterMarkers.Checked && mainWindow.Check_ChapterMarkers.Enabled)
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

            if (Properties.Settings.Default.dvdnav)
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