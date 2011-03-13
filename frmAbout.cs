/*  frmAbout.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;

    /// <summary>
    /// The About Window
    /// </summary>
    public partial class frmAbout : Form
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="frmAbout"/> class.
        /// </summary>
        public frmAbout()
        {
            InitializeComponent();

            string nightly = Properties.Settings.Default.hb_version.Contains("svn") ? " (SVN / Nightly Build)" : string.Empty;
            lbl_GUIBuild.Text = Properties.Settings.Default.hb_version + " (" + Properties.Settings.Default.hb_build + ") " + nightly;
        }

        /// <summary>
        /// Button - Close the window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}