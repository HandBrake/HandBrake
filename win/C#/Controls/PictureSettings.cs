using System;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using Handbrake.Parsing;

namespace Handbrake.Controls
{
    public partial class PictureSettings : UserControl
    {
        private bool _preventChangingWidth, _preventChangingHeight;
        private int _maxWidth, _maxHeight, _lastEncodeWidth, _lastEncodeHeight;
        private double _anamorphicRatio, _displayRatio;
        private Title _title;
        private double storageAspect;
        public event EventHandler PictureSettingsChanged;

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
            get { return _title; }
            set
            {
                _title = value;
                _displayRatio = ((double)_title.Resolution.Width * _title.ParVal.Width / _title.ParVal.Height) / _title.Resolution.Height;

                Enabled = _title != null;

                MaximumWidth = _title.Resolution.Width;
                MaximumHeight = _title.Resolution.Height;

                updownParWidth.Value = _title.ParVal.Width;
                updownParHeight.Value = _title.ParVal.Height;

                // Set the source resolution
                lbl_src_res.Text = Source.Resolution.Width + " x " + Source.Resolution.Height;

                // Set ratios
                _anamorphicRatio = (double)Source.Resolution.Width / Source.Resolution.Height;

                // Set the encode width and height
                EncodeWidth = _title.Resolution.Width;
                EncodeHeight = _title.Resolution.Height;

                _lastEncodeWidth = _title.Resolution.Width;
                _lastEncodeHeight = _title.Resolution.Height;

                // Set cropping
                CroppingValues = _title.AutoCropDimensions;

                lbl_Aspect.Text = _title.AspectRatio.ToString();

                UpdateAnamorphicValue();
            }
        }

        /// <summary>
        /// Gets or sets the resolution of the displayed video.
        /// </summary>
        public Size DisplayResolution { get; set; }

        public int EncodeWidth { get { return (int)text_width.Value; } set { text_width.Value = value; } }

        public int EncodeHeight { get { return (int)text_height.Value; } set { text_height.Value = value; } }

        public int[] CroppingValues
        {
            get
            {
                return new int[4]
                {
                    (int)crop_top.Value,
                    (int)crop_bottom.Value,
                    (int)crop_left.Value,
                    (int)crop_right.Value
                };
            }
            set
            {
                if (value.Length != 4)
                {
                    throw new ArgumentException("The cropping values given must have a length of 4.");
                }

                crop_top.Value = value[0];
                crop_bottom.Value = value[1];
                crop_left.Value = value[2];
                crop_right.Value = value[3];
            }
        }

        /// <summary>
        /// Gets or sets the maximum allowable width of the encoded video.
        /// </summary>
        public int MaximumWidth
        {
            get { return _maxWidth; }
            set
            {
                _maxWidth = value > 0 ? value : (Source != null ? Source.Resolution.Width : 2560);
            }
        }

        /// <summary>
        /// Gets or sets the maximum allowable height of the encoded video.
        /// </summary>
        public int MaximumHeight
        {
            get { return _maxHeight; }
            set
            {
                _maxHeight = value > 0 ? value : (Source != null ? Source.Resolution.Height : 2560);
            }
        }      

        public void setMax()
        {

        }

        /// <summary>
        /// Updates the anamorphic value shown as the display width.
        /// </summary>
        private void UpdateAnamorphicValue()
        {
            if (_title == null || _title.ParVal.IsEmpty)
                return;

            // Set globally useful values
            double width;
            double par;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 2:
                    int actualWidth = (int)text_width.Value - (int)crop_left.Value - (int)crop_right.Value;
                    int source_display_width = Source.Resolution.Width * Source.ParVal.Width / Source.ParVal.Height;
                    int source_cropped_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                    par = ((double) text_height.Value*source_display_width/source_cropped_height)/actualWidth;
                    width = (actualWidth * par);
                    width = Math.Truncate(width);
                    break;
                default:
                    {
                        if (drp_anamorphic.SelectedIndex == 1) // Strict
                            par = (double)Source.ParVal.Width / Source.ParVal.Height;
                        else // Custom
                            par = (double)updownParWidth.Value / (double)updownParHeight.Value;

                        // Store the latest DAR
                        double rawWidth = (double)text_width.Value * par;
                        _displayRatio = rawWidth / (double)text_height.Value;

                        width = (int)Math.Round(rawWidth);
                        break;
                    }
            }

            labelDisplaySize.Text = width + " x " + text_height.Value;

