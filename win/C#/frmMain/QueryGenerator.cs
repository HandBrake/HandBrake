using System;
using System.Collections;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.IO;

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
        /// <returns>Returns a CLI query String.</returns>
        public string GeneratePreviewQuery(frmMain mainWindow)
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

            query += " -c 2";

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
        /// <param name="source"></param>
        /// <returns></returns>
        public string generateTabbedComponentsQuery(frmMain mainWindow)
        {
            string query = "";

            // The Output Settings box above the tabbed section.
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

            // Picture Settings Tab
            #region Picture Settings Tab

            if (mainWindow.text_width.Text != "")
                query += " -w " + mainWindow.text_width.Text;

            if (mainWindow.text_height.Text != "")
                query += " -l " + mainWindow.text_height.Text;

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

            switch (mainWindow.drp_deInterlace_option.Text)
            {
                case "None":
                    query += "";
                    break;
                case "Fast":
                    query += " --deinterlace=\"fast\"";
                    break;
                case "Slow":
                    query += " --deinterlace=\"slow\"";
                    break;
                case "Slower":
                    query += " --deinterlace=\"slower\"";
                    break;
                case "Slowest":
                    query += " --deinterlace=\"slowest\"";
                    break;
                default:
                    query += "";
                    break;
            }

            if (mainWindow.check_decomb.Checked)
            {
                string decombValue = Properties.Settings.Default.decomb;
                if (decombValue != "" && decombValue != Properties.Settings.Default.default_decomb)
                    query += " --decomb=\"" + decombValue + "\"";
                else
                    query += " --decomb ";
            }

            if (mainWindow.drp_anamorphic.SelectedIndex == 1)
                query += " -p ";
            else if (mainWindow.drp_anamorphic.SelectedIndex == 2)
                query += " -P ";

            if (mainWindow.slider_deblock.Value != 4)
                query += " --deblock=" + mainWindow.slider_deblock.Value;

            if (mainWindow.check_detelecine.Checked)
                query += " --detelecine";
            #endregion

            // Video Settings Tab
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
            if (mainWindow.text_bitrate.Text != "")
                query += " -b " + mainWindow.text_bitrate.Text;

            if (mainWindow.text_filesize.Text != "")
                query += " -S " + mainWindow.text_filesize.Text;

            // Video Quality Setting
            double videoQuality = mainWindow.slider_videoQuality.Value;
            if (videoQuality != 0)
            {
                videoQuality = videoQuality / 100;
                query += " -q " + videoQuality.ToString(new CultureInfo("en-US"));
            }

            if (mainWindow.check_2PassEncode.Checked)
                query += " -2 ";

            if (mainWindow.check_turbo.Checked)
                query += " -T ";

            if (mainWindow.drp_videoFramerate.Text != "Same as source")
                query += " -r " + mainWindow.drp_videoFramerate.Text;

            switch (mainWindow.drp_deNoise.Text)
            {
                case "None":
                    query += "";
                    break;
                case "Weak":
                    query += " --denoise=\"weak\"";
                    break;
                case "Medium":
                    query += " --denoise=\"medium\"";
                    break;
                case "Strong":
                    query += " --denoise=\"strong\"";
                    break;
                default:
                    query += "";
                    break;
            }
            #endregion

            // Audio Settings Tab
            #region Audio Settings Tab
            // Track 1
            string track1 = mainWindow.drp_track1Audio.Text;
            string aencoder1 = mainWindow.drp_audenc_1.Text;
            string audioBitrate1 = mainWindow.drp_audbit_1.Text;
            string audioSampleRate1 = mainWindow.drp_audsr_1.Text;
            string Mixdown1 = mainWindow.drp_audmix_1.Text;
            string drc1 = mainWindow.trackBar1.Value.ToString();

            // Track 2
            string track2 = mainWindow.drp_track2Audio.Text;
            string aencoder2 = mainWindow.drp_audenc_2.Text;
            string audioBitrate2 = mainWindow.drp_audbit_2.Text;
            string audioSampleRate2 = mainWindow.drp_audsr_2.Text;
            string Mixdown2 = mainWindow.drp_audmix_2.Text;
            string drc2 = mainWindow.trackBar2.Value.ToString();

            // Track 3
            string track3 = mainWindow.drp_track3Audio.Text;
            string aencoder3 = mainWindow.drp_audenc_3.Text;
            string audioBitrate3 = mainWindow.drp_audbit_3.Text;
            string audioSampleRate3 = mainWindow.drp_audsr_3.Text;
            string Mixdown3 = mainWindow.drp_audmix_3.Text;
            string drc3 = mainWindow.trackBar3.Value.ToString();

            // Track 4
            string track4 = mainWindow.drp_track4Audio.Text;
            string aencoder4 = mainWindow.drp_audenc_4.Text;
            string audioBitrate4 = mainWindow.drp_audbit_4.Text;
            string audioSampleRate4 = mainWindow.drp_audsr_4.Text;
            string Mixdown4 = mainWindow.drp_audmix_4.Text;
            string drc4 = mainWindow.trackBar4.Value.ToString();

            //
            // Audio Track Selections
            //
            if (track1 == "Automatic")
                query += " -a 1";
            else if (track1 != "None")
            {
                string[] tempSub = track1.Split(' ');
                query += " -a " + tempSub[0];
            }

            if (track2 == "Automatic")
                query += ",1";
            else if (track2 != "None")
            {
                string[] tempSub;
                tempSub = track2.Split(' ');

                if (track1 == "None")
                    query += " -a none," + tempSub[0];
                else
                    query += "," + tempSub[0];
            }

            if (track3 != "None")
            {
                string[] tempSub;
                tempSub = track3.Split(' ');
                query += "," + tempSub[0];
            }

            if (track4 != "None")
            {
                string[] tempSub;
                tempSub = track4.Split(' ');
                query += "," + tempSub[0];
            }

            //
            // Audio Encoder
            //
            if (aencoder1 != "")
                query += " -E " + getAudioEncoder(aencoder1);

            if (aencoder2 != "")
            {
                if (aencoder1 == string.Empty)
                    query += " -E faac," + getAudioEncoder(aencoder2);
                else
                    query += "," + getAudioEncoder(aencoder2);
            }

            if (aencoder3 != "")
                query += "," + getAudioEncoder(aencoder3);

            if (aencoder4 != "")
                query += "," + getAudioEncoder(aencoder4);

            //
            // Audio Bitrate Selections
            //
            if (audioBitrate1 != "")
                query += " -B " + audioBitrate1;

            if (audioBitrate2 != "")
            {
                if (audioBitrate1 == string.Empty)
                    query += " -B 160," + audioBitrate2;
                else
                    query += "," + audioBitrate2;
            }

            if (audioBitrate3 != "")
                query += "," + audioBitrate3;

            if (audioBitrate4 != "")
                query += "," + audioBitrate4;


            //Audio Sample Rate   - audioSampleRate
            if (audioSampleRate1 != "")
                query += " -R " + audioSampleRate1.Replace("Auto", "0");

            if (audioSampleRate2 != "")
            {
                if (audioSampleRate1 == string.Empty)
                    query += " -R 0," + audioSampleRate2.Replace("Auto", "0");
                else
                    query += "," + audioSampleRate2.Replace("Auto", "0");
            }
            else
            {
                // All this is a hack, because when AppleTV is selected, there is no sample rate selected. so just add a 48
                // It should probably be setup later so the GUI widget has the value 48 in it.

                if ((track2 != "") && (track2 != "None"))
                {
                    if (audioSampleRate1 == string.Empty)
                        query += " -R 0,0";
                    else
                        query += ",0";
                }
            }

            if (audioSampleRate3 != "")
                query += "," + audioSampleRate3.Replace("Auto", "0");

            if (audioSampleRate4 != "")
                query += "," + audioSampleRate4.Replace("Auto", "0");

            //
            // Audio Mixdown Selections
            //

            if (Mixdown1 != "")
                query += " -6 " + getMixDown(Mixdown1);
            else
                query += " -6 dpl2";

            if (Mixdown2 != "" && track2 != "None")
                query += "," + getMixDown(Mixdown2);

            if (Mixdown3 != "" && track3 != "None" && track2 != "None")
                query += "," + getMixDown(Mixdown3);

            if (Mixdown4 != "" && track4 != "None" && track3 != "None")
                query += "," + getMixDown(Mixdown4);


            //
            // DRC
            //
            double value = 0;

            value = mainWindow.trackBar1.Value / 10.0;
            value++;

            if (value > 1.0)
                query += " -D " + value;
            else
                query += " -D 1";

            value = mainWindow.trackBar2.Value / 10.0;
            value++;
            if (track2 != "None" && drc2 != "0")
                query += "," + value;
            else if (track2 != "None" && drc2 == "0")
                query += ",1";

            value = mainWindow.trackBar3.Value / 10.0;
            value++;
            if (track3 != "None" && drc3 != "0")
                query += "," + value;
            else if (track3 != "None" && drc3 == "0")
                query += ",1";

            value = mainWindow.trackBar4.Value / 10.0;
            value++;
            if (track4 != "None" && drc4 != "0")
                query += "," + value;
            else if (track4 != "None" && drc4 == "0")
                query += ",1";

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

            // Chapter Markers Tab
            #region Chapter Markers

            // Attach Source name and dvd title to the start of the chapters.csv filename.
            // This is for the queue. It allows different chapter name files for each title.
            string source_name = mainWindow.text_source.Text;
            string[] sourceName = source_name.Split('\\');
            source_name = sourceName[sourceName.Length - 1];
            source_name = source_name.Replace("\"", "");

            string source_title = mainWindow.drp_dvdtitle.Text;
            string[] titlesplit = source_title.Split(' ');
            source_title = titlesplit[0];

            if (mainWindow.Check_ChapterMarkers.Checked)
            {
                if ((source_name.Trim() != "Click 'Source' to continue") && (source_name.Trim() != ""))
                {
                    string path = "";
                    if (source_title != "Automatic")
                        path = Path.Combine(Path.GetTempPath(), source_name + "-" + source_title + "-chapters.csv");
                    else
                        path = Path.Combine(Path.GetTempPath(), source_name + "-chapters.csv");

                    if (chapterCSVSave(mainWindow, path) == false)
                        query += " -m ";
                    else
                        query += " --markers=" + "\"" + path + "\"";
                }
                else
                    query += " -m";
            }
            #endregion

            // H264 Tab
            #region  H264 Tab
            if (mainWindow.rtf_x264Query.Text != "")
                query += " -x " + mainWindow.rtf_x264Query.Text;
            #endregion

            // Other
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
                    return "dpl2";
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
                    return "dpl2";
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
                MessageBox.Show("Unable to save Chapter Makrers file! \nChapter marker names will NOT be saved in your encode \n\n" + exc.ToString(), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return false;
            }
        }
    }
}
