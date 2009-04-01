/*  Decomb.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class Decomb : UserControl
    {
        public Decomb()
        {
            InitializeComponent();
            drop_decomb.SelectedIndex = 0;
        }

        private void drop_decomb_SelectedIndexChanged(object sender, EventArgs e)
        {
            text_custom.Visible = drop_decomb.Text == "Custom";

            valueChanged(null);
        }

        public string getDropValue
        {
            get { return drop_decomb.Text; }
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
                switch (drop_decomb.Text)
                {
                    case "Off":
                        query = "";
                        break;
                    case "Default":
                        query = " --decomb";
                        break;
                    case "Custom":
                        query = " --decomb=\"" + text_custom.Text + "\"";
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
                    drop_decomb.SelectedIndex = 0;
                    break;
                case "Default":
                    drop_decomb.SelectedIndex = 1;
                    break;
                default:
                    drop_decomb.SelectedIndex = 2;
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
