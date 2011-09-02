/*  TitleSpecificScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.ToolWindows
{
    using System;
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Title Specific Scan
    /// </summary>
    public partial class BatchAdd : Form
    {
        /// <summary>
        /// The standard display count texts
        /// </summary>
        private const string DisplayAddCount = "This will add {0} items.";

        /// <summary>
        /// The Source Data (IF Available)
        /// </summary>
        private readonly Source sourceData;

        /// <summary>
        /// The User Setting Service.
        /// </summary>
        private readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;
            
        /// <summary>
        /// Initializes a new instance of the <see cref="BatchAdd"/> class.
        /// </summary>
        /// <param name="sourceData">
        /// The source Data.
        /// </param>
        public BatchAdd(Source sourceData)
        {
            this.sourceData = sourceData;
            InitializeComponent();

            // Get the Default values for batch encoding.
            this.minDuration.Text = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.BatchMinDuration);
            this.maxDuration.Text = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.BatchMaxDuration);
            this.UpdateEncodeDisplay();
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
            this.UserSettingService.SetUserSetting(UserSettingConstants.BatchMinDuration, this.minDuration.Text);
            this.UserSettingService.SetUserSetting(UserSettingConstants.BatchMaxDuration, this.maxDuration.Text);
            this.DialogResult = DialogResult.OK;
        }

        /// <summary>
        /// Gets the minimum duration that the user entered.
        /// </summary>
        public TimeSpan Min
        {
            get
            {
                TimeSpan title;
                TimeSpan.TryParse(this.minDuration.Text, out title);

                return title;
            }
        }

        /// <summary>
        /// Gets the maximum duration that the user entered.
        /// </summary>
        public TimeSpan Max
        {
            get
            {
                TimeSpan title;
                TimeSpan.TryParse(this.maxDuration.Text, out title);

                return title;
            }
        }

        /// <summary>
        /// Update the Display which shows the number of titles that will be added.
        /// </summary>
        private void UpdateEncodeDisplay()
        {
            int count = this.sourceData.Titles.Count(title => title.Duration.TotalSeconds > this.Min.TotalSeconds && title.Duration.TotalSeconds < this.Max.TotalSeconds);

            if (count > 0)
            {
                lbl_display.Text = string.Format(DisplayAddCount, count);
                lbl_display.Visible = true;
            }
            else
            {
                lbl_display.Visible = false;
            }
        }

        /// <summary>
        /// Min Duration has changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void minDuration_TextChanged(object sender, EventArgs e)
        {
            this.UpdateEncodeDisplay();
        }

        /// <summary>
        /// Max duration was changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void maxDuration_TextChanged(object sender, EventArgs e)
        {
            this.UpdateEncodeDisplay();
        }
    }
}