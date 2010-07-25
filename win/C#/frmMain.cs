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
    using System.Reflection;
    using System.Threading;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using Model;
    using Presets;
    using Properties;

    /// <summary>
    /// The Main Window
    /// </summary>
    public partial class frmMain : Form
    {
        // Objects which may be used by one or more other objects *************
        private IQueue encodeQueue = new Queue();
        private PresetsHandler presetHandler = new PresetsHandler();

        // Windows ************************************************************
        private frmQueue queueWindow;
        private frmPreview qtpreview;
        private frmActivityWindow activityWindow;

        // Globals: Mainly used for tracking. *********************************
        public Title selectedTitle;
        public string sourcePath;
        private SourceType selectedSourceType;
        private string dvdDrivePath;
        private string dvdDriveLabel;
        private Preset currentlySelectedPreset;
        private DVD currentSource;
        private IScan SourceScan = new ScanService();
        private List<DriveInformation> drives;

        // Delegates **********************************************************
        private delegate void UpdateWindowHandler();

        // Applicaiton Startup ************************************************

        #region Properties

        /// <summary>
        /// Gets SourceName.
        /// </summary>
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

        #region Application Startup

        /// <summary>
        /// Initializes a new instance of the <see cref="frmMain"/> class.
        /// </summary>
        /// <param name="args">
        /// The arguments passed in on application startup.
        /// </param>
        public frmMain(string[] args)
        {
            InitializeComponent();

            // Update the users config file with the CLI version data.
            Main.SetCliVersionData();
            Main.CheckForValidCliVersion();

            if (Settings.Default.hb_version.Contains("svn"))
            {
                Version v = Assembly.GetExecutingAssembly().GetName().Version;
                this.Text += " " + v.ToString(4);
            }

            // Check for new versions, if update checking is enabled
            if (Settings.Default.updateStatus)
            {
                if (DateTime.Now.Subtract(Settings.Default.lastUpdateCheckDate).TotalDays > Properties.Settings.Default.daysBetweenUpdateCheck)
                {
                    Main.BeginCheckForUpdates(new AsyncCallback(UpdateCheckDone), false);
                }
            }

            // Clear the log files in the background
            if (Settings.Default.clearOldLogs)
            {
                Thread clearLog = new Thread(Main.ClearOldLogs);
                clearLog.Start();
            }

            // Setup the GUI components
            LoadPresetPanel(); // Load the Preset Panel
            treeView_presets.ExpandAll();
            lbl_encode.Text = string.Empty;
            drop_mode.SelectedIndex = 0;
            queueWindow = new frmQueue(encodeQueue, this); // Prepare the Queue
            if (!Settings.Default.QueryEditorTab)
                tabs_panel.TabPages.RemoveAt(7); // Remove the query editor tab if the user does not want it enabled.
            if (Settings.Default.tooltipEnable)
                ToolTip.Active = true;

            // Load the user's default settings or Normal Preset
            if (Settings.Default.defaultPreset != string.Empty && presetHandler.GetPreset(Properties.Settings.Default.defaultPreset) != null)
            {
                string query = presetHandler.GetPreset(Settings.Default.defaultPreset).Query;
                if (query != null)
                {
                    x264Panel.Reset2Defaults();

                    QueryParser presetQuery = QueryParser.Parse(query);
                    PresetLoader.LoadPreset(this, presetQuery, Settings.Default.defaultPreset);

                    x264Panel.StandardizeOptString();
                    x264Panel.SetCurrentSettingsInPanel();
                }
            }
            else
                loadNormalPreset();

            // Register with Growl (if not using Growl for the encoding completion action, this wont hurt anything)
            GrowlCommunicator.Register();

            // Event Handlers and Queue Recovery
            events();
            Main.RecoverQueue(encodeQueue);

            // If have a file passed in via command arguemtents, check it's a file and try scanning it.
            if (args.Length >= 1 && (File.Exists(args[0]) || Directory.Exists(args[0])))
            {
                this.StartScan(args[0], 0);
            }
        }

        /// <summary>
        /// When the update check is done, process the results.
        /// </summary>
        /// <param name="result">IAsyncResult result</param>
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
                if ((bool)result.AsyncState)
                    Main.ShowExceptiowWindow("Unable to check for updates, Please try again later.", ex.ToString());
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
            PictureSettings.PictureSettingsChanged += new EventHandler(changePresetLabel);

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
            PictureSettings.PictureSettingsChanged -= new EventHandler(changePresetLabel);

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
            this.currentlySelectedPreset = null;
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
                if (!string.IsNullOrEmpty(fileList[0]))
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
            SetEncodeStarted();
            encodeQueue.EncodeStatusChanged += EncodeQueue_EncodeStatusChanged;
        }

        private void encodeEnded(object sender, EventArgs e)
        {
            encodeQueue.EncodeStatusChanged -= EncodeQueue_EncodeStatusChanged;
            SetEncodeFinished();
        }
        #endregion

        // User Interface Menus / Tool Strips *********************************

        #region File Menu

        /// <summary>
        /// Kill The scan menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_killCLI_Click(object sender, EventArgs e)
        {
            KillScan();
        }

        /// <summary>
        /// Exit the Application Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        #endregion

        #region Tools Menu

        /// <summary>
        /// Menu - Start Button
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_encode_Click(object sender, EventArgs e)
        {
            queueWindow.Show();
        }

        /// <summary>
        /// Menu - Display the Log Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_encodeLog_Click(object sender, EventArgs e)
        {
            this.btn_ActivityWindow_Click(this, null);
        }

        /// <summary>
        /// Menu - Display the Options Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_options_Click(object sender, EventArgs e)
        {
            Form options = new frmOptions(this);
            options.ShowDialog();
        }

        #endregion

        #region Presets Menu

        /// <summary>
        /// Reset the Built in Presets
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Delete the selected preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_delete_preset_Click(object sender, EventArgs e)
        {
            presetHandler.RemoveBuiltInPresets();
            LoadPresetPanel(); // Reload the preset panel
        }

        /// <summary>
        /// Select the Normal preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            loadNormalPreset();
        }

        /// <summary>
        /// Import a plist Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_importMacPreset_Click(object sender, EventArgs e)
        {
            ImportPreset();
        }

        /// <summary>
        /// Export a Plist Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_exportMacPreset_Click(object sender, EventArgs e)
        {
            ExportPreset();
        }

        /// <summary>
        /// Create a new Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_new_preset_Click(object sender, EventArgs e)
        {
            Form preset = new frmAddPreset(this, presetHandler);
            if (preset.ShowDialog() == DialogResult.OK)
            {
                TreeNode presetTreeview = new TreeNode(presetHandler.LastPresetAdded.Name) { ForeColor = Color.Black };
                treeView_presets.Nodes.Add(presetTreeview);
                presetHandler.LastPresetAdded = null;
            }
        }

        #endregion

        #region Help Menu

        /// <summary>
        /// Menu - Display the User Guide Web Page
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_user_guide_Click(object sender, EventArgs e)
        {
            Process.Start("http://trac.handbrake.fr/wiki/HandBrakeGuide");
        }

        /// <summary>
        /// Menu - Check for Updates
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_UpdateCheck_Click(object sender, EventArgs e)
        {
            lbl_updateCheck.Visible = true;
            Main.BeginCheckForUpdates(new AsyncCallback(this.UpdateCheckDoneMenu), false);
        }

        /// <summary>
        /// Menu - Display the About Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_about_Click(object sender, EventArgs e)
        {
            using (frmAbout About = new frmAbout())
            {
                About.ShowDialog();
            }
        }

        #endregion

        #region Preset Bar

        /// <summary>
        /// RMenu - Expand All
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void pmnu_expandAll_Click(object sender, EventArgs e)
        {
            treeView_presets.ExpandAll();
        }

        /// <summary>
        /// RMenu - Collaspe All
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void pmnu_collapse_Click(object sender, EventArgs e)
        {
            treeView_presets.CollapseAll();
        }

        /// <summary>
        /// Menu - Import Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void pmnu_import_Click(object sender, EventArgs e)
        {
            ImportPreset();
        }

        /// <summary>
        /// RMenu - Save Changes to Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void pmnu_saveChanges_Click(object sender, EventArgs e)
        {
            // TODO this requires a re-think since the Query Editor has changed.
            DialogResult result =
                MessageBox.Show(
                    "Do you wish to include picture settings when updating the preset: " +
                    treeView_presets.SelectedNode.Text, "Update Preset", MessageBoxButtons.YesNoCancel,
                    MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
                presetHandler.Update(treeView_presets.SelectedNode.Text,
                                     QueryGenerator.GenerateQueryForPreset(this, QueryPictureSettingsMode.SourceMaximum, true, 0, 0), true);
            else if (result == DialogResult.No)
                presetHandler.Update(treeView_presets.SelectedNode.Text,
                                     QueryGenerator.GenerateQueryForPreset(this, QueryPictureSettingsMode.SourceMaximum, true, 0, 0), false);
        }

        /// <summary>
        /// RMenu - Delete Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void pmnu_delete_click(object sender, EventArgs e)
        {
            if (treeView_presets.SelectedNode != null)
            {
                presetHandler.Remove(treeView_presets.SelectedNode.Text);
                treeView_presets.Nodes.Remove(treeView_presets.SelectedNode);
            }
            treeView_presets.Select();
        }

        /// <summary>
        /// Preset Menu Is Opening. Setup the Menu
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Button - Add a preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_addPreset_Click(object sender, EventArgs e)
        {
            Form preset = new frmAddPreset(this, presetHandler);
            if (preset.ShowDialog() == DialogResult.OK)
            {
                TreeNode presetTreeview = new TreeNode(presetHandler.LastPresetAdded.Name) { ForeColor = Color.Black };
                treeView_presets.Nodes.Add(presetTreeview);
                presetHandler.LastPresetAdded = null;
            }
        }

        /// <summary>
        /// Button - remove a Preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Button - Set the selected preset as the default
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// PresetBar Mouse Down event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Preset Bar after selecting the preset
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void treeView_presets_AfterSelect(object sender, TreeViewEventArgs e)
        {
            selectPreset();
        }

        /// <summary>
        /// Preset Bar - Handle the Delete Key
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Select the selected preset and setup the GUI
        /// </summary>
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

                    if (query != null)
                    {
                        // Ok, Reset all the H264 widgets before changing the preset
                        x264Panel.Reset2Defaults();

                        // Send the query from the file to the Query Parser class
                        QueryParser presetQuery = QueryParser.Parse(query);

                        // Now load the preset
                        PresetLoader.LoadPreset(this, presetQuery, presetName);

                        // The x264 widgets will need updated, so do this now:
                        x264Panel.StandardizeOptString();
                        x264Panel.SetCurrentSettingsInPanel();

                        // Finally, let this window have a copy of the preset settings.
                        this.currentlySelectedPreset = preset;
                        PictureSettings.SetPresetCropWarningLabel(preset);
                    }
                }
            }
        }

        /// <summary>
        /// Load the Normal Preset
        /// </summary>
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

        /// <summary>
        /// Import a plist preset
        /// </summary>
        private void ImportPreset()
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
                        PresetLoader.LoadPreset(this, parsed, parsed.PresetName);
                        presetHandler.Update(parsed.PresetName + " (Imported)",
                                             QueryGenerator.GenerateFullQuery(this),
                                             parsed.UsesPictureSettings);
                    }
                }
                else
                {
                    PresetLoader.LoadPreset(this, parsed, parsed.PresetName);
                    if (presetHandler.Add(parsed.PresetName + " (Imported)",
                                          QueryGenerator.GenerateFullQuery(this),
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

        /// <summary>
        /// Export a plist Preset
        /// </summary>
        private void ExportPreset()
        {
            MessageBox.Show("This feature has not been implimented yet.", "Not Implimented", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;

            /*SaveFileDialog savefiledialog = new SaveFileDialog();
            savefiledialog.Filter = "plist|*.plist";

            if (treeView_presets.SelectedNode != null)
            {

                if (savefiledialog.ShowDialog() == DialogResult.OK)
                {
                    Preset preset = presetHandler.GetPreset(treeView_presets.SelectedNode.Text);
                    PlistPresetHandler.Export(savefiledialog.FileName, preset);
                }
            }*/
        }

        #endregion

        #region ToolStrip

        /// <summary>
        /// Toolbar - When the Source button is clicked, Clear any DVD drives and add any available DVD drives that can be used as a source.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_source_Click(object sender, EventArgs e)
        {
            // Remove old Drive Menu Items.
            List<ToolStripMenuItem> itemsToRemove = new List<ToolStripMenuItem>();
            foreach (var item in btn_source.DropDownItems)
            {
                if (item.GetType() == typeof(ToolStripMenuItem))
                {
                    ToolStripMenuItem menuItem = (ToolStripMenuItem)item;
                    if (menuItem.Name.StartsWith("Drive"))
                    {
                        itemsToRemove.Add(menuItem);
                    }
                }
            }

            foreach (ToolStripMenuItem item in itemsToRemove)
                btn_source.DropDownItems.Remove(item);

            Thread driveInfoThread = new Thread(SetDriveSelectionMenuItem);
            driveInfoThread.Start();
        }

        /// <summary>
        /// Toolbar - Start The Encode
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_start_Click(object sender, EventArgs e)
        {
            if (btn_start.Text == "Stop")
            {
                DialogResult result = !Properties.Settings.Default.showCliForInGuiEncodeStatus
                             ? MessageBox.Show(
                                 "Are you sure you wish to cancel the encode?\n\nPlease note: Stopping this encode will render the file unplayable. ",
                                 "Cancel Encode?",
                                 MessageBoxButtons.YesNo,
                                 MessageBoxIcon.Question)
                             : MessageBox.Show(
                                 "Are you sure you wish to cancel the encode?",
                                 "Cancel Encode?",
                                 MessageBoxButtons.YesNo,
                                 MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    // Pause The Queue
                    encodeQueue.Pause();

                    if (Settings.Default.showCliForInGuiEncodeStatus)
                        encodeQueue.SafelyClose();
                    else
                        encodeQueue.Stop();
                }
            }
            else
            {
                // If we have a custom query, then we'll want to figure out what the new source and destination is, otherwise we'll just use the gui components.
                string jobSourcePath = !string.IsNullOrEmpty(rtf_query.Text) ? Main.GetSourceFromQuery(rtf_query.Text) : sourcePath;
                string jobDestination = !string.IsNullOrEmpty(rtf_query.Text) ? Main.GetDestinationFromQuery(rtf_query.Text) : text_destination.Text;

                if (encodeQueue.Count != 0 || (!string.IsNullOrEmpty(jobSourcePath) && !string.IsNullOrEmpty(jobDestination)))
                {
                    string generatedQuery = QueryGenerator.GenerateFullQuery(this);
                    string specifiedQuery = rtf_query.Text != string.Empty
                                                ? rtf_query.Text
                                                : QueryGenerator.GenerateFullQuery(this);
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
                    if (!string.IsNullOrEmpty(jobDestination) && File.Exists(jobDestination))
                    {
                        overwrite = MessageBox.Show(
                                "The destination file already exists. Are you sure you want to overwrite it?",
                                "Overwrite File?",
                                MessageBoxButtons.YesNo,
                                MessageBoxIcon.Question);
                    }

                    if (overwrite == DialogResult.Yes)
                    {
                        if (encodeQueue.Count == 0)
                            encodeQueue.Add(query, this.GetTitle(), jobSourcePath, jobDestination, (rtf_query.Text != string.Empty));

                        queueWindow.SetQueue();
                        if (encodeQueue.Count > 1)
                            queueWindow.Show(false);

                        SetEncodeStarted(); // Encode is running, so setup the GUI appropriately
                        encodeQueue.Start(); // Start The Queue Encoding Process
                    }

                    this.Focus();
                }
                else if (string.IsNullOrEmpty(sourcePath) || string.IsNullOrEmpty(text_destination.Text))
                    MessageBox.Show("No source or destination selected.", "Warning", MessageBoxButtons.OK,
                                    MessageBoxIcon.Warning);
            }
        }

        /// <summary>
        /// Toolbar - Add the current job to the Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_add2Queue_Click(object sender, EventArgs e)
        {
            // Get the CLI query or use the query editor if it's not empty.
            string query = QueryGenerator.GenerateFullQuery(this);
            if (!string.IsNullOrEmpty(rtf_query.Text))
                query = rtf_query.Text;

            // If we have a custom query, then we'll want to figure out what the new source and destination is, otherwise we'll just use the gui components.
            string jobSourcePath = !string.IsNullOrEmpty(rtf_query.Text) ? Main.GetSourceFromQuery(rtf_query.Text) : sourcePath;
            string jobDestination = !string.IsNullOrEmpty(rtf_query.Text) ? Main.GetDestinationFromQuery(rtf_query.Text) : text_destination.Text;

            // Make sure we have a Source and Destination.
            if (string.IsNullOrEmpty(jobSourcePath) || string.IsNullOrEmpty(jobDestination))
            {
                MessageBox.Show("No source or destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // Make sure the destination path exists.
            if (!Directory.Exists(Path.GetDirectoryName(jobDestination)))
            {
                MessageBox.Show("Destination Path does not exist.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // Make sure we don't have a duplciate on the queue.
            if (encodeQueue.CheckForDestinationDuplicate(jobDestination))
            {
                DialogResult result =
                    MessageBox.Show(
                        "There is already a queue item for this destination path. \n\n If you continue, the encode will be overwritten. Do you wish to continue?",
                        "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (result == DialogResult.Yes)
                    encodeQueue.Add(query, this.GetTitle(), jobSourcePath, jobDestination, (rtf_query.Text != string.Empty));
            }
            else
                encodeQueue.Add(query, this.GetTitle(), jobSourcePath, jobDestination, (rtf_query.Text != string.Empty));

            lbl_encode.Text = encodeQueue.Count + " encode(s) pending in the queue";

            queueWindow.Show();
        }

        /// <summary>
        /// Toolbar - Show the Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_showQueue_Click(object sender, EventArgs e)
        {
            queueWindow.Show();
            queueWindow.Activate();
        }

        /// <summary>
        /// Toolbar - Show the Preview Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Toolbar - Show the Activity log Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_ActivityWindow_Click(object sender, EventArgs e)
        {
            if (this.activityWindow == null || !this.activityWindow.IsHandleCreated)
                this.activityWindow = new frmActivityWindow(encodeQueue, SourceScan);

            this.activityWindow.Show();
            this.activityWindow.Activate();
        }

        #endregion

        #region System Tray Icon

        /// <summary>
        /// Handle Resizing of the main window when deaing with the Notify Icon
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
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

        /// <summary>
        /// Double Click the Tray Icon
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void notifyIcon_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.Visible = true;
            this.Activate();
            this.WindowState = FormWindowState.Normal;
            notifyIcon.Visible = false;
        }

        /// <summary>
        /// Tray Icon - Restore Menu Item - Resture the Window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_restore_Click(object sender, EventArgs e)
        {
            this.Visible = true;
            this.Activate();
            this.WindowState = FormWindowState.Normal;
            notifyIcon.Visible = false;
        }

        #endregion

        #region Main Window and Tab Control

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
            ToolStripMenuItem item = sender as ToolStripMenuItem;
            if (item != null)
            {
                string driveId = item.Name.Replace("Drive", string.Empty);
                int id;
                if (int.TryParse(driveId, out id))
                {
                    this.dvdDrivePath = drives[id].RootDirectory;
                    this.dvdDriveLabel = drives[id].VolumeLabel;

                    if (this.dvdDrivePath == null) return;
                    this.selectedSourceType = SourceType.DvdDrive;
                    SelectSource(this.dvdDrivePath);
                }
            }
        }

        private void SelectSource(string file)
        {
            Check_ChapterMarkers.Enabled = true;
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

            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();

            // If the dropdown is set to automatic nothing else needs to be done.
            // Otheriwse if its not, title data has to be loaded from parsing.
            if (drp_dvdtitle.Text != "Automatic")
            {
                selectedTitle = drp_dvdtitle.SelectedItem as Title;
                lbl_duration.Text = selectedTitle.Duration.ToString();
                PictureSettings.CurrentlySelectedPreset = this.currentlySelectedPreset;
                PictureSettings.Source = selectedTitle; // Setup Picture Settings Tab Control

                // Populate the Angles dropdown
                drop_angle.Items.Clear();
                if (!Properties.Settings.Default.noDvdNav)
                {
                    drop_angle.Visible = true;
                    lbl_angle.Visible = true;

                    for (int i = 1; i <= selectedTitle.AngleCount; i++)
                        drop_angle.Items.Add(i.ToString());

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
                AudioSettings.SetTrackList(selectedTitle, this.currentlySelectedPreset);

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

            Control ctl = (Control)sender;
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
                            data_chpt.Rows[n].Cells[0].ValueType = typeof(int);
                            data_chpt.Rows[n].Cells[1].ValueType = typeof(string);
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
                        duration = duration / selectedTitle.Fps;
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
                        drop_chapterFinish.Text = (selectedTitle.Fps * selectedTitle.Duration.TotalSeconds).ToString();
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
                            !Path.GetExtension(DVD_Save.FileName).Equals(".mkv", StringComparison.InvariantCultureIgnoreCase))
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
                    double multiplier = 1.0 / cqStep;
                    double value = slider_videoQuality.Value * multiplier;

                    slider_videoQuality.Maximum = (int)(51 / Properties.Settings.Default.x264cqstep);

                    if (value < slider_videoQuality.Maximum)
                        slider_videoQuality.Value = slider_videoQuality.Maximum - (int)value;

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
        /// When the FrameRate is not Same As Source, show the Max/Constant Mode dropdown
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void drp_videoFramerate_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.drp_videoFramerate.SelectedIndex == 0)
            {
                this.checkMaximumFramerate.Visible = false;
                this.checkMaximumFramerate.CheckState = CheckState.Unchecked;
            }
            else
            {
                this.checkMaximumFramerate.Visible = true;
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

        private double cachedCqStep = Properties.Settings.Default.x264cqstep;

        /// <summary>
        /// Update the CQ slider for x264 for a new CQ step. This is set from option
        /// </summary>
        public void setQualityFromSlider()
        {
            // Work out the current RF value.
            double cqStep = this.cachedCqStep;
            double rfValue = 51.0 - slider_videoQuality.Value * cqStep;

            // Change the maximum value for the slider
            slider_videoQuality.Maximum = (int)(51 / Properties.Settings.Default.x264cqstep);

            // Reset the CQ slider to RF0
            slider_videoQuality.Value = slider_videoQuality.Maximum;

            // Reset the CQ slider back to the previous value as close as possible
            double cqStepNew = Properties.Settings.Default.x264cqstep;
            double rfValueCurrent = 51.0 - slider_videoQuality.Value * cqStepNew;
            while (rfValueCurrent < rfValue)
            {
                slider_videoQuality.Value--;
                rfValueCurrent = 51.0 - slider_videoQuality.Value * cqStepNew;
            }

            // Cache the CQ step for the next calculation
            this.cachedCqStep = Properties.Settings.Default.x264cqstep;
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
                    double rfValue = 51.0 - slider_videoQuality.Value * cqStep;
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

        private void btn_export_Click(object sender, EventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "Csv File|*.csv";
            saveFileDialog.DefaultExt = "csv";
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                string filename = saveFileDialog.FileName;

                Main.SaveChapterMarkersToCsv(this, filename);
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
            rtf_query.Text = QueryGenerator.GenerateFullQuery(this);
        }

        private void btn_clear_Click(object sender, EventArgs e)
        {
            rtf_query.Clear();
        }

        #endregion

        // MainWindow Components, Actions and Functions ***********************

        #region Source Scan

        /// <summary>
        /// Start the Scan Process
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        private void StartScan(string filename, int title)
        {
            // Setup the GUI components for the scan.
            sourcePath = filename;

            this.DisableGUI();

            // Start the Scan
            try
            {
                SourceScan.Scan(sourcePath, title);
                SourceScan.ScanStatusChanged += new EventHandler(SourceScan_ScanStatusChanged);
                SourceScan.ScanCompleted += new EventHandler(SourceScan_ScanCompleted);
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - StartScan " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Update the Status label for the scan
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void SourceScan_ScanStatusChanged(object sender, EventArgs e)
        {
            UpdateScanStatusLabel();
        }

        /// <summary>
        /// Update the UI after the scan has completed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void SourceScan_ScanCompleted(object sender, EventArgs e)
        {
            UpdateGuiAfterScan();
        }

        /// <summary>
        /// Update the Scan Status Label
        /// </summary>
        private void UpdateScanStatusLabel()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateWindowHandler(UpdateScanStatusLabel));
                return;
            }
            lbl_encode.Text = SourceScan.ScanStatus;
        }

        /// <summary>
        /// Reset the GUI when the scan has completed
        /// </summary>
        private void UpdateGuiAfterScan()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateWindowHandler(UpdateGuiAfterScan));
                return;
            }

            try
            {
                currentSource = SourceScan.SouceData;

                // Setup some GUI components
                drp_dvdtitle.Items.Clear();
                if (currentSource.Titles.Count != 0)
                    drp_dvdtitle.Items.AddRange(currentSource.Titles.ToArray());

                foreach (Title title in currentSource.Titles)
                {
                    if (title.MainTitle)
                    {
                        drp_dvdtitle.SelectedItem = title;
                    }
                }

                if (drp_dvdtitle.SelectedItem == null && drp_dvdtitle.Items.Count > 0)
                {
                    drp_dvdtitle.SelectedIndex = 0;
                }

                // Enable the creation of chapter markers if the file is an image of a dvd
                if (drop_chapterStart.Items.Count > 0)
                {
                    int start, end;
                    int.TryParse(drop_chapterStart.Items[0].ToString(), out start);
                    int.TryParse(drop_chapterFinish.Items[drop_chapterFinish.Items.Count - 1].ToString(), out end);
                    if (end > start) Check_ChapterMarkers.Enabled = true;
                    else
                    {
                        Check_ChapterMarkers.Enabled = false;
                        Check_ChapterMarkers.Checked = false;
                        data_chpt.Rows.Clear();
                    }
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

        /// <summary>
        /// Enable the GUI
        /// </summary>
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

        /// <summary>
        /// Disable the GUI
        /// </summary>
        private void DisableGUI()
        {
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
        }

        /// <summary>
        /// Kill the Scan
        /// </summary>
        private void KillScan()
        {
            SourceScan.ScanCompleted -= new EventHandler(SourceScan_ScanCompleted);
            EnableGUI();
            ResetGUI();

            SourceScan.Stop();

            lbl_encode.Text = "Scan Cancelled!";
        }

        /// <summary>
        /// Reset the GUI
        /// </summary>
        private void ResetGUI()
        {
            drp_dvdtitle.Items.Clear();
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();
            lbl_duration.Text = "Select a Title";
            PictureSettings.lbl_src_res.Text = "Select a Title";
            sourcePath = String.Empty;
            text_destination.Text = String.Empty;
            selectedTitle = null;
        }

        /// <summary>
        /// Update the Source Label
        /// </summary>
        private void UpdateSourceLabel()
        {
            labelSource.Text = string.IsNullOrEmpty(sourcePath) ? "Select \"Source\" to continue." : this.SourceName;

            if (selectedTitle != null)
                if (!string.IsNullOrEmpty(selectedTitle.SourceName))
                    // If it's one of multiple source files, make sure we don't use the folder name
                    labelSource.Text = Path.GetFileName(selectedTitle.SourceName);
        }

        /// <summary>
        /// Take a job from the Queue, rescan it, and reload the GUI for that job.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void RecievingJob(Job job)
        {
            string query = job.Query;
            StartScan(job.Source, job.Title);

            if (query != null)
            {
                // Ok, Reset all the H264 widgets before changing the preset
                x264Panel.Reset2Defaults();

                // Send the query from the file to the Query Parser class
                QueryParser presetQuery = QueryParser.Parse(query);

                // Now load the preset
                PresetLoader.LoadPreset(this, presetQuery, "Load Back From Queue");

                // The x264 widgets will need updated, so do this now:
                x264Panel.StandardizeOptString();
                x264Panel.SetCurrentSettingsInPanel();

                // Finally, let this window have a copy of the preset settings.
                this.currentlySelectedPreset = null;
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
                ProgressBarStatus.Visible = false;
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
                ProgressBarStatus.Value = 0;
                ProgressBarStatus.Visible = true;
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
        /// Display the Encode Status
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueue_EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EncodeProgressEventArgs e)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Encode.EncodeProgessStatus(EncodeQueue_EncodeStatusChanged), new[] { sender, e });
                return;
            }

            lbl_encode.Text =
                string.Format(
                "{0:00.00}%,  FPS: {1:000.0},  Avg FPS: {2:000.0},  Time Remaining: {3},  Encode(s) Pending {4}",
                e.PercentComplete,
                e.CurrentFrameRate,
                e.AverageFrameRate,
                e.EstimatedTimeLeft,
                encodeQueue.Count);

            ProgressBarStatus.Value = (int)Math.Round(e.PercentComplete);
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

                drives = Main.GetDrives();

                List<ToolStripMenuItem> menuItems = new List<ToolStripMenuItem>();
                foreach (DriveInformation drive in drives)
                {
                    ToolStripMenuItem menuItem = new ToolStripMenuItem
                        {
                            Name = drive.ToString(),
                            Text = drive.RootDirectory + " (" + drive.VolumeLabel + ")",
                            Image = Resources.disc_small
                        };
                    menuItem.Click += new EventHandler(mnu_dvd_drive_Click);
                    menuItems.Add(menuItem);
                }

                foreach (ToolStripMenuItem item in menuItems)
                    btn_source.DropDownItems.Add(item);
            }
            catch (Exception exc)
            {
                MessageBox.Show("Error in SetDriveSelectionMenuItem" + exc);
            }
        }

        /// <summary>
        /// Access the preset Handler and setup the preset panel.
        /// </summary>
        private void LoadPresetPanel()
        {
            if (presetHandler.CheckIfPresetsAreOutOfDate())
                if (!Settings.Default.presetNotification)
                    MessageBox.Show(this,
                                    "HandBrake has determined your built-in presets are out of date... These presets will now be updated.",
                                    "Preset Update", MessageBoxButtons.OK, MessageBoxIcon.Information);

            presetHandler.GetPresetPanel(ref treeView_presets);
            treeView_presets.Update();
        }

        /// <summary>
        /// Get the title from the selected item in the title dropdown.
        /// </summary>
        /// <returns>
        /// The title.
        /// </returns>
        private int GetTitle()
        {
            int title = 0;
            if (drp_dvdtitle.SelectedItem != null)
            {
                string[] titleInfo = drp_dvdtitle.SelectedItem.ToString().Split(' ');
                int.TryParse(titleInfo[0], out title);
            }

            return title;
        }

        /// <summary>
        /// Handle the Update Check Finishing.
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        private void UpdateCheckDoneMenu(IAsyncResult result)
        {
            // Make sure it's running on the calling thread
            if (InvokeRequired)
            {
                Invoke(new MethodInvoker(() => this.UpdateCheckDoneMenu(result)));
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
                if ((bool)result.AsyncState)
                    MessageBox.Show(
                        "Unable to check for updates, Please try again later.\n\nDetailed Error Information:\n" + ex,
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        #endregion

        #region Overrides

        /// <summary>
        /// Handle GUI shortcuts
        /// </summary>
        /// <param name="msg">Message</param>
        /// <param name="keyData">Keys</param>
        /// <returns>Bool</returns>
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
        /// <param name="e">FormClosingEventArgs</param>
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            try
            {
                // If currently encoding, the queue isn't paused, and there are queue items to process, prompt to confirm close.
                if (encodeQueue.IsEncoding)
                {
                    DialogResult result =
                        MessageBox.Show(
                            "HandBrake is currently encoding. Closing HandBrake will stop the current encode and will result in an unplayable file.\n\nDo you want to close HandBrake?",
                            "Close HandBrake?",
                            MessageBoxButtons.YesNo,
                            MessageBoxIcon.Question);

                    if (result == DialogResult.No)
                    {
                        e.Cancel = true;
                        return;
                    }

                    encodeQueue.Stop();
                }

                if (SourceScan.IsScanning)
                {
                    SourceScan.ScanCompleted -= new EventHandler(SourceScan_ScanCompleted);
                    SourceScan.Stop();
                }
            }
            catch (Exception exc)
            {
                Main.ShowExceptiowWindow("HandBrake was not able to shutdown properly. You may need to forcefully quit HandBrake CLI from TaskManager if it's still running.", exc.ToString());
            }
            finally
            {
                base.OnFormClosing(e);
            }
        }

        #endregion

        // This is the END of the road ****************************************
    }
}