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
        Presets.PresetsHandler presetCode;
        private string query = "";

        public frmAddPreset(frmMain fmw, string query_string, Presets.PresetsHandler presetHandler)
        {
            InitializeComponent();
            frmMainWindow = fmw;
            presetCode = presetHandler;
            this.query = query_string;
        }

        private void btn_add_Click(object sender, EventArgs e)
        {
            Boolean pictureSettings = false;
            if (check_pictureSettings.Checked)
                pictureSettings = true;

            if (presetCode.addPreset(txt_preset_name.Text.Trim(), query, pictureSettings) == true)
            {
                frmMainWindow.loadPresetPanel();
                this.Close();
            }
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

    }
}