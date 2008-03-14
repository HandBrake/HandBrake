/*  frmOptions.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmOptions : Form
    {
        /// <summary>
        /// When the form loads, Initialise all the setting components with their correct values
        /// </summary>
        public frmOptions()
        {
            InitializeComponent();
            if (Properties.Settings.Default.updateStatus == "Checked")
            {
                check_updateCheck.CheckState = CheckState.Checked;
            }

            if (Properties.Settings.Default.defaultSettings == "Checked")
            {
                check_userDefaultSettings.CheckState = CheckState.Checked;
            }
            drp_processors.Text = Properties.Settings.Default.Processors;
            drp_Priority.Text = Properties.Settings.Default.processPriority;
            drp_completeOption.Text = Properties.Settings.Default.CompletionOption;

            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                check_tooltip.CheckState = CheckState.Checked;
            }

            if (Properties.Settings.Default.autoNaming == "Checked")
            {
                check_autoNaming.CheckState = CheckState.Checked;
            }

            text_an_path.Text = Properties.Settings.Default.autoNamePath;

            if (text_an_path.Text == "")
                text_an_path.Text = "Click 'Browse' to set the default location";              
        }

        #region Options
        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.updateStatus = check_updateCheck.CheckState.ToString();
        }

        private void check_userDefaultSettings_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.defaultSettings = check_userDefaultSettings.CheckState.ToString();
        }

        private void drp_processors_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.Processors = drp_processors.Text;
        }

        private void drp_Priority_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.processPriority = drp_Priority.Text;
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.Save(); // Small hack for Vista. Seems to work fine on XP without this
            this.Close();
        }

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.tooltipEnable = check_tooltip.CheckState.ToString();
        }

        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.CompletionOption = drp_completeOption.Text;
        }

        private void check_autoNaming_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.autoNaming = check_autoNaming.CheckState.ToString();
        }

        private void btn_browse_Click(object sender, EventArgs e)
        {
            pathFinder.ShowDialog();
            text_an_path.Text = pathFinder.SelectedPath;
        }

        private void text_an_path_TextChanged(object sender, EventArgs e)
        {
            if (text_an_path.Text == "")
            {
                Properties.Settings.Default.autoNamePath = "";
                text_an_path.Text = "Click 'Browse' to set the default location";
            }
            else
                Properties.Settings.Default.autoNamePath = text_an_path.Text;
        }
        #endregion







    }
}