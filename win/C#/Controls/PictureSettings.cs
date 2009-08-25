using System;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using Handbrake.Parsing;

namespace Handbrake.Controls
{
    public partial class PictureSettings : UserControl
    {
        private readonly CultureInfo Culture = new CultureInfo("en-US", false);
        public event EventHandler PictureSettingsChanged;

        private Boolean _preventChangingWidth, _preventChangingHeight, _preventChangingCustom, _preventChangingDisplayWidth;
        private int _presetMaximumWidth, _presetMaximumHeight;
        private double _cachedDar;
        private Title _sourceTitle;

        public PictureSettings()
        {
            InitializeComponent();

            drp_anamorphic.SelectedIndex = 1;
            drp_modulus.SelectedIndex = 0;
        }

        /// <summary>
        /// Gets or sets the source media used by this control.
        /// </summary>
        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Title Source
        {
            private get { return _sourceTitle; }
            set
            {
                _sourceTitle = value;
                Enabled = _sourceTitle != null;

                // Set the Aspect Ratio
                lbl_Aspect.Text = _sourceTitle.AspectRatio.ToString(Culture);
                lbl_src_res.Text = _sourceTitle.Resolution.Width + " x " + _sourceTitle.Resolution.Height;

                // Set the Recommended Cropping values
                crop_top.Value = GetCropMod2Clean(_sourceTitle.AutoCropDimensions[0]);
                crop_bottom.Value = GetCropMod2Clean(_sourceTitle.AutoCropDimensions[1]);
                crop_left.Value = GetCropMod2Clean(_sourceTitle.AutoCropDimensions[2]);
                crop_right.Value = GetCropMod2Clean(_sourceTitle.AutoCropDimensions[3]);

                // Set the Resolution Boxes
                if (drp_anamorphic.SelectedIndex == 0)
                {
                    if (text_width.Value == 0) // Only update the values if the fields don't already have values.
                        text_width.Value = _sourceTitle.Resolution.Width;

                    check_KeepAR.Checked = true; // Forces Resolution to be correct.
                }
                else
                {
                    if (text_width.Value == 0 && text_height.Value == 0)// Only update the values if the fields don't already have values.
                    {
                        text_width.Value = _sourceTitle.Resolution.Width;
                        text_height.Value = _sourceTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                    }

                    labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
                }

                updownDisplayWidth.Value = CalculateAnamorphicSizes().Width;
                updownParWidth.Value = _sourceTitle.ParVal.Width;
                updownParHeight.Value = _sourceTitle.ParVal.Height;
                _cachedDar = (double)updownDisplayWidth.Value / (double)text_height.Value;
            }
        }

        /// <summary>
        /// Gets or sets the maximum allowable size for the encoded resolution. Set a value to
        /// "0" if the maximum does not matter.
        /// </summary>
        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Size PresetMaximumResolution
        {
            get { return new Size(_presetMaximumWidth, _presetMaximumHeight); }
            set
            {
                _presetMaximumWidth = value.Width;
                _presetMaximumHeight = value.Height;

                if (value.Width != 0 && value.Height != 0)
                    lbl_max.Text = "Max Width / Height";
                else if (value.Width != 0)
                    lbl_max.Text = "Max Width";
                else if (value.Height != 0)
                    lbl_max.Text = "Max Height";
                else
                    lbl_max.Text = "";
            }
        }

        // Picture Controls
        private void text_width_ValueChanged(object sender, EventArgs e)
        {
            if (_preventChangingWidth)
                return;

            // Make sure the new value doesn't exceed the maximum
            if (Source != null)
                if (text_width.Value > Source.Resolution.Width)
                    text_width.Value = Source.Resolution.Width;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked && Source != null)
                    {
                        _preventChangingHeight = true;

                        int width = (int)text_width.Value;

                        double crop_width = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        double crop_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                        if (SourceAspect.Width == 0 && SourceAspect.Height == 0)
                            break;

                        double newHeight = ((double)width * Source.Resolution.Width * SourceAspect.Height * crop_height) /
                                           (Source.Resolution.Height * SourceAspect.Width * crop_width);
                        text_height.Value = (decimal)GetModulusValue(newHeight);

                        _preventChangingHeight = false;
                    }
                    break;
                case 3:
                    if (check_KeepAR.CheckState == CheckState.Unchecked && Source != null)
                    {
                        if (_preventChangingCustom)
                            break;

                        _preventChangingDisplayWidth = true;
                        updownDisplayWidth.Value = text_width.Value * updownParWidth.Value / updownParHeight.Value;
                        _preventChangingDisplayWidth = false;

                        labelDisplaySize.Text = Math.Truncate(updownDisplayWidth.Value) + "x" + text_height.Value;
                    }

