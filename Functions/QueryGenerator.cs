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
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using Handbrake.Model;

    /// <summary>
    /// Generate a CLI Query for HandBrakeCLI
    /// </summary>
    public class QueryGenerator
    {
        private static readonly IUserSettingService UserSettingService = new UserSettingService();

        /// <summary>
        /// The Culture
        /// </summary>
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        public static string GenerateQueryForPreset(frmMain mainWindow, QueryPictureSettingsMode mode, bool filters, int width, int height)
        {
            string query = string.Empty;

            query += GenerateTabbedComponentsQuery(mainWindow, filters, mode, width, height);

            return query;
        }

        public static string GeneratePreviewQuery(frmMain mainWindow, int duration, string preview)
        {
            string query = string.Empty;

            query += SourceQuery(mainWindow, 3, duration, preview);

            query += DestinationQuery(mainWindow, QueryEncodeMode.Preview);

            query += GenerateTabbedComponentsQuery(mainWindow, true, QueryPictureSettingsMode.UserInterfaceSettings, 0, 0);

            return query;
        }

        public static string GenerateFullQuery(frmMain mainWindow)
        {
            string query = string.Empty;

            query += SourceQuery(mainWindow, mainWindow.drop_mode.SelectedIndex, 0, null);

            query += DestinationQuery(mainWindow, QueryEncodeMode.Standard);

            query += GenerateTabbedComponentsQuery(mainWindow, true, QueryPictureSettingsMode.UserInterfaceSettings, 0, 0);

            return query;
        }

        #region Individual Query Sections

        private static string GenerateTabbedComponentsQuery(frmMain mainWindow, bool filters, QueryPictureSettingsMode mode, int width, int height)
        {
            string query = string.Empty;

            // Output Settings
            query += OutputSettingsQuery(mainWindow);

            // Filters Panel
            if (filters)
                query += FiltersQuery(mainWindow);

            // Picture Settings
            query += PictureSettingsQuery(mainWindow, mode, width, height);

            // Video Settings
            query += VideoSettingsQuery(mainWindow);

            // Audio Settings
            query += AudioSettingsQuery(mainWindow);

            // Subtitles Panel
            query += mainWindow.Subtitles.GetCliQuery;

            // Chapter Markers
            query += ChapterMarkersQuery(mainWindow);

            // X264 Panel
            query += X264Query(mainWindow);

            // Extra Settings
            query += ExtraSettings();

            return query;
        }

        private static string SourceQuery(frmMain mainWindow, int mode, int duration, string preview)
        {
            string query = string.Empty;

            if (!string.IsNullOrEmpty(mainWindow.sourcePath) && mainWindow.sourcePath.Trim() != "Select \"Source\" to continue")
            {
                if (mainWindow.sourcePath.EndsWith("\\"))
                {
                    query = " -i " + mainWindow.sourcePath;
                }
                else
                {
                    query = " -i " + '"' + mainWindow.sourcePath + '"';
                }
            }

            if (mainWindow.drp_dvdtitle.Text != string.Empty)
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            if (!UserSettingService.GetUserSettingBoolean(UserSettingConstants.DisableLibDvdNav) && mainWindow.drop_angle.Items.Count != 0)
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
                    break;
                default:
                    break;
            }

            return query;
        }

        private static string DestinationQuery(frmMain mainWindow, QueryEncodeMode mode)
        {
            string query = string.Empty;

            if (string.IsNullOrEmpty(mainWindow.text_destination.Text))
            {
                return string.Empty;
            }

            if (mode != QueryEncodeMode.Preview)
                query += string.Format(" -o \"{0}\" ", mainWindow.text_destination.Text);
            else
            {
                if (mainWindow.text_destination.Text != string.Empty)
                    query += string.Format(" -o \"{0}\" ", mainWindow.text_destination.Text.Replace(".m", "_sample.m"));
            }

            return query;
        }

        private static string OutputSettingsQuery(frmMain mainWindow)
        {
            string query = string.Empty;

            query += " -f " + mainWindow.drop_format.Text.ToLower().Replace(" file", string.Empty);

            // These are output settings features
            if (mainWindow.check_largeFile.Checked)
                query += " -4 ";

            if (mainWindow.check_iPodAtom.Checked)
                query += " -I ";

            if (mainWindow.check_optimiseMP4.Checked)
                query += " -O ";

            return query;
        }

        private static string PictureSettingsQuery(frmMain mainWindow, QueryPictureSettingsMode mode, int width, int height)
        {
            string query = string.Empty;

            if (mode == QueryPictureSettingsMode.UserInterfaceSettings)
            {
                if (mainWindow.PictureSettings.text_width.Value != 0)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex != 1) // Prevent usage for strict anamorphic
                        query += " -w " + mainWindow.PictureSettings.text_width.Text;

                if (mainWindow.PictureSettings.text_height.Value != 0 &&
                    mainWindow.PictureSettings.text_height.Text != string.Empty)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 0 ||
                        mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 3) // Prevent usage for strict anamorphic
                        query += " -l " + mainWindow.PictureSettings.text_height.Text;
            }
            else if (mode == QueryPictureSettingsMode.Custom) // For Add Preset Only.
            {
                query += " -X " + width;
                query += " -Y " + height;
            }
            else if (mode == QueryPictureSettingsMode.SourceMaximum) // For Add Preset Only.
            {
                if (mainWindow.PictureSettings.text_width.Value != 0)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex != 1) // Prevent usage for strict anamorphic
                        query += " -X " + mainWindow.PictureSettings.text_width.Text;

                if (mainWindow.PictureSettings.text_height.Value != 0 &&
                    mainWindow.PictureSettings.text_height.Text != string.Empty)
                    if (mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 0 ||
                        mainWindow.PictureSettings.drp_anamorphic.SelectedIndex == 3) // Prevent usage for strict anamorphic
                        query += " -Y " + mainWindow.PictureSettings.text_height.Text;
            }

            string cropTop = mainWindow.PictureSettings.crop_top.Text;
            string cropBottom = mainWindow.PictureSettings.crop_bottom.Text;
            string cropLeft = mainWindow.PictureSettings.crop_left.Text;
            string cropRight = mainWindow.PictureSettings.crop_right.Text;

            if (mainWindow.PictureSettings.check_customCrop.Checked && mode != QueryPictureSettingsMode.None)
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

            return query;
        }

        private static string FiltersQuery(frmMain mainWindow)
        {
            return mainWindow.Filters.GetCliQuery;
        }

        private static string VideoSettingsQuery(frmMain mainWindow)
        {
            string query = string.Empty;

            switch (mainWindow.drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    query += " -e ffmpeg";
                    break;
                case "MPEG-2 (FFmpeg)":
                    query += " -e ffmpeg2";
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

            // Video Quality Setting
            if (mainWindow.radio_cq.Checked)
            {
                double cqStep = UserSettingService.GetUserSettingDouble(UserSettingConstants.X264Step);
                double value;
                switch (mainWindow.drp_videoEncoder.Text)
                {
                    case "MPEG-4 (FFmpeg)":
                    case "MPEG-2 (FFmpeg)":
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

            if (mainWindow.drp_videoFramerate.SelectedIndex == 0)
            {
                // If we use Same as Source, we can either output CFR or VFR
                query += mainWindow.radio_constantFramerate.Checked ? " --cfr " : " --vfr ";
            }
            else
            {
                // We have a hard framerate set, so we can either be Constant or peak (VFR) framerate
                query += mainWindow.radio_constantFramerate.Checked ? " --cfr " : " --pfr ";
            }

            return query;
        }

        private static string AudioSettingsQuery(frmMain mainWindow)
        {
            // Queries for each option
            string tracks = string.Empty;
            string encoders = string.Empty;
            string mixdowns = string.Empty;
            string samplerates = string.Empty;
            string bitrates = string.Empty;
            string drvValues = string.Empty;
            string gainValues = string.Empty;

            // If we have no audio tracks, set the query to none
            if (mainWindow.AudioSettings.AudioTracks.ToList().Count == 0)
            {
                return " -a none";
            }

            // Generate the sub queries
            foreach (AudioTrack audioTrack in mainWindow.AudioSettings.AudioTracks)
            {
                // Audio Track (-a)
                string track = audioTrack.Track.HasValue ? audioTrack.Track.ToString() : "1"; // Default to "1"
                tracks += string.IsNullOrEmpty(tracks) ? track : string.Format(",{0}", track);

                // Audio Encoder  (-E)
                encoders += string.IsNullOrEmpty(encoders)
                              ? GetAudioEncoder(EnumHelper<AudioEncoder>.GetDescription(audioTrack.Encoder))
                              : string.Format(",{0}", GetAudioEncoder(EnumHelper<AudioEncoder>.GetDescription(audioTrack.Encoder)));

                // Audio Mixdowns (-6)
                mixdowns += string.IsNullOrEmpty(mixdowns)
                              ? GetMixDown(EnumHelper<Mixdown>.GetDescription(audioTrack.MixDown))
                              : string.Format(",{0}", GetMixDown(EnumHelper<Mixdown>.GetDescription(audioTrack.MixDown)));

                // Audio Samplerates (-R)
                string rate = audioTrack.SampleRate == 0 ? "Auto" : audioTrack.SampleRate.ToString(); // Default to "Auto"
                samplerates += string.IsNullOrEmpty(samplerates) ? rate : string.Format(",{0}", rate);

                // Audio Bitrates (-B)
                bitrates += string.IsNullOrEmpty(bitrates)
                                ? audioTrack.Bitrate.ToString(Culture)
                                : string.Format(",{0}", audioTrack.Bitrate);

                // Audio DRC Values
                drvValues += string.IsNullOrEmpty(drvValues) ? audioTrack.DRC.ToString(Culture) : string.Format(",{0}", audioTrack.DRC.ToString(Culture));

                // Audio Gain Control
                gainValues += string.IsNullOrEmpty(gainValues) ? audioTrack.Gain.ToString(Culture) : string.Format(",{0}", audioTrack.Gain.ToString(Culture));
            }

            return string.Format(" -a {0} -E {1} -B {2} -6 {3} -R {4} -D {5} --gain={6}", tracks, encoders, bitrates, mixdowns, samplerates, drvValues, gainValues);
        }

        private static string ChapterMarkersQuery(frmMain mainWindow)
        {
            string query = string.Empty;

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

            return query;
        }

        private static string X264Query(frmMain mainWindow)
        {
            string advancedOptions = string.Empty;
            if (mainWindow.drp_videoEncoder.SelectedItem.ToString().Contains("FFmpeg"))
            {
                advancedOptions = string.IsNullOrEmpty(mainWindow.advancedEncoderOpts.AdavancedQuery.Trim())
                       ? string.Empty
                       : mainWindow.advancedEncoderOpts.AdavancedQuery;
            }
            else if (mainWindow.drp_videoEncoder.SelectedItem.ToString().Contains("x264"))
            {
                advancedOptions = string.IsNullOrEmpty(mainWindow.x264Panel.X264Query.Trim())
                       ? string.Empty
                       : mainWindow.x264Panel.X264Query;
            }

            return !string.IsNullOrEmpty(advancedOptions) ? " -x " + advancedOptions : string.Empty;
        }

        private static string ExtraSettings()
        {
            string query = string.Empty;

            // Verbosity Level
            query += " --verbose=" + UserSettingService.GetUserSettingInt(UserSettingConstants.Verbosity);

            // LibDVDNav
            if (UserSettingService.GetUserSettingBoolean(UserSettingConstants.DisableLibDvdNav))
                query += " --no-dvdnav";

            return query;
        }

        #endregion

        #region Helpers

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
                    return "copy:ac3";
                case "DTS Passthru":
                    return "copy:dts";
                case "AC3 (ffmpeg)":
                    return "ac3";
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
        #endregion
    }
}