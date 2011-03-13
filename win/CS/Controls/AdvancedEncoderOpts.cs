/*  AdvancedEncoderOpts.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System.Windows.Forms;

    /// <summary>
    /// The x264 Panel
    /// </summary>
    public partial class AdvancedEncoderOpts : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AdvancedEncoderOpts"/> class. 
        /// </summary>
        public AdvancedEncoderOpts()
        {
            InitializeComponent();

            if (Properties.Settings.Default.tooltipEnable)
                ToolTip.Active = true;
        }

        /// <summary>
        /// Gets or sets the X264 query string
        /// </summary>
        public string AdavancedQuery
        {
            get
            {
                return advancedQuery.Text;
            }

            set
            {
                advancedQuery.Text = value;
            }
        }

        public bool IsDisabled
        {
            set
            {
                if (value)
                {
                    this.advancedQuery.Enabled = false;
                    this.advancedQuery.Text = "Advanced encoder option passthrough is not currently supported for the encoder you have chosen.";
                }
                else
                {
                    this.advancedQuery.Enabled = true;
                    this.advancedQuery.Text = string.Empty;
                }
            }
        }
    }
}