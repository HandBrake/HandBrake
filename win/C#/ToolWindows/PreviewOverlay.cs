/*  TitleSpecificScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.ToolWindows
{
    using System;
    using System.Windows.Forms;

    /// <summary>
    /// Title Specific Scan
    /// </summary>
    public partial class PreviewOverlay : Form
    {
        public PreviewOverlay()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Gets the preview frame that the user entered.
        /// </summary>
        public int Preview
        {
            get
            {
                int value;
                int.TryParse(drp_preview.SelectedItem.ToString(), out value);

                return value;
            }
        }

        /// <summary>
        /// Gets the duration that the user entered.
        /// </summary>
        public int Duration
        {
            get
            {
                int value;
                int.TryParse(drp_preview.SelectedItem.ToString(), out value);

                return value;
            }
        }

        /// <summary>
        /// Play the video with Quicktime
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The EventArgs</param>
        private void PlayWithQtClick(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
        }
    }
}