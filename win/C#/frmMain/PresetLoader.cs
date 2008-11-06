using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace Handbrake
{
    class PresetLoader
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

            // Set the public max width and max height varibles in frmMain
            // These are used by the query generator to determine if it should use -X or -w  / -Y or -h
            if (presetQuery.MaxWidth != 0)
            {
                mainWindow.text_width.Text = presetQuery.MaxWidth.ToString();
                mainWindow.maxWidth = presetQuery.MaxWidth;
            }

            if (presetQuery.MaxHeight != 0)
            {
                mainWindow.text_height.Text = presetQuery.MaxHeight.ToString();
                mainWindow.maxHeight = presetQuery.MaxHeight;
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
    }
}
