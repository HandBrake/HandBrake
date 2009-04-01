/*  Deinterlace.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */
using System;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class Deinterlace : UserControl
    {
        public Deinterlace()
        {
            InitializeComponent();
            drop_deinterlace.SelectedIndex = 0;
        }

        private void drop_detelecine_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_custom.Visible = drop_deinterlace.Text == "Custom";

            valueChanged(null);
        }

        public string getDropValue
        {
            get { return drop_deinterlace.Text; }
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
                switch (drop_deinterlace.Text)
                {
                    case "None":
                        query = "";
                        break;
                    case "Fast":
                        query = " --deinterlace=\"fast\"";
                        break;
                    case "Slow":
                        query = " --deinterlace=\"slow\"";
                        break;
                    case "Slower":
                        query = " --deinterlace=\"slower\"";
                        break;
                    case "Custom":
                        query = " --deinterlace=\"" + text_custom.Text + "\"";
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
                    text_custom.Text = value;
                    text_custom.Visible = true;
                    break;
            }
        }

        public event EventHandler onChange;
        protected virtual void valueChanged(EventArgs e)
        {
            if (onChange != null)
                onChange(this, e);
        }

    }
}
