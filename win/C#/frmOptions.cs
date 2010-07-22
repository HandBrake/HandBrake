/*  frmOptions.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices;

    using Properties;

    /// <summary>
    /// The Options Screen
    /// </summary>
    public partial class frmOptions : Form
    {
        private frmMain mainWindow;

        public frmOptions(frmMain mw)
        {
            InitializeComponent();
            mainWindow = mw;

            IDictionary<string, string> langList = Main.MapLanguages();
            foreach (string item in langList.Keys)
                drop_preferredLang.Items.Add(item);

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (Settings.Default.tooltipEnable)
            {
                check_tooltip.CheckState = CheckState.Checked;
                ToolTip.Active = true;
            }

            // Update Check
            if (Settings.Default.updateStatus)
                check_updateCheck.CheckState = CheckState.Checked;

            // Days between update checks
            switch (Settings.Default.daysBetweenUpdateCheck)
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

            // On Encode Completeion Action
            drp_completeOption.Text = Settings.Default.CompletionOption;

            // Growl.
            if (Settings.Default.growlEncode)
                check_growlEncode.CheckState = CheckState.Checked;

            if (Settings.Default.growlQueue)
                check_GrowlQueue.CheckState = CheckState.Checked;

            // Enable auto naming feature.
            if (Settings.Default.autoNaming)
                check_autoNaming.CheckState = CheckState.Checked;

            // Store the auto name path
            text_an_path.Text = Settings.Default.autoNamePath;
            if (text_an_path.Text == string.Empty)
                text_an_path.Text = "Click 'Browse' to set the default location";

            // Store auto name format
            txt_autoNameFormat.Text = Settings.Default.autoNameFormat;

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            if (Settings.Default.useM4v)
                check_m4v.CheckState = CheckState.Checked;

            // Remove Underscores
            check_removeUnderscores.Checked = Settings.Default.AutoNameRemoveUnderscore;

            // Title case
            check_TitleCase.Checked = Settings.Default.AutoNameTitleCase;

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            txt_vlcPath.Text = Settings.Default.VLC_Path;

            // #############################
            // Audio and Subtitles Tab
            // #############################

            drop_preferredLang.SelectedItem = Settings.Default.NativeLanguage;

            if (Settings.Default.DubAudio)
                radio_dub.Checked = true;
            else
                radio_foreignAndSubs.Checked = true;

            // #############################
            // CLI
            // #############################

            // Priority level for encodes
            drp_Priority.Text = Settings.Default.processPriority;

            check_preventSleep.Checked = Settings.Default.preventSleep; 

            // Log Verbosity Level
            cb_logVerboseLvl.SelectedIndex = Settings.Default.verboseLevel;

            // Save logs in the same directory as encoded files
            if (Settings.Default.saveLogWithVideo)
                check_saveLogWithVideo.CheckState = CheckState.Checked;

            // Save Logs in a specified path
            if (Settings.Default.saveLogToSpecifiedPath)
                check_logsInSpecifiedLocation.CheckState = CheckState.Checked;

            // The saved log path
            text_logPath.Text = Settings.Default.saveLogPath;

            check_clearOldLogs.Checked = Settings.Default.clearOldLogs;

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            if (Settings.Default.trayIconAlerts)
                check_trayStatusAlerts.CheckState = CheckState.Checked;

            // Tray Balloon popups
            if (Settings.Default.MainWindowMinimize)
                check_mainMinimize.CheckState = CheckState.Checked;

            // Enable / Disable Query editor tab
            if (Settings.Default.QueryEditorTab)
                check_queryEditorTab.CheckState = CheckState.Checked;
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;

            // Prompt on inconsistant queries
            check_promptOnUnmatchingQueries.Checked = Settings.Default.PromptOnUnmatchingQueries;

            // Preset update notification
            if (Settings.Default.presetNotification)
                check_disablePresetNotification.CheckState = CheckState.Checked;

            // Show CLI Window
            check_showCliForInGUIEncode.Checked = Settings.Default.showCliForInGuiEncodeStatus;

            // Set the preview count
            drop_previewScanCount.SelectedItem = Settings.Default.previewScanCount.ToString();

            // x264 step
            string step = Settings.Default.x264cqstep.ToString(new CultureInfo("en-US"));
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
            if (Settings.Default.noDvdNav)
                check_dvdnav.CheckState = CheckState.Checked;
        }

        #region General

        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.updateStatus = check_updateCheck.Checked;
        }

        private void drop_updateCheckDays_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_updateCheckDays.SelectedIndex)
            {
                case 0:
                    Settings.Default.daysBetweenUpdateCheck = 1;
                    break;
                case 1:
                    Settings.Default.daysBetweenUpdateCheck = 7;
                    break;
                case 2:
                    Settings.Default.daysBetweenUpdateCheck = 30;
                    break;
            }
        }

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.tooltipEnable = check_tooltip.Checked;
        }

        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.CompletionOption = drp_completeOption.Text;
        }

        private void check_GrowlQueue_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.growlQueue = check_GrowlQueue.Checked;
        }

        private void check_growlEncode_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.growlEncode = check_growlEncode.Checked;
        }

        private void check_autoNaming_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.autoNaming = check_autoNaming.Checked;
        }

        private void txt_autoNameFormat_TextChanged(object sender, EventArgs e)
        {
            Settings.Default.autoNameFormat = txt_autoNameFormat.Text;
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
                Settings.Default.autoNamePath = string.Empty;
                text_an_path.Text = "Click 'Browse' to set the default location";
            }
            else
                Settings.Default.autoNamePath = text_an_path.Text;
        }

        private void check_m4v_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.useM4v = check_m4v.Checked;
        }

        private void check_removeUnderscores_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.AutoNameRemoveUnderscore = check_removeUnderscores.Checked;
        }

        private void check_TitleCase_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.AutoNameTitleCase = check_TitleCase.Checked;
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
            Settings.Default.VLC_Path = txt_vlcPath.Text;
        }

        #endregion

        #region Audio and Subtitles

        private void drop_preferredLang_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.NativeLanguage = drop_preferredLang.SelectedItem.ToString();
        }

        private void radio_dub_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_dub.Checked)
                Settings.Default.DubAudio = true;
        }

        private void radio_foreignAndSubs_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_foreignAndSubs.Checked)
                Settings.Default.DubAudio = false;
        }

        #endregion

        #region CLI

        private void drp_Priority_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.processPriority = drp_Priority.Text;
        }

        private void check_preventSleep_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.preventSleep = check_preventSleep.Checked;
        }

        private void cb_logVerboseLvl_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.verboseLevel = cb_logVerboseLvl.SelectedIndex;
        }

        private void check_saveLogWithVideo_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.saveLogWithVideo = check_saveLogWithVideo.Checked;
        }

        private void check_logsInSpecifiedLocation_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.saveLogToSpecifiedPath = check_logsInSpecifiedLocation.Checked;
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
            Settings.Default.saveLogPath = text_logPath.Text;
        }

        private void btn_viewLogs_Click(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process
                              {
                                  StartInfo = {FileName = windir + @"\explorer.exe", Arguments = logDir}
                              };
            prc.Start();
        }

        private void btn_clearLogs_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you wish to clear the log file directory?", "Clear Logs", 
                                                  MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                Main.ClearLogs();
                MessageBox.Show(this, "HandBrake's Log file directory has been cleared!", "Notice", MessageBoxButtons.OK, 
                                MessageBoxIcon.Information);
            }
        }

        private void check_clearOldLogs_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.clearOldLogs = check_clearOldLogs.Checked;
        }

        #endregion

        #region Advanced

        private void check_mainMinimize_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.MainWindowMinimize = check_mainMinimize.Checked;
            check_trayStatusAlerts.Enabled = check_mainMinimize.Checked;
        }

        private void check_trayStatusAlerts_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.trayIconAlerts = check_trayStatusAlerts.Checked;
        }

        private void check_queryEditorTab_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.QueryEditorTab = check_queryEditorTab.Checked;
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;
        }

        private void check_promptOnUnmatchingQueries_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.PromptOnUnmatchingQueries = check_promptOnUnmatchingQueries.Checked;
        }

        private void check_disablePresetNotification_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.presetNotification = check_disablePresetNotification.Checked;
        }

        private void check_showCliForInGUIEncode_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.showCliForInGuiEncodeStatus = check_showCliForInGUIEncode.Checked;
        }

        private void drop_previewScanCount_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.previewScanCount = int.Parse(drop_previewScanCount.SelectedItem.ToString());
        }

        private void x264step_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_x264step.SelectedIndex)
            {
                case 0:
                    Settings.Default.x264cqstep = 1.0;
                    break;
                case 1:
                    Settings.Default.x264cqstep = 0.50;
                    break;
                case 2:
                    Settings.Default.x264cqstep = 0.25;
                    break;
                case 3:
                    Settings.Default.x264cqstep = 0.20;
                    break;
            }
            mainWindow.setQualityFromSlider();
        }

        private void check_dvdnav_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.noDvdNav = check_dvdnav.Checked;
        }

        #endregion

        private void btn_close_Click(object sender, EventArgs e)
        {
            Settings.Default.Save(); // Small hack for Vista. Seems to work fine on XP without this
            UpdateApplicationServicesSettings();

            this.Close();
        }

        /// <summary>
        /// Initialize App Services
        /// </summary>
        private static void UpdateApplicationServicesSettings()
        {
            string versionId = String.Format("Windows GUI {1} {0}", Settings.Default.hb_build, Settings.Default.hb_version);
            Init.SetupSettings(versionId, Program.InstanceId, Settings.Default.CompletionOption, Settings.Default.noDvdNav,
                               Settings.Default.growlEncode, Settings.Default.growlQueue,
                               Settings.Default.processPriority, Settings.Default.saveLogPath, Settings.Default.saveLogToSpecifiedPath,
                               Settings.Default.saveLogWithVideo, Settings.Default.showCliForInGuiEncodeStatus, Settings.Default.preventSleep);
        }
    }
}