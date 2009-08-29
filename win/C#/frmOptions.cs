/*  frmOptions.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows.Forms;
using Handbrake.Functions;

namespace Handbrake
{
    public partial class frmOptions : Form
    {
        private frmMain mainWindow;

        public frmOptions(frmMain mw)
        {
            InitializeComponent();
            mainWindow = mw;

            IDictionary<string, string> langList = Main.mapLanguages();
            foreach (string item in langList.Keys)
                drop_preferredLang.Items.Add(item);

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (Properties.Settings.Default.tooltipEnable)
            {
                check_tooltip.CheckState = CheckState.Checked;
                ToolTip.Active = true;
            }

            // Setup Widgets to match settings.
            if (Properties.Settings.Default.updateStatus)
                check_updateCheck.CheckState = CheckState.Checked;

            // enable loading of default user settings.
            if (Properties.Settings.Default.defaultSettings)
                check_userDefaultSettings.CheckState = CheckState.Checked;

            // On Encode Completeion Action
            drp_completeOption.Text = Properties.Settings.Default.CompletionOption;

            // Growl.
            if (Properties.Settings.Default.growlEncode)
                check_growlEncode.CheckState = CheckState.Checked;

            if (Properties.Settings.Default.growlQueue)
                check_GrowlQueue.CheckState = CheckState.Checked;

            // Enable auto naming feature.
            if (Properties.Settings.Default.autoNaming)
                check_autoNaming.CheckState = CheckState.Checked;

            // Store the auto name path
            text_an_path.Text = Properties.Settings.Default.autoNamePath;
            if (text_an_path.Text == string.Empty)
                text_an_path.Text = "Click 'Browse' to set the default location";

            // Store auto name format
            txt_autoNameFormat.Text = Properties.Settings.Default.autoNameFormat;

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            if (Properties.Settings.Default.useM4v)
                check_m4v.CheckState = CheckState.Checked;

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            txt_vlcPath.Text = Properties.Settings.Default.VLC_Path;

            // #############################
            // Audio and Subtitles Tab
            // #############################

            drop_preferredLang.SelectedItem = Properties.Settings.Default.NativeLanguage;

            if (Properties.Settings.Default.DubAudio)
                radio_dub.Checked = true;
            else
                radio_foreignAndSubs.Checked = true;


            // #############################
            // CLI
            // #############################

            // Enable Start CLI minimized
            if (Properties.Settings.Default.cli_minimized)
                check_cli_minimized.CheckState = CheckState.Checked;

            // Number of processor cores
            drp_processors.Text = Properties.Settings.Default.Processors;

            // Priority level for encodes
            drp_Priority.Text = Properties.Settings.Default.processPriority;

            // Log Verbosity Level
            cb_logVerboseLvl.SelectedIndex = Properties.Settings.Default.verboseLevel;


            // Save logs in the same directory as encoded files
            if (Properties.Settings.Default.saveLogWithVideo)
                check_saveLogWithVideo.CheckState = CheckState.Checked;

            // Save Logs in a specified path
            if (Properties.Settings.Default.saveLogToSpecifiedPath)
                check_logsInSpecifiedLocation.CheckState = CheckState.Checked;

            // The saved log path
            text_logPath.Text = Properties.Settings.Default.saveLogPath;


            // #############################
            // Advanced
            // #############################

            // Enable GUI DVD Drive detection code
            if (Properties.Settings.Default.drive_detection)
                btn_drive_detect.CheckState = CheckState.Checked;

            // Minimise to Tray
            if (Properties.Settings.Default.trayIconAlerts)
                check_trayStatusAlerts.CheckState = CheckState.Checked;

            // Tray Balloon popups
            if (Properties.Settings.Default.MainWindowMinimize)
                check_mainMinimize.CheckState = CheckState.Checked;

            // Enable / Disable Query editor tab
            if (Properties.Settings.Default.QueryEditorTab)
                check_queryEditorTab.CheckState = CheckState.Checked;

            // Prompt on inconsistant queries
            check_promptOnUnmatchingQueries.Checked = Properties.Settings.Default.PromptOnUnmatchingQueries;

            // Preset update notification
            if (Properties.Settings.Default.presetNotification)
                check_disablePresetNotification.CheckState = CheckState.Checked;

            // Experimental In-GUI encode status indicator.
            if (Properties.Settings.Default.enocdeStatusInGui)
                check_inGuiStatus.CheckState = CheckState.Checked;

            // Days between update checks
            switch (Properties.Settings.Default.daysBetweenUpdateCheck)
            {
                case 1:
                    drop_updateCheckDays.SelectedIndex = 0;
                    break;
                case 7:
                    drop_updateCheckDays.SelectedIndex = 1;
                    break;
                case 30:
                    drop_updateCheckDays.SelectedIndex = 2;
                    break;
            }

            // x264 step
            string step = Properties.Settings.Default.x264cqstep.ToString(new CultureInfo("en-US"));
            switch (step)
            {
                case "1":
                    drop_x264step.SelectedIndex = 0;
                    break;
                case "0.5":
                    drop_x264step.SelectedIndex = 1;
                    break;
                case "0.25":
                    drop_x264step.SelectedIndex = 2;
                    break;
                case "0.2":
                    drop_x264step.SelectedIndex = 3;
                    break;
            }

            // Use Experimental dvdnav
            if (Properties.Settings.Default.dvdnav)
                check_dvdnav.CheckState = CheckState.Checked;

            // #############################
            // Debug
            // #############################
            if (Properties.Settings.Default.disableResCalc)
                check_disableResCalc.Checked = true;
        }

