/*  frmAddPreset.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Drawing;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmAddPreset : Form
    {
        private readonly frmMain _frmMainWindow;
        readonly Presets.PresetsHandler _presetCode;
        private readonly string _query = "";

        public frmAddPreset(frmMain fmw, string queryString, Presets.PresetsHandler presetHandler)
        {
            InitializeComponent();
            _frmMainWindow = fmw;
            _presetCode = presetHandler;
            this._query = queryString;
        }

        private void btn_add_Click(object sender, EventArgs e)
        {
            if (_presetCode.Add(txt_preset_name.Text.Trim(), _query, check_pictureSettings.Checked))
            {
                TreeNode presetTreeview = new TreeNode(txt_preset_name.Text.Trim()) { ForeColor = Color.Black };
                _frmMainWindow.treeView_presets.Nodes.Add(presetTreeview);
                this.Close();
            } 
            else
                MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}