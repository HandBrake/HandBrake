/*  PresetLoader.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake.Functions
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
        public static void presetLoader(frmMain mainWindow, QueryParser presetQuery, string name, Boolean pictureSettings)
        {
            // ---------------------------
            // Setup the GUI
            // ---------------------------

            #region Source
            // Reset some vaules to stock first to prevent errors.
            mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            // Now load all the new settings onto the main window
            if (presetQuery.Format != null)
            {
                string destination = mainWindow.text_destination.Text;
                destination = destination.Replace(".mp4", "." + presetQuery.Format);
                destination = destination.Replace(".m4v", "." + presetQuery.Format);
                destination = destination.Replace(".mkv", "." + presetQuery.Format);
                mainWindow.text_destination.Text = destination;
            }

            #endregion

            #region Destination and Output Settings

            if (presetQuery.Format != null)
            {
                if (presetQuery.Format == "mp4")
                    mainWindow.drop_format.SelectedIndex = 0;
                else if (presetQuery.Format == "m4v")
                    mainWindow.drop_format.SelectedIndex = 1;
                else if (presetQuery.Format == "mkv")
                    mainWindow.drop_format.SelectedIndex = 2;

                if (presetQuery.ChapterMarkers && presetQuery.Format == "mp4")
                    mainWindow.drop_format.SelectedIndex = 1;
            }

            mainWindow.check_iPodAtom.CheckState = presetQuery.IpodAtom ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.check_optimiseMP4.CheckState = presetQuery.OptimizeMP4 ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.check_largeFile.CheckState = presetQuery.LargeMP4 ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.setContainerOpts(); // select the container options according to the selected format

            #endregion

            #region Picture
            mainWindow.PictureSettings.check_autoCrop.Checked = true;
            if (presetQuery.CropBottom == "0" && presetQuery.CropTop == "0")
                if (presetQuery.CropLeft == "0" && presetQuery.CropRight == "0")
                    mainWindow.PictureSettings.check_customCrop.Checked = true;

            if (pictureSettings) // only Load picture settings if the perset requires it
            {
                if (presetQuery.CropTop != null)
                {
                    int top, bottom, left, right;
                    int.TryParse(presetQuery.CropTop, out top);
                    int.TryParse(presetQuery.CropBottom, out bottom);
                    int.TryParse(presetQuery.CropLeft, out left);
                    int.TryParse(presetQuery.CropRight, out right);

                    mainWindow.PictureSettings.check_customCrop.Checked = true;
                    mainWindow.PictureSettings.crop_top.Value = top;
                    mainWindow.PictureSettings.crop_bottom.Value = bottom;
                    mainWindow.PictureSettings.crop_left.Value = left;
                    mainWindow.PictureSettings.crop_right.Value = right;
                }
            }

            // Reset maxWidth and MaxHeight to 0
            mainWindow.PictureSettings.maxWidth = 0;
            mainWindow.PictureSettings.maxHeight = 0;

            // Set the width and height
            if (presetQuery.Width != 0)
                mainWindow.PictureSettings.text_width.Value = presetQuery.Width;
            else if (presetQuery.MaxWidth == 0)
                mainWindow.PictureSettings.text_width.Value = 0;

            mainWindow.PictureSettings.text_height.Value = presetQuery.Height != 0 ? presetQuery.Height : 0;

            // Max Width/Height override Width/Height
            if (presetQuery.MaxWidth != 0)
            {
                mainWindow.PictureSettings.text_width.Value = presetQuery.MaxWidth;
                mainWindow.PictureSettings.maxWidth = presetQuery.MaxWidth;
            }

            if (presetQuery.MaxHeight != 0)
            {
                mainWindow.PictureSettings.text_height.Value = presetQuery.MaxHeight;
                mainWindow.PictureSettings.maxHeight = presetQuery.MaxHeight;
            }
            mainWindow.PictureSettings.setMax();

            // Set the anamorphic mode 0,1,2,3
            mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = presetQuery.AnamorphicMode;

            // Custom Anamorphic Controls
            mainWindow.PictureSettings.check_KeepAR.CheckState = presetQuery.keepDisplayAsect ? CheckState.Checked : CheckState.Unchecked;
            mainWindow.PictureSettings.txt_displayWidth.Text = presetQuery.displayWidthValue.ToString();
            mainWindow.PictureSettings.txt_parWidth.Text = presetQuery.pixelAspectWidth.ToString();
            mainWindow.PictureSettings.txt_parHeight.Text = presetQuery.pixelAspectHeight.ToString();
            mainWindow.PictureSettings.drop_modulus.SelectedItem = presetQuery.AnamorphicModulus;


            #endregion

            #region Filters
            mainWindow.Filters.setDecomb(presetQuery.Decomb);
            mainWindow.Filters.setDeInterlace(presetQuery.DeInterlace);
            mainWindow.Filters.setDeNoise(presetQuery.DeNoise);
            mainWindow.Filters.setDeTelecine(presetQuery.DeTelecine);
            mainWindow.Filters.setDeBlock(presetQuery.DeBlock);
            mainWindow.Filters.setGrayScale(presetQuery.Grayscale);
            #endregion

            #region Video
            mainWindow.drp_videoEncoder.Text = presetQuery.VideoEncoder;

            if (presetQuery.AverageVideoBitrate != null)
            {
                mainWindow.radio_avgBitrate.Checked = true;
                mainWindow.text_bitrate.Text = presetQuery.AverageVideoBitrate;
            }
            if (presetQuery.VideoTargetSize != null)
            {
                mainWindow.radio_targetFilesize.Checked = true;
                mainWindow.text_filesize.Text = presetQuery.VideoTargetSize;
            }

            // Quality
            if (presetQuery.VideoQuality != 0)
            {
                mainWindow.radio_cq.Checked = true;
                if (presetQuery.VideoEncoder == "H.264 (x264)")
                {
                    int value;
                    double x264step = Properties.Settings.Default.x264cqstep;
                    double presetValue = presetQuery.VideoQuality;

                    double x = 51 / x264step;

                    double calculated = presetValue / x264step;
                    calculated = x - calculated;

                    int.TryParse(calculated.ToString(), out value);

                    // This will sometimes occur when the preset was generated 
                    // with a different granularity, so, round and try again.
                    if (value == 0)
                    {
                        double val = Math.Round(calculated, 0);
                        int.TryParse(val.ToString(), out value);
                    }
                    mainWindow.slider_videoQuality.Value = value;
                }
                else
                {
                    int presetVal;
                    int.TryParse(presetQuery.VideoQuality.ToString(), out presetVal);
                    mainWindow.slider_videoQuality.Value = presetVal;
                }
            }

            mainWindow.check_2PassEncode.CheckState = presetQuery.TwoPass ? CheckState.Checked : CheckState.Unchecked;


            mainWindow.drp_videoFramerate.Text = presetQuery.VideoFramerate;

            mainWindow.check_turbo.CheckState = presetQuery.TurboFirstPass ? CheckState.Checked : CheckState.Unchecked;

            #endregion

            #region Chapter Markers

            if (presetQuery.ChapterMarkers)
            {
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Checked;
                mainWindow.Check_ChapterMarkers.Enabled = true;
            }
            else
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Unchecked;

            #endregion

            #region Audio
            // Clear the audio listing
            mainWindow.AudioSettings.clearAudioList();

            if (presetQuery.AudioInformation != null)
                foreach (AudioTrack track in presetQuery.AudioInformation)
                {
                    ListViewItem newTrack = new ListViewItem(mainWindow.AudioSettings.getNewID().ToString());

                    newTrack.SubItems.Add("Automatic");
                    newTrack.SubItems.Add(track.Encoder);
                    newTrack.SubItems.Add(track.MixDown);
                    newTrack.SubItems.Add(track.SampleRate);
                    if (track.Encoder.Contains("AC3"))
                        newTrack.SubItems.Add("Auto");
                    else
                        newTrack.SubItems.Add(track.Bitrate);
                    newTrack.SubItems.Add(track.DRC);
                    mainWindow.AudioSettings.addTrackForPreset(newTrack);
                }
            #endregion

            #region Other
            mainWindow.x264Panel.x264Query = presetQuery.H264Query;

            // Set the preset name
            mainWindow.groupBox_output.Text = "Output Settings (Preset: " + name + ")";
            #endregion
        }
    }
}