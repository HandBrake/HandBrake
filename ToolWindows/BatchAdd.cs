/*  TitleSpecificScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.ToolWindows
{
    using System;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Title Specific Scan
    /// </summary>
    public partial class BatchAdd : Form
    {
        /// <summary>
        /// The User Setting Service.
        /// </summary>
        private readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;

        /// <summary>
        /// Initializes a new instance of the <see cref="BatchAdd"/> class.
        /// </summary>
        public BatchAdd()
        {
            InitializeComponent();

            // Get the Default values for batch encoding.
            this.minDuration.Text = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.BatchMinDuration).ToString();
            this.maxDuration.Text = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.BatchMaxDuration).ToString();
        }

        /// <summary>
        /// Button Cancel Click Event Handler
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The EventArgs</param>
        private void BtnCancelClick(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
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
        /// Gets the minimum duration that the user entered.
        /// </summary>
        public int Min
        {
            get
            {
                int title;
                int.TryParse(this.minDuration.Text, out title);

                return title;
            }
        }

        /// <summary>
        /// Gets the maximum duration that the user entered.
        /// </summary>
        public int Max
        {
            get
            {
                int title;
                int.TryParse(this.maxDuration.Text, out title);

                return title;
            }
        }
    }
}