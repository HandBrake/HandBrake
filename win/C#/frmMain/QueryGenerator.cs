/*  QueryGenerator.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using System.Collections.Generic;

namespace Handbrake
{
    class QueryGenerator
    {
        /// <summary>
        /// Generates a CLI query based on the GUI widgets.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns>The CLI String</returns>
        public string GenerateTheQuery(frmMain mainWindow)
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
        public string GeneratePreviewQuery(frmMain mainWindow, string duration, string preview)
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

            query += " --start-at-preview " + preview;
            query += " --stop-at-duration " + duration + " ";

            // Destination tab
            if (mainWindow.text_destination.Text != "")
                query += " -o " + '"' + mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm") + '"';

            query += generateTabbedComponentsQuery(mainWindow);
            return query;
        }

        /// <summary>
        /// Generates part of the CLI query, for the tabbed components only.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns></returns>
        public string generateTabbedComponentsQuery(frmMain mainWindow)
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
            if (mainWindow.maxWidth == 0)
            {

                if (mainWindow.text_width.Text != "")
                    query += " -w " + mainWindow.text_width.Text;
            }
            else
            {
                if (mainWindow.text_width.Text != "")
                    query += " -X " + mainWindow.text_width.Text;
            }

            // Use MaxHeight for built-in presets and height for user settings.
            if (mainWindow.maxHeight == 0)
            {
                if (mainWindow.text_height.Text != "")
                    query += " -l " + mainWindow.text_height.Text;
            }
            else
            {
                if (mainWindow.text_height.Text != "")
                    query += " -Y " + mainWindow.text_height.Text;
            }

            string cropTop = mainWindow.text_top.Text;
            string cropBottom = mainWindow.text_bottom.Text;
            string cropLeft = mainWindow.text_left.Text;
            string cropRight = mainWindow.text_right.Text;

            if (mainWindow.check_customCrop.Checked)
            {
                if (mainWindow.text_top.Text == string.Empty)
                    cropTop = "0";
                if (mainWindow.text_bottom.Text == string.Empty)
                    cropBottom = "0";
                if (mainWindow.text_left.Text == string.Empty)
                    cropLeft = "0";
                if (mainWindow.text_right.Text == string.Empty)
                    cropRight = "0";

                query += " --crop " + cropTop + ":" + cropBottom + ":" + cropLeft + ":" + cropRight;
            }

            if (mainWindow.drp_anamorphic.SelectedIndex == 1)
                query += " -p ";
            else if (mainWindow.drp_anamorphic.SelectedIndex == 2)
                query += " -P ";

            if (mainWindow.slider_deblock.Value != 4)
                query += " --deblock=" + mainWindow.slider_deblock.Value;

            
            #endregion

            #region Filters
            query += mainWindow.ctl_detelecine.getCLIQuery;
            query += mainWindow.ctl_decomb.getCLIQuery;
            query += mainWindow.ctl_deinterlace.getCLIQuery;
            query += mainWindow.ctl_denoise.getCLIQuery;
            #endregion

            #region Video Settings Tab

