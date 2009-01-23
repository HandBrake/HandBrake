using System;
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
        /// <param name="pictureSettings">Save picture settings in the preset</param>
        public void presetLoader(frmMain mainWindow, Functions.QueryParser presetQuery, string name, Boolean pictureSettings)
        {
            // ---------------------------
            // Setup the GUI
            // ---------------------------

            // Source tab
            #region source
            // Reset some vaules to stock first to prevent errors.
            mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            // Now load all the new settings onto the main window
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

                if (presetQuery.ChapterMarkers && presetQuery.Format == "mp4")
                    mainWindow.drop_format.SelectedIndex = 1;
            }

            if (presetQuery.IpodAtom)
                mainWindow.check_iPodAtom.CheckState = CheckState.Checked;
            else
                mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            if (presetQuery.OptimizeMP4)
                mainWindow.check_optimiseMP4.CheckState = CheckState.Checked;
            else
                mainWindow.check_optimiseMP4.CheckState = CheckState.Unchecked;

            #endregion

            // Picture Settings Tab
            #region Picture
            mainWindow.check_autoCrop.Checked = true;
            if (presetQuery.CropBottom == "0" && presetQuery.CropTop == "0")
                if (presetQuery.CropLeft == "0" && presetQuery.CropRight == "0")
                    mainWindow.check_customCrop.Checked = true;

            mainWindow.text_width.Text = "";
            mainWindow.text_height.Text = "";

            if (pictureSettings)
            {
                if (presetQuery.CropTop != null)
                {
                    int top, bottom, left, right;
                    int.TryParse(presetQuery.CropTop, out top);
                    int.TryParse(presetQuery.CropBottom, out bottom);
                    int.TryParse(presetQuery.CropLeft, out left);
                    int.TryParse(presetQuery.CropRight, out right);

                    mainWindow.check_customCrop.Checked = true;
                    mainWindow.text_top.Value = top;
                    mainWindow.text_bottom.Value = bottom;
                    mainWindow.text_left.Value = left;
                    mainWindow.text_right.Value = right;
                }

                if (presetQuery.Width != 0)
                    mainWindow.text_width.Text = presetQuery.Width.ToString();

                if (presetQuery.Height != 0)
                    mainWindow.text_height.Text = presetQuery.Height.ToString();
            }

            mainWindow.drp_deInterlace_option.Text = presetQuery.DeInterlace;
            mainWindow.drp_deNoise.Text = presetQuery.DeNoise;

            if (presetQuery.Decomb)
                mainWindow.check_decomb.CheckState = CheckState.Checked;
            else
                mainWindow.check_decomb.CheckState = CheckState.Unchecked;

            if (presetQuery.DeTelecine)
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

            if (presetQuery.Anamorphic)
                mainWindow.drp_anamorphic.SelectedIndex = 1;
            else
                mainWindow.drp_anamorphic.SelectedIndex = 0;

            if (presetQuery.LooseAnamorphic)
                mainWindow.drp_anamorphic.SelectedIndex = 2;
            else
            {
                if (presetQuery.Anamorphic != true)
                    mainWindow.drp_anamorphic.SelectedIndex = 0;
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
                mainWindow.SliderValue.Text = ql + "%";
            }

            if (presetQuery.TwoPass)
                mainWindow.check_2PassEncode.CheckState = CheckState.Checked;
            else
                mainWindow.check_2PassEncode.CheckState = CheckState.Unchecked;

            if (presetQuery.Grayscale)
                mainWindow.check_grayscale.CheckState = CheckState.Checked;
            else
                mainWindow.check_grayscale.CheckState = CheckState.Unchecked;

            mainWindow.drp_videoFramerate.Text = presetQuery.VideoFramerate;

            if (presetQuery.TurboFirstPass)
                mainWindow.check_turbo.CheckState = CheckState.Checked;
            else
                mainWindow.check_turbo.CheckState = CheckState.Unchecked;

            if (presetQuery.LargeMP4)
                mainWindow.check_largeFile.CheckState = CheckState.Checked;
            else
            {
                mainWindow.check_largeFile.CheckState = CheckState.Unchecked;
                mainWindow.check_largeFile.BackColor = Color.Transparent;
            }
            #endregion

            // Chapter Markers Tab
            #region Chapter Markers

            if (presetQuery.ChapterMarkers)
            {
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Checked;
                mainWindow.Check_ChapterMarkers.Enabled = true;
            }
            else
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Unchecked;

            #endregion

            // Audio Settings Tab
            #region Audio
            // Clear the audio listing
            mainWindow.lv_audioList.Items.Clear();

            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            ListViewItem newTrack;
            if (presetQuery.AudioTrack1 != "None")
            {
                newTrack = new ListViewItem("Automatic");
                newTrack.SubItems.Add(presetQuery.AudioEncoder1);
                newTrack.SubItems.Add(presetQuery.AudioTrackMix1);
                newTrack.SubItems.Add(presetQuery.AudioSamplerate1);
                newTrack.SubItems.Add(presetQuery.AudioBitrate1);
                newTrack.SubItems.Add(presetQuery.DRC1.ToString());
                mainWindow.lv_audioList.Items.Add(newTrack);
            }

            if (presetQuery.AudioTrack2 != "None")
            {
                newTrack = new ListViewItem("Automatic");
                newTrack.SubItems.Add(presetQuery.AudioEncoder2);
                newTrack.SubItems.Add(presetQuery.AudioTrackMix2);
                newTrack.SubItems.Add(presetQuery.AudioSamplerate2);
                newTrack.SubItems.Add(presetQuery.AudioBitrate2);
                newTrack.SubItems.Add(presetQuery.DRC2.ToString());
                mainWindow.lv_audioList.Items.Add(newTrack);
            }

            if (presetQuery.AudioTrack3 != "None")
            {
                newTrack = new ListViewItem("Automatic");
                newTrack.SubItems.Add(presetQuery.AudioEncoder3);
                newTrack.SubItems.Add(presetQuery.AudioTrackMix3);
                newTrack.SubItems.Add(presetQuery.AudioSamplerate3);
                newTrack.SubItems.Add(presetQuery.AudioBitrate3);
                newTrack.SubItems.Add(presetQuery.DRC3.ToString());
                mainWindow.lv_audioList.Items.Add(newTrack);
            }

            if (presetQuery.AudioTrack4 != "None")
            {
                newTrack = new ListViewItem("Automatic");
                newTrack.SubItems.Add(presetQuery.AudioEncoder4);
                newTrack.SubItems.Add(presetQuery.AudioTrackMix4);
                newTrack.SubItems.Add(presetQuery.AudioSamplerate4);
                newTrack.SubItems.Add(presetQuery.AudioBitrate4);
                newTrack.SubItems.Add(presetQuery.DRC4.ToString());
                mainWindow.lv_audioList.Items.Add(newTrack);
            }

            // Subtitle Stuff
            mainWindow.drp_subtitle.Text = presetQuery.Subtitles;

            if (presetQuery.ForcedSubtitles)
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