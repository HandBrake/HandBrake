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
        Functions.Common hb_common_func = new Functions.Common();
        private frmMain frmMainWindow;
        Functions.Presets presetCode;

        public frmAddPreset(frmMain fmw, Functions.Presets presetHandler)
        {
            InitializeComponent();
            frmMainWindow = fmw;
            presetCode = presetHandler;
        }

        private void btn_add_Click(object sender, EventArgs e)
        {
            String query = hb_common_func.GenerateTheQuery(frmMainWindow);

            if (presetCode.addPreset(txt_preset_name.Text.Trim(), query) == true)
            {
                frmMainWindow.loadPresetPanel();
                this.Close();
            }
        }

    }
}