                    if (check_KeepAR.CheckState == CheckState.Checked && Source != null)
                    {
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;
                    }
                    break;
                default:
                    labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
                    break;
            }

            _preventChangingWidth = false;
        }
        private void text_height_ValueChanged(object sender, EventArgs e)
        {
            if (_preventChangingHeight)
                return;

            if (Source != null)
                if (text_height.Value > Source.Resolution.Height)
                    text_height.Value = Source.Resolution.Height;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked && Source != null)
                    {
                        _preventChangingWidth = true;

                        double crop_width = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        double crop_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                        double new_width = ((double)text_height.Value * Source.Resolution.Height * SourceAspect.Width * crop_width) /
                                            (Source.Resolution.Width * SourceAspect.Height * crop_height);

                        text_width.Value = (decimal)GetModulusValue(new_width);

                        _preventChangingWidth = false;
                    }
                    break;
                case 3:
                    labelDisplaySize.Text = Math.Truncate(updownDisplayWidth.Value) + "x" + text_height.Value;

                    if (check_KeepAR.CheckState == CheckState.Checked && Source != null)
                    {
                        // - Changes DISPLAY WIDTH to keep DAR
                        // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                        // - Changes PIXEL HEIGHT to STORAGE WIDTH
                        // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                        double rawCalculatedDisplayWidth = (double)text_height.Value * _cachedDar;

                        _preventChangingDisplayWidth = true; // Start Guards
                        _preventChangingWidth = true;

                        updownDisplayWidth.Value = (decimal)rawCalculatedDisplayWidth;
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;

                        _preventChangingWidth = false; // Reset Guards
                        _preventChangingDisplayWidth = false;
                    }

                    break;
                default:
                    labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
                    break;
            }

            _preventChangingHeight = false;
        }
        private void check_KeepAR_CheckedChanged(object sender, EventArgs e)
        {
            //Force TextWidth to recalc height
            if (check_KeepAR.Checked)
                text_width_ValueChanged(this, new EventArgs());

            // Disable the Custom Anamorphic Par Controls if checked.
            if (drp_anamorphic.SelectedIndex == 3)
            {
                updownParWidth.Enabled = !check_KeepAR.Checked;
                updownParHeight.Enabled = !check_KeepAR.Checked;
            }

            // Raise the Picture Settings Changed Event
            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
        private void updownDisplayWidth_ValueChanged(object sender, EventArgs e)
        {
            if (_preventChangingDisplayWidth == false && check_KeepAR.CheckState == CheckState.Unchecked)
            {
                _preventChangingCustom = true;
                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;
                _preventChangingCustom = false;
            }

            if (_preventChangingDisplayWidth == false && check_KeepAR.CheckState == CheckState.Checked)
            {
                // - Changes HEIGHT to keep DAR
                // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                // - Changes PIXEL HEIGHT to STORAGE WIDTH
                // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                // Calculate new Height Value
                int modulus = 16;
                int.TryParse(drp_modulus.SelectedItem.ToString(), out modulus);

                int rawCalculatedHeight = (int)((int)updownDisplayWidth.Value / _cachedDar);
                int modulusHeight = rawCalculatedHeight - (rawCalculatedHeight % modulus);

                // Update value
                _preventChangingHeight = true;
                text_height.Value = (decimal)modulusHeight;
                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;
                _preventChangingHeight = false;
            }

        }

        // Anamorphic Controls
        private void drp_anamorphic_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    text_width.Enabled = true;
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = false;
                    labelDisplaySize.Visible = false;

                    check_KeepAR.Checked = true;
                    drp_modulus.SelectedIndex = 0;

                    if (check_KeepAR.Checked)
                        text_width_ValueChanged(this, new EventArgs());
                    // Don't update display size if we're not using anamorphic
                    return;
                case 1:
                    text_width.Enabled = false;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;
                    drp_modulus.SelectedIndex = 0;

                    check_KeepAR.Checked = true;
                    break;
                case 2:
                    text_width.Enabled = true;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;
                    drp_modulus.SelectedIndex = 0;

                    check_KeepAR.Checked = true;
                    break;
                case 3:
                    text_width.Enabled = true;
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;

                    SetCustomAnamorphicOptionsVisible(true);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;

                    check_KeepAR.Checked = true;
                    break;
            }

            labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;

            if (check_KeepAR.Checked)
                text_width_ValueChanged(this, new EventArgs());

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
        private void drp_modulus_SelectedIndexChanged(object sender, EventArgs e)
        {
            _preventChangingWidth = true;
            _preventChangingHeight = true;

            text_width.Value = (decimal)GetModulusValue((double)text_width.Value);
            text_height.Value = (decimal)GetModulusValue((double)text_height.Value);

            _preventChangingWidth = false;
            _preventChangingHeight = false;

            text_width.Increment = int.Parse(drp_modulus.SelectedItem.ToString());
            text_height.Increment = int.Parse(drp_modulus.SelectedItem.ToString());

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }

        // Cropping Controls
        private void check_autoCrop_CheckedChanged(object sender, EventArgs e)
        {
            crop_top.Enabled = check_customCrop.Checked;
            crop_bottom.Enabled = check_customCrop.Checked;
            crop_left.Enabled = check_customCrop.Checked;
            crop_right.Enabled = check_customCrop.Checked;
        }
        private void crop_ValueChanged(object sender, EventArgs e)
        {
            text_width_ValueChanged(this, new EventArgs());
        }

        // GUI Functions
        private void SetCustomAnamorphicOptionsVisible(bool visible)
        {
            lbl_modulus.Visible = visible;
            lbl_displayWidth.Visible = visible;
            lbl_parWidth.Visible = visible;
            lbl_parHeight.Visible = visible;

            drp_modulus.Visible = visible;
            updownDisplayWidth.Visible = visible;
            updownParWidth.Visible = visible;
            updownParHeight.Visible = visible;
        }

        // Calculation Functions
        private Size SourceAspect
        {
            get
            {
                if (Source != null) // display aspect = (width * par_width) / (height * par_height)
                    return new Size((Source.ParVal.Width * Source.Resolution.Width), (Source.ParVal.Height * Source.Resolution.Height));

                return new Size(0, 0); // Fall over to 16:9 and hope for the best
            }
        }
        private Size CalculateAnamorphicSizes()
        {
            if (Source != null)
            {
                /* Set up some variables to make the math easier to follow. */
                int cropped_width = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                int cropped_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                double storage_aspect = (double)cropped_width / cropped_height;

                /* Figure out what width the source would display at. */
                double source_display_width = (double)cropped_width * Source.ParVal.Width / Source.ParVal.Height;

                /*
                     3 different ways of deciding output dimensions:
                      - 1: Strict anamorphic, preserve source dimensions
                      - 2: Loose anamorphic, round to mod16 and preserve storage aspect ratio
                      - 3: Power user anamorphic, specify everything
                  */
                double width, height;
                switch (drp_anamorphic.SelectedIndex)
                {
                    default:
                    case 1:
                        /* Strict anamorphic */
                        double displayWidth = ((double)cropped_width * Source.ParVal.Width / Source.ParVal.Height);
                        displayWidth = Math.Round(displayWidth, 0);
                        Size output = new Size((int)displayWidth, cropped_height);
                        return output;
                    case 2:
                        /* "Loose" anamorphic.
                            - Uses mod16-compliant dimensions,
                            - Allows users to set the width
                        */
                        width = (int)text_width.Value - (int)crop_left.Value - (int)crop_right.Value;
                        width = GetModulusValue(width); /* Time to get picture width that divide cleanly.*/

                        height = (width / storage_aspect) + 0.5;
                        height = GetModulusValue(height); /* Time to get picture height that divide cleanly.*/

                        /* The film AR is the source's display width / cropped source height.
                           The output display width is the output height * film AR.
                           The output PAR is the output display width / output storage width. */
                        double pixel_aspect_width = height * source_display_width / cropped_height;
                        double pixel_aspect_height = width;

                        double disWidthLoose = (width * pixel_aspect_width / pixel_aspect_height);
                        return new Size((int)disWidthLoose, (int)height);
                    case 3:

                        // Get the User Interface Values
                        double UIdisplayWidth;
                        double.TryParse(updownDisplayWidth.Text, out UIdisplayWidth);

                        /* Anamorphic 3: Power User Jamboree - Set everything based on specified values */
                        height = GetModulusValue((double)text_height.Value);

                        if (check_KeepAR.Checked)
                            return new Size((int)Math.Truncate(UIdisplayWidth), (int)height);

                        return new Size((int)Math.Truncate(UIdisplayWidth), (int)height);
                }
            }

            // Return a default value of 0,0 to indicate failure
            return new Size(0, 0);
        }
        private double GetModulusValue(double value)
        {
            int mod = int.Parse(drp_modulus.SelectedItem.ToString());
            double remainder = value % mod;

            if (remainder == 0)
                return value;

            return remainder >= ((double)mod / 2) ? value + (mod - remainder) : value - remainder;
        }
        private static int GetCropMod2Clean(int value)
        {
            int remainder = value % 2;
            if (remainder == 0) return value;
            return (value + remainder);
        }
    }
}