/*  PresetLoader.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System.Drawing;
    using System.Windows.Forms;

    /// <summary>
    /// Load a preset into the main Window
    /// </summary>
    public class PresetLoader
    {
        /// <summary>
        /// This function takes in a Query which has been parsed by QueryParser and
        /// set's all the GUI widgets correctly.
        /// </summary>
        /// <param name="mainWindow">
        /// FrmMain window
        /// </param>
        /// <param name="presetQuery">
        /// The Parsed CLI Query
        /// </param>
        /// <param name="name">
        /// Name of the preset
        /// </param>
        /// <param name="pictureSettings">
        /// Save picture settings in the preset
        /// </param>
        public static void LoadPreset(frmMain mainWindow, QueryParser presetQuery, string name, bool pictureSettings)
        {
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
                if (presetQuery.Format == "mp4" || presetQuery.Format == "m4v")
                {
                    if (mainWindow.drop_format.SelectedIndex == 0)
                        mainWindow.SetExtension(".mp4");
                    else
                        mainWindow.drop_format.SelectedIndex = 0;
                }
                else if (presetQuery.Format == "mkv")
                {
                    if (mainWindow.drop_format.SelectedIndex == 1)
                        mainWindow.SetExtension(".mkv");
                    else
                        mainWindow.drop_format.SelectedIndex = 1;
                }
            }

            mainWindow.check_iPodAtom.CheckState = presetQuery.IpodAtom ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.check_optimiseMP4.CheckState = presetQuery.OptimizeMP4
                                                          ? CheckState.Checked
                                                          : CheckState.Unchecked;

            mainWindow.check_largeFile.CheckState = presetQuery.LargeMP4 ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.setContainerOpts(); // select the container options according to the selected format

            #endregion

            #region Picture

            mainWindow.PictureSettings.check_autoCrop.Checked = true;
            if (pictureSettings) // only Load picture settings if the perset requires it
            {
                if (presetQuery.CropValues != null)
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

            // Set the anamorphic mode 0,1,2,3
            mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = presetQuery.AnamorphicMode;

            // Keep Aspect Ration Anamorphic Setting.
            mainWindow.PictureSettings.check_KeepAR.CheckState = presetQuery.KeepDisplayAsect
                                                                     ? CheckState.Checked
                                                                     : CheckState.Unchecked;

            // Set the Width and height as Required.
            if (presetQuery.Width != 0)
                mainWindow.PictureSettings.text_width.Value = presetQuery.Width;

            if (presetQuery.Height != 0)
                mainWindow.PictureSettings.text_height.Value = presetQuery.Height;

            // Max Width/Height override Width/Height
            if (presetQuery.MaxWidth != 0)
                mainWindow.PictureSettings.text_width.Value = presetQuery.MaxWidth;

            if (presetQuery.MaxHeight != 0)
                mainWindow.PictureSettings.text_height.Value = presetQuery.MaxHeight;

            mainWindow.PictureSettings.PresetMaximumResolution = new Size(presetQuery.MaxWidth, presetQuery.MaxHeight);

            // Case where both height and max height are 0 - For built-in presets
            if (presetQuery.MaxHeight == 0 && presetQuery.Height == 0)
                mainWindow.PictureSettings.text_height.Value = 0;

            if (presetQuery.MaxWidth == 0 && presetQuery.Width == 0)
                if (mainWindow.selectedTitle != null && mainWindow.selectedTitle.Resolution.Width != 0)
                    mainWindow.PictureSettings.text_width.Value = mainWindow.selectedTitle.Resolution.Width;

            // Aspect Ratio for non anamorphic sources
            if (presetQuery.AnamorphicMode == 0)
                mainWindow.PictureSettings.check_KeepAR.CheckState = presetQuery.Height == 0
                                                                         ? CheckState.Checked
                                                                         : CheckState.Unchecked;

            // Custom Anamorphic Controls
            mainWindow.PictureSettings.updownDisplayWidth.Text = presetQuery.DisplayWidthValue.ToString();
            mainWindow.PictureSettings.updownParHeight.Text = presetQuery.PixelAspectWidth.ToString();
            mainWindow.PictureSettings.updownParWidth.Text = presetQuery.PixelAspectHeight.ToString();
            mainWindow.PictureSettings.drp_modulus.SelectedItem = presetQuery.AnamorphicModulus;

            #endregion

            #region Filters

            mainWindow.Filters.SetDecomb(presetQuery.Decomb);
            mainWindow.Filters.SetDeInterlace(presetQuery.DeInterlace);
            mainWindow.Filters.SetDeNoise(presetQuery.DeNoise);
            mainWindow.Filters.SetDeTelecine(presetQuery.DeTelecine);
            mainWindow.Filters.SetDeBlock(presetQuery.DeBlock);
            mainWindow.Filters.SetGrayScale(presetQuery.Grayscale);

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
            if (presetQuery.VideoQuality != -1)
            {
                mainWindow.radio_cq.Checked = true;
                mainWindow.slider_videoQuality.Value = QualityToSliderValue(presetQuery.VideoEncoder, presetQuery.VideoQuality);
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

            mainWindow.AudioSettings.LoadTracks(presetQuery.AudioInformation);

            #endregion

            #region Other

            mainWindow.x264Panel.X264Query = presetQuery.H264Query;

            // Set the preset name
            mainWindow.labelPreset.Text = "Output Settings (Preset: " + name + ")";

            #endregion
        }

        /// <summary>
        /// Convert a Quality Value to a position value for the Video Quality slider
        /// </summary>
        /// <param name="videoEncoder">The selected video encoder</param>
        /// <param name="value">The Quality value</param>
        /// <returns>The position on the video quality slider</returns>
        private static int QualityToSliderValue(string videoEncoder, float value)
        {
            int sliderValue = 0;
            switch (videoEncoder)
            {
                case "MPEG-4 (FFmpeg)":
                    sliderValue = 32 - (int)value;
                    break;
                case "H.264 (x264)":
                    double cqStep = Properties.Settings.Default.x264cqstep;
                    sliderValue = (int)((51.0 / cqStep) - (value/cqStep));
                    break;
                case "VP3 (Theora)":
                    sliderValue = (int)value;
                    break;
            }

            return sliderValue;
        }
    }
}