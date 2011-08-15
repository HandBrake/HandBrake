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
    using System.IO;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using Handbrake.Properties;

    /// <summary>
    /// The Options Window
    /// </summary>
    public partial class frmOptions : Form
    {
        private readonly frmMain mainWindow;

        private readonly IUserSettingService userSettingService = new UserSettingService();
        private bool optionsWindowLoading = true;

        public frmOptions(frmMain mw)
        {
            InitializeComponent();
            mainWindow = mw;

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();

            foreach (string selectedItem in Properties.Settings.Default.SelectedLanguages)
            {
                // removing wrong keys when a new Language list comes out.
                if (langList.ContainsKey(selectedItem))
                    listBox_selectedLanguages.Items.Add(selectedItem);
            }

            foreach (string item in langList.Keys)
            {
                drop_preferredLang.Items.Add(item);

                // In the available languages should be no "Any" and no selected language.
                if ((item != "Any") && (!Properties.Settings.Default.SelectedLanguages.Contains(item)))
                {
                    listBox_availableLanguages.Items.Add(item);
                }
            }

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (Properties.Settings.Default.tooltipEnable)
            {
                check_tooltip.CheckState = CheckState.Checked;
                ToolTip.Active = true;
            }

            // Update Check
            if (Properties.Settings.Default.updateStatus)
                check_updateCheck.CheckState = CheckState.Checked;

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

            // On Encode Completeion Action
            drp_completeOption.Text = userSettingService.GetUserSetting<string>("WhenCompleteAction");

            // Growl.
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlEncode))
                check_growlEncode.CheckState = CheckState.Checked;

            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlQueue))
                check_GrowlQueue.CheckState = CheckState.Checked;

            check_sendFileTo.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile);
            lbl_sendFileTo.Text = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo));
            txt_SendFileArgs.Text = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            if (Properties.Settings.Default.autoNaming)
                check_autoNaming.CheckState = CheckState.Checked;

            // Store the auto name path
            text_an_path.Text = Properties.Settings.Default.autoNamePath;
            if (text_an_path.Text == string.Empty)
                text_an_path.Text = "Click 'Browse' to set the default location";

            // Store auto name format
            txt_autoNameFormat.Text = Properties.Settings.Default.autoNameFormat;

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            cb_mp4FileMode.SelectedIndex = Properties.Settings.Default.useM4v;

            // Remove Underscores
            check_removeUnderscores.Checked = Properties.Settings.Default.AutoNameRemoveUnderscore;

            // Title case
            check_TitleCase.Checked = Properties.Settings.Default.AutoNameTitleCase;

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            txt_vlcPath.Text = Properties.Settings.Default.VLC_Path;

            // #############################
            // Audio and Subtitles Tab
            // #############################

            drop_preferredLang.SelectedItem = Properties.Settings.Default.NativeLanguage;

            if (Settings.Default.DubMode != 255)
            {
                switch (Settings.Default.DubMode)
                {
                    case 0:
                        Settings.Default.DubModeAudio = 2;
                        Settings.Default.DubModeSubtitle = 0;
                        Settings.Default.DubMode = 255;
                        break;
                    case 1:
                        Settings.Default.DubModeAudio = 4;
                        Settings.Default.DubModeSubtitle = 0;
                        Settings.Default.DubMode = 255;
                        break;
                    case 2:
                        Settings.Default.DubModeAudio = 2;
                        Settings.Default.DubModeSubtitle = 4;
                        Settings.Default.DubMode = 255;
                        break;
                    case 3:
                        Settings.Default.DubModeAudio = 4;
                        Settings.Default.DubModeSubtitle = 4;
                        Settings.Default.DubMode = 255;
                        break;
                    default:
                        Settings.Default.DubMode = 255;
                        break;
                }
            }

            cb_audioMode.SelectedIndex = Settings.Default.DubModeAudio;
            cb_subtitleMode.SelectedIndex = Settings.Default.DubModeSubtitle;

            check_AddOnlyOneAudioPerLanguage.Checked = Properties.Settings.Default.addOnlyOneAudioPerLanguage;

            check_AddCCTracks.Checked = Properties.Settings.Default.useClosedCaption;

            // #############################
            // CLI
            // #############################

            // Priority level for encodes
            drp_Priority.Text = userSettingService.GetUserSetting<string>(UserSettingConstants.ProcessPriority);

            check_preventSleep.Checked = userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep);

            // Log Verbosity Level
            cb_logVerboseLvl.SelectedIndex = userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity);

            // Save logs in the same directory as encoded files
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo))
                check_saveLogWithVideo.CheckState = CheckState.Checked;

            // Save Logs in a specified path
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory))
                check_logsInSpecifiedLocation.CheckState = CheckState.Checked;

            // The saved log path
            text_logPath.Text = userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory);

            check_clearOldLogs.Checked = Properties.Settings.Default.clearOldLogs;

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            if (Properties.Settings.Default.trayIconAlerts)
                check_trayStatusAlerts.CheckState = CheckState.Checked;

            // Tray Balloon popups
            if (Properties.Settings.Default.MainWindowMinimize)
                check_mainMinimize.CheckState = CheckState.Checked;

            // Enable / Disable Query editor tab
            if (Properties.Settings.Default.QueryEditorTab)
                check_queryEditorTab.CheckState = CheckState.Checked;
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;

            // Prompt on inconsistant queries
            check_promptOnUnmatchingQueries.Checked = Properties.Settings.Default.PromptOnUnmatchingQueries;

            // Preset update notification
            if (Properties.Settings.Default.presetNotification)
                check_disablePresetNotification.CheckState = CheckState.Checked;

            // Show CLI Window
            check_showCliForInGUIEncode.Checked = userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowCLI);

            // Set the preview count
            drop_previewScanCount.SelectedItem = Properties.Settings.Default.previewScanCount.ToString();

            // x264 step
            string step = userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step).ToString(new CultureInfo("en-US"));
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

            // Min Title Length
            ud_minTitleLength.Value = this.userSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration); 

            // Use Experimental dvdnav
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav))
                check_dvdnav.CheckState = CheckState.Checked;

            optionsWindowLoading = false;
        }

        #region General

        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.updateStatus = check_updateCheck.Checked;
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

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.tooltipEnable = check_tooltip.Checked;
        }

        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, drp_completeOption.Text);
        }

        private void check_GrowlQueue_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.GrowlQueue, check_GrowlQueue.Checked);
        }

        private void check_growlEncode_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.GrowlEncode, check_growlEncode.Checked);
        }

        private void btn_SendFileToPath_Click(object sender, EventArgs e)
        {
            openExecutable.ShowDialog();
            if (!string.IsNullOrEmpty(openExecutable.FileName))
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.SendFileTo, openExecutable.FileName);
                lbl_sendFileTo.Text = Path.GetFileNameWithoutExtension(openExecutable.FileName);
            }
        }

        private void check_sendFileTo_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFile, check_sendFileTo.Checked);
        }

        private void txt_SendFileArgs_TextChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileToArgs, txt_SendFileArgs.Text);
        }

        #endregion

        #region Output File
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
                Properties.Settings.Default.autoNamePath = string.Empty;
                text_an_path.Text = "Click 'Browse' to set the default location";
            }
            else
                Properties.Settings.Default.autoNamePath = text_an_path.Text;

            if (text_an_path.Text.ToLower() == "{source_path}" && !optionsWindowLoading)
            {
                MessageBox.Show(
                    "Be careful with this feature. Make sure you can write to the same folder as the source! \n\n If you are encoding from a DVD, do not use this feature as HandBrake will not be able to write to the DVD!",
                    "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }

            if (text_an_path.Text.ToLower().Contains("{source_path}") && !text_an_path.Text.StartsWith("{source_path}"))
            {
                MessageBox.Show("Note you can not use the {source_path} within a path. {source_path} is the full source file path.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void cb_mp4FileMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.useM4v = cb_mp4FileMode.SelectedIndex;
        }

        private void check_removeUnderscores_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.AutoNameRemoveUnderscore = check_removeUnderscores.Checked;
        }

        private void check_TitleCase_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.AutoNameTitleCase = check_TitleCase.Checked;
        }

        #endregion

        #region Picture

        private void btn_vlcPath_Click(object sender, EventArgs e)
        {
            openExecutable.ShowDialog();
            if (openExecutable.FileName != string.Empty)
                txt_vlcPath.Text = openExecutable.FileName;
        }

        private void txt_vlcPath_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.VLC_Path = txt_vlcPath.Text;
        }

        #endregion

        #region Audio and Subtitles

        private void drop_preferredLang_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.NativeLanguage = drop_preferredLang.SelectedItem.ToString();

            if (Settings.Default.NativeLanguage == "Any")
            {
                cb_audioMode.Enabled = false;
                cb_subtitleMode.Enabled = false;
                check_AddOnlyOneAudioPerLanguage.Enabled = false;

                audioSelectionPanel.Enabled = false;
            }
            else
            {
                cb_audioMode.Enabled = true;
                cb_subtitleMode.Enabled = true;
                check_AddOnlyOneAudioPerLanguage.Enabled = true;

                audioSelectionPanel.Enabled = true;
            }
        }

        private void button_removeLanguage_Click(object sender, EventArgs e)
        {
            if (listBox_selectedLanguages.SelectedItems.Count > 0)
            {
                String[] movedItems = new String[listBox_selectedLanguages.SelectedItems.Count];

                listBox_selectedLanguages.SelectedItems.CopyTo(movedItems, 0);

                listBox_availableLanguages.Items.AddRange(movedItems);

                listBox_selectedLanguages.SelectedItems.Clear();
                foreach (string item in movedItems)
                {
                    listBox_selectedLanguages.Items.Remove(item);

                    if (Properties.Settings.Default.SelectedLanguages.Contains(item))
                        Properties.Settings.Default.SelectedLanguages.Remove(item);
                }
            }
        }

        private void button_addLanguage_Click(object sender, EventArgs e)
        {
            if (listBox_availableLanguages.SelectedItems.Count > 0)
            {
                String[] movedItems = new String[listBox_availableLanguages.SelectedItems.Count];

                listBox_availableLanguages.SelectedItems.CopyTo(movedItems, 0);

                listBox_selectedLanguages.Items.AddRange(movedItems);
                Properties.Settings.Default.SelectedLanguages.AddRange(movedItems);

                listBox_availableLanguages.SelectedItems.Clear();
                foreach (string item in movedItems)
                {
                    listBox_availableLanguages.Items.Remove(item);
                }
            }
        }

        private void button_clearLanguage_Click(object sender, EventArgs e)
        {
            if (listBox_selectedLanguages.Items.Count > 0)
            {
                String[] movedItems = new String[listBox_selectedLanguages.Items.Count];

                listBox_selectedLanguages.Items.CopyTo(movedItems, 0);

                listBox_availableLanguages.Items.AddRange(movedItems);

                foreach (string item in movedItems)
                {
                    listBox_selectedLanguages.Items.Remove(item);

                    if (Properties.Settings.Default.SelectedLanguages.Contains(item))
                        Properties.Settings.Default.SelectedLanguages.Remove(item);
                }
            }
        }

        private void button_moveLanguageUp_Click(object sender, EventArgs e)
        {
            int ilevel = 0;
            if (listBox_selectedLanguages.SelectedItems.Count > 0)
            {
                ListBox.SelectedIndexCollection selectedItems = listBox_selectedLanguages.SelectedIndices;
                int[] index_selectedItems = new int[selectedItems.Count];

                for (int i = 0; i < selectedItems.Count; i++)
                    index_selectedItems[i] = selectedItems[i];

                for (int i = 0; i < index_selectedItems.Length; i++)
                {
                    ilevel = index_selectedItems[i];

                    if ((ilevel - 1 >= 0) && (ilevel - 1 >= i))
                    {
                        String lvitem = (String)listBox_selectedLanguages.Items[ilevel];
                        listBox_selectedLanguages.Items.Remove(lvitem);
                        listBox_selectedLanguages.Items.Insert(ilevel - 1, lvitem);
                        listBox_selectedLanguages.SetSelected(ilevel - 1, true);

                        // Do the same on the Property.
                        Properties.Settings.Default.SelectedLanguages.Remove(lvitem);
                        Properties.Settings.Default.SelectedLanguages.Insert(ilevel - 1, lvitem);
                    }
                }
            }
        }

        private void button_moveLanguageDown_Click(object sender, EventArgs e)
        {
            int ilevel = 0;
            if (listBox_selectedLanguages.SelectedItems.Count > 0)
            {
                ListBox.SelectedIndexCollection selectedItems = listBox_selectedLanguages.SelectedIndices;
                int[] index_selectedItems = new int[selectedItems.Count];

                for (int i = 0; i < selectedItems.Count; i++)
                    index_selectedItems[i] = selectedItems[i];

                for (int i = index_selectedItems.Length - 1; i >= 0; i--)
                {
                    ilevel = index_selectedItems[i];

                    if ((ilevel + 1 >= 0) && (ilevel + 1 <= listBox_selectedLanguages.Items.Count - (index_selectedItems.Length - i)))
                    {
                        String lvitem = (String)listBox_selectedLanguages.Items[ilevel];
                        listBox_selectedLanguages.Items.Remove(lvitem);
                        listBox_selectedLanguages.Items.Insert(ilevel + 1, lvitem);
                        listBox_selectedLanguages.SetSelected(ilevel + 1, true);

                        // Do the same on the Property.
                        Properties.Settings.Default.SelectedLanguages.Remove(lvitem);
                        Properties.Settings.Default.SelectedLanguages.Insert(ilevel + 1, lvitem);
                    }
                }
            }
        }

        private void listBox_selectedLanguages_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.button_removeLanguage_Click(this, new EventArgs());
        }

        private void listBox_availableLanguages_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.button_addLanguage_Click(this, new EventArgs());
        }

        private void check_AddOnlyOneAudioPerLanguage_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.addOnlyOneAudioPerLanguage = check_AddOnlyOneAudioPerLanguage.Checked;
        }

        private void check_AddCCTracks_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Default.useClosedCaption = check_AddCCTracks.Checked;
        }

        private void cb_audioMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.DubModeAudio = cb_audioMode.SelectedIndex;
        }

        private void cb_subtitleMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            Settings.Default.DubModeSubtitle = cb_subtitleMode.SelectedIndex;
        }

        #endregion

        #region CLI

        private void drp_Priority_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.ProcessPriority, drp_Priority.Text);
        }

        private void check_preventSleep_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.PreventSleep, check_preventSleep.Checked);
        }

        private void cb_logVerboseLvl_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.Verbosity, cb_logVerboseLvl.SelectedIndex);
        }

        private void check_saveLogWithVideo_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogWithVideo, check_saveLogWithVideo.Checked);
        }

        private void check_logsInSpecifiedLocation_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogToCopyDirectory, check_logsInSpecifiedLocation.Checked);
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
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogCopyDirectory, text_logPath.Text);
        }

        private void btn_viewLogs_Click(object sender, EventArgs e)
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process();
            prc.StartInfo.FileName = windir + @"\explorer.exe";
            prc.StartInfo.Arguments = logDir;
            prc.Start();
        }

        private void btn_clearLogs_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you wish to clear the log file directory?", "Clear Logs",
                                                  MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                GeneralUtilities.ClearLogFiles(0);
                MessageBox.Show(this, "HandBrake's Log file directory has been cleared!", "Notice", MessageBoxButtons.OK,
                                MessageBoxIcon.Information);
            }
        }

        private void check_clearOldLogs_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.clearOldLogs = check_clearOldLogs.Checked;
        }

        #endregion

        #region Advanced

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

        private void check_showCliForInGUIEncode_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.ShowCLI, check_showCliForInGUIEncode.Checked);
        }

        private void drop_previewScanCount_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.previewScanCount = int.Parse(drop_previewScanCount.SelectedItem.ToString());
        }

        private void x264step_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_x264step.SelectedIndex)
            {
                case 0:
                    userSettingService.SetUserSetting(UserSettingConstants.X264Step, 1.0);
                    break;
                case 1:
                    userSettingService.SetUserSetting(UserSettingConstants.X264Step, 0.5);
                    break;
                case 2:
                    userSettingService.SetUserSetting(UserSettingConstants.X264Step, 0.25);
                    break;
                case 3:
                    userSettingService.SetUserSetting(UserSettingConstants.X264Step, 0.2);
                    break;
            }
            mainWindow.setQualityFromSlider();
        }

        private void check_dvdnav_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.DisableLibDvdNav, check_dvdnav.Checked);
        }

        private void ud_minTitleLength_ValueChanged(object sender, EventArgs e)
        {
            int value;
            if (int.TryParse(ud_minTitleLength.Value.ToString(), out value))
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.MinScanDuration, value);
            }
        }

        #endregion

        private void btn_close_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.Save();
            this.Close();
        }
    }
}