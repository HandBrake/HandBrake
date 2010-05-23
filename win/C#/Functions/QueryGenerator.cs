/*  QueryGenerator.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Windows.Forms;

    /// <summary>
    /// Generate a CLI Query for HandBrakeCLI
    /// </summary>
    public class QueryGenerator
    {
        /// <summary>
        /// Generates a full CLI query for either encoding or previe encoeds if duration and preview are defined.
        /// </summary>
        /// <param name="mainWindow">
        /// The Main Window
        /// </param>
        /// <param name="mode">
        /// What Mode. (Point to Point Encoding)  Chapters, Seconds, Frames OR Preview Encode
        /// </param>
        /// <param name="duration">
        /// Time in seconds for preview mode
        /// </param>
        /// <param name="preview">
        /// Preview --start-at-preview (int) 
        /// </param>
        /// <returns>
        /// CLI Query 
        /// </returns>
        public static string GenerateCliQuery(frmMain mainWindow, int mode, int duration, string preview)
        {
            string query = string.Empty;

            if (!string.IsNullOrEmpty(mainWindow.sourcePath) &&
                mainWindow.sourcePath.Trim() != "Select \"Source\" to continue")
                query = " -i " + '"' + mainWindow.sourcePath + '"';

            if (mainWindow.drp_dvdtitle.Text != string.Empty)
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            if (!Properties.Settings.Default.noDvdNav && mainWindow.drop_angle.Items.Count != 0)
                query += " --angle " + mainWindow.drop_angle.SelectedItem;

            // Decide what part of the video we want to encode.
            switch (mode)
            {
                case 0: // Chapters
                    if (mainWindow.drop_chapterFinish.Text == mainWindow.drop_chapterStart.Text &&
                        mainWindow.drop_chapterStart.Text != string.Empty)
                        query += string.Format(" -c {0}", mainWindow.drop_chapterStart.Text);
                    else if (mainWindow.drop_chapterStart.Text != string.Empty &&
                             mainWindow.drop_chapterFinish.Text != string.Empty)
                        query += string.Format(" -c {0}-{1}", mainWindow.drop_chapterStart.Text,
                                               mainWindow.drop_chapterFinish.Text);
                    break;
                case 1: // Seconds
                    int start, end;
                    int.TryParse(mainWindow.drop_chapterStart.Text, out start);
                    int.TryParse(mainWindow.drop_chapterFinish.Text, out end);
                    int calculatedDuration = end - start;

                    query += string.Format(" --start-at duration:{0} --stop-at duration:{1}", mainWindow.drop_chapterStart.Text, calculatedDuration);
                    break;
                case 2: // Frames
                    int.TryParse(mainWindow.drop_chapterStart.Text, out start);
                    int.TryParse(mainWindow.drop_chapterFinish.Text, out end);
                    calculatedDuration = end - start;

                    query += string.Format(" --start-at frame:{0} --stop-at frame:{1}", mainWindow.drop_chapterStart.Text, calculatedDuration);
                    break;
                case 3: // Preview
                    query += " --previews " + Properties.Settings.Default.previewScanCount + " ";
                    query += " --start-at-preview " + preview;
                    query += " --stop-at duration:" + duration + " ";

                    if (mainWindow.text_destination.Text != string.Empty)
                        query += string.Format(" -o \"{0}\" ", mainWindow.text_destination.Text.Replace(".m", "_sample.m"));
                    break;
                default:
                    break;
            }
            if (mode != 3)
                query += string.Format(" -o \"{0}\" ", mainWindow.text_destination.Text);

            query += GenerateTabbedComponentsQuery(mainWindow);

            return query;
        }

        /// <summary>
        /// Generates part of the CLI query, for the tabbed components only.
        /// </summary>
        /// <param name="mainWindow">frmMain the main window</param>
        /// <returns>The CLI Query for the Tab Screens on the main window</returns>
        public static string GenerateTabbedComponentsQuery(frmMain mainWindow)
        {
            string query = string.Empty;

            #region Output Settings Box

            query += " -f " + mainWindow.drop_format.Text.ToLower().Replace(" file", string.Empty);

            // These are output settings features
            if (mainWindow.check_largeFile.Checked)
                query += " -4 ";

            if (mainWindow.check_iPodAtom.Checked)
                query += " -I ";

            if (mainWindow.check_optimiseMP4.Checked)
                query += " -O ";

            #endregion

            #region Picture Settings Tab

            if (mainWindow.PictureSettings.text_width.Value != 0)
                if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex != 1) // Prevent usage for strict anamorphic
                    query += " -w " + mainWindow.PictureSettings.text_width.Text;

            if (mainWindow.PictureSettings.text_height.Value != 0 && mainWindow.PictureSettings.text_height.Text != string.Empty)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 0 || mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 3) // Prevent usage for strict anamorphic
                        query += " -l " + mainWindow.PictureSettings.text_height.Text;

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
                case 0:
                    if (mainWindow.PictureSettings.drp_modulus.SelectedIndex != 0)
                        query += " --modulus " + mainWindow.PictureSettings.drp_modulus.SelectedItem;
                    break;
                case 1:
                    query += " --strict-anamorphic ";
                    break;
                case 2:
                    query += " --loose-anamorphic ";
                    if (mainWindow.PictureSettings.drp_modulus.SelectedIndex != 0)
                        query += " --modulus " + mainWindow.PictureSettings.drp_modulus.SelectedItem;
                    break;
                case 3:
                    query += " --custom-anamorphic ";

                    if (mainWindow.PictureSettings.drp_modulus.SelectedIndex != 0)
                        query += " --modulus " + mainWindow.PictureSettings.drp_modulus.SelectedItem;

                    if (mainWindow.PictureSettings.check_KeepAR.Checked)
                        query += " --display-width " + mainWindow.PictureSettings.updownDisplayWidth.Text + " ";

                    if (mainWindow.PictureSettings.check_KeepAR.Checked)
                        query += " --keep-display-aspect ";

                    if (!mainWindow.PictureSettings.check_KeepAR.Checked)
                        if (mainWindow.PictureSettings.updownParWidth.Text != string.Empty &&
                            mainWindow.PictureSettings.updownParHeight.Text != string.Empty)
                            query += " --pixel-aspect " + mainWindow.PictureSettings.updownParWidth.Text + ":" +
                                     mainWindow.PictureSettings.updownParHeight.Text + " ";
                    break;
            }

            #endregion

            // Filters Panel
            query += mainWindow.Filters.GetCliQuery;

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
                        value = 51 - (mainWindow.slider_videoQuality.Value * cqStep);
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

            DataGridView audioTracks = mainWindow.AudioSettings.GetAudioPanel();
            List<string> tracks = new List<string>();
            List<string> codecs = new List<string>();
            List<string> mixdowns = new List<string>();
            List<string> samplerates = new List<string>();
            List<string> bitrates = new List<string>();
            List<string> drcs = new List<string>();

            // No Audio
            if (audioTracks.Rows.Count == 0)
                query += " -a none ";

            // Gather information about each audio track and store them in the declared lists.
            foreach (DataGridViewRow row in audioTracks.Rows)
            {
                // Audio Track (-a)
                if (row.Cells[1].Value.ToString() == "Automatic")
                    tracks.Add("1");
                else if (row.Cells[1].Value.ToString() != "None")
                {
                    string[] tempSub = row.Cells[1].Value.ToString().Split(' ');
                    tracks.Add(tempSub[0]);
                }

                // Audio Codec (-E)
                if (row.Cells[2].Value.ToString() != String.Empty)
                    codecs.Add(GetAudioEncoder(row.Cells[2].Value.ToString()));

                // Audio Mixdown (-6)
                if (row.Cells[3].Value.ToString() != String.Empty)
                    mixdowns.Add(GetMixDown(row.Cells[3].Value.ToString()));

                // Sample Rate (-R)
                if (row.Cells[4].Value.ToString() != String.Empty)
                    samplerates.Add(row.Cells[4].Value.ToString());

                // Audio Bitrate (-B)
                if (row.Cells[5].Value.ToString() != String.Empty)
                    bitrates.Add(row.Cells[5].Value.ToString().Replace("Auto", "auto"));

                // DRC (-D)
                if (row.Cells[6].Value.ToString() != String.Empty)
                    drcs.Add(row.Cells[6].Value.ToString());
            }

            // Audio Track (-a)
            string audioItems = string.Empty;
            bool firstLoop = true;

            foreach (string item in tracks)
            {
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -a " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Codec (-E)
            foreach (string item in codecs)
            {
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -E " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Mixdown (-6)
            foreach (string item in mixdowns)
            {
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -6 " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Sample Rate (-R)
            foreach (string item in samplerates)
            {
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -R " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Bitrate (-B)
            foreach (string item in bitrates)
            {
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -B " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // DRC (-D)
            foreach (var itm in drcs)
            {
                string item = itm.ToString(new CultureInfo("en-US"));
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -D " + audioItems;

            #endregion

            // Subtitles Panel
            query += mainWindow.Subtitles.GetCliQuery;

            #region Chapter Markers

            // Attach Source name and dvd title to the start of the chapters.csv filename.
            // This is for the queue. It allows different chapter name files for each title.
            string[] destNameSplit = mainWindow.text_destination.Text.Split('\\');
            string destName = destNameSplit[destNameSplit.Length - 1];
            destName = destName.Replace("\"", string.Empty);
            destName = destName.Replace(".mp4", string.Empty).Replace(".m4v", string.Empty).Replace(".mkv", string.Empty);

            string sourceTitle = mainWindow.drp_dvdtitle.Text;
            string[] titlesplit = sourceTitle.Split(' ');
            sourceTitle = titlesplit[0];

            if (mainWindow.Check_ChapterMarkers.Checked && mainWindow.Check_ChapterMarkers.Enabled)
            {
                if (destName.Trim() != String.Empty)
                {
                    string path = sourceTitle != "Automatic"
                                      ? Path.Combine(Path.GetTempPath(), destName + "-" + sourceTitle + "-chapters.csv")
                                      : Path.Combine(Path.GetTempPath(), destName + "-chapters.csv");

                    if (ChapterCsvSave(mainWindow, path) == false)
                        query += " -m ";
                    else
                        query += " --markers=" + "\"" + path + "\"";
                }
                else
                    query += " -m";
            }

            #endregion

            // X264 Panel
            query += " -x " + mainWindow.x264Panel.X264Query;

            // Verbosity Level
            query += " -v " + Properties.Settings.Default.verboseLevel;

            // LibDVDNav
            if (Properties.Settings.Default.noDvdNav)
                query += " --no-dvdnav";

            return query;
        }

        /// <summary>
        /// Return the CLI Mixdown name
        /// </summary>
        /// <param name="selectedAudio">GUI mixdown name</param>
        /// <returns>CLI mixdown name</returns>
        private static string GetMixDown(string selectedAudio)
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
        /// Get the CLI Audio Encoder name
        /// </summary>
        /// <param name="selectedEncoder">
        /// String The GUI Encode name
        /// </param>
        /// <returns>
        /// String CLI encoder name
        /// </returns>
        private static string GetAudioEncoder(string selectedEncoder)
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
                    return string.Empty;
            }
        }

        /// <summary>
        /// Create a CSV file with the data from the Main Window Chapters tab
        /// </summary>
        /// <param name="mainWindow">Main Window</param>
        /// <param name="filePathName">Path to save the csv file</param>
        /// <returns>True if successful </returns>
        private static bool ChapterCsvSave(frmMain mainWindow, string filePathName)
        {
            try
            {
                string csv = string.Empty;

                foreach (DataGridViewRow row in mainWindow.data_chpt.Rows)
                {
                    csv += row.Cells[0].Value.ToString();
                    csv += ",";
                    csv += row.Cells[1].Value.ToString().Replace(",", "\\,");
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