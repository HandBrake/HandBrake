/*  frmMain.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Windows.Forms;
    using EncodeQueue;
    using Functions;
    using Model;
    using Parsing;
    using Presets;

    public partial class frmMain : Form
    {
        // Objects which may be used by one or more other objects *************
        private Queue encodeQueue = new Queue();
        private PresetsHandler presetHandler = new PresetsHandler();

        // Globals: Mainly used for tracking. *********************************
        public Title selectedTitle;
        private frmQueue queueWindow;
        private frmPreview qtpreview;
        private frmActivityWindow ActivityWindow;
        private Form splash;
        public string sourcePath;
        private string lastAction;
        private SourceType selectedSourceType;
        private string dvdDrivePath;
        private string dvdDriveLabel;
        private Preset CurrentlySelectedPreset;
        private DVD currentSource;

        // Delegates **********************************************************
        private delegate void UpdateWindowHandler();

        // Applicaiton Startup ************************************************

        #region Application Startup

        public frmMain()
        {
            // Load and setup the splash screen in this thread
            splash = new frmSplashScreen();
            splash.Show(this);
            Label lblStatus = new Label {Size = new Size(150, 20), Location = new Point(182, 102)};
            splash.Controls.Add(lblStatus);

            InitializeComponent();

            // Update the users config file with the CLI version data.
            lblStatus.Text = "Setting Version Data ...";
            Application.DoEvents();
            Main.SetCliVersionData();

            // Show the form, but leave disabled until preloading is complete then show the main form
            this.Enabled = false;
            this.Show();
            Application.DoEvents(); // Forces frmMain to draw

            // Check for new versions, if update checking is enabled
            if (Properties.Settings.Default.updateStatus)
            {
                DateTime now = DateTime.Now;
                DateTime lastCheck = Properties.Settings.Default.lastUpdateCheckDate;
                TimeSpan elapsed = now.Subtract(lastCheck);
                if (elapsed.TotalDays > Properties.Settings.Default.daysBetweenUpdateCheck)
                {
                    lblStatus.Text = "Checking for updates ...";
                    Application.DoEvents();

                    Main.BeginCheckForUpdates(new AsyncCallback(UpdateCheckDone), false);
                }
            }

            // Clear the log files in the background
            if (Properties.Settings.Default.clearOldLogs)
            {
                lblStatus.Text = "Clearing Old Log Files ...";
                Application.DoEvents();
                Thread clearLog = new Thread(Main.ClearOldLogs);
                clearLog.Start();
            }

            // Setup the GUI components
            lblStatus.Text = "Setting up the GUI ...";
            Application.DoEvents();
            LoadPresetPanel(); // Load the Preset Panel
            treeView_presets.ExpandAll();
            lbl_encode.Text = string.Empty;
            drop_mode.SelectedIndex = 0;
            queueWindow = new frmQueue(encodeQueue, this); // Prepare the Queue
            if (!Properties.Settings.Default.QueryEditorTab)
                tabs_panel.TabPages.RemoveAt(7); // Remove the query editor tab if the user does not want it enabled.

            // Load the user's default settings or Normal Preset
            if (Properties.Settings.Default.defaultPreset != string.Empty)
            {
                if (presetHandler.GetPreset(Properties.Settings.Default.defaultPreset) != null)
                {
                    string query = presetHandler.GetPreset(Properties.Settings.Default.defaultPreset).Query;
                    bool loadPictureSettings =
                        presetHandler.GetPreset(Properties.Settings.Default.defaultPreset).PictureSettings;

                    if (query != null)
                    {
                        x264Panel.Reset2Defaults();

                        QueryParser presetQuery = QueryParser.Parse(query);
                        PresetLoader.LoadPreset(this, presetQuery, Properties.Settings.Default.defaultPreset, 
                                                loadPictureSettings);

                        x264Panel.X264_StandardizeOptString();
                        x264Panel.X264_SetCurrentSettingsInPanel();
                    }
                }
                else
                    loadNormalPreset();
            }
            else
                loadNormalPreset();

            // Enabled GUI tooltip's if Required
            if (Properties.Settings.Default.tooltipEnable)
                ToolTip.Active = true;

            // Register with Growl (if not using Growl for the encoding completion action, this wont hurt anything)
            GrowlCommunicator.Register();

            // Finished Loading
            lblStatus.Text = "Loading Complete!";
            Application.DoEvents();
            splash.Close();
            splash.Dispose();
            this.Enabled = true;

            // Event Handlers and Queue Recovery
            events();
            queueRecovery();
        }

        private void UpdateCheckDone(IAsyncResult result)
        {
            if (InvokeRequired)
            {
                Invoke(new MethodInvoker(() => UpdateCheckDone(result)));
                return;
            }

            UpdateCheckInformation info;

            try
            {
                info = Main.EndCheckForUpdates(result);

                if (info.NewVersionAvailable)
                {
                    frmUpdater updateWindow = new frmUpdater(info.BuildInformation);
                    updateWindow.ShowDialog();
                }
            }
            catch (Exception ex)
            {
                if ((bool) result.AsyncState)
                    MessageBox.Show(
                        "Unable to check for updates, Please try again later.\n\nDetailed Error Information:\n" + ex, 
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        // Startup Functions   
        private void queueRecovery()
        {
            if (Main.CheckQueueRecovery())
            {
                DialogResult result =
                    MessageBox.Show(
                        "HandBrake has detected unfinished items on the queue from the last time the application was launched. Would you like to recover these?", 
                        "Queue Recovery Possible", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                    encodeQueue.LoadQueueFromFile("hb_queue_recovery.xml"); // Start Recovery
                else
                {
                    // Remove the Queue recovery file if the user doesn't want to recovery the last queue.
                    string queuePath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml");
                    if (File.Exists(queuePath))
                        File.Delete(queuePath);
                }
            }
        }

        #endregion

        #region Properties

        public string SourceName
        {
            get
            {
                if (this.selectedSourceType == SourceType.DvdDrive)
                {
                    return this.dvdDriveLabel;
                }

                if (Path.GetFileNameWithoutExtension(this.sourcePath) != "VIDEO_TS")
                    return Path.GetFileNameWithoutExtension(this.sourcePath);

                return Path.GetFileNameWithoutExtension(Path.GetDirectoryName(this.sourcePath));
            }
        }

        #endregion

        #region Events

        // Encoding Events for setting up the GUI
        private void events()
        {
            // Handle Widget changes when preset is selected.
            RegisterPresetEventHandler();

            // Handle Window Resize
            if (Properties.Settings.Default.MainWindowMinimize)
                this.Resize += new EventHandler(frmMain_Resize);

            // Handle Encode Start / Finish / Pause

            encodeQueue.QueuePauseRequested += new EventHandler(encodePaused);
            encodeQueue.EncodeStarted += new EventHandler(encodeStarted);
            encodeQueue.EncodeEnded += new EventHandler(encodeEnded);

            // Handle a file being draged onto the GUI.
            this.DragEnter += new DragEventHandler(frmMain_DragEnter);
            this.DragDrop += new DragEventHandler(frmMain_DragDrop);
        }

        // Change the preset label to custom when a user changes a setting. Don't want to give the impression that users can change settings and still be using a preset
        private void RegisterPresetEventHandler()
        {
            // Output Settings
            drop_format.SelectedIndexChanged += new EventHandler(changePresetLabel);
            check_largeFile.CheckedChanged += new EventHandler(changePresetLabel);
            check_iPodAtom.CheckedChanged += new EventHandler(changePresetLabel);
            check_optimiseMP4.CheckedChanged += new EventHandler(changePresetLabel);

            // Picture Settings
            // PictureSettings.PictureSettingsChanged += new EventHandler(changePresetLabel);

            // Filter Settings
            Filters.FilterSettingsChanged += new EventHandler(changePresetLabel);

            // Video Tab
            drp_videoEncoder.SelectedIndexChanged += new EventHandler(changePresetLabel);
            check_2PassEncode.CheckedChanged += new EventHandler(changePresetLabel);
            check_turbo.CheckedChanged += new EventHandler(changePresetLabel);
            text_filesize.TextChanged += new EventHandler(changePresetLabel);
            text_bitrate.TextChanged += new EventHandler(changePresetLabel);
            slider_videoQuality.ValueChanged += new EventHandler(changePresetLabel);

            // Audio Panel
            AudioSettings.AudioListChanged += new EventHandler(changePresetLabel);

            // Advanced Tab
            x264Panel.rtf_x264Query.TextChanged += new EventHandler(changePresetLabel);
        }

        private void UnRegisterPresetEventHandler()
        {
            // Output Settings 
            drop_format.SelectedIndexChanged -= new EventHandler(changePresetLabel);
            check_largeFile.CheckedChanged -= new EventHandler(changePresetLabel);
            check_iPodAtom.CheckedChanged -= new EventHandler(changePresetLabel);
            check_optimiseMP4.CheckedChanged -= new EventHandler(changePresetLabel);

            // Picture Settings
            // PictureSettings.PictureSettingsChanged -= new EventHandler(changePresetLabel);

            // Filter Settings
            Filters.FilterSettingsChanged -= new EventHandler(changePresetLabel);

            // Video Tab
            drp_videoEncoder.SelectedIndexChanged -= new EventHandler(changePresetLabel);
            check_2PassEncode.CheckedChanged -= new EventHandler(changePresetLabel);
            check_turbo.CheckedChanged -= new EventHandler(changePresetLabel);
            text_filesize.TextChanged -= new EventHandler(changePresetLabel);
            text_bitrate.TextChanged -= new EventHandler(changePresetLabel);
            slider_videoQuality.ValueChanged -= new EventHandler(changePresetLabel);

            // Audio Panel
            AudioSettings.AudioListChanged -= new EventHandler(changePresetLabel);

            // Advanced Tab
            x264Panel.rtf_x264Query.TextChanged -= new EventHandler(changePresetLabel);
        }

        private void changePresetLabel(object sender, EventArgs e)
        {
            labelPreset.Text = "Output Settings (Preset: Custom)";
            CurrentlySelectedPreset = null;
        }

        private static void frmMain_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop, false))
                e.Effect = DragDropEffects.All;
        }

        private void frmMain_DragDrop(object sender, DragEventArgs e)
        {
            string[] fileList = e.Data.GetData(DataFormats.FileDrop) as string[];
            sourcePath = string.Empty;

            if (fileList != null)
            {
                if (fileList[0] != string.Empty)
                {
                    this.selectedSourceType = SourceType.VideoFile;
                    StartScan(fileList[0], 0);
                }
                else
                    UpdateSourceLabel();
            }
            else
                UpdateSourceLabel();
        }

        private void encodeStarted(object sender, EventArgs e)
        {
            lastAction = "encode";
            SetEncodeStarted();

            // Experimental HBProc Process Monitoring.
            if (Properties.Settings.Default.enocdeStatusInGui)
            {
                Thread encodeMon = new Thread(EncodeMonitorThread);
                encodeMon.Start();
            }
        }

        private void encodeEnded(object sender, EventArgs e)
        {
            SetEncodeFinished();
        }

        private void encodePaused(object sender, EventArgs e)
        {
            SetEncodeFinished();
        }

        #endregion

        // User Interface Menus / Tool Strips *********************************

        #region File Menu

        private void mnu_killCLI_Click(object sender, EventArgs e)
        {
            KillScan();
        }

        private void mnu_exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        #endregion

        #region Tools Menu

        private void mnu_encode_Click(object sender, EventArgs e)
        {
            queueWindow.Show();
        }

        private void mnu_encodeLog_Click(object sender, EventArgs e)
        {
            frmActivityWindow dvdInfoWindow = new frmActivityWindow(lastAction);
            dvdInfoWindow.Show();
        }

        private void mnu_options_Click(object sender, EventArgs e)
        {
            Form options = new frmOptions(this);
            options.ShowDialog();
        }

        #endregion

        #region Presets Menu

        private void mnu_presetReset_Click(object sender, EventArgs e)
        {
            presetHandler.UpdateBuiltInPresets();
            LoadPresetPanel();
            if (treeView_presets.Nodes.Count == 0)
                MessageBox.Show(
                    "Unable to load the presets.xml file. Please select \"Update Built-in Presets\" from the Presets Menu. \nMake sure you are running the program in Admin mode if running on Vista. See Windows FAQ for details!", 
                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show("Presets have been updated!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);

            treeView_presets.ExpandAll();
        }

        private void mnu_delete_preset_Click(object sender, EventArgs e)
        {
            presetHandler.RemoveBuiltInPresets();
            LoadPresetPanel(); // Reload the preset panel
        }

        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            loadNormalPreset();
        }

        private void mnu_importMacPreset_Click(object sender, EventArgs e)
        {
            importPreset();
        }

        private void btn_new_preset_Click(object sender, EventArgs e)
        {
            Form preset = new frmAddPreset(this, QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null), 
                                           presetHandler);
            preset.ShowDialog();
        }

        #endregion

        #region Help Menu

        private void mnu_user_guide_Click(object sender, EventArgs e)
        {
            Process.Start("http://trac.handbrake.fr/wiki/HandBrakeGuide");
        }

        private void mnu_handbrake_home_Click(object sender, EventArgs e)
        {
            Process.Start("http://handbrake.fr");
        }

        private void mnu_UpdateCheck_Click(object sender, EventArgs e)
        {
            lbl_updateCheck.Visible = true;
            Main.BeginCheckForUpdates(new AsyncCallback(updateCheckDoneMenu), false);
        }

        private void updateCheckDoneMenu(IAsyncResult result)
        {
            // Make sure it's running on the calling thread
            if (InvokeRequired)
            {
                Invoke(new MethodInvoker(() => updateCheckDoneMenu(result)));
                return;
            }
            UpdateCheckInformation info;
            try
            {
                // Get the information about the new build, if any, and close the window
                info = Main.EndCheckForUpdates(result);

                if (info.NewVersionAvailable && info.BuildInformation != null)
                {
                    frmUpdater updateWindow = new frmUpdater(info.BuildInformation);
                    updateWindow.ShowDialog();
                }
                else
                    MessageBox.Show("There are no new updates at this time.", "Update Check", MessageBoxButtons.OK, 
                                    MessageBoxIcon.Information);
                lbl_updateCheck.Visible = false;
                return;
            }
            catch (Exception ex)
            {
                if ((bool) result.AsyncState)
                    MessageBox.Show(
                        "Unable to check for updates, Please try again later.\n\nDetailed Error Information:\n" + ex, 
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void mnu_about_Click(object sender, EventArgs e)
        {
            using (frmAbout About = new frmAbout())
            {
                About.ShowDialog();
            }
        }

        #endregion

        #region Preset Bar

        // Right Click Menu Code
        private void pmnu_expandAll_Click(object sender, EventArgs e)
        {
            treeView_presets.ExpandAll();
        }

        private void pmnu_collapse_Click(object sender, EventArgs e)
        {
            treeView_presets.CollapseAll();
        }

        private void pmnu_import_Click(object sender, EventArgs e)
        {
            importPreset();
        }

        private void pmnu_saveChanges_Click(object sender, EventArgs e)
        {
            DialogResult result =
                MessageBox.Show(
                    "Do you wish to include picture settings when updating the preset: " +
                    treeView_presets.SelectedNode.Text, "Update Preset", MessageBoxButtons.YesNoCancel, 
                    MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
                presetHandler.Update(treeView_presets.SelectedNode.Text, 
                                     QueryGenerator.GenerateTabbedComponentsQuery(this), true);
            else if (result == DialogResult.No)
                presetHandler.Update(treeView_presets.SelectedNode.Text, 
                                     QueryGenerator.GenerateTabbedComponentsQuery(this), false);
        }

        private void pmnu_delete_click(object sender, EventArgs e)
        {
            if (treeView_presets.SelectedNode != null)
            {
                presetHandler.Remove(treeView_presets.SelectedNode.Text);
                treeView_presets.Nodes.Remove(treeView_presets.SelectedNode);
            }
            treeView_presets.Select();
        }

        private void presets_menu_Opening(object sender, CancelEventArgs e)
        {
            // Make sure that the save menu is always disabled by default
            pmnu_saveChanges.Enabled = false;

            // Now enable the save menu if the selected preset is a user preset
            if (treeView_presets.SelectedNode != null)
                if (presetHandler.CheckIfUserPresetExists(treeView_presets.SelectedNode.Text))
                    pmnu_saveChanges.Enabled = true;

            treeView_presets.Select();
        }

        // Presets Management
        private void btn_addPreset_Click(object sender, EventArgs e)
        {
            Form preset = new frmAddPreset(this, QueryGenerator.GenerateTabbedComponentsQuery(this), presetHandler);
            preset.ShowDialog();
        }

        private void btn_removePreset_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you wish to delete the selected preset?", "Preset", 
                                                  MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                if (treeView_presets.SelectedNode != null)
                {
                    presetHandler.Remove(treeView_presets.SelectedNode.Text);
                    treeView_presets.Nodes.Remove(treeView_presets.SelectedNode);
                }
            }
            treeView_presets.Select();
        }

        private void btn_setDefault_Click(object sender, EventArgs e)
        {
            if (treeView_presets.SelectedNode != null)
            {
                DialogResult result = MessageBox.Show("Are you sure you wish to set this preset as the default?", 
                                                      "Preset", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    Properties.Settings.Default.defaultPreset = treeView_presets.SelectedNode.Text;
                    Properties.Settings.Default.Save();
                    MessageBox.Show("New default preset set.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            else
                MessageBox.Show("Please select a preset first.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        private void treeview_presets_mouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
                treeView_presets.SelectedNode = treeView_presets.GetNodeAt(e.Location);
            else if (e.Button == MouseButtons.Left)
            {
                if (treeView_presets.GetNodeAt(e.Location) != null)
                {
                    if (labelPreset.Text.Contains(treeView_presets.GetNodeAt(e.Location).Text))
                        selectPreset();
                }
            }

            treeView_presets.Select();
        }

        private void treeView_presets_AfterSelect(object sender, TreeViewEventArgs e)
        {
            selectPreset();
        }

        private void treeView_presets_deleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DialogResult result = MessageBox.Show("Are you sure you wish to delete the selected preset?", "Preset", 
                                                      MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    if (treeView_presets.SelectedNode != null)
                        presetHandler.Remove(treeView_presets.SelectedNode.Text);

                    // Remember each nodes expanded status so we can reload it
                    List<bool> nodeStatus = new List<bool>();
                    foreach (TreeNode node in treeView_presets.Nodes)
                        nodeStatus.Add(node.IsExpanded);

                    // Now reload the preset panel
                    LoadPresetPanel();

                    // And finally, re-expand any of the nodes if required
                    int i = 0;
                    foreach (TreeNode node in treeView_presets.Nodes)
                    {
                        if (nodeStatus[i])
                            node.Expand();

                        i++;
                    }
                }
            }
        }

        private void selectPreset()
        {
            if (treeView_presets.SelectedNode != null)
            {
                // Ok, so, we've selected a preset. Now we want to load it.
                string presetName = treeView_presets.SelectedNode.Text;
                Preset preset = presetHandler.GetPreset(presetName);
                if (preset != null)
                {
                    string query = presetHandler.GetPreset(presetName).Query;
                    bool loadPictureSettings = presetHandler.GetPreset(presetName).PictureSettings;

                    if (query != null)
                    {
                        // Ok, Reset all the H264 widgets before changing the preset
                        x264Panel.Reset2Defaults();

                        // Send the query from the file to the Query Parser class
                        QueryParser presetQuery = QueryParser.Parse(query);

                        // Now load the preset
                        PresetLoader.LoadPreset(this, presetQuery, presetName, loadPictureSettings);

                        // The x264 widgets will need updated, so do this now:
                        x264Panel.X264_StandardizeOptString();
                        x264Panel.X264_SetCurrentSettingsInPanel();

                        // Finally, let this window have a copy of the preset settings.
                        CurrentlySelectedPreset = preset;
                        PictureSettings.SetPresetCropWarningLabel(preset);
                    }
                }
            }
        }

        private void loadNormalPreset()
        {
            foreach (TreeNode treenode in treeView_presets.Nodes)
            {
                foreach (TreeNode node in treenode.Nodes)
                {
                    if (node.Text.Equals("Normal"))
                        treeView_presets.SelectedNode = treeView_presets.Nodes[treenode.Index].Nodes[0];
                }
            }
        }

        private void importPreset()
        {
            if (openPreset.ShowDialog() == DialogResult.OK)
            {
                QueryParser parsed = PlistPresetHandler.Import(openPreset.FileName);
                if (presetHandler.CheckIfUserPresetExists(parsed.PresetName + " (Imported)"))
                {
                    DialogResult result =
                        MessageBox.Show("This preset appears to already exist. Would you like to overwrite it?", 
                                        "Overwrite preset?", 
                                        MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                    if (result == DialogResult.Yes)
                    {
                        PresetLoader.LoadPreset(this, parsed, parsed.PresetName, parsed.UsesPictureSettings);
                        presetHandler.Update(parsed.PresetName + " (Imported)", 
                                             QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null), 
                                             parsed.UsesPictureSettings);
                    }
                }
                else
                {
                    PresetLoader.LoadPreset(this, parsed, parsed.PresetName, parsed.UsesPictureSettings);
                    presetHandler.Add(parsed.PresetName, 
                                      QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null), 
                                      parsed.UsesPictureSettings);

                    if (presetHandler.Add(parsed.PresetName + " (Imported)", 
                                          QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null), 
                                          parsed.UsesPictureSettings))
                    {
                        TreeNode preset_treeview = new TreeNode(parsed.PresetName + " (Imported)")
                                                       {
                                                          ForeColor = Color.Black
                                                       };
                        treeView_presets.Nodes.Add(preset_treeview);
                    }
                }
            }
        }

        #endregion

        #region ToolStrip

        private void btn_source_Click(object sender, EventArgs e)
        {
            mnu_dvd_drive.Visible = true;
            Thread driveInfoThread = new Thread(SetDriveSelectionMenuItem);
            driveInfoThread.Start();
        }

        private void btn_start_Click(object sender, EventArgs e)
        {
            if (btn_start.Text == "Stop")
            {
                DialogResult result;
                if (Properties.Settings.Default.enocdeStatusInGui &&
                    !Properties.Settings.Default.showCliForInGuiEncodeStatus)
                {
                    result = MessageBox.Show(
                        "Are you sure you wish to cancel the encode?\n\nPlease note, when 'Enable in-GUI encode status' is enabled, stopping this encode will render the file unplayable. ", 
                        "Cancel Encode?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                }
                else
                {
                    result = MessageBox.Show("Are you sure you wish to cancel the encode?", "Cancel Encode?", 
                                             MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                }

                if (result == DialogResult.Yes)
                {
                    // Pause The Queue
                    encodeQueue.Pause();

                    if (Properties.Settings.Default.enocdeStatusInGui &&
                        !Properties.Settings.Default.showCliForInGuiEncodeStatus)
                    {
                        encodeQueue.Stop();
                        if (encodeQueue.HbProcess != null)
                            encodeQueue.HbProcess.WaitForExit();
                    }
                    else
                    {
                        encodeQueue.SafelyClose();
                    }

                    // Update the GUI
                    SetEncodeFinished();
                }
            }
            else
            {
                if (encodeQueue.Count != 0 ||
                    (!string.IsNullOrEmpty(sourcePath) && !string.IsNullOrEmpty(text_destination.Text)))
                {
                    string generatedQuery = QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null);
                    string specifiedQuery = rtf_query.Text != string.Empty
                                                ? rtf_query.Text
                                                : QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null);
                    string query = string.Empty;

                    // Check to make sure the generated query matches the GUI settings
                    if (Properties.Settings.Default.PromptOnUnmatchingQueries && !string.IsNullOrEmpty(specifiedQuery) &&
                        generatedQuery != specifiedQuery)
                    {
                        DialogResult result = MessageBox.Show("The query under the \"Query Editor\" tab " +
                                                              "does not match the current GUI settings.\n\nBecause the manual query takes " +
                                                              "priority over the GUI, your recently updated settings will not be taken " +
                                                              "into account when encoding this job." +
                                                              Environment.NewLine + Environment.NewLine +
                                                              "Do you want to replace the manual query with the updated GUI-generated query?", 
                                                              "Manual Query does not Match GUI", 
                                                              MessageBoxButtons.YesNoCancel, MessageBoxIcon.Asterisk, 
                                                              MessageBoxDefaultButton.Button3);

                        switch (result)
                        {
                            case DialogResult.Yes:
                                // Replace the manual query with the generated one
                                query = generatedQuery;
                                rtf_query.Text = generatedQuery;
                                break;
                            case DialogResult.No:
                                // Use the manual query
                                query = specifiedQuery;
                                break;
                            case DialogResult.Cancel:
                                // Don't start the encode
                                return;
                        }
                    }
                    else
                    {
                        query = specifiedQuery;
                    }

                    DialogResult overwrite = DialogResult.Yes;
                    if (text_destination.Text != string.Empty)
                        if (File.Exists(text_destination.Text))
                            overwrite =
                                MessageBox.Show(
                                    "The destination file already exists. Are you sure you want to overwrite it?", 
                                    "Overwrite File?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                    if (overwrite == DialogResult.Yes)
                    {
                        if (encodeQueue.Count == 0)
                            encodeQueue.Add(query, sourcePath, text_destination.Text, (rtf_query.Text != string.Empty));

                        queueWindow.SetQueue();
                        if (encodeQueue.Count > 1)
                            queueWindow.Show(false);

                        SetEncodeStarted(); // Encode is running, so setup the GUI appropriately
                        encodeQueue.Start(); // Start The Queue Encoding Process
                        lastAction = "encode"; // Set the last action to encode - Used for activity window.
                    }
                    if (ActivityWindow != null)
                        ActivityWindow.SetEncodeMode();

                    this.Focus();
                }
                else if (string.IsNullOrEmpty(sourcePath) || string.IsNullOrEmpty(text_destination.Text))
                    MessageBox.Show("No source or destination selected.", "Warning", MessageBoxButtons.OK, 
                                    MessageBoxIcon.Warning);
            }
        }

        private void btn_add2Queue_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(sourcePath) || string.IsNullOrEmpty(text_destination.Text))
                MessageBox.Show("No source or destination selected.", "Warning", MessageBoxButtons.OK, 
                                MessageBoxIcon.Warning);
            else
            {
                string query = QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null);
                if (rtf_query.Text != string.Empty)
                    query = rtf_query.Text;

                if (encodeQueue.CheckForDestinationDuplicate(text_destination.Text))
                {
                    DialogResult result =
                        MessageBox.Show(
                            "There is already a queue item for this destination path. \n\n If you continue, the encode will be overwritten. Do you wish to continue?", 
                            "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                    if (result == DialogResult.Yes)
                        encodeQueue.Add(query, sourcePath, text_destination.Text, (rtf_query.Text != string.Empty));
                }
                else
                    encodeQueue.Add(query, sourcePath, text_destination.Text, (rtf_query.Text != string.Empty));

                lbl_encode.Text = encodeQueue.Count + " encode(s) pending in the queue";

                queueWindow.Show();
            }
        }

        private void btn_showQueue_Click(object sender, EventArgs e)
        {
            queueWindow.Show();
            queueWindow.Activate();
        }

        private void tb_preview_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(sourcePath) || string.IsNullOrEmpty(text_destination.Text))
                MessageBox.Show("No source or destination selected.", "Warning", MessageBoxButtons.OK, 
                                MessageBoxIcon.Warning);
            else
            {
                if (qtpreview == null)
                {
                    qtpreview = new frmPreview(this);
                    qtpreview.Show();
                }
                else if (qtpreview.IsDisposed)
                {
                    qtpreview = new frmPreview(this);
                    qtpreview.Show();
                }
                else
                    MessageBox.Show(qtpreview, "The preview window is already open!", "Warning", MessageBoxButtons.OK, 
                                    MessageBoxIcon.Warning);
            }
        }

        private void btn_ActivityWindow_Click(object sender, EventArgs e)
        {
            if (ActivityWindow == null || !ActivityWindow.IsHandleCreated)
                ActivityWindow = new frmActivityWindow(lastAction);
            else
                switch (lastAction)
                {
                    case "scan":
                        ActivityWindow.SetScanMode();
                        break;
                    case "encode":
                        ActivityWindow.SetEncodeMode();
                        break;
                    default:
                        ActivityWindow.SetEncodeMode();
                        break;
                }

            ActivityWindow.Show();
            ActivityWindow.Activate();
        }

        #endregion

        #region System Tray Icon

        private void frmMain_Resize(object sender, EventArgs e)
        {
            if (FormWindowState.Minimized == this.WindowState)
            {
                notifyIcon.Visible = true;
                this.Hide();
            }
            else if (FormWindowState.Normal == this.WindowState)
                notifyIcon.Visible = false;
        }

        private void notifyIcon_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.Visible = true;
            this.Activate();
            this.WindowState = FormWindowState.Normal;
            notifyIcon.Visible = false;
        }

        private void btn_restore_Click(object sender, EventArgs e)
        {
            this.Visible = true;
            this.Activate();
            this.WindowState = FormWindowState.Normal;
            notifyIcon.Visible = false;
        }

        #endregion

        #region Tab Control

        // Source
        private void btn_dvd_source_Click(object sender, EventArgs e)
        {
            if (DVD_Open.ShowDialog() == DialogResult.OK)
            {
                this.selectedSourceType = SourceType.Folder;
                SelectSource(DVD_Open.SelectedPath);
            }
            else
                UpdateSourceLabel();
        }

        private void btn_file_source_Click(object sender, EventArgs e)
        {
            if (ISO_Open.ShowDialog() == DialogResult.OK)
            {
                this.selectedSourceType = SourceType.VideoFile;
                SelectSource(ISO_Open.FileName);
            }
            else
                UpdateSourceLabel();
        }

        private void mnu_dvd_drive_Click(object sender, EventArgs e)
        {
            if (this.dvdDrivePath == null) return;
            this.selectedSourceType = SourceType.DvdDrive;
            SelectSource(this.dvdDrivePath);
        }

        private void SelectSource(string file)
        {
            Check_ChapterMarkers.Enabled = true;
            lastAction = "scan";
            sourcePath = string.Empty;

            if (file == string.Empty) // Must have a file or path
            {
                UpdateSourceLabel();
                return;
            }

            sourcePath = Path.GetFileName(file);
            StartScan(file, 0);
        }

        private void drp_dvdtitle_Click(object sender, EventArgs e)
        {
            if ((drp_dvdtitle.Items.Count == 1) && (drp_dvdtitle.Items[0].ToString() == "Automatic"))
                MessageBox.Show(
                    "There are no titles to select. Please load a source file by clicking the 'Source' button above before trying to select a title.", 
                    "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
        }

        private void drp_dvdtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            UnRegisterPresetEventHandler();
            drop_mode.SelectedIndex = 0;

            PictureSettings.lbl_Aspect.Text = "Select a Title"; // Reset some values on the form
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();

            // If the dropdown is set to automatic nothing else needs to be done.
            // Otheriwse if its not, title data has to be loased from parsing.
            if (drp_dvdtitle.Text != "Automatic")
            {
                selectedTitle = drp_dvdtitle.SelectedItem as Title;
                lbl_duration.Text = selectedTitle.Duration.ToString();
                PictureSettings.CurrentlySelectedPreset = CurrentlySelectedPreset;
                PictureSettings.Source = selectedTitle; // Setup Picture Settings Tab Control

                // Populate the Angles dropdown
                drop_angle.Items.Clear();
                if (!Properties.Settings.Default.noDvdNav)
                {
                    drop_angle.Visible = true;
                    lbl_angle.Visible = true;
                    drop_angle.Items.AddRange(selectedTitle.Angles.ToArray());
                    if (drop_angle.Items.Count != 0)
                        drop_angle.SelectedIndex = 0;
                }
                else
                {
                    drop_angle.Visible = false;
                    lbl_angle.Visible = false;
                }

                // Populate the Start chapter Dropdown
                drop_chapterStart.Items.Clear();
                drop_chapterStart.Items.AddRange(selectedTitle.Chapters.ToArray());
                if (drop_chapterStart.Items.Count > 0)
                    drop_chapterStart.Text = drop_chapterStart.Items[0].ToString();

                // Populate the Final Chapter Dropdown
                drop_chapterFinish.Items.Clear();
                drop_chapterFinish.Items.AddRange(selectedTitle.Chapters.ToArray());
                if (drop_chapterFinish.Items.Count > 0)
                    drop_chapterFinish.Text = drop_chapterFinish.Items[drop_chapterFinish.Items.Count - 1].ToString();

                // Populate the Audio Channels Dropdown
                AudioSettings.SetTrackList(selectedTitle);

                // Populate the Subtitles dropdown
                Subtitles.SetSubtitleTrackAuto(selectedTitle.Subtitles.ToArray());
            }
            // Update the source label if we have multiple streams
            if (selectedTitle != null)
                if (!string.IsNullOrEmpty(selectedTitle.SourceName))
                    labelSource.Text = labelSource.Text = Path.GetFileName(selectedTitle.SourceName);

            // Run the AutoName & ChapterNaming functions
            if (Properties.Settings.Default.autoNaming)
            {
                string autoPath = Main.AutoName(this);
                if (autoPath != null)
                    text_destination.Text = autoPath;
                else
                    MessageBox.Show(
                        "You currently have \"Automatically name output files\" enabled for the destination file box, but you do not have a default directory set.\n\nYou should set a \"Default Path\" in HandBrakes preferences. (See 'Tools' menu -> 'Options' -> 'General' Tab -> 'Default Path')", 
                        "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }

            data_chpt.Rows.Clear();
            if (selectedTitle.Chapters.Count != 1)
            {
                DataGridView chapterGridView = Main.ChapterNaming(data_chpt, drop_chapterFinish.Text);
                if (chapterGridView != null)
                    data_chpt = chapterGridView;
            }
            else
            {
                Check_ChapterMarkers.Checked = false;
                Check_ChapterMarkers.Enabled = false;
            }

            // Hack to force the redraw of the scrollbars which don't resize properly when the control is disabled.
            data_chpt.Columns[0].Width = 166;
            data_chpt.Columns[0].Width = 165;

            RegisterPresetEventHandler();
        }

        private void chapersChanged(object sender, EventArgs e)
        {
            if (drop_mode.SelectedIndex != 0) // Function is not used if we are not in chapters mode.
                return;

            Control ctl = (Control) sender;
            int chapterStart, chapterEnd;
            int.TryParse(drop_chapterStart.Text, out chapterStart);
            int.TryParse(drop_chapterFinish.Text, out chapterEnd);

            switch (ctl.Name)
            {
                case "drop_chapterStart":
                    if (drop_chapterFinish.SelectedIndex == -1 && drop_chapterFinish.Items.Count != 0)
                        drop_chapterFinish.SelectedIndex = drop_chapterFinish.Items.Count - 1;

                    if (chapterEnd != 0)
                        if (chapterStart > chapterEnd)
                            drop_chapterFinish.Text = chapterStart.ToString();
                    break;
                case "drop_chapterFinish":
                    if (drop_chapterStart.Items.Count >= 1 && drop_chapterStart.SelectedIndex == -1)
                        drop_chapterStart.SelectedIndex = 0;

                    if (chapterStart != 0)
                        if (chapterEnd < chapterStart)
                            drop_chapterFinish.Text = chapterStart.ToString();

                    // Add more rows to the Chapter menu if needed.
                    if (Check_ChapterMarkers.Checked)
                    {
                        int i = data_chpt.Rows.Count, finish = 0;
                        int.TryParse(drop_chapterFinish.Text, out finish);

                        while (i < finish)
                        {
                            int n = data_chpt.Rows.Add();
                            data_chpt.Rows[n].Cells[0].Value = (i + 1);
                            data_chpt.Rows[n].Cells[1].Value = "Chapter " + (i + 1);
                            data_chpt.Rows[n].Cells[0].ValueType = typeof (int);
                            data_chpt.Rows[n].Cells[1].ValueType = typeof (string);
                            i++;
                        }
                    }
                    break;
            }

            // Update the Duration
            lbl_duration.Text =
                Main.CalculateDuration(drop_chapterStart.SelectedIndex, drop_chapterFinish.SelectedIndex, selectedTitle)
                    .ToString();

            // Run the Autonaming function
            if (Properties.Settings.Default.autoNaming)
                text_destination.Text = Main.AutoName(this);

            // Disable chapter markers if only 1 chapter is selected.
            if (chapterStart == chapterEnd)
            {
                Check_ChapterMarkers.Enabled = false;
                btn_importChapters.Enabled = false;
                data_chpt.Enabled = false;
            }
            else
            {
                Check_ChapterMarkers.Enabled = true;
                if (Check_ChapterMarkers.Checked)
                {
                    btn_importChapters.Enabled = true;
                    data_chpt.Enabled = true;
                }
            }
        }

        private void SecondsOrFramesChanged(object sender, EventArgs e)
        {
            int start, end;
            int.TryParse(drop_chapterStart.Text, out start);
            int.TryParse(drop_chapterFinish.Text, out end);
            double duration = end - start;

            switch (drop_mode.SelectedIndex)
            {
                case 1:
                    lbl_duration.Text = TimeSpan.FromSeconds(duration).ToString();
                    return;
                case 2:
                    if (selectedTitle != null)
                    {
                        duration = duration/selectedTitle.Fps;
                        lbl_duration.Text = TimeSpan.FromSeconds(duration).ToString();
                    }
                    else
                        lbl_duration.Text = "--:--:--";

                    return;
            }
        }

        private void drop_mode_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Reset
            this.drop_chapterFinish.TextChanged -= new EventHandler(this.SecondsOrFramesChanged);
            this.drop_chapterStart.TextChanged -= new EventHandler(this.SecondsOrFramesChanged);

            // Do Work
            switch (drop_mode.SelectedIndex)
            {
                case 0:
                    drop_chapterStart.DropDownStyle = ComboBoxStyle.DropDownList;
                    drop_chapterFinish.DropDownStyle = ComboBoxStyle.DropDownList;
                    if (drop_chapterStart.Items.Count != 0)
                    {
                        drop_chapterStart.SelectedIndex = 0;
                        drop_chapterFinish.SelectedIndex = drop_chapterFinish.Items.Count - 1;
                    }
                    else
                        lbl_duration.Text = "--:--:--";
                    return;
                case 1:
                    this.drop_chapterStart.TextChanged += new EventHandler(this.SecondsOrFramesChanged);
                    this.drop_chapterFinish.TextChanged += new EventHandler(this.SecondsOrFramesChanged);
                    drop_chapterStart.DropDownStyle = ComboBoxStyle.Simple;
                    drop_chapterFinish.DropDownStyle = ComboBoxStyle.Simple;
                    if (selectedTitle != null)
                    {
                        drop_chapterStart.Text = "0";
                        drop_chapterFinish.Text = selectedTitle.Duration.TotalSeconds.ToString();
                    }
                    return;
                case 2:
                    this.drop_chapterStart.TextChanged += new EventHandler(this.SecondsOrFramesChanged);
                    this.drop_chapterFinish.TextChanged += new EventHandler(this.SecondsOrFramesChanged);
                    drop_chapterStart.DropDownStyle = ComboBoxStyle.Simple;
                    drop_chapterFinish.DropDownStyle = ComboBoxStyle.Simple;
                    if (selectedTitle != null)
                    {
                        drop_chapterStart.Text = "0";
                        drop_chapterFinish.Text = (selectedTitle.Fps*selectedTitle.Duration.TotalSeconds).ToString();
                    }
                    return;
            }
        }

        // Destination
        private void btn_destBrowse_Click(object sender, EventArgs e)
        {
            // This removes the file extension from the filename box on the save file dialog.
            // It's daft but some users don't realise that typing an extension overrides the dropdown extension selected.
            DVD_Save.FileName = Path.GetFileNameWithoutExtension(text_destination.Text);

            if (Path.IsPathRooted(text_destination.Text))
                DVD_Save.InitialDirectory = Path.GetDirectoryName(text_destination.Text);

            // Show the dialog and set the main form file path
            if (drop_format.SelectedIndex.Equals(0))
                DVD_Save.FilterIndex = 1;
            else if (drop_format.SelectedIndex.Equals(1))
                DVD_Save.FilterIndex = 2;

            if (DVD_Save.ShowDialog() == DialogResult.OK)
            {
                // Add a file extension manually, as FileDialog.AddExtension has issues with dots in filenames
                switch (DVD_Save.FilterIndex)
                {
                    case 1:
                        if (
                            !Path.GetExtension(DVD_Save.FileName).Equals(".mp4", 
                                                                         StringComparison.InvariantCultureIgnoreCase))
                            if (Properties.Settings.Default.useM4v)
                                DVD_Save.FileName = DVD_Save.FileName.Replace(".mp4", ".m4v").Replace(".mkv", ".m4v");
                            else
                                DVD_Save.FileName = DVD_Save.FileName.Replace(".m4v", ".mp4").Replace(".mkv", ".mp4");
                        break;
                    case 2:
                        if (
                            !Path.GetExtension(DVD_Save.FileName).Equals(".mkv", 
                                                                         StringComparison.InvariantCultureIgnoreCase))
                            DVD_Save.FileName = DVD_Save.FileName.Replace(".mp4", ".mkv").Replace(".m4v", ".mkv");
                        break;
                    default:
                        // do nothing  
                        break;
                }
                text_destination.Text = DVD_Save.FileName;

                // Quicktime requires .m4v file for chapter markers to work. If checked, change the extension to .m4v (mp4 and m4v are the same thing)
                if (Check_ChapterMarkers.Checked && DVD_Save.FilterIndex != 2)
                    SetExtension(".m4v");
            }
        }

        private void text_destination_TextChanged(object sender, EventArgs e)
        {
            string path = text_destination.Text;
            if (path.EndsWith(".mp4") || path.EndsWith(".m4v"))
                drop_format.SelectedIndex = 0;
            else if (path.EndsWith(".mkv"))
                drop_format.SelectedIndex = 1;
        }

        // Output Settings
        private void drop_format_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drop_format.SelectedIndex)
            {
                case 0:
                    if (Properties.Settings.Default.useM4v || Check_ChapterMarkers.Checked ||
                        AudioSettings.RequiresM4V() || Subtitles.RequiresM4V())
                        SetExtension(".m4v");
                    else
                        SetExtension(".mp4");
                    break;
                case 1:
                    SetExtension(".mkv");
                    break;
            }

            AudioSettings.SetContainer(drop_format.Text);
            Subtitles.SetContainer(drop_format.SelectedIndex);

            if (drop_format.Text.Contains("MP4"))
            {
                if (drp_videoEncoder.Items.Contains("VP3 (Theora)"))
                {
                    drp_videoEncoder.Items.Remove("VP3 (Theora)");
                    drp_videoEncoder.SelectedIndex = 1;
                }
            }
            else if (drop_format.Text.Contains("MKV"))
                drp_videoEncoder.Items.Add("VP3 (Theora)");
        }

        public void SetExtension(string newExtension)
        {
            if (newExtension == ".mp4" || newExtension == ".m4v")
                if (Properties.Settings.Default.useM4v || Check_ChapterMarkers.Checked || AudioSettings.RequiresM4V() ||
                    Subtitles.RequiresM4V())
                    newExtension = ".m4v";
                else
                    newExtension = ".mp4";

            if (Path.HasExtension(newExtension))
                text_destination.Text = Path.ChangeExtension(text_destination.Text, newExtension);
        }

        // Video Tab
        private void drp_videoEncoder_SelectedIndexChanged(object sender, EventArgs e)
        {
            setContainerOpts();

            // Turn off some options which are H.264 only when the user selects a non h.264 encoder
            if (drp_videoEncoder.Text.Contains("H.264"))
            {
                if (check_2PassEncode.CheckState == CheckState.Checked)
                    check_turbo.Enabled = true;

                tab_advanced.Enabled = true;
                if ((drop_format.Text.Contains("MP4")) || (drop_format.Text.Contains("M4V")))
                    check_iPodAtom.Enabled = true;
                else
                    check_iPodAtom.Enabled = false;
            }
            else
            {
                check_turbo.CheckState = CheckState.Unchecked;
                check_turbo.Enabled = false;
                tab_advanced.Enabled = false;
                x264Panel.X264Query = string.Empty;
                check_iPodAtom.Enabled = false;
                check_iPodAtom.Checked = false;
            }

            // Setup the CQ Slider
            switch (drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    if (slider_videoQuality.Value > 31)
                        slider_videoQuality.Value = 20; // Just reset to 70% QP 10 on encode change.
                    slider_videoQuality.Minimum = 1;
                    slider_videoQuality.Maximum = 31;
                    break;
                case "H.264 (x264)":
                    slider_videoQuality.Minimum = 0;
                    slider_videoQuality.TickFrequency = 1;

                    CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
                    double cqStep = Properties.Settings.Default.x264cqstep;
                    double multiplier = 1.0/cqStep;
                    double value = slider_videoQuality.Value*multiplier;

                    switch (Properties.Settings.Default.x264cqstep.ToString(culture))
                    {
                        case "0.2":
                            slider_videoQuality.Maximum = 255;
                            break;
                        case "0.25":
                            slider_videoQuality.Maximum = 204;
                            break;
                        case "0.5":
                            slider_videoQuality.Maximum = 102;
                            break;
                        case "1":
                            slider_videoQuality.Maximum = 51;
                            break;
                        default:
                            slider_videoQuality.Maximum = 51;
                            break;
                    }
                    if (value < slider_videoQuality.Maximum)
                        slider_videoQuality.Value = slider_videoQuality.Maximum - (int) value;

                    break;
                case "VP3 (Theora)":
                    if (slider_videoQuality.Value > 63)
                        slider_videoQuality.Value = 45; // Just reset to 70% QP 45 on encode change.
                    slider_videoQuality.Minimum = 0;
                    slider_videoQuality.Maximum = 63;
                    break;
            }
        }

        /// <summary>
        /// Set the container format options
        /// </summary>
        public void setContainerOpts()
        {
            if ((drop_format.Text.Contains("MP4")) || (drop_format.Text.Contains("M4V")))
            {
                check_largeFile.Enabled = true;
                check_optimiseMP4.Enabled = true;
                check_iPodAtom.Enabled = true;
            }
            else
            {
                check_largeFile.Enabled = false;
                check_optimiseMP4.Enabled = false;
                check_iPodAtom.Enabled = false;
                check_largeFile.Checked = false;
                check_optimiseMP4.Checked = false;
                check_iPodAtom.Checked = false;
            }
        }

        private double _cachedCqStep = Properties.Settings.Default.x264cqstep;

        /// <summary>
        /// Update the CQ slider for x264 for a new CQ step. This is set from option
        /// </summary>
        public void setQualityFromSlider()
        {
            // Work out the current RF value.
            double cqStep = _cachedCqStep;
            double rfValue = 51.0 - slider_videoQuality.Value*cqStep;

            // Change the maximum value for the slider
            switch (Properties.Settings.Default.x264cqstep.ToString(new CultureInfo("en-US")))
            {
                case "0.2":
                    slider_videoQuality.Maximum = 255;
                    break;
                case "0.25":
                    slider_videoQuality.Maximum = 204;
                    break;
                case "0.5":
                    slider_videoQuality.Maximum = 102;
                    break;
                case "1":
                    slider_videoQuality.Maximum = 51;
                    break;
                default:
                    slider_videoQuality.Maximum = 51;
                    break;
            }

            // Reset the CQ slider to RF0
            slider_videoQuality.Value = slider_videoQuality.Maximum;

            // Reset the CQ slider back to the previous value as close as possible
            double cqStepNew = Properties.Settings.Default.x264cqstep;
            double rfValueCurrent = 51.0 - slider_videoQuality.Value*cqStepNew;
            while (rfValueCurrent < rfValue)
            {
                slider_videoQuality.Value--;
                rfValueCurrent = 51.0 - slider_videoQuality.Value*cqStepNew;
            }

            // Cache the CQ step for the next calculation
            _cachedCqStep = Properties.Settings.Default.x264cqstep;
        }

        private void slider_videoQuality_Scroll(object sender, EventArgs e)
        {
            double cqStep = Properties.Settings.Default.x264cqstep;
            switch (drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    lbl_SliderValue.Text = "QP:" + (32 - slider_videoQuality.Value);
                    break;
                case "H.264 (x264)":
                    double rfValue = 51.0 - slider_videoQuality.Value*cqStep;
                    rfValue = Math.Round(rfValue, 2);
                    lbl_SliderValue.Text = "RF:" + rfValue.ToString(new CultureInfo("en-US"));
                    break;
                case "VP3 (Theora)":
                    lbl_SliderValue.Text = "QP:" + slider_videoQuality.Value;
                    break;
            }
        }

        private void radio_targetFilesize_CheckedChanged(object sender, EventArgs e)
        {
            text_bitrate.Enabled = false;
            text_filesize.Enabled = true;
            slider_videoQuality.Enabled = false;

            check_2PassEncode.Enabled = true;
        }

        private void radio_avgBitrate_CheckedChanged(object sender, EventArgs e)
        {
            text_bitrate.Enabled = true;
            text_filesize.Enabled = false;
            slider_videoQuality.Enabled = false;

            check_2PassEncode.Enabled = true;
        }

        private void radio_cq_CheckedChanged(object sender, EventArgs e)
        {
            text_bitrate.Enabled = false;
            text_filesize.Enabled = false;
            slider_videoQuality.Enabled = true;

            check_2PassEncode.Enabled = false;
            check_2PassEncode.CheckState = CheckState.Unchecked;
        }

        private void check_2PassEncode_CheckedChanged(object sender, EventArgs e)
        {
            if (check_2PassEncode.CheckState.ToString() == "Checked")
            {
                if (drp_videoEncoder.Text.Contains("H.264"))
                    check_turbo.Enabled = true;
            }
            else
            {
                check_turbo.Enabled = false;
                check_turbo.CheckState = CheckState.Unchecked;
            }
        }

        // Chapter Marker Tab
        private void Check_ChapterMarkers_CheckedChanged(object sender, EventArgs e)
        {
            if (Check_ChapterMarkers.Checked)
            {
                if (drop_format.SelectedIndex != 1)
                    SetExtension(".m4v");
                data_chpt.Enabled = true;
                btn_importChapters.Enabled = true;
            }
            else
            {
                if (drop_format.SelectedIndex != 1 && !Properties.Settings.Default.useM4v)
                    SetExtension(".mp4");
                data_chpt.Enabled = false;
                btn_importChapters.Enabled = false;
            }
        }

        private void btn_importChapters_Click(object sender, EventArgs e)
        {
            if (File_ChapterImport.ShowDialog() == DialogResult.OK)
            {
                string filename = File_ChapterImport.FileName;
                DataGridView imported = Main.ImportChapterNames(data_chpt, filename);
                if (imported != null)
                    data_chpt = imported;
            }
        }

        private void mnu_resetChapters_Click(object sender, EventArgs e)
        {
            data_chpt.Rows.Clear();
            DataGridView chapterGridView = Main.ChapterNaming(data_chpt, drop_chapterFinish.Text);
            if (chapterGridView != null)
            {
                data_chpt = chapterGridView;
            }
        }

        // Query Editor Tab
        private void btn_generate_Query_Click(object sender, EventArgs e)
        {
            rtf_query.Text = QueryGenerator.GenerateCliQuery(this, drop_mode.SelectedIndex, 0, null);
        }

        private void btn_clear_Click(object sender, EventArgs e)
        {
            rtf_query.Clear();
        }

        #endregion

        // MainWindow Components, Actions and Functions ***********************

        #region Source Scan

        public bool isScanning { get; set; }
        private Scan SourceScan;

        private void StartScan(string filename, int title)
        {
            // Setup the GUI components for the scan.
            sourcePath = filename;
            foreach (Control ctrl in Controls)
                if (!(ctrl is StatusStrip || ctrl is MenuStrip || ctrl is ToolStrip))
                    ctrl.Enabled = false;

            lbl_encode.Visible = true;
            lbl_encode.Text = "Scanning ...";
            btn_source.Enabled = false;
            btn_start.Enabled = false;
            btn_showQueue.Enabled = false;
            btn_add2Queue.Enabled = false;
            tb_preview.Enabled = false;
            mnu_killCLI.Visible = true;

            if (ActivityWindow != null)
                ActivityWindow.SetScanMode();

            // Start the Scan
            try
            {
                isScanning = true;
                SourceScan = new Scan();
                SourceScan.ScanSource(sourcePath, title);
                SourceScan.ScanStatusChanged += new EventHandler(SourceScan_ScanStatusChanged);
                SourceScan.ScanCompleted += new EventHandler(SourceScan_ScanCompleted);
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - StartScan " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void SourceScan_ScanStatusChanged(object sender, EventArgs e)
        {
            UpdateScanStatusLabel();
        }

        private void SourceScan_ScanCompleted(object sender, EventArgs e)
        {
            UpdateGuiAfterScan();
        }

        private void UpdateScanStatusLabel()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateWindowHandler(UpdateScanStatusLabel));
                return;
            }
            lbl_encode.Text = SourceScan.ScanStatus();
        }

        private void UpdateGuiAfterScan()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateWindowHandler(UpdateGuiAfterScan));
                return;
            }

            try
            {
                currentSource = SourceScan.SouceData();

                // Setup some GUI components
                drp_dvdtitle.Items.Clear();
                if (currentSource.Titles.Count != 0)
                    drp_dvdtitle.Items.AddRange(currentSource.Titles.ToArray());

                // Now select the longest title
                if (currentSource.Titles.Count != 0)
                    drp_dvdtitle.SelectedItem = Main.SelectLongestTitle(currentSource);

                // Enable the creation of chapter markers if the file is an image of a dvd.
                if (sourcePath.ToLower().Contains(".iso") || sourcePath.Contains("VIDEO_TS") ||
                    Directory.Exists(Path.Combine(sourcePath, "VIDEO_TS")))
                    Check_ChapterMarkers.Enabled = true;
                else
                {
                    Check_ChapterMarkers.Enabled = false;
                    Check_ChapterMarkers.Checked = false;
                    data_chpt.Rows.Clear();
                }

                // If no titles were found, Display an error message
                if (drp_dvdtitle.Items.Count == 0)
                {
                    MessageBox.Show(
                        "No Title(s) found. \n\nYour Source may be copy protected, badly mastered or in a format which HandBrake does not support. \nPlease refer to the Documentation and FAQ (see Help Menu).", 
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    sourcePath = string.Empty;
                }
                UpdateSourceLabel();

                // Enable the GUI components and enable any disabled components
                EnableGUI();
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - updateUIafterScan " + exc, "Error", MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                EnableGUI();
            }
        }

        private void EnableGUI()
        {
            try
            {
                if (InvokeRequired)
                    BeginInvoke(new UpdateWindowHandler(EnableGUI));
                lbl_encode.Text = "Scan Completed";
                foreach (Control ctrl in Controls)
                    ctrl.Enabled = true;
                btn_start.Enabled = true;
                btn_showQueue.Enabled = true;
                btn_add2Queue.Enabled = true;
                tb_preview.Enabled = true;
                btn_source.Enabled = true;
                mnu_killCLI.Visible = false;
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - EnableGUI() " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void KillScan()
        {
            try
            {
                SourceScan.ScanCompleted -= new EventHandler(SourceScan_ScanCompleted);
                EnableGUI();
                ResetGUI();

                if (SourceScan.ScanProcess() != null)
                    SourceScan.ScanProcess().Kill();

                lbl_encode.Text = "Scan Cancelled!";
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    "Unable to kill HandBrakeCLI.exe \nYou may need to manually kill HandBrakeCLI.exe using the Windows Task Manager if it does not close automatically within the next few minutes. \n\nError Information: \n" +
                    ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void ResetGUI()
        {
            drp_dvdtitle.Items.Clear();
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();
            lbl_duration.Text = "Select a Title";
            PictureSettings.lbl_src_res.Text = "Select a Title";
            PictureSettings.lbl_Aspect.Text = "Select a Title";
            sourcePath = String.Empty;
            text_destination.Text = String.Empty;
            selectedTitle = null;
            isScanning = false;
        }

        private void UpdateSourceLabel()
        {
            labelSource.Text = string.IsNullOrEmpty(sourcePath) ? "Select \"Source\" to continue." : this.SourceName;

            if (selectedTitle != null)
                if (!string.IsNullOrEmpty(selectedTitle.SourceName))
                    // If it's one of multiple source files, make sure we don't use the folder name
                    labelSource.Text = Path.GetFileName(selectedTitle.SourceName);
        }

        public void RecievingJob(Job job)
        {
            string query = job.Query;
            StartScan(job.Source, 0);


            if (query != null)
            {
                // Ok, Reset all the H264 widgets before changing the preset
                x264Panel.Reset2Defaults();

                // Send the query from the file to the Query Parser class
                QueryParser presetQuery = QueryParser.Parse(query);

                // Now load the preset
                PresetLoader.LoadPreset(this, presetQuery, "Load Back From Queue", true);

                // The x264 widgets will need updated, so do this now:
                x264Panel.X264_StandardizeOptString();
                x264Panel.X264_SetCurrentSettingsInPanel();

                // Finally, let this window have a copy of the preset settings.
                CurrentlySelectedPreset = null;
                PictureSettings.SetPresetCropWarningLabel(null);
            }
        }

        #endregion

        #region GUI Functions and Actions

        /// <summary>
        /// Set the GUI to it's finished encoding state.
        /// </summary>
        private void SetEncodeFinished()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(SetEncodeFinished));
                    return;
                }

                lbl_encode.Text = "Encoding Finished";
                btn_start.Text = "Start";
                btn_start.ToolTipText = "Start the encoding process";
                btn_start.Image = Properties.Resources.Play;

                // If the window is minimized, display the notification in a popup.
                if (Properties.Settings.Default.trayIconAlerts)
                    if (FormWindowState.Minimized == this.WindowState)
                    {
                        notifyIcon.BalloonTipText = lbl_encode.Text;
                        notifyIcon.ShowBalloonTip(500);
                    }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        /// <summary>
        /// Set the GUI to it's started encoding state.
        /// </summary>
        private void SetEncodeStarted()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(SetEncodeStarted));
                    return;
                }

                lbl_encode.Visible = true;
                lbl_encode.Text = "Encoding with " + encodeQueue.Count + " encode(s) pending";
                btn_start.Text = "Stop";
                btn_start.ToolTipText = "Stop the encoding process.";
                btn_start.Image = Properties.Resources.stop;
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        /// <summary>
        /// Set the DVD Drive selection in the "Source" Menu
        /// </summary>
        private void SetDriveSelectionMenuItem()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(SetDriveSelectionMenuItem));
                    return;
                }

                List<DriveInformation> drives = Main.GetDrives();

                if (drives.Count == 0)
                {
                    mnu_dvd_drive.Text = "[No DVD Drive Ready]";
                    return;
                }

                this.dvdDrivePath = drives[0].RootDirectory + "VIDEO_TS";
                this.dvdDriveLabel = drives[0].VolumeLabel;
                mnu_dvd_drive.Text = this.dvdDrivePath + " (" + this.dvdDriveLabel + ")";
            }
            catch (Exception)
            {
                mnu_dvd_drive.Text = "[No DVD Drive Ready / Found]";
            }
        }

        /// <summary>
        /// Access the preset Handler and setup the preset panel.
        /// </summary>
        private void LoadPresetPanel()
        {
            if (presetHandler.CheckIfPresetsAreOutOfDate())
                if (!Properties.Settings.Default.presetNotification)
                    MessageBox.Show(splash, 
                                    "HandBrake has determined your built-in presets are out of date... These presets will now be updated.", 
                                    "Preset Update", MessageBoxButtons.OK, MessageBoxIcon.Information);

            presetHandler.GetPresetPanel(ref treeView_presets);
            treeView_presets.Update();
        }

        #endregion

        #region Overrides

        /// <summary>
        /// Handle GUI shortcuts
        /// </summary>
        /// <param name="msg"></param>
        /// <param name="keyData"></param>
        /// <returns></returns>
        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == (Keys.Control | Keys.S))
            {
                btn_start_Click(this, new EventArgs());
                return true;
            }

            if (keyData == (Keys.Control | Keys.A))
            {
                btn_add2Queue_Click(this, new EventArgs());
                return true;
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }

        /// <summary>
        /// If the queue is being processed, prompt the user to confirm application close.
        /// </summary>
        /// <param name="e"></param>
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            // If currently encoding, the queue isn't paused, and there are queue items to process, prompt to confirm close.
            if ((encodeQueue.IsEncoding) && (!encodeQueue.PauseRequested) && (encodeQueue.Count > 0))
            {
                DialogResult result =
                    MessageBox.Show(
                        "HandBrake has queue items to process. Closing HandBrake will not stop the current encoding, but will stop processing the queue.\n\nDo you want to close HandBrake?", 
                        "Close HandBrake?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.No)
                    e.Cancel = true;
            }
            base.OnFormClosing(e);
        }

        #endregion

        #region In-GUI Encode Status (Experimental)

        /// <summary>
        /// Starts a new thread to monitor and process the CLI encode status
        /// </summary>
        private void EncodeMonitorThread()
        {
            try
            {
                Parser encode = new Parser(encodeQueue.HbProcess.StandardOutput.BaseStream);
                encode.OnEncodeProgress += EncodeOnEncodeProgress;
                while (!encode.EndOfStream)
                    encode.readEncodeStatus();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Displays the Encode status in the GUI
        /// </summary>
        /// <param name="Sender"></param>
        /// <param name="CurrentTask"></param>
        /// <param name="TaskCount"></param>
        /// <param name="PercentComplete"></param>
        /// <param name="CurrentFps"></param>
        /// <param name="AverageFps"></param>
        /// <param name="TimeRemaining"></param>
        private void EncodeOnEncodeProgress(object Sender, int CurrentTask, int TaskCount, float PercentComplete, 
                                            float CurrentFps, float AverageFps, TimeSpan TimeRemaining)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new EncodeProgressEventHandler(EncodeOnEncodeProgress), 
                                 new[]
                                     {
                                         Sender, CurrentTask, TaskCount, PercentComplete, CurrentFps, AverageFps, 
                                         TimeRemaining
                                     });
                return;
            }
            lbl_encode.Text =
                string.Format("Encode Progress: {0}%,       FPS: {1},       Avg FPS: {2},       Time Remaining: {3} ", 
                              PercentComplete, CurrentFps, AverageFps, TimeRemaining);
        }

        #endregion

        // This is the END of the road ****************************************
    }
}