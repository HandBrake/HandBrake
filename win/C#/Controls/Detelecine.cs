/*  Detelecine.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class Detelecine : UserControl
    {
        public Detelecine()
        {
            InitializeComponent();
            drop_detelecine.SelectedIndex = 0;
        }

        private void drop_detelecine_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_custom.Visible = drop_detelecine.Text == "Custom";
        }

        public string getDropValue
        {
            get { return drop_detelecine.Text; }
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
                switch (drop_detelecine.Text)
                {
                    case "Off":
                        query = "";
                        break;
                    case "Default":
                        query = " --detelecine";
                        break;
                    case "Custom":
                        query = " --detelecine=\"" + text_custom.Text + "\"";
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
                case "Off":
                    drop_detelecine.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_detelecine.SelectedIndex = 1;
                    break;
                default:
                    drop_detelecine.SelectedIndex = 2;
                    text_custom.Text = value;
                    text_custom.Visible = true;
                    break;
            }
        }

    }
}
