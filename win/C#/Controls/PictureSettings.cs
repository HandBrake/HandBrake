using System;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using Handbrake.Parsing;

namespace Handbrake.Controls
{
    // TODO
    // - Tie in the cropping controls.
    // - Cleanup this code. It's a bit messy.
    
    public partial class PictureSettings : UserControl
    {
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        // Globals
        public int maxWidth, maxHeight;
        int widthVal, heightVal;
        private double darValue;
        private double storageAspect; // Storage Aspect Cache for current source and title
        public Title selectedTitle { private get; set; }
        private Boolean heightChangeGuard;
        private Boolean looseAnamorphicHeightGuard;
        private Boolean heightModJumpGaurd;

        // Window Setup
        public PictureSettings()
        {
            InitializeComponent();
            lbl_max.Text = "";
            drop_modulus.SelectedIndex = 0;
            storageAspect = 0;
        }
        public void setComponentsAfterScan(Title st)
        {
            storageAspect = 0;
            selectedTitle = st;
            // Set the Aspect Ratio
            lbl_Aspect.Text = selectedTitle.AspectRatio.ToString(Culture);
            lbl_src_res.Text = selectedTitle.Resolution.Width + " x " + selectedTitle.Resolution.Height;

            // Set the Recommended Cropping values
            crop_top.Value = selectedTitle.AutoCropDimensions[0];
            crop_bottom.Value = selectedTitle.AutoCropDimensions[1];
            crop_left.Value = selectedTitle.AutoCropDimensions[2];
            crop_right.Value = selectedTitle.AutoCropDimensions[3];


            // Set the Resolution Boxes
            text_width.Value = selectedTitle.Resolution.Width;

            if (drp_anamorphic.SelectedIndex == 0)
                text_height.Value = cacluateHeight(selectedTitle.Resolution.Width);
            else if (drp_anamorphic.SelectedIndex == 1 || drp_anamorphic.SelectedIndex == 3)
            {
                heightModJumpGaurd = true;
                text_height.Value = selectedTitle.Resolution.Height - (int) crop_top.Value - (int) crop_bottom.Value;
            }
            else if (drp_anamorphic.SelectedIndex == 2)
            {
                heightModJumpGaurd = false;
                text_height.Value = selectedTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
            }

            if (drp_anamorphic.SelectedIndex == 3)
            {
                txt_parWidth.Text = selectedTitle.ParVal.Width.ToString();
                txt_parHeight.Text = selectedTitle.ParVal.Height.ToString();
                txt_displayWidth.Text = displayWidth().ToString(Culture);
            }

            setMax();
        }
        public void setMax()
        {
            if (maxWidth != 0 && maxHeight != 0)
                lbl_max.Text = "Max Width / Height";
            else if (maxWidth != 0)
                lbl_max.Text = "Max Width";
            else
                lbl_max.Text = "";
        }