        #region General
        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.updateStatus = check_updateCheck.Checked;
        }

        private void check_userDefaultSettings_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.defaultSettings = check_userDefaultSettings.Checked;
        }

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.tooltipEnable = check_tooltip.Checked;
        }
        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.CompletionOption = drp_completeOption.Text;
        }

        private void check_GrowlQueue_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.growlQueue = check_GrowlQueue.Checked;
        }

        private void check_growlEncode_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.growlEncode = check_growlEncode.Checked;
        }

        private void check_autoNaming_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.autoNaming = check_autoNaming.Checked;
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

        private void check_m4v_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.useM4v = check_m4v.Checked;
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

        #region Audio and Subtitles
        private void drop_preferredLang_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.NativeLanguage = drop_preferredLang.SelectedItem.ToString();
        }
        private void radio_dub_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_dub.Checked)
                Properties.Settings.Default.DubAudio = true;
        }
        private void radio_foreignAndSubs_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_foreignAndSubs.Checked)
                Properties.Settings.Default.DubAudio = false;
        }       
        #endregion

        #region CLI
        private void check_cli_minimized_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.cli_minimized = check_cli_minimized.Checked;
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
            Properties.Settings.Default.saveLogWithVideo = check_saveLogWithVideo.Checked;
        }

        private void check_logsInSpecifiedLocation_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.saveLogToSpecifiedPath = check_logsInSpecifiedLocation.Checked;
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

        private void btn_viewLogs_Click(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            System.Diagnostics.Process prc = new System.Diagnostics.Process();
            prc.StartInfo.FileName = windir + @"\explorer.exe";
            prc.StartInfo.Arguments = logDir;
            prc.Start();
        }

        private void btn_clearLogs_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you wish to clear the log file directory?", "Clear Logs", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                Main.clearLogs();
                MessageBox.Show(this, "HandBrake's Log file directory has been cleared!", "Notice", MessageBoxButtons.OK,
                                MessageBoxIcon.Information);
            }
        }
        #endregion

        #region Advanced
        private void btn_drive_detect_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.drive_detection = btn_drive_detect.Checked;
        }

        private void check_mainMinimize_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.MainWindowMinimize = check_mainMinimize.Checked;
            check_trayStatusAlerts.Enabled = check_mainMinimize.Checked;
        }

        private void check_trayStatusAlerts_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.trayIconAlerts = check_trayStatusAlerts.Checked;
        }

        private void check_queryEditorTab_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.QueryEditorTab = check_queryEditorTab.Checked;
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;
        }

        private void check_promptOnUnmatchingQueries_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.PromptOnUnmatchingQueries = check_promptOnUnmatchingQueries.Checked;
        }

        private void check_disablePresetNotification_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.presetNotification = check_disablePresetNotification.Checked;
        }

        private void check_inGuiStatus_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.enocdeStatusInGui = check_inGuiStatus.Checked;
        }

        private void drop_updateCheckDays_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_updateCheckDays.SelectedIndex)
            {
                case 0:
                    Properties.Settings.Default.daysBetweenUpdateCheck = 1;
                    break;
                case 1:
                    Properties.Settings.Default.daysBetweenUpdateCheck = 7;
                    break;
                case 2:
                    Properties.Settings.Default.daysBetweenUpdateCheck = 30;
                    break;
            }
        }

        private void x264step_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_x264step.SelectedIndex)
            {
                case 0:
                    Properties.Settings.Default.x264cqstep = 1.0;
                    break;
                case 1:
                    Properties.Settings.Default.x264cqstep = 0.50;
                    break;
                case 2:
                    Properties.Settings.Default.x264cqstep = 0.25;
                    break;
                case 3:
                    Properties.Settings.Default.x264cqstep = 0.20;
                    break;
            }
            mainWindow.setQualityFromSlider();
        }

        private void check_dvdnav_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.dvdnav = check_dvdnav.Checked;
        }
        #endregion

        #region Debug
        private void check_disableResCalc_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.disableResCalc = check_disableResCalc.Checked;
        }
        #endregion

        private void btn_close_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.Save(); // Small hack for Vista. Seems to work fine on XP without this
            this.Close();
        }
    }
}