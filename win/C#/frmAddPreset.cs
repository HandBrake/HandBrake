/*  frmAddPreset.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace Handbrake
{
    public partial class frmAddPreset : Form
    {
        private frmMain frmMainWindow;
        public frmAddPreset(frmMain fmw)
        {
            InitializeComponent();
            frmMainWindow = fmw;
        }

        private void btn_add_Click(object sender, EventArgs e)
        {
            if (txt_preset_name.Text.Trim() == "")
                MessageBox.Show("You have not entered a name for the preset.","Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else if (txt_preset_name.Text.Trim().Contains("--"))
                MessageBox.Show("The preset name can not contain two dashes '--'","Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                Functions.Common hb_common_func = new Functions.Common();

                string userPresets = Application.StartupPath.ToString() + "\\user_presets.dat";
                try
                {
                    // Create a StreamWriter and open the file
                    StreamWriter line = File.AppendText(userPresets);

                    // Generate and write the preset string to the file
                    String query = hb_common_func.GenerateTheQuery(frmMainWindow);
                    String preset = "+ " + txt_preset_name.Text + ":  " + query;
                    line.WriteLine(preset);

                    // close the stream
                    line.Close();
                    line.Dispose();
                    MessageBox.Show("Your profile has been sucessfully added.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }
                frmMainWindow.loadPresetPanel();
                this.Close();
            }
        }
    }
}



