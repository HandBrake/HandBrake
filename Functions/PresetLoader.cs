/*  PresetLoader.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System.Drawing;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    using OutputFormat = HandBrake.ApplicationServices.Model.Encoding.OutputFormat;

    /// <summary>
    /// Load a preset into the main Window
    /// </summary>
    public class PresetLoader
    {
        /// <summary>
        /// The User Setting Service.
        /// </summary>
        private static readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;

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
        public static void LoadPreset(frmMain mainWindow, EncodeTask presetQuery, string name)
        {
            #region Source

            // Reset some vaules to stock first to prevent errors.
            mainWindow.check_iPodAtom.CheckState = CheckState.Unchecked;

            // Now load all the new settings onto the main window
            string destination = mainWindow.text_destination.Text;
            destination = destination.Replace(".mp4", "." + presetQuery.OutputFormat);
            destination = destination.Replace(".m4v", "." + presetQuery.OutputFormat);
            destination = destination.Replace(".mkv", "." + presetQuery.OutputFormat);
            mainWindow.text_destination.Text = destination;

            #endregion

            #region Destination and Output Settings

            if (presetQuery.OutputFormat == OutputFormat.Mp4 || presetQuery.OutputFormat == OutputFormat.M4V)
            {
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    mainWindow.SetExtension(".mp4");
                }
                else
                {
                    mainWindow.drop_format.SelectedIndex = 0;
                }
            }
            else if (presetQuery.OutputFormat == OutputFormat.Mkv)
            {
                if (mainWindow.drop_format.SelectedIndex == 1)
                {
                    mainWindow.SetExtension(".mkv");
                }
                else
                {
                    mainWindow.drop_format.SelectedIndex = 1;
                }
            }

            mainWindow.check_iPodAtom.CheckState = presetQuery.IPod5GSupport ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.check_optimiseMP4.CheckState = presetQuery.OptimizeMP4
                                                          ? CheckState.Checked
                                                          : CheckState.Unchecked;

            mainWindow.check_largeFile.CheckState = presetQuery.LargeFile ? CheckState.Checked : CheckState.Unchecked;

            mainWindow.setContainerOpts(); // select the container options according to the selected format

            #endregion

            #region Picture

            mainWindow.PictureSettings.check_autoCrop.Checked = true;
            if (presetQuery.IsCustomCropping)
            {
                mainWindow.PictureSettings.check_customCrop.Checked = true;
                mainWindow.PictureSettings.crop_top.Value = presetQuery.Cropping.Top;
                mainWindow.PictureSettings.crop_bottom.Value = presetQuery.Cropping.Bottom;
                mainWindow.PictureSettings.crop_left.Value = presetQuery.Cropping.Left;
                mainWindow.PictureSettings.crop_right.Value = presetQuery.Cropping.Right;
            }

            // Set the anamorphic mode 0,1,2,3

            switch (presetQuery.Anamorphic)
            {
                case Anamorphic.None:
                    mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = 0;
                    break;
                case Anamorphic.Strict:
                    mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = 1;
                    break;
                case Anamorphic.Loose:
                    mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = 2;
                    break;
                case Anamorphic.Custom:
                    mainWindow.PictureSettings.drp_anamorphic.SelectedIndex = 3;
                    break;
            }

            // Keep Aspect Ration Anamorphic Setting.
            mainWindow.PictureSettings.check_KeepAR.CheckState = presetQuery.KeepDisplayAspect
                                                                     ? CheckState.Checked
                                                                     : CheckState.Unchecked;

            // Set the Width and height as Required.
            if (presetQuery.Width.HasValue)
            {
                mainWindow.PictureSettings.text_width.Value = presetQuery.Width.Value;
            }

            if (presetQuery.Height.HasValue)
            {
                mainWindow.PictureSettings.text_height.Value = presetQuery.Height.Value;
            }

            // Max Width/Height override Width/Height
            if (presetQuery.MaxWidth.HasValue)
            {
                mainWindow.PictureSettings.text_width.Value = presetQuery.MaxWidth.Value;
            }

            if (presetQuery.MaxHeight.HasValue)
            {
                mainWindow.PictureSettings.text_height.Value = presetQuery.MaxHeight.Value;
            }

           mainWindow.PictureSettings.PresetMaximumResolution = new Size(
                    presetQuery.MaxWidth.HasValue ? presetQuery.MaxWidth.Value : 0,
                    presetQuery.MaxHeight.HasValue ? presetQuery.MaxHeight.Value : 0);

            // Case where both height and max height are 0 - For built-in presets
            if (presetQuery.MaxHeight == 0 && presetQuery.Height == 0)
            {
                mainWindow.PictureSettings.text_height.Value = 0;
            }

            if (presetQuery.MaxWidth == 0 && presetQuery.Width == 0)
            {
                if (mainWindow.selectedTitle != null && mainWindow.selectedTitle.Resolution.Width != 0)
                {
                    mainWindow.PictureSettings.text_width.Value = mainWindow.selectedTitle.Resolution.Width;
                }
            }

            // Aspect Ratio for non anamorphic sources
            if (presetQuery.Anamorphic == Anamorphic.None)
            {
                mainWindow.PictureSettings.check_KeepAR.CheckState = presetQuery.Height == 0
                                                                         ? CheckState.Checked
                                                                         : CheckState.Unchecked;
            }

            // Custom Anamorphic Controls
            mainWindow.PictureSettings.updownDisplayWidth.Text = presetQuery.DisplayWidth.ToString();
            mainWindow.PictureSettings.updownParHeight.Text = presetQuery.PixelAspectY.ToString();
            mainWindow.PictureSettings.updownParWidth.Text = presetQuery.PixelAspectX.ToString();
            mainWindow.PictureSettings.drp_modulus.SelectedItem = presetQuery.Modulus.ToString();

            #endregion

            #region Filters

            mainWindow.Filters.SetDecomb(presetQuery.Decomb, presetQuery.CustomDecomb);
            mainWindow.Filters.SetDeInterlace(presetQuery.Deinterlace, presetQuery.CustomDeinterlace);
            mainWindow.Filters.SetDeNoise(presetQuery.Denoise, presetQuery.CustomDenoise);
            mainWindow.Filters.SetDeTelecine(presetQuery.Detelecine, presetQuery.CustomDetelecine);
            mainWindow.Filters.SetDeBlock(presetQuery.Deblock);
            mainWindow.Filters.SetGrayScale(presetQuery.Grayscale);

            #endregion

            #region Video

            switch (presetQuery.VideoEncoder)
            {
                case VideoEncoder.X264:
                    mainWindow.drp_videoEncoder.SelectedIndex = 0;
                    break;
                case VideoEncoder.FFMpeg:
                    mainWindow.drp_videoEncoder.SelectedIndex = 1;
                    break;
                case VideoEncoder.FFMpeg2:
                    mainWindow.drp_videoEncoder.SelectedIndex = 2;
                    break;
                case VideoEncoder.Theora:
                    mainWindow.drp_videoEncoder.SelectedIndex = 3;
                    break;
            }

            // Quality
            if (presetQuery.Quality != null)
            {
                mainWindow.radio_cq.Checked = true;
                mainWindow.slider_videoQuality.Value = QualityToSliderValue(presetQuery.VideoEncoder, presetQuery.Quality);
                mainWindow.check_2PassEncode.CheckState = CheckState.Unchecked;
                mainWindow.check_turbo.CheckState = CheckState.Unchecked;
            }
            else if (presetQuery.VideoBitrate != null)
            {
                mainWindow.radio_avgBitrate.Checked = true;
                mainWindow.text_bitrate.Text = presetQuery.VideoBitrate.ToString();
                mainWindow.check_2PassEncode.CheckState = presetQuery.TwoPass ? CheckState.Checked : CheckState.Unchecked;
                mainWindow.check_turbo.CheckState = presetQuery.TurboFirstPass ? CheckState.Checked : CheckState.Unchecked;
            }              

            if (presetQuery.Framerate != null)
            {
                mainWindow.drp_videoFramerate.Text = presetQuery.Framerate.ToString();
            }
            else
            {
                mainWindow.drp_videoFramerate.SelectedIndex = 0;
            }


            if (presetQuery.Framerate != null)
            {
                // Constant or Peak Framerate for a set framerate.
                if (presetQuery.FramerateMode == FramerateMode.CFR)
                    mainWindow.radio_constantFramerate.Checked = true;
                else
                    mainWindow.radio_peakAndVariable.Checked = true;
            }
            else
            {
                // Constant or Variable Framerate for Same as Source.
                if (presetQuery.FramerateMode == FramerateMode.CFR)
                    mainWindow.radio_constantFramerate.Checked = true;
                else
                    mainWindow.radio_peakAndVariable.Checked = true;
            }

            #endregion

            #region Chapter Markers

            if (presetQuery.IncludeChapterMarkers)
            {
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Checked;
                mainWindow.Check_ChapterMarkers.Enabled = true;
            }
            else
            {
                mainWindow.Check_ChapterMarkers.CheckState = CheckState.Unchecked;
            }

            #endregion

            #region Audio

            mainWindow.AudioSettings.LoadTracks(presetQuery.AudioTracks);

            #endregion

            #region Other

            mainWindow.x264Panel.X264Query = presetQuery.AdvancedEncoderOptions;

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
        private static int QualityToSliderValue(VideoEncoder videoEncoder, double? value)
        {
            if (!value.HasValue)
            {
                // Default to a sensible level.
                return 20;
            }

            int sliderValue = 0;
            switch (videoEncoder)
            {
                case VideoEncoder.FFMpeg:
                case VideoEncoder.FFMpeg2:
                    sliderValue = 32 - (int)value;
                    break;
                case VideoEncoder.X264:
                    double cqStep = UserSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step);
                    sliderValue = (int)((51.0 / cqStep) - (value / cqStep));
                    break;
                case VideoEncoder.Theora:
                    sliderValue = (int)value;
                    break;
            }

            return sliderValue;
        }
    }
}