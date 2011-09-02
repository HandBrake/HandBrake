/*  frmOptions.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using Handbrake.Properties;

    /// <summary>
    /// The Options Window
    /// </summary>
    public partial class frmOptions : Form
    {
        private readonly frmMain mainWindow;

        private readonly IUserSettingService userSettingService = ServiceManager.UserSettingService;
        private bool optionsWindowLoading = true;

        public frmOptions(frmMain mw)
        {
            InitializeComponent();
            mainWindow = mw;

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();

            foreach (string selectedItem in this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages))
            {
                // removing wrong keys when a new Language list comes out.
                if (langList.ContainsKey(selectedItem))
                    listBox_selectedLanguages.Items.Add(selectedItem);
            }

            foreach (string item in langList.Keys)
            {
                drop_preferredLang.Items.Add(item);

                // In the available languages should be no "Any" and no selected language.
                if ((item != "Any") && (!this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages).Contains(item)))
                {
                    listBox_availableLanguages.Items.Add(item);
                }
            }

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TooltipEnable))
            {
                check_tooltip.CheckState = CheckState.Checked;
                ToolTip.Active = true;
            }

            // Update Check
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus))
                check_updateCheck.CheckState = CheckState.Checked;

            // Days between update checks
            switch (this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck))
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
            if (userSettingService.GetUserSetting<bool>(HandBrake.ApplicationServices.ASUserSettingConstants.GrowlEncode))
                check_growlEncode.CheckState = CheckState.Checked;

            if (userSettingService.GetUserSetting<bool>(HandBrake.ApplicationServices.ASUserSettingConstants.GrowlQueue))
                check_GrowlQueue.CheckState = CheckState.Checked;

            check_sendFileTo.Checked = this.userSettingService.GetUserSetting<bool>(HandBrake.ApplicationServices.ASUserSettingConstants.SendFile);
            lbl_sendFileTo.Text = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(HandBrake.ApplicationServices.ASUserSettingConstants.SendFileTo));
            txt_SendFileArgs.Text = this.userSettingService.GetUserSetting<string>(HandBrake.ApplicationServices.ASUserSettingConstants.SendFileToArgs);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
                check_autoNaming.CheckState = CheckState.Checked;

            // Store the auto name path
            text_an_path.Text = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath);
            if (text_an_path.Text == string.Empty)
                text_an_path.Text = "Click 'Browse' to set the default location";

            // Store auto name format
            txt_autoNameFormat.Text = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            cb_mp4FileMode.SelectedIndex = this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v);

            // Remove Underscores
            check_removeUnderscores.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore);

            // Title case
            check_TitleCase.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase);

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            txt_vlcPath.Text = this.userSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path);

            // #############################
            // Audio and Subtitles Tab
            // #############################

            drop_preferredLang.SelectedItem = this.userSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage);

            //if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubMode) != 255)
            //{
            //    switch (this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubMode))
            //    {
            //        case 0:
            //            Settings.Default.DubModeAudio = 2;
            //            Settings.Default.DubModeSubtitle = 0;
            //            Settings.Default.DubMode = 255;
            //            break;
            //        case 1:
            //            Settings.Default.DubModeAudio = 4;
            //            Settings.Default.DubModeSubtitle = 0;
            //            Settings.Default.DubMode = 255;
            //            break;
            //        case 2:
            //            Settings.Default.DubModeAudio = 2;
            //            Settings.Default.DubModeSubtitle = 4;
            //            Settings.Default.DubMode = 255;
            //            break;
            //        case 3:
            //            Settings.Default.DubModeAudio = 4;
            //            Settings.Default.DubModeSubtitle = 4;
            //            Settings.Default.DubMode = 255;
            //            break;
            //        default:
            //            Settings.Default.DubMode = 255;
            //            break;
            //    }
            //}

            cb_audioMode.SelectedIndex = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);
            cb_subtitleMode.SelectedIndex = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeSubtitle);

            check_AddOnlyOneAudioPerLanguage.Checked =
                this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage);

            check_AddCCTracks.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseClosedCaption);

            // #############################
            // CLI
            // #############################

            // Priority level for encodes
            drp_Priority.Text = userSettingService.GetUserSetting<string>(ASUserSettingConstants.ProcessPriority);

            check_preventSleep.Checked = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep);

            // Log Verbosity Level
            cb_logVerboseLvl.SelectedIndex = userSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity);

            // Save logs in the same directory as encoded files
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogWithVideo))
                check_saveLogWithVideo.CheckState = CheckState.Checked;

            // Save Logs in a specified path
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogToCopyDirectory))
                check_logsInSpecifiedLocation.CheckState = CheckState.Checked;

            // The saved log path
            text_logPath.Text = userSettingService.GetUserSetting<string>(ASUserSettingConstants.SaveLogCopyDirectory);

            check_clearOldLogs.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TrayIconAlerts))
                check_trayStatusAlerts.CheckState = CheckState.Checked;

            // Tray Balloon popups
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize))
                check_mainMinimize.CheckState = CheckState.Checked;

            // Enable / Disable Query editor tab
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.QueryEditorTab))
                check_queryEditorTab.CheckState = CheckState.Checked;
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;

            // Prompt on inconsistant queries
            check_promptOnUnmatchingQueries.Checked = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PromptOnUnmatchingQueries);

            // Preset update notification
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
                check_disablePresetNotification.CheckState = CheckState.Checked;

            // Show CLI Window
            check_showCliForInGUIEncode.Checked = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.ShowCLI);

            // Set the preview count
            drop_previewScanCount.SelectedItem = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount).ToString();

            // x264 step
            string step = userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step).ToString(new CultureInfo("en-US"));
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
            ud_minTitleLength.Value = this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.MinScanDuration); 

            // Use Experimental dvdnav
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav))
                check_dvdnav.CheckState = CheckState.Checked;

            optionsWindowLoading = false;
        }

        #region General

        private void check_updateCheck_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, check_updateCheck.Checked);
        }

        private void drop_updateCheckDays_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_updateCheckDays.SelectedIndex)
            {
                case 0:
                    this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, 1);
                    break;
                case 1:
                    this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, 7);
                    break;
                case 2:
                    this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, 30);
                    break;
            }
        }

        private void check_tooltip_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.TooltipEnable, check_tooltip.Checked);
        }

        private void drp_completeOption_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.WhenCompleteAction, drp_completeOption.Text);
        }

        private void check_GrowlQueue_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.GrowlQueue, check_GrowlQueue.Checked);
        }

        private void check_growlEncode_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.GrowlEncode, check_growlEncode.Checked);
        }

        private void btn_SendFileToPath_Click(object sender, EventArgs e)
        {
            openExecutable.ShowDialog();
            if (!string.IsNullOrEmpty(openExecutable.FileName))
            {
                this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFileTo, openExecutable.FileName);
                lbl_sendFileTo.Text = Path.GetFileNameWithoutExtension(openExecutable.FileName);
            }
        }

        private void check_sendFileTo_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFile, check_sendFileTo.Checked);
        }

        private void txt_SendFileArgs_TextChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFileToArgs, txt_SendFileArgs.Text);
        }

        #endregion

        #region Output File
        private void check_autoNaming_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNaming, check_autoNaming.Checked);
        }

        private void txt_autoNameFormat_TextChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameFormat, txt_autoNameFormat.Text);
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
                this.userSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, string.Empty);
                text_an_path.Text = "Click 'Browse' to set the default location";
            }
            else
                this.userSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, text_an_path.Text);

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
            this.userSettingService.SetUserSetting(UserSettingConstants.UseM4v, cb_mp4FileMode.SelectedIndex);
        }

        private void check_removeUnderscores_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameRemoveUnderscore, check_removeUnderscores.Checked);
        }

        private void check_TitleCase_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameTitleCase, check_TitleCase.Checked);
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
            this.userSettingService.SetUserSetting(UserSettingConstants.VLC_Path, txt_vlcPath.Text);
        }

        #endregion

        #region Audio and Subtitles

        private void drop_preferredLang_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.NativeLanguage, drop_preferredLang.SelectedItem.ToString());

            if (this.userSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage) == "Any")
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

                    StringCollection languages = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
                    if (languages.Contains(item))
                    {
                        languages.Remove(item);
                        this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, languages);
                    }
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

                StringCollection languages = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
                languages.AddRange(movedItems);
                this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, languages);
      
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

                    StringCollection languages = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
                    if (languages.Contains(item))
                    {
                        languages.Remove(item);
                        this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, languages);
                    }
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
                        StringCollection languages = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
                        languages.Remove(lvitem);
                        languages.Insert(ilevel - 1, lvitem);
                        this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, languages);
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
                        StringCollection languages = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
                        languages.Remove(lvitem);
                        languages.Insert(ilevel + 1, lvitem);
                        this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, languages);
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
            this.userSettingService.SetUserSetting(UserSettingConstants.AddOnlyOneAudioPerLanguage, check_AddOnlyOneAudioPerLanguage.Checked);
        }

        private void check_AddCCTracks_CheckedChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.UseClosedCaption, check_AddCCTracks.Checked);
        }

        private void cb_audioMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.DubModeAudio, cb_audioMode.SelectedIndex);
        }

        private void cb_subtitleMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.userSettingService.SetUserSetting(UserSettingConstants.DubModeSubtitle, cb_subtitleMode.SelectedIndex);
        }

        #endregion

        #region CLI

        private void drp_Priority_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.ProcessPriority, drp_Priority.Text);
        }

        private void check_preventSleep_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.PreventSleep, check_preventSleep.Checked);
        }

        private void cb_logVerboseLvl_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.Verbosity, cb_logVerboseLvl.SelectedIndex);
        }

        private void check_saveLogWithVideo_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogWithVideo, check_saveLogWithVideo.Checked);
        }

        private void check_logsInSpecifiedLocation_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogToCopyDirectory, check_logsInSpecifiedLocation.Checked);
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
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogCopyDirectory, text_logPath.Text);
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
            userSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, check_clearOldLogs.Checked);
        }

        #endregion

        #region Advanced

        private void check_mainMinimize_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, check_mainMinimize.Checked);
            check_trayStatusAlerts.Enabled = check_mainMinimize.Checked;
        }

        private void check_trayStatusAlerts_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.TrayIconAlerts, check_trayStatusAlerts.Checked);
        }

        private void check_queryEditorTab_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.QueryEditorTab, check_queryEditorTab.Checked);
            check_promptOnUnmatchingQueries.Enabled = check_queryEditorTab.Checked;
        }

        private void check_promptOnUnmatchingQueries_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.PromptOnUnmatchingQueries, check_promptOnUnmatchingQueries.Checked);
        }

        private void check_disablePresetNotification_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.PresetNotification, check_disablePresetNotification.Checked);
        }

        private void check_showCliForInGUIEncode_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.ShowCLI, check_showCliForInGUIEncode.Checked);
        }

        private void drop_previewScanCount_SelectedIndexChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.PreviewScanCount, int.Parse(drop_previewScanCount.SelectedItem.ToString()));
        }

        private void x264step_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_x264step.SelectedIndex)
            {
                case 0:
                    userSettingService.SetUserSetting(ASUserSettingConstants.X264Step, 1.0);
                    break;
                case 1:
                    userSettingService.SetUserSetting(ASUserSettingConstants.X264Step, 0.5);
                    break;
                case 2:
                    userSettingService.SetUserSetting(ASUserSettingConstants.X264Step, 0.25);
                    break;
                case 3:
                    userSettingService.SetUserSetting(ASUserSettingConstants.X264Step, 0.2);
                    break;
            }
            mainWindow.setQualityFromSlider();
        }

        private void check_dvdnav_CheckedChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(ASUserSettingConstants.DisableLibDvdNav, check_dvdnav.Checked);
        }

        private void ud_minTitleLength_ValueChanged(object sender, EventArgs e)
        {
            int value;
            if (int.TryParse(ud_minTitleLength.Value.ToString(), out value))
            {
                this.userSettingService.SetUserSetting(ASUserSettingConstants.MinScanDuration, value);
            }
        }

        #endregion

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}