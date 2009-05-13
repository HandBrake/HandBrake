/*  frmOptions.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmOptions : Form
    {
        public frmOptions()
        {
            InitializeComponent();

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (Properties.Settings.Default.tooltipEnable == "Checked")
            {
                check_tooltip.CheckState = CheckState.Checked;
                ToolTip.Active = true;
            }

            // Setup Widgets to match settings.
            if (Properties.Settings.Default.updateStatus == "Checked")
                check_updateCheck.CheckState = CheckState.Checked;

            // enable loading of default user settings.
            if (Properties.Settings.Default.defaultSettings == "Checked")
                check_userDefaultSettings.CheckState = CheckState.Checked;

            // On Encode Completeion Action
            drp_completeOption.Text = Properties.Settings.Default.CompletionOption;
            
            // Enable auto naming feature.
            if (Properties.Settings.Default.autoNaming == "Checked")
                check_autoNaming.CheckState = CheckState.Checked;

            // Store the auto name path
            text_an_path.Text = Properties.Settings.Default.autoNamePath;
            if (text_an_path.Text == string.Empty)
                text_an_path.Text = "Click 'Browse' to set the default location";

            // Store auto name format
            txt_autoNameFormat.Text = Properties.Settings.Default.autoNameFormat;

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            txt_vlcPath.Text = Properties.Settings.Default.VLC_Path;

            // #############################
            // CLI
            // #############################

            // Enable Start CLI minimized
            if (Properties.Settings.Default.cli_minimized == "Checked")
                check_cli_minimized.CheckState = CheckState.Checked;

            // Number of processor cores
            drp_processors.Text = Properties.Settings.Default.Processors;

            // Priority level for encodes
            drp_Priority.Text = Properties.Settings.Default.processPriority;

            // Log Verbosity Level
            cb_logVerboseLvl.SelectedIndex = Properties.Settings.Default.verboseLevel;


            // Save logs in the same directory as encoded files
            if (Properties.Settings.Default.saveLogWithVideo == "Checked")
                check_saveLogWithVideo.CheckState = CheckState.Checked;

            // Save Logs in a specified path
            if (Properties.Settings.Default.saveLogToSpecifiedPath == "Checked")
                check_logsInSpecifiedLocation.CheckState = CheckState.Checked;

            // The saved log path
            text_logPath.Text = Properties.Settings.Default.saveLogPath;


            // #############################
            // Advanced
            // #############################

            // Unstable Snapshot checking should only be visible for stable builds.
            if (Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
            {
                lbl_appcastUnstable.Visible = false;
                check_snapshot.Visible = false;
            }

            // Enable snapshot updating
            if (Properties.Settings.Default.checkSnapshot == "Checked")
                check_snapshot.CheckState = CheckState.Checked;

            // Enable GUI DVD Drive detection code
            if (Properties.Settings.Default.drive_detection == "Checked")
                btn_drive_detect.CheckState = CheckState.Checked;

            // Enable / Disable Query editor tab
            if (Properties.Settings.Default.QueryEditorTab == "Checked")
                check_queryEditorTab.CheckState = CheckState.Checked;

            // Preset update notification
            if (Properties.Settings.Default.presetNotification == "Checked")
                check_disablePresetNotification.CheckState = CheckState.Checked;

            // Experimental In-GUI encode status indicator.
            if (Properties.Settings.Default.enocdeStatusInGui == "Checked")
                check_inGuiStatus.CheckState = CheckState.Checked;

            // Enable snapshot updating
            if (Properties.Settings.Default.MainWindowMinimize == "Checked")
                check_mainMinimize.CheckState = CheckState.Checked;

            // x264 step
            drop_x264step.SelectedItem = Properties.Settings.Default.x264cqstep;

            // Use Experimental dvdnav
            if (Properties.Settings.Default.dvdnav == "Checked")
                check_dvdnav.CheckState = CheckState.Checked;
        }

        #region General
        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.updateStatus = check_updateCheck.CheckState.ToString();
        }

        private void check_userDefaultSettings_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.defaultSettings = check_userDefaultSettings.CheckState.ToString();
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

        private void txt_autoNameFormat_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.autoNameFormat = txt_autoNameFormat.Text;
        }

        private void btn_browse_Click(object sender, EventArgs e)
        {
            pathFinder.ShowDialog();
            text_an_path.Text = pathFinder.SelectedPath;
        }

        private void text_an_path_TextChanged(object sender, EventArgs e)
        {
            if (text_an_path.Text == string.Empty)
            {
                Properties.Settings.Default.autoNamePath = "";
                text_an_path.Text = "Click 'Browse' to set the default location";
            }
            else
                Properties.Settings.Default.autoNamePath = text_an_path.Text;
        }
        #endregion

        #region Picture
        private void btn_vlcPath_Click(object sender, EventArgs e)
        {
            openFile_vlc.ShowDialog();
            if (openFile_vlc.FileName != string.Empty)
                txt_vlcPath.Text = openFile_vlc.FileName;
        }
        private void txt_vlcPath_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.VLC_Path = txt_vlcPath.Text;
        }
        #endregion

        #region CLI
        private void check_cli_minimized_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.cli_minimized = check_cli_minimized.CheckState.ToString();
        }

        private void drp_processors_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.Processors = drp_processors.Text;
        }

        private void drp_Priority_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.processPriority = drp_Priority.Text;
        }

        private void cb_logVerboseLvl_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.verboseLevel = cb_logVerboseLvl.SelectedIndex;
        }

        private void check_saveLogWithVideo_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.saveLogWithVideo = check_saveLogWithVideo.CheckState.ToString();
        }
        private void check_logsInSpecifiedLocation_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.saveLogToSpecifiedPath = check_logsInSpecifiedLocation.CheckState.ToString();
        }

        private void btn_saveLog_Click(object sender, EventArgs e)
        {
            pathFinder.SelectedPath = String.Empty;
            pathFinder.ShowDialog();
            if (pathFinder.SelectedPath != string.Empty)
                text_logPath.Text = pathFinder.SelectedPath;
        }

        private void text_logPath_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.saveLogPath = text_logPath.Text;
        }

        #endregion

        #region Advanced
        private void btn_drive_detect_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.drive_detection = btn_drive_detect.CheckState.ToString();
        }

        private void check_mainMinimize_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.MainWindowMinimize = check_mainMinimize.CheckState.ToString();
        }

        private void check_queryEditorTab_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.QueryEditorTab = check_queryEditorTab.CheckState.ToString();
        }

        private void check_disablePresetNotification_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.presetNotification = check_disablePresetNotification.CheckState.ToString();
        }

        private void check_inGuiStatus_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.enocdeStatusInGui = check_inGuiStatus.CheckState.ToString();
        } 

        private void check_snapshot_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.checkSnapshot = check_snapshot.CheckState.ToString();
        }

        private void x264step_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.x264cqstep = drop_x264step.Text;
        }

        private void check_dvdnav_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.dvdnav = check_dvdnav.CheckState.ToString();
        } 
        #endregion

        private void btn_close_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.Save(); // Small hack for Vista. Seems to work fine on XP without this
            this.Close();
        }

    }
}