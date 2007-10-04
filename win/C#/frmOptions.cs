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

            if (Properties.Settings.Default.verbose == "Checked")
            {
                check_verbose.CheckState = CheckState.Checked;
            }

            if (Properties.Settings.Default.GuiDebug == "Checked")
            {
                check_guiDebug.CheckState = CheckState.Checked;
            }

            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                check_tooltip.CheckState = CheckState.Checked;
            }

            if (Properties.Settings.Default.showPresets == "Checked")
            {
                check_showPreset.CheckState = CheckState.Checked;
            }
        }

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

        private void check_verbose_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.verbose = check_verbose.CheckState.ToString();
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.Save(); // Small hack for Vista. Seems to work fine on XP without this
            this.Close();
        }

        private void check_guiDebug_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.GuiDebug = check_guiDebug.CheckState.ToString();
        }

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.tooltipEnable = check_tooltip.CheckState.ToString();
        }

        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.CompletionOption = drp_completeOption.Text;
        }

        private void check_showPreset_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.showPresets = check_showPreset.CheckState.ToString();
        }  
 

    }
}