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
        #region Presets

        /// <summary>
        /// Update the presets.dat file with the latest version of HandBrak's presets from the CLI
        /// </summary>
        public void grabCLIPresets()
        {
            string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
            string presetsPath = Path.Combine(Application.StartupPath, "presets.dat");

            string strCmdLine = String.Format(@"cmd /c """"{0}"" --preset-list >""{1}"" 2>&1""", handbrakeCLIPath, presetsPath);

            ProcessStartInfo hbGetPresets = new ProcessStartInfo("CMD.exe", strCmdLine);
            hbGetPresets.WindowStyle = ProcessWindowStyle.Hidden;

            Process hbproc = Process.Start(hbGetPresets);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();
        }

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
                mainWindow.slider_deblock.Value = 0;
                mainWindow.lbl_deblockVal.Text = "0";
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

            if (presetQuery.VFR == true)
                mainWindow.check_vfr.CheckState = CheckState.Checked;
            else
                mainWindow.check_vfr.CheckState = CheckState.Unchecked;
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
            {
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Checked;
                mainWindow.drop_format.SelectedIndex = 1;
            }
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

            mainWindow.drp_audmix_1.Text = presetQuery.AudioTrackMix1;
            mainWindow.drp_audmix_2.Text = presetQuery.AudioTrackMix2;
            mainWindow.drp_audmix_3.Text = presetQuery.AudioTrackMix3;
            mainWindow.drp_audmix_4.Text = presetQuery.AudioTrackMix4;


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

        #endregion

        #region Query Generator Functions

        /// <summary>
        /// Generates a CLI query based on the GUI widgets.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <returns>The CLI String</returns>
        public string GenerateTheQuery(frmMain mainWindow)
        {
            // Source tab
            #region source
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

            #endregion

            // Destination tab
            #region Destination
            if (mainWindow.text_destination.Text != "")
                query += " -o " + '"' + mainWindow.text_destination.Text + '"';
            #endregion

            query += generateTabbedComponentsQuery(mainWindow, mainWindow.text_source.Text);
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
            #region source
            string query = "";

            if ((mainWindow.text_source.Text != "") && (mainWindow.text_source.Text.Trim() != "Click 'Source' to continue"))
                query = " -i " + '"' + mainWindow.text_source.Text + '"';

            if (mainWindow.drp_dvdtitle.Text != "Automatic")
            {
                string[] titleInfo = mainWindow.drp_dvdtitle.Text.Split(' ');
                query += " -t " + titleInfo[0];
            }

            query += " -c 2";
            #endregion

            // Destination tab
            #region Destination
            if (mainWindow.text_destination.Text != "")
                query += " -o " + '"' + mainWindow.text_destination.Text.Replace(".m", "_sample.m").Replace(".avi", "_sample.avi").Replace(".ogm", "_sample.ogm") + '"';

            #endregion

            query += generateTabbedComponentsQuery(mainWindow, mainWindow.text_source.Text);
            return query;
        }

        /// <summary>
        /// Generates part of the CLI query, for the tabbed components only.
        /// </summary>
        /// <param name="mainWindow"></param>
        /// <param name="source"></param>
        /// <returns></returns>
        private string generateTabbedComponentsQuery(frmMain mainWindow, string source)
        {
            string query = "";

            query += " -f " + mainWindow.drop_format.Text.ToLower().Replace(" file", "");

            // Picture Settings Tab
            #region Picture Settings Tab

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

            if (mainWindow.check_grayscale.Checked)
                query += " -g ";

            if (mainWindow.drp_anamorphic.SelectedIndex == 1)
                query += " -p ";
            else if (mainWindow.drp_anamorphic.SelectedIndex == 2)
                query += " -P ";

            if (mainWindow.slider_deblock.Value != 0)
                query += " --deblock=" + mainWindow.slider_deblock.Value;

            if (mainWindow.check_detelecine.Checked)
                query += " --detelecine";

            if (mainWindow.check_vfr.Checked)
                query += " -V ";
            #endregion

            // Video Settings Tab
            #region Video Settings Tab
            // These are output settings features
            if (mainWindow.check_largeFile.Checked)
                query += " -4 ";

            if (mainWindow.check_iPodAtom.Checked)
                query += " -I ";

            if (mainWindow.check_optimiseMP4.Checked)
                query += " -O ";

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

            if (mainWindow.drp_videoFramerate.Text != "Same as source")
            {
                if (!mainWindow.check_vfr.Checked)
                    query += " -r " + mainWindow.drp_videoFramerate.Text;
            }

            if (mainWindow.check_turbo.Checked)
                query += " -T ";



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
            string[] sourceName = source.Split('\\');
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

        #endregion

        #region Actions, Versioning etc

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

        #endregion

    }
}