        // Basic Picture Setting Controls
        private void text_width_ValueChanged(object sender, EventArgs e)
        {
            maxWidth = 0;
            setMax();
            
            // Get the Modulus
            int mod;
            int.TryParse(drop_modulus.SelectedItem.ToString(), out mod);

            // Increase or decrease value by the correct mod.
            text_width.Value = widthChangeMod(mod);

            // Mode Switch
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (calculateUnchangeValue(true) != -1 && check_KeepAR.Checked)
                        text_height.Value = calculateUnchangeValue(true);
                    break;
                case 1:
                    lbl_anamorphic.Text = strictAnamorphic();
                    break;
                case 2:
                    lbl_anamorphic.Text = looseAnamorphic();
                    break;
                case 3:
                    customAnamorphic(text_width);
                    break;
            }
                
        }
        private void text_height_ValueChanged(object sender, EventArgs e)
        {
            maxHeight = 0;
            setMax();

            // Get the Modulus
            int mod;
            int.TryParse(drop_modulus.SelectedItem.ToString(), out mod);

            // Increase or decrease value by the correct mod.
            if (drp_anamorphic.SelectedIndex != 2 && !heightModJumpGaurd)
            {
                decimal val = heightChangeMod(mod);
                heightChangeGuard = true;
                if (text_height.Value != val)
                {
                    heightChangeGuard = false;
                    text_height.Value = val;
                }
            }

            // Mode Switch
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (drp_anamorphic.SelectedIndex != 3)
                    {
                        if (calculateUnchangeValue(false) != -1)
                            text_width.Value = calculateUnchangeValue(false);
                    }
                    break;
                case 1:
                    lbl_anamorphic.Text = strictAnamorphic();
                    break;
                case 2:
                    if (!looseAnamorphicHeightGuard)
                        lbl_anamorphic.Text = looseAnamorphic();
                    break;
                case 3:
                    if (!heightChangeGuard)
                        customAnamorphic(text_height);
                    break;
            }
            heightChangeGuard = false;
            looseAnamorphicHeightGuard = false;
            heightModJumpGaurd = false;
        }
        private void check_KeepAR_CheckedChanged(object sender, EventArgs e)
        {
            // Recalculate Height based on width when enabled
            if (drp_anamorphic.SelectedIndex != 3 && check_KeepAR.Checked && selectedTitle != null)
                text_height.Value = cacluateHeight(widthVal);

            // Enable Par Width/Height Check boxes if Keep AR is enabled, otherwise disable them
            if (check_KeepAR.Checked)
            {
                txt_parWidth.Enabled = false;
                txt_parHeight.Enabled = false;
            }
            else
            {
                txt_parWidth.Enabled = true;
                txt_parHeight.Enabled = true;
            }
        }
        private void drp_anamorphic_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0: // None
                    text_height.Enabled = true;
                    text_width.Enabled = true;
                    check_KeepAR.CheckState = CheckState.Checked;
                    check_KeepAR.Enabled = true;
                    disableCustomAnaControls();
                    if (selectedTitle != null)
                    {
                        text_width.Value = selectedTitle.Resolution.Width;
                        text_height.Value = selectedTitle.Resolution.Height;
                    }
                    check_KeepAR.Enabled = true;
                    lbl_anamorphic.Text = "";
                    lbl_anamprohicLbl.Visible = false;
                    break;
                case 1: // Strict
                    if (selectedTitle != null)
                    {
                        heightModJumpGaurd = true;
                        text_width.Value = selectedTitle.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        text_height.Value = selectedTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                    }
                    text_height.Enabled = false;
                    text_width.Enabled = false;
                    check_KeepAR.CheckState = CheckState.Unchecked;
                    check_KeepAR.Enabled = false;
                    disableCustomAnaControls();
                    lbl_anamorphic.Text = strictAnamorphic();
                    lbl_anamprohicLbl.Visible = true;
                    break;
                case 2: // Loose
                    disableCustomAnaControls();
                    storageAspect = 0;
                    text_height.Enabled = false;
                    text_width.Enabled = true;
                    if (selectedTitle != null)
                    {
                        heightModJumpGaurd = true;
                        text_width.Value = selectedTitle.Resolution.Width;
                        text_height.Value = selectedTitle.Resolution.Height - (int) crop_top.Value - (int) crop_bottom.Value;
                    }
                    lbl_anamorphic.Text = looseAnamorphic();
                    lbl_anamprohicLbl.Visible = true;
                    break;
                case 3: // Custom

                    // Display Elements
                    enableCustomAnaControls();
                    text_height.Enabled = true;
                    text_width.Enabled = true;

                    // Actual Work  
                    if (selectedTitle != null)
                    {
                        heightModJumpGaurd = true;
                        widthVal = selectedTitle.Resolution.Width;
                        text_width.Value = selectedTitle.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        text_height.Value = selectedTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                        txt_parWidth.Text = selectedTitle.ParVal.Width.ToString();
                        txt_parHeight.Text = selectedTitle.ParVal.Height.ToString();
                        txt_displayWidth.Text = displayWidth().ToString(Culture);
                    }
   
                    darValue = calculateDar();

                    check_KeepAR.CheckState = CheckState.Checked;
                    check_KeepAR.Enabled = true;
                    lbl_anamprohicLbl.Visible = true;

                    break;
            }
        }

        // Custom Anamorphic Controls
        private void txt_displayWidth_Keyup(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
                customAnamorphic(txt_displayWidth);
        }
        private void txt_parHeight_Keyup(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
                customAnamorphic(txt_parHeight);
        }
        private void txt_parWidth_Keyup(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
                customAnamorphic(txt_parWidth);
        }
        
        // Cropping Controls
        private void check_autoCrop_CheckedChanged(object sender, EventArgs e)
        {
            crop_left.Enabled = false;
            crop_right.Enabled = false;
            crop_top.Enabled = false;
            crop_bottom.Enabled = false;
        }
        private void check_customCrop_CheckedChanged(object sender, EventArgs e)
        {
            crop_left.Enabled = true;
            crop_right.Enabled = true;
            crop_top.Enabled = true;
            crop_bottom.Enabled = true;
            if (selectedTitle != null)
            {
                crop_top.Value = selectedTitle.AutoCropDimensions[0];
                crop_bottom.Value = selectedTitle.AutoCropDimensions[1];
                crop_left.Value = selectedTitle.AutoCropDimensions[2];
                crop_right.Value = selectedTitle.AutoCropDimensions[3];
            }
            else
            {
                crop_left.Value = 0;
                crop_right.Value = 0;
                crop_top.Value = 0;
                crop_bottom.Value = 0;
            }
        }
        private void crop_left_ValueChanged(object sender, EventArgs e)
        {
            if (crop_left.Value % 2 != 0)
                crop_left.Value++;
        }
        private void crop_right_ValueChanged(object sender, EventArgs e)
        {
            if (crop_right.Value % 2 != 0)
                crop_right.Value++;
        }
        private void crop_top_ValueChanged(object sender, EventArgs e)
        {
            if (crop_top.Value % 2 != 0)
                crop_top.Value++;
        }
        private void crop_bottom_ValueChanged(object sender, EventArgs e)
        {
            if (crop_bottom.Value % 2 != 0)
                crop_bottom.Value++;
        }

        // Custom Anamorphic Code
        private void customAnamorphic(Control control)
        {
            // Get and parse all the required values
            int cropLeft = (int)crop_left.Value;
            int cropRight = (int)crop_right.Value;

            int width = (int)text_width.Value;
            int cropped_width = width - cropLeft - cropRight;

            int mod = 16;
            int.TryParse(drop_modulus.SelectedItem.ToString(), out mod);

            int parW, parH;
            double displayWidth;
            int.TryParse(txt_parWidth.Text, out parW);
            int.TryParse(txt_parHeight.Text, out parH);
            double.TryParse(txt_displayWidth.Text, out displayWidth);

            /* NOT KEEPING DISPLAY ASPECT
             * Changing STORAGE WIDTH changes DISPLAY WIDTH to STORAGE WIDTH * PIXEL WIDTH / PIXEL HEIGHT
             * Changing PIXEL dimensions changes DISPLAY WIDTH to STORAGE WIDTH * PIXEL WIDTH / PIXEL HEIGHT
             * Changing DISPLAY WIDTH changes PIXEL WIDTH to DISPLAY WIDTH and PIXEL HEIGHT to STORAGE WIDTH
             * Changing HEIGHT just....changes the height.
             */
            if (!check_KeepAR.Checked)
            {
                switch (control.Name)
                {
                    case "text_width":
                        double dw = (double)cropped_width * parW / parH;
                        dw = Math.Round(dw, 2);
                        txt_displayWidth.Text = dw.ToString(Culture);
                        break;
                    case "txt_parWidth":
                        double dwpw = (double)cropped_width * parW / parH;
                        dwpw = Math.Round(dwpw, 2);
                        txt_displayWidth.Text = dwpw.ToString(Culture);
                        break;
                    case "txt_parHeight":
                        double dwph = (double)cropped_width * parW / parH;
                        dwph = Math.Round(dwph, 2);
                        txt_displayWidth.Text = dwph.ToString(Culture);
                        break;
                    case "txt_displayWidth":
                        txt_parWidth.Text = Math.Round(displayWidth, 0).ToString();
                        txt_parHeight.Text = text_width.Text;
                        break;
                }
            }

            /*
             * KEEPING DISPLAY ASPECT RATIO
             * DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)
             * Disable editing: PIXEL WIDTH, PIXEL HEIGHT
             * Changing DISPLAY WIDTH:
             *     Changes HEIGHT to keep DAR
             *     Changes PIXEL WIDTH to new DISPLAY WIDTH
             *     Changes PIXEL HEIGHT to STORAGE WIDTH
             * Changing HEIGHT
             *     Changes DISPLAY WIDTH to keep DAR
             *     Changes PIXEL WIDTH to new DISPLAY WIDTH
             *     Changes PIXEL HEIGHT to STORAGE WIDTH
             * Changing STORAGE_WIDTH:
             *     Changes PIXEL WIDTH to DISPLAY WIDTH
             *     Changes PIXEL HEIGHT to new STORAGE WIDTH 
             */

            if (check_KeepAR.Checked)
            {
                switch (control.Name)
                {
                    case "txt_displayWidth":
                        heightChangeGuard = true;
                        text_height.Value = (decimal)getHeightKeepDar();  //Changes HEIGHT to keep DAR
                        //darValue = calculateDar(); // Cache the dar value
                        txt_parWidth.Text = txt_displayWidth.Text;
                        txt_parHeight.Text = cropped_width.ToString();
                        break;
                    case "text_height":
                        heightChangeGuard = true;
                        txt_displayWidth.Text = getDisplayWidthKeepDar().ToString(Culture);  //Changes DISPLAY WIDTH to keep DAR
                        txt_parWidth.Text = txt_displayWidth.Text;
                        txt_parHeight.Text = cropped_width.ToString();
                        break; 
                    case "text_width":
                        txt_parWidth.Text = txt_displayWidth.Text;
                        txt_parHeight.Text = cropped_width.ToString();
                        break;
                }
            }
        }
        private double getDisplayWidthKeepDar()
        {
            double displayWidth;
            double.TryParse(txt_displayWidth.Text, out displayWidth);
            double currentDar = calculateDar();
            double newDwValue = displayWidth;

            // Correct display width up or down to correct for dar.           
            if (currentDar > darValue)
            {
                while (currentDar > darValue)
                {
                    displayWidth--;
                    newDwValue = displayWidth;
                    currentDar = calculateDarByVal(text_height.Value, displayWidth);
                }
            }
            else
            {
                while (currentDar < darValue)
                {
                    displayWidth++;
                    newDwValue = displayWidth;
                    currentDar = calculateDarByVal(text_height.Value, displayWidth);
                }
            }

            return Math.Round(newDwValue, 2);
        }
        private double getHeightKeepDar()
        {
            double displayWidth;
            double.TryParse(txt_displayWidth.Text, out displayWidth);
            double currentDar = calculateDar();
            double newHeightVal = heightVal;

            // Correct display width up or down.
            if (currentDar > darValue)
            {
                while (currentDar > darValue)
                {
                    heightVal++;
                    newHeightVal = heightVal;
                    currentDar = calculateDarByVal(heightVal, displayWidth);
                }
            }
            else
            {
                while (currentDar < darValue)
                {
                    heightVal--;
                    newHeightVal = heightVal;
                    currentDar = calculateDarByVal(heightVal, displayWidth);
                }
            }

            return newHeightVal;
        }
        private double calculateDar()
        {
            // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)
            double displayWidth;
            double.TryParse(txt_displayWidth.Text, out displayWidth);

            double calculatedDar = displayWidth / (int)text_height.Value;

            return calculatedDar;
        }
        private double calculateDarByVal(decimal croppedHeight, double displayWidth)
        {
            // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)
            double calculatedDar = darValue;
            if (croppedHeight > 0)
                calculatedDar = displayWidth / (double)croppedHeight;

            return calculatedDar;
        }
        private int displayWidth()
        {
            if (selectedTitle != null)
            {
                int actualWidth = (int)text_width.Value;
                int displayWidth = 0;
                int parW, parH;

                int.TryParse(txt_parWidth.Text, out parW);
                int.TryParse(txt_parHeight.Text, out parH);

                if (drp_anamorphic.SelectedIndex != 3)
                    displayWidth = (actualWidth * selectedTitle.ParVal.Width / selectedTitle.ParVal.Height);
                else if (parW > 0 && parH > 0)
                    displayWidth = (actualWidth * parW / parH);

                return displayWidth;
            }
            return -1;
        }

        // Resolution calculation and controls
        private decimal widthChangeMod(int mod)
        {
            // Increase or decrease the height based on the users input.
            decimal returnVal = text_width.Value > widthVal ? getResolutionJump(mod, text_width.Value, true) : getResolutionJump(mod, text_width.Value, false);

            // Make sure we don't go above source value
            if (selectedTitle != null)
                if (selectedTitle.Resolution.Width < returnVal)
                    returnVal = selectedTitle.Resolution.Width;

            /*if (returnVal < 64)
                returnVal = 64; */

            // Set the global tracker
            widthVal = (int)returnVal;

            return returnVal;
        }
        private decimal heightChangeMod(int mod)
        {
            // Increase or decrease the height based on the users input.
            decimal returnVal = text_height.Value > heightVal ? getResolutionJump(mod, text_height.Value, true) : getResolutionJump(mod, text_height.Value, false);

            // Make sure we don't go above source value
            if (selectedTitle != null)
                if (selectedTitle.Resolution.Height < returnVal)
                    returnVal = selectedTitle.Resolution.Height;

            /*if (returnVal < 64)
                returnVal = 64;*/

            // Set the global tracker
            heightVal = (int)returnVal;

            return returnVal;
        }
        private decimal calculateUnchangeValue(Boolean widthChangeFromControl)
        {
            decimal newValue = -1;
            if (selectedTitle != null && drp_anamorphic.SelectedIndex != 3 && drp_anamorphic.SelectedIndex != 2)
                if (widthChangeFromControl)
                    newValue = cacluateHeight(widthVal);
                else
                {
                    if (check_KeepAR.Checked)
                        newValue = cacluateWidth(heightVal);
                }

            return newValue;
        }
        private int getResolutionJump(int mod, decimal value, Boolean up)
        {
            if (up)
                while ((value % mod) != 0)
                    value++;
            else
                while ((value % mod) != 0)
                    value--;

            return (int)value;
        }
        private double getModulusAuto(int mod, double value)
        {
            int modDiv2 = mod / 2;

            if ((value % mod) != 0)
            {
                double modVal = (int)value % mod;
                if (modVal >= modDiv2)
                {
                    modVal = 16 - modVal;
                    value = (int)value + (int)modVal;
                }
                else
                {
                    value = (int)value - (int)modVal;
                }
            }
            return value;
        }
        private int cacluateHeight(int width)
        {
            if (selectedTitle != null)
            {
                int aw = 0;
                int ah = 0;
                if (selectedTitle.AspectRatio == 1.78F)
                {
                    aw = 16;
                    ah = 9;
                }
                if (selectedTitle.AspectRatio == 1.33F)
                {
                    aw = 4;
                    ah = 3;
                }

                if (aw != 0)
                {
                    // Crop_Width = Title->Width - crop_Left - crop_right
                    // Crop_Height = Title->Height - crop_top - crop_bottom
                    double crop_width = selectedTitle.Resolution.Width - (double) crop_left.Value -
                                        (double) crop_right.Value;
                    double crop_height = selectedTitle.Resolution.Height - (double) crop_top.Value -
                                         (double) crop_bottom.Value;

                    double new_height = (width*selectedTitle.Resolution.Width*ah*crop_height)/
                                        (selectedTitle.Resolution.Height*aw*crop_width);

                    new_height = drp_anamorphic.SelectedIndex == 3 ? getModulusAuto(int.Parse(drop_modulus.SelectedItem.ToString()), new_height) : getModulusAuto(16, new_height);

                    int x = int.Parse(new_height.ToString());
                    /*if (x < 64)
                        x = 64; */
                    return x;
                }
            }
            return 0;
        }
        private int cacluateWidth(int height)
        {
            int aw = 0;
            int ah = 0;
            if (selectedTitle.AspectRatio == 1.78F)
            {
                aw = 16;
                ah = 9;
            }
            if (selectedTitle.AspectRatio == 1.33F)
            {
                aw = 4;
                ah = 3;
            }

            if (aw != 0)
            {

                double crop_width = selectedTitle.Resolution.Width - (double)crop_left.Value - (double)crop_right.Value;
                double crop_height = selectedTitle.Resolution.Height - (double)crop_top.Value - (double)crop_bottom.Value;

                double new_width = (height * selectedTitle.Resolution.Height * aw * crop_width) /
                                    (selectedTitle.Resolution.Width * ah * crop_height);

                if (drp_anamorphic.SelectedIndex == 3)
                    new_width = getModulusAuto(int.Parse(drop_modulus.SelectedItem.ToString()), new_width);
                else
                    new_width = getModulusAuto(16, new_width);


                int x = int.Parse(new_width.ToString());

                if (x < 64)
                    x = 64;

                return x;
            }
            return 64;
        }

        // Calculate Resolution for Anamorphic functions
        private string strictAnamorphic()
        {
            // TODO Make sure cropping is Mod2
            if (selectedTitle != null)
            {
                // Calculate the Actual Height
                int actualWidth = (int)text_width.Value - (int)crop_left.Value - (int)crop_right.Value; ;
                int actualHeight = selectedTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                // Calculate Actual Width
                double displayWidth = ((double)actualWidth * selectedTitle.ParVal.Width / selectedTitle.ParVal.Height);
                return Math.Round(displayWidth, 0) + "x" + actualHeight;
            }
            return "Select a Title";
        }
        private string looseAnamorphic()
        {
            if (selectedTitle != null)
            {
                // Get some values
                int actualWidth = (int)text_width.Value - (int)crop_left.Value - (int)crop_right.Value;

                int source_display_width = selectedTitle.Resolution.Width * selectedTitle.ParVal.Width / selectedTitle.ParVal.Height;
                int source_cropped_height = selectedTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                // Calculate storage Aspect and cache it for reuse
                if (storageAspect == 0)
                    storageAspect = (double)actualWidth / source_cropped_height;               

                // Calculate the new height based on the input cropped width
                double hcalc = (actualWidth / storageAspect) + 0.5;
                double newHeight = getModulusAuto(16, hcalc);
                looseAnamorphicHeightGuard = true;

                if (newHeight < 64)
                    newHeight = 64;
                text_height.Value = (decimal)newHeight;   // BUG Out of Range Exception with Width too low here.

                // Calculate the anamorphic width
                double parW = newHeight * source_display_width / source_cropped_height;
                double parH = actualWidth;
                double displayWidth = (actualWidth * parW / parH);

                // Now correct DisplayWidth to maintain Aspect ratio.  ActualHeight was mod16'd and thus AR is slightly different than the worked out displayWidths
                return Math.Truncate(displayWidth) + "x" + newHeight;  
            }
            return "Select a Title";

        }
        
        // GUI
        private void disableCustomAnaControls()
        {
            // Disable Custom Anamorphic Stuff
            lbl_modulus.Visible = false;
            lbl_displayWidth.Visible = false;
            lbl_parWidth.Visible = false;
            lbl_parHeight.Visible = false;
            drop_modulus.Visible = false;
            txt_displayWidth.Visible = false;
            txt_parWidth.Visible = false;
            txt_parHeight.Visible = false;
            check_KeepAR.Enabled = false;
        }
        private void enableCustomAnaControls()
        {
            // Disable Custom Anamorphic Stuff
            lbl_modulus.Visible = true;
            lbl_displayWidth.Visible = true;
            lbl_parWidth.Visible = true;
            lbl_parHeight.Visible = true;
            drop_modulus.Visible = true;
            txt_displayWidth.Visible = true;
            txt_parWidth.Visible = true;
            txt_parHeight.Visible = true;
            check_KeepAR.Enabled = true;
        }

    }
}