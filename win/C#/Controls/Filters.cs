/*  Filters.cs $
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Windows.Forms;

    public partial class Filters : UserControl
    {
        /// <summary>
        /// The Filter settings have changed
        /// </summary>
        public event EventHandler FilterSettingsChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="Filters"/> class. 
        /// Creates a new instance of Filters
        /// </summary>
        public Filters()
        {
            InitializeComponent();
            drop_decomb.SelectedIndex = 0;
            drop_deinterlace.SelectedIndex = 0;
            drop_denoise.SelectedIndex = 0;
            drop_detelecine.SelectedIndex = 0;
        }

        /// <summary>
        /// Returns the CLI query for the query generator.
        /// </summary>
        public string GetCliQuery
        {
            get
            {
                string query = string.Empty;

                switch (drop_detelecine.Text) // DeTelecine
                {
                    case "Off":
                        query += string.Empty;
                        break;
                    case "Default":
                        query += " --detelecine";
                        break;
                    case "Custom":
                        query += " --detelecine=\"" + text_customDT.Text + "\"";
                        break;
                    default:
                        query += string.Empty;
                        break;
                }


                switch (drop_decomb.Text) // Decomb
                {
                    case "Off":
                        query += string.Empty;
                        break;
                    case "Default":
                        query += " --decomb";
                        break;
                    case "Custom":
                        query += " --decomb=\"" + text_customDC.Text + "\"";
                        break;
                    default:
                        query += string.Empty;
                        break;
                }

                switch (drop_deinterlace.Text) // DeInterlace
                {
                    case "None":
                        query += string.Empty;
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
                        query += string.Empty;
                        break;
                }

                switch (drop_denoise.Text) // Denoise
                {
                    case "None":
                        query += string.Empty;
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
                        query += string.Empty;
                        break;
                }

                if (slider_deblock.Value != 4)
                    query += " --deblock=" + slider_deblock.Value;

                if (check_grayscale.Checked)
                    query += " -g ";

                return query;
            }
        }

        /// <summary>
        /// Set the Detelecine control
        /// </summary>
        /// <param name="value">The value part of the CLI string</param>
        public void SetDeTelecine(string value)
        {
            text_customDT.Text = string.Empty;
            text_customDT.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_detelecine.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_detelecine.SelectedIndex = 2;
                    break;
                default:
                    drop_detelecine.SelectedIndex = 1;
                    text_customDT.Text = value;
                    text_customDT.Visible = true;
                    break;
            }
        }

        /// <summary>
        /// Set the Denoise control
        /// </summary>
        /// <param name="value">The value part of the CLI string</param>
        public void SetDeNoise(string value)
        {
            text_customDN.Text = string.Empty;
            text_customDN.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_denoise.SelectedIndex = 0;
                    break;
                case "Weak":
                    drop_denoise.SelectedIndex = 2;
                    break;
                case "Medium":
                    drop_denoise.SelectedIndex = 3;
                    break;
                case "Strong":
                    drop_denoise.SelectedIndex = 4;
                    break;
                default:
                    drop_denoise.SelectedIndex = 1;
                    text_customDN.Text = value;
                    text_customDN.Visible = true;
                    break;
            }
        }

        /// <summary>
        /// Set the Deinterlace Control
        /// </summary>
        /// <param name="value">The value part of the CLI string</param>
        public void SetDeInterlace(string value)
        {
            text_customDI.Text = string.Empty;
            text_customDI.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_deinterlace.SelectedIndex = 0;
                    break;
                case "Fast":
                    drop_deinterlace.SelectedIndex = 2;
                    break;
                case "Slow":
                    drop_deinterlace.SelectedIndex = 3;
                    break;
                case "Slower":
                    drop_deinterlace.SelectedIndex = 4;

                    break;
                default:
                    drop_deinterlace.SelectedIndex = 1;
                    text_customDI.Text = value;
                    text_customDI.Visible = true;
                    break;
            }
        }

        /// <summary>
        /// Set the Decomb Control
        /// </summary>
        /// <param name="value">The value part of the CLI string</param>
        public void SetDecomb(string value)
        {
            text_customDC.Text = string.Empty;
            text_customDC.Visible = false;
            switch (value)
            {
                case "Off":
                    drop_decomb.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_decomb.SelectedIndex = 2;
                    break;
                default:
                    drop_decomb.SelectedIndex = 1;
                    text_customDC.Text = value;
                    text_customDC.Visible = true;
                    break;
            }
        }

        /// <summary>
        /// Set the Deblock Control
        /// </summary>
        /// <param name="value">The deblock value</param>
        public void SetDeBlock(int value)
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

        /// <summary>
        /// Set the grayscale control
        /// </summary>
        /// <param name="value">Boolean value</param>
        public void SetGrayScale(bool value)
        {
            check_grayscale.CheckState = value ? CheckState.Checked : CheckState.Unchecked;
        }

        // Controls
        private void DropDetelecineSelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDT.Visible = drop_detelecine.Text == "Custom";
            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }

        private void DropDecombSelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDC.Visible = drop_decomb.Text == "Custom";
            if (drop_decomb.SelectedIndex != 0 && drop_deinterlace.SelectedIndex != 0)
                drop_deinterlace.SelectedIndex = 0;

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }

        private void DropDeinterlaceSelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDI.Visible = drop_deinterlace.Text == "Custom";
            if (drop_decomb.SelectedIndex != 0 && drop_deinterlace.SelectedIndex != 0)
                drop_decomb.SelectedIndex = 0;

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }

        private void DropDenoiseSelectedIndexChanged(object sender, EventArgs e)
        {
            text_customDN.Visible = drop_denoise.Text == "Custom";

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }

        private void SliderDeblockScroll(object sender, EventArgs e)
        {
            lbl_deblockVal.Text = slider_deblock.Value == 4 ? "Off" : slider_deblock.Value.ToString();

            // A Filter has changed so raise a FilterSettingsChanged event.
            if (this.FilterSettingsChanged != null)
                this.FilterSettingsChanged(this, new EventArgs());
        }
    }
}