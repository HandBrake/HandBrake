/*  TitleSpecificScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;

    /// <summary>
    /// Title Specific Scan
    /// </summary>
    public partial class TitleSpecificScan : Form
    {
        public TitleSpecificScan()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Button Cancel Click Event Handler
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The EventArgs</param>
        private void BtnCancelClick(object sender, EventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// Button Scan Click Event Handler
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The EventArgs</param>
        private void BtnScanClick(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
        }

        /// <summary>
        /// Gets the title that the user entered.
        /// </summary>
        public int Title
        {
            get
            {
                int title;
                int.TryParse(this.titleNumber.Text, out title);

                return title;
            }
        }
    }
}