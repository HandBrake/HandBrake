/*  frmAddPreset.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using Presets;

    /// <summary>
    /// The Add Preset Window
    /// </summary>
    public partial class frmAddPreset : Form
    {
        /// <summary>
        /// The Main  Window
        /// </summary>
        private readonly frmMain mainWindow;

        /// <summary>
        /// The Preset Handler
        /// </summary>
        private readonly PresetsHandler presetCode;

        /// <summary>
        /// The CLI Query
        /// </summary>
        private readonly string query = string.Empty;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmAddPreset"/> class.
        /// </summary>
        /// <param name="fmw">
        /// The fmw.
        /// </param>
        /// <param name="queryString">
        /// The query string.
        /// </param>
        /// <param name="presetHandler">
        /// The preset handler.
        /// </param>
        public frmAddPreset(frmMain fmw, string queryString, PresetsHandler presetHandler)
        {
            InitializeComponent();
            mainWindow = fmw;
            presetCode = presetHandler;
            this.query = queryString;
        }

        /// <summary>
        /// Handle the Add button event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnAddClick(object sender, EventArgs e)
        {
            if (presetCode.Add(txt_preset_name.Text.Trim(), query, check_pictureSettings.Checked))
            {
                TreeNode presetTreeview = new TreeNode(txt_preset_name.Text.Trim()) {ForeColor = Color.Black};
                mainWindow.treeView_presets.Nodes.Add(presetTreeview);
                this.Close();
            }
            else
                MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", 
                                MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        /// <summary>
        /// Handle the Cancel button event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnCancelClick(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}