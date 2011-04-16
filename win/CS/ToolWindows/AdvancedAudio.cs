/*  Advanced Audio.cs
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.ToolWindows
{
    using System;
    using System.Globalization;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Model.Encoding;

    /// <summary>
    /// Advanced Audio Panel
    /// </summary>
    public partial class AdvancedAudio : Form
    {
        // Culture Info
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        /// <summary>
        /// The Advanced Audio Panel
        /// </summary>
        public AdvancedAudio()
        {
            InitializeComponent();
        }

        private AudioTrack track;

        /// <summary>
        /// Gets or sets the Audio Track to alter.
        /// </summary>
        public AudioTrack Track
        {
            get
            {
                return this.track;
            }
            set
            {
                this.track = value;

                if (this.track == null)
                {
                    return;
                }

                // Set the Gain Control.
                if (track.Gain == 0)
                {
                    gainTrackBar.Value = 21; // The centre point
                }
                else if (track.Gain > 0)
                {
                    gainTrackBar.Value = 21 + track.Gain;
                }
                else if (track.Gain < 0)
                {
                    gainTrackBar.Value = 20 - Math.Abs(track.Gain);
                }

                lbl_GainValue.Text = string.Format("{0} dB", track.Gain);

                // Set the DRC Control
                double drcValue = 0;
                int drcCalculated;
                if (track.DRC != 0)
                    drcValue = ((track.DRC * 10) + 1) - 10;
                int.TryParse(drcValue.ToString(Culture), out drcCalculated);
                tb_drc.Value = drcCalculated;
                lbl_drc.Text = track.DRC.ToString();
            }
        }

        /// <summary>
        /// Close the window
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Args</param>
        private void btn_close_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        /// <summary>
        /// Set the Gain value for the audio track.
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Arg</param>
        private void gainTrackBar_Scroll(object sender, EventArgs e)
        {
            // Figure out the Gain.
            int gain = 0;
            if (gainTrackBar.Value == 21)
            {
                gain = 0;
            }
            else if (gainTrackBar.Value > 21)
            {
                gain = gainTrackBar.Value - 21;
            }
            else if (gainTrackBar.Value < 21)
            {
                gain = (-20 + gainTrackBar.Value);     
            } 

            lbl_GainValue.Text = string.Format("{0} dB", gain);

            // Figure out the DRC Value
            double drcValue = 0;
            int drcCalculated;
            if (track.DRC != 0)
                drcValue = ((track.DRC * 10) + 1) - 10;
            int.TryParse(drcValue.ToString(Culture), out drcCalculated);
            tb_drc.Value = drcCalculated;

            // Set the model.
            if (this.track == null)
            {
                return;
            }
            this.Track.Gain = gain;
        }

        /// <summary>
        /// The Dynamic Range Controller
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Args</param>
        private void tb_drc_Scroll(object sender, EventArgs e)
        {
            double value;
            if (tb_drc.Value == 0) value = 0;
            else
                value = ((tb_drc.Value - 1) / 10.0) + 1;

            lbl_drc.Text = value.ToString();
            track.DRC = value;
        }
    }
}
