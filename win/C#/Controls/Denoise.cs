/*  DeNoise.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */
using System;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class Denoise : UserControl
    {
        public Denoise()
        {
            InitializeComponent();
            drop_denoise.SelectedIndex = 0;
        }

        private void drop_decomb_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drop_denoise.Text == "Custom")
                text_custom.Visible = true;
            else
                text_custom.Visible = false;
        }

        public string getDropValue
        {
            get { return drop_denoise.Text; }
        }

        public string getCustomValue
        {
            get { return text_custom.Text; }
        }

        public string getCLIQuery
        {
            get
            {
                string query;
                switch (drop_denoise.Text)
                {
                    case "None":
                        query = "";
                        break;
                    case "Weak":
                        query = " --denoise=\"weak\"";
                        break;
                    case "Medium":
                        query = " --denoise=\"medium\"";
                        break;
                    case "Strong":
                        query = " --denoise=\"strong\"";
                        break;
                    case "Custom":
                        query = " --denoise=\"" + text_custom.Text + "\"";
                        break;
                    default:
                        query = "";
                        break;
                }
                return query;
            }
        }

        public void setOption(string value)
        {
            text_custom.Text = "";
            text_custom.Visible = false;
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
                    text_custom.Text = value;
                    text_custom.Visible = true;
                    break;
            }
        }

    }
}
