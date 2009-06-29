using System;
using System.Windows.Forms;

namespace Handbrake.Controls
{
    public partial class Filters : UserControl
    {
        public event EventHandler FilterSettingsChanged;

        public Filters()
        {
            InitializeComponent();
            drop_decomb.SelectedIndex = 0;
            drop_deinterlace.SelectedIndex = 0;
            drop_denoise.SelectedIndex = 0;
            drop_detelecine.SelectedIndex = 0;
        }

        // Controls
        private void drop_detelecine_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDT.Visible = drop_detelecine.Text == "Custom";
            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }
        private void drop_decomb_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDC.Visible = drop_decomb.Text == "Custom";
            if (drop_decomb.SelectedIndex != 0 && drop_deinterlace.SelectedIndex != 0)
                drop_deinterlace.SelectedIndex = 0;

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }
        private void drop_deinterlace_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDI.Visible = drop_deinterlace.Text == "Custom";
            if (drop_decomb.SelectedIndex != 0 && drop_deinterlace.SelectedIndex != 0)
                drop_decomb.SelectedIndex = 0;

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }
        private void drop_denoise_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDN.Visible = drop_denoise.Text == "Custom";

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }
        private void slider_deblock_Scroll(object sender, EventArgs e)
        {
            lbl_deblockVal.Text = slider_deblock.Value == 4 ? "Off" : slider_deblock.Value.ToString();

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }

        // Returns the CLI query for the query generator.
        public string getCLIQuery
        {
            get
            {
                string query = "";

                switch (drop_detelecine.Text)  // DeTelecine
                {
                    case "Off":
                        query += "";
                        break;
                    case "Default":
                        query += " --detelecine";
                        break;
                    case "Custom":
                        query += " --detelecine=\"" + text_customDT.Text + "\"";
                        break;
                    default:
                        query += "";
                        break;
                }


                switch (drop_decomb.Text) // Decomb
                {
                    case "Off":
                        query += "";
                        break;
                    case "Default":
                        query += " --decomb";
                        break;
                    case "Custom":
                        query += " --decomb=\"" + text_customDC.Text + "\"";
                        break;
                    default:
                        query += "";
                        break;
                }

                switch (drop_deinterlace.Text) // DeInterlace
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
                    case "Custom":
                        query += " --deinterlace=\"" + text_customDI.Text + "\"";
                        break;
                    default:
                        query += "";
                        break;
                }

                switch (drop_denoise.Text) // Denoise
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
                    case "Custom":
                        query += " --denoise=\"" + text_customDN.Text + "\"";
                        break;
                    default:
                        query += "";
                        break;
                }

                if (slider_deblock.Value != 4)
                    query += " --deblock=" + slider_deblock.Value;

                if (check_grayscale.Checked)
                    query += " -g ";

                return query;
            }
        }

        // Setup for each component for the preset loader.
        public void setDeTelecine(string value)
        {
            text_customDT.Text = "";
            text_customDT.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_detelecine.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_detelecine.SelectedIndex = 1;
                    break;
                default:
                    drop_detelecine.SelectedIndex = 2;
                    text_customDT.Text = value;
                    text_customDT.Visible = true;
                    break;
            }
        }
        public void setDeNoise(string value)
        {
            text_customDN.Text = "";
            text_customDN.Visible = false;
            switch (value)
            {
                case "None":
                    drop_denoise.SelectedIndex = 0;
                    break;
                case "Weak":
                    drop_denoise.SelectedIndex = 1;
                    break;
                case "Medium":
                    drop_denoise.SelectedIndex = 2;
                    break;
                case "Strong":
                    drop_denoise.SelectedIndex = 3;
                    break;
                default:
                    drop_denoise.SelectedIndex = 4;
                    text_customDN.Text = value;
                    text_customDN.Visible = true;
                    break;
            }
        }
        public void setDeInterlace(string value)
        {
            text_customDI.Text = "";
            text_customDI.Visible = false;
            switch (value)
            {

                case "None":
                    drop_deinterlace.SelectedIndex = 0;
                    break;
                case "Fast":
                    drop_deinterlace.SelectedIndex = 1;
                    break;
                case "Slow":
                    drop_deinterlace.SelectedIndex = 2;
                    break;
                case "Slower":
                    drop_deinterlace.SelectedIndex = 3;

                    break;
                default:
                    drop_deinterlace.SelectedIndex = 4;
                    text_customDI.Text = value;
                    text_customDI.Visible = true;
                    break;
            }
        }
        public void setDecomb(string value)
        {
            text_customDC.Text = "";
            text_customDC.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_decomb.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_decomb.SelectedIndex = 1;
                    break;
                default:
                    drop_decomb.SelectedIndex = 2;
                    text_customDC.Text = value;
                    text_customDC.Visible = true;
                    break;
            }
        }
        public void setDeBlock(int value)
        {
            if (value != 0)
            {
                slider_deblock.Value = value;
                lbl_deblockVal.Text = value.ToString();
            }
            else
            {
                slider_deblock.Value = 4;
                lbl_deblockVal.Text = "Off";
            }
        }
        public void setGrayScale(bool value)
        {
            check_grayscale.CheckState = value ? CheckState.Checked : CheckState.Unchecked;
        } 
    }
}