            updownDisplayWidth.Value = (decimal)width;
            updownParWidth.Value = (decimal)width;
            updownParHeight.Value = text_width.Value;
        }

        /// <summary>
        /// Sets the visibility of the advanced anamorphic options.
        /// </summary>
        /// <param name="value">Whether or not the options should be visible.</param>
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

        /// <summary>
        /// Gets the normalized value from one given by the user that is divisible by the number
        /// set in <see cref="_modulus"/>.
        /// </summary>
        /// <param name="value">The value to normalize</param>
        /// <returns>A number that is divisible by <see cref="_modulus"/>.</returns>
        /// <remarks>
        /// The way that some video codecs, such as x264, compress video is by creating "macroblocks" 
        /// that are seperated into defined squares, often 16x16 pixels. Because of this, if the width 
        /// and height of the encoded video are not each divisible by the modulus defined, video quality
        /// will suffer. This method takes the supplied value and normalizes it to the nearest mutliple
        /// of <see cref="_modulus"/>.
        /// </remarks>
        private int GetModulusValue(int value)
        {
            int mod = int.Parse(drp_modulus.SelectedItem.ToString());
            int remainder = value % mod;

            if (remainder == 0)
                return value;

            return remainder >= mod / 2 ? value + (mod - remainder) : value - remainder;
        }

        private void ApplyStrictAnamorphic()
        {
            if (_anamorphicRatio == 0)
                return;

            _preventChangingWidth = true;
            _preventChangingHeight = true;

            text_width.Value = _title.Resolution.Width;
            text_height.Value = _title.Resolution.Height;

            _preventChangingWidth = false;
            _preventChangingHeight = false;
        }

        /// <summary>
        /// Loosely anamorphs encode width and height values.
        /// </summary>
        private void ApplyLooseAnamorphic()
        {
            // Prevents DivideByZeroExceptions
            if (_anamorphicRatio == 0)
                return;

            int actualWidth = (int)text_width.Value - (int)crop_left.Value - (int)crop_right.Value;
            int source_cropped_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

            if (storageAspect == 0)
                storageAspect = (double)actualWidth / source_cropped_height;
            double hcalc = (actualWidth / storageAspect) + 0.5;
            double newHeight = GetModulusValue((int)hcalc);

            text_width.Value = GetModulusValue((int)text_width.Value);
            text_height.Value = (decimal)newHeight;

            UpdateAnamorphicValue();
        }

        /// <summary>
        /// Anamorphs encode width and height based on the custom options specified.
        /// </summary>
        private void ApplyCustomAnamorphic(Control ctrlUpdated)
        {
            // Make sure the PAR values are set correctly
            if (updownParWidth.Value == 0)
                updownParWidth.Value = Source.ParVal.Width;
            if (updownParHeight.Value == 0)
                updownParHeight.Value = Source.ParVal.Height;

            // Set various values
            int parWidth = (int)updownParWidth.Value;
            int parHeight = (int)updownParHeight.Value;

            if (!check_KeepAR.Checked)
            {
                switch (ctrlUpdated.Name)
                {
                    case "text_width":
                    case "updownParWidth":
                    case "updownParHeight":
                        updownDisplayWidth.Value = Math.Round(text_width.Value * parWidth / parHeight);
                        break;
                    case "updownDisplayWidth":
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;
                        break;
                }
            }
            else
            {
                switch (ctrlUpdated.Name)
                {
                    case "updownDisplayWidth":
                        _preventChangingHeight = true;

                        text_height.Value = GetModulusValue((int)((double)updownDisplayWidth.Value / _displayRatio));

                        _preventChangingHeight = false;
                        goto case "text_width";
                    case "text_height":
                        updownDisplayWidth.Value = GetModulusValue((int)((double)text_width.Value * _anamorphicRatio * _displayRatio));
                        goto case "text_width";
                    case "text_width":
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;
                        break;
                }
            }
        }



        private void text_width_ValueChanged(object sender, EventArgs e)
        {
            if (_preventChangingWidth)
                return;

            _preventChangingWidth = true;

            if (text_width.Value > MaximumWidth)
            {
                text_width.Value = MaximumWidth;
            }

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked)
                    {
                        _preventChangingHeight = true;

                        decimal newHeight = text_width.Value / (decimal)_anamorphicRatio;
                        text_height.Value = newHeight > MaximumHeight ? MaximumHeight : newHeight;

                        _preventChangingHeight = false;
                    }
                    break;
                case 1:
                    ApplyStrictAnamorphic();
                    break;
                case 2:
                    ApplyLooseAnamorphic();
                    break;
                case 3:
                    ApplyCustomAnamorphic((Control)sender);
                    break;
            }

            _preventChangingWidth = false;
        }

        private void text_height_ValueChanged(object sender, EventArgs e)
        {
            if (_preventChangingHeight)
                return;

            _preventChangingHeight = true;

            if (text_height.Value > MaximumHeight)
            {
                text_height.Value = MaximumHeight;
            }

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked)
                    {
                        _preventChangingWidth = true;

                        decimal newWidth = text_height.Value * (decimal)_anamorphicRatio;
                        text_width.Value = newWidth > MaximumWidth ? MaximumWidth : newWidth;

                        _preventChangingWidth = false;
                    }
                    break;
                case 3:
                    ApplyCustomAnamorphic((Control)sender);
                    break;
            }

            _preventChangingHeight = false;
        }

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
                    break;
                case 1:
                    text_width.Enabled = false;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;

                    check_KeepAR.Checked = true;
                    break;
                case 2:
                    text_width.Enabled = true;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;

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

            UpdateAnamorphicValue();
        }

        private void check_KeepAR_CheckedChanged(object sender, EventArgs e)
        {
            if (drp_anamorphic.SelectedIndex != 3)
            {
                if (check_KeepAR.Checked)
                {
                    text_width_ValueChanged(this, new EventArgs());
                }
            }
            else
            {
                updownParWidth.Enabled = !check_KeepAR.Checked;
                updownParHeight.Enabled = !check_KeepAR.Checked;
            }
        }

        private void crop_ValueChanged(object sender, EventArgs e)
        {
            text_width_ValueChanged(this, new EventArgs());
        }

        private void check_autoCrop_CheckedChanged(object sender, EventArgs e)
        {
            crop_top.Enabled = check_customCrop.Checked;
            crop_bottom.Enabled = check_customCrop.Checked;
            crop_left.Enabled = check_customCrop.Checked;
            crop_right.Enabled = check_customCrop.Checked;
        }

        private void drp_modulus_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_width.Increment = int.Parse(drp_modulus.SelectedItem.ToString());
            text_height.Increment = int.Parse(drp_modulus.SelectedItem.ToString());
        }
    }
}