            switch (mainWindow.drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    query += " -e ffmpeg";
                    break;
                case "MPEG-4 (XviD)":
                    query += " -e xvid";
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
                float value;
                switch (mainWindow.drp_videoEncoder.Text)
                {
                    case "MPEG-4 (FFmpeg)":
                        value = 31 - (mainWindow.slider_videoQuality.Value -1);
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                    case "MPEG-4 (XviD)":
                        value = 31 - (mainWindow.slider_videoQuality.Value - 1);
                        query += " -q " + value.ToString(new CultureInfo("en-US"));
                        break;
                    case "H.264 (x264)":
                        float divided;
                        float.TryParse(Properties.Settings.Default.x264cqstep, out divided);
                        value = 51 - mainWindow.slider_videoQuality.Value * divided;
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

            ListView audioTracks = mainWindow.lv_audioList;
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
                    string[] tempSub = row.Text.Split(' ');
                    tracks.Add(tempSub[0]);
                }

                // Audio Codec (-E)
                if (row.SubItems[1].Text != String.Empty)
                    codecs.Add(getAudioEncoder(row.SubItems[1].Text));

                // Audio Mixdown (-6)
                if (row.SubItems[2].Text != String.Empty)
                    mixdowns.Add(getMixDown(row.SubItems[2].Text));

                // Sample Rate (-R)
                if (row.SubItems[3].Text != String.Empty)
                    samplerates.Add(row.SubItems[3].Text.Replace("Auto", "0"));

                // Audio Bitrate (-B)
                if (row.SubItems[4].Text != String.Empty)
                    bitrates.Add(row.SubItems[4].Text.Replace("Auto", "0"));

                // DRC (-D)
                if (row.SubItems[5].Text != String.Empty)
                    drcs.Add(row.SubItems[5].Text);
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

            // Subtitles
            string subtitles = mainWindow.drp_subtitle.Text;
            if (subtitles == "Autoselect")
                query += " -U ";
            else if (subtitles != "" && subtitles != "None")
            {
                string[] tempSub;
                tempSub = subtitles.Split(' ');
                query += " -s " + tempSub[0];
            }

            if (mainWindow.check_forced.Checked)
                query += " -F ";

            #endregion

            #region Chapter Markers

            // Attach Source name and dvd title to the start of the chapters.csv filename.
            // This is for the queue. It allows different chapter name files for each title.
            string[] destName =  mainWindow.text_destination.Text.Split('\\');
            string dest_name = destName[destName.Length - 1];
            dest_name = dest_name.Replace("\"", "");
            dest_name = dest_name.Replace(".mp4", "").Replace(".m4v", "").Replace(".avi", "").Replace(".mkv", "").Replace(".ogm", "");

            string source_title = mainWindow.drp_dvdtitle.Text;
            string[] titlesplit = source_title.Split(' ');
            source_title = titlesplit[0];

            if (mainWindow.Check_ChapterMarkers.Checked)
            {
                if (dest_name.Trim() != String.Empty)
                {
                    string path;
                    if (source_title != "Automatic")
                        path = Path.Combine(Path.GetTempPath(), dest_name + "-" + source_title + "-chapters.csv");
                    else
                        path = Path.Combine(Path.GetTempPath(), dest_name + "-chapters.csv");

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
            if (mainWindow.rtf_x264Query.Text != "")
                query += " -x " + mainWindow.rtf_x264Query.Text;
            #endregion

            #region Processors / Other
            string processors = Properties.Settings.Default.Processors;
            if (processors != "Automatic")
                query += " -C " + processors + " ";

            query += " -v ";
            #endregion

            return query;
        }

        /// <summary>
        /// Get the CLI equive of the audio mixdown from the widget name.
        /// </summary>
        /// <param name="selectedAudio"></param>
        /// <returns></returns>
        /// 
        private string getMixDown(string selectedAudio)
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

        /// <summary>
        /// Get the CLI equiv of the audio encoder from the widget name.
        /// </summary>
        /// <param name="selectedEncoder"></param>
        /// <returns></returns>
        /// 
        private string getAudioEncoder(string selectedEncoder)
        {
            switch (selectedEncoder)
            {
                case "AAC":
                    return "faac";
                case "MP3":
                    return "lame";
                case "Vorbis":
                    return "vorbis";
                case "AC3":
                    return "ac3";
                default:
                    return "";
            }
        }

        /// <summary>
        /// This function saves the data in the chapters tab, dataGridView into a CSV file called chapters.csv
        /// in a directory specified by file_path_name
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <param name="file_path_name"></param>
        /// <returns></returns>
        private Boolean chapterCSVSave(frmMain mainWindow, string file_path_name)
        {
            try
            {
                StringBuilder csv = new StringBuilder();

                foreach (DataGridViewRow row in mainWindow.data_chpt.Rows)
                {
                    csv.Append(row.Cells[0].Value.ToString());
                    csv.Append(",");
                    csv.Append(row.Cells[1].Value.ToString());
                    csv.Append(Environment.NewLine);
                }
                StreamWriter file = new StreamWriter(file_path_name);
                file.Write(csv.ToString());
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