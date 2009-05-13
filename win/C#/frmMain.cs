/*  frmMain.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Threading;
using Handbrake.Functions;
using Handbrake.Presets;
using Handbrake.Queue;
using Handbrake.Parsing;

namespace Handbrake
{
    public partial class frmMain : Form
    {
        // Objects which may be used by one or more other objects
        QueueHandler encodeQueue = new QueueHandler();
        PresetsHandler presetHandler = new PresetsHandler();
        QueryGenerator queryGen = new QueryGenerator();

        // Globals: Mainly used for tracking.
        Title selectedTitle;
        DVD thisDVD;
        private frmQueue queueWindow;
        private frmPreview qtpreview;
        private Form splash;
        public int maxWidth;
        public int maxHeight;

        // Delegates
        private delegate void UpdateWindowHandler();
        private delegate void UpdateStatusChanger();

        // Applicaiton Startup ************************************************

        #region Application Startup
        public frmMain()
        {
            // Load and setup the splash screen in this thread
            splash = new frmSplashScreen();
            splash.Show();
            Label lblStatus = new Label { Size = new Size(250, 20), Location = new Point(10, 280) };
            splash.Controls.Add(lblStatus);

            InitializeComponent();

            // Update the users config file with the CLI version data.
            lblStatus.Text = "Setting Version Data ...";
            Application.DoEvents();
            Main.setCliVersionData();

            // Show the form, but leave disabled until preloading is complete then show the main form
            this.Enabled = false;
            this.Show();
            Application.DoEvents(); // Forces frmMain to draw

            // Check for new versions, if update checking is enabled
            if (Properties.Settings.Default.updateStatus == "Checked")
            {
                lblStatus.Text = "Checking for updates ...";
                Application.DoEvents();

                Thread updateCheckThread = new Thread(startupUpdateCheck);
                updateCheckThread.Start();
            }

            // Setup the GUI components
            lblStatus.Text = "Setting up the GUI ...";
            Application.DoEvents();
            loadPresetPanel();                       // Load the Preset Panel
            treeView_presets.ExpandAll();
            lbl_encode.Text = "";
            lbl_max.Text = "";
            queueWindow = new frmQueue(encodeQueue);        // Prepare the Queue
            if (Properties.Settings.Default.QueryEditorTab != "Checked")
                tabs_panel.TabPages.RemoveAt(6); // Remove the query editor tab if the user does not want it enabled.

            // Load the user's default settings or Normal Preset
            if (Properties.Settings.Default.defaultSettings == "Checked" && Properties.Settings.Default.defaultPreset != "")
            {
                if (presetHandler.getPreset(Properties.Settings.Default.defaultPreset) != null)
                {
                    string query = presetHandler.getPreset(Properties.Settings.Default.defaultPreset).Query;
                    Boolean loadPictureSettings = presetHandler.getPreset(Properties.Settings.Default.defaultPreset).PictureSettings;

                    if (query != null)
                    {
                        //Ok, Reset all the H264 widgets before changing the preset
                        x264Panel.reset2Defaults();

                        // Send the query from the file to the Query Parser class, then load the preset
                        QueryParser presetQuery = QueryParser.Parse(query);
                        PresetLoader.presetLoader(this, presetQuery, Properties.Settings.Default.defaultPreset, loadPictureSettings);

                        // The x264 widgets will need updated, so do this now:
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
            if (Properties.Settings.Default.tooltipEnable == "Checked")
                ToolTip.Active = true;

            //Finished Loading
            lblStatus.Text = "Loading Complete!";
            Application.DoEvents();
            splash.Close();
            splash.Dispose();
            this.Enabled = true;

            // Event Handlers and Queue Recovery
            events();
            queueRecovery();
        }

        // Startup Functions   
        private void startupUpdateCheck()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateStatusChanger(startupUpdateCheck));
                    return;
                }

                Boolean update = Main.updateCheck(false);
                if (update)
                {
                    frmUpdater updateWindow = new frmUpdater();
                    updateWindow.Show();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(splash, "Unable to perform update check. If this problem persists, you can turn of update checking in the program options. \nError Information: \n\n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void queueRecovery()
        {
            if (Main.check_queue_recovery())
            {
                DialogResult result = MessageBox.Show("HandBrake has detected unfinished items on the queue from the last time the application was launched. Would you like to recover these?", "Queue Recovery Possible", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                    encodeQueue.recoverQueue("hb_queue_recovery.xml"); // Start Recovery
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

        #region Events
        // Encoding Events for setting up the GUI
        private void events()
        {
            // Handle Window Resize
            if (Properties.Settings.Default.MainWindowMinimize == "Checked")
                this.Resize += new EventHandler(frmMain_Resize);

            // Handle Encode Start / Finish / Pause
            encodeQueue.OnEncodeEnded += new EventHandler(encodeEnded);
            encodeQueue.OnPaused += new EventHandler(encodePaused);
            encodeQueue.OnEncodeStart += new EventHandler(encodeStarted);

            // Handle a file being draged onto the GUI.
            this.DragEnter += new DragEventHandler(frmMain_DragEnter);
            this.DragDrop += new DragEventHandler(frmMain_DragDrop);
        }

        private static void frmMain_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop, false))
                e.Effect = DragDropEffects.All;
        }
        private void frmMain_DragDrop(object sender, DragEventArgs e)
        {
            string[] fileList = e.Data.GetData(DataFormats.FileDrop) as string[];
            if (fileList != null)
            {
                if (fileList[0].StartsWith("\\"))
                    MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                else
                {
                    if (fileList[0] != "")
                    {
                        setupGUIforScan(fileList[0]);
                        startScan(fileList[0]);
                    }
                    else
                        text_source.Text = "Click 'Source' to continue";
                }
            }
        }
        private void encodeStarted(object sender, EventArgs e)
        {
            lastAction = "encode";
            setEncodeStarted();

            // Experimental HBProc Process Monitoring.
            if (Properties.Settings.Default.enocdeStatusInGui == "Checked")
            {
                HBProcess = encodeQueue.hbProc;
                Thread EncodeMon = new Thread(encodeMonitorThread);
                EncodeMon.Start();
            }
        }
        private void encodeEnded(object sender, EventArgs e)
        {
            setEncodeFinished();
        }
        private void encodePaused(object sender, EventArgs e)
        {
            setEncodeFinished();
        }
        #endregion

        // User Interface Menus / Tool Strips *********************************

        #region File Menu
        private void mnu_killCLI_Click(object sender, EventArgs e)
        {
            killScan();
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
            String file;
            file = lastAction == "scan" ? "last_scan_log.txt" : "last_encode_log.txt";

            frmActivityWindow dvdInfoWindow = new frmActivityWindow(file, encodeQueue, this);
            dvdInfoWindow.Show();
        }
        private void mnu_options_Click(object sender, EventArgs e)
        {
            Form Options = new frmOptions();
            Options.ShowDialog();
        }
        #endregion

        #region Presets Menu
        private void mnu_presetReset_Click(object sender, EventArgs e)
        {
            presetHandler.updateBuiltInPresets();
            loadPresetPanel();
            if (treeView_presets.Nodes.Count == 0)
                MessageBox.Show("Unable to load the presets.xml file. Please select \"Update Built-in Presets\" from the Presets Menu \nMake sure you are running the program in Admin mode if running on Vista. See Windows FAQ for details!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show("Presets have been updated!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);

            treeView_presets.ExpandAll();
        }
        private void mnu_delete_preset_Click(object sender, EventArgs e)
        {
            // Empty the preset file
            string presetsFile = Application.StartupPath + "\\presets.xml";
            if (File.Exists(presetsFile))
                File.Delete(presetsFile);

            try
            {
                FileStream strm = new FileStream(presetsFile, FileMode.Create, FileAccess.Write);
                strm.Close();
                strm.Dispose();
            }
            catch (Exception exc)
            {
                MessageBox.Show("An error has occured during the preset removal process.\n If you are using Windows Vista, you may need to run under Administrator Mode to complete this task. \n" + exc);
            }

            // Reload the preset panel
            loadPresetPanel();
        }
        private void mnu_SelectDefault_Click(object sender, EventArgs e)
        {
            loadNormalPreset();
        }
        private void btn_new_preset_Click(object sender, EventArgs e)
        {
            // Remember each nodes expanded status so we can reload it
            List<Boolean> nodeStatus = saveTreeViewState();
            nodeStatus.Add(true);

            Form preset = new frmAddPreset(this, queryGen.GenerateTheQuery(this), presetHandler);
            preset.ShowDialog();

            // Now reload the TreeView states
            loadTreeViewStates(nodeStatus);
        }
        #endregion

        #region Help Menu
        private void mnu_handbrake_forums_Click(object sender, EventArgs e)
        {
            Process.Start("http://forum.handbrake.fr/");
        }
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
            Boolean update = Main.updateCheck(true);
            if (update)
            {
                frmUpdater updateWindow = new frmUpdater();
                updateWindow.Show();
            }
            else
                MessageBox.Show("There are no new updates at this time.", "Update Check", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        private void mnu_about_Click(object sender, EventArgs e)
        {
            Form About = new frmAbout();
            About.ShowDialog();
        }
        #endregion

        #region Debug Menu
        private void mnu_qptest_Click(object sender, EventArgs e)
        {
            QueryParserTester qptest = new QueryParserTester();
            qptest.Show();
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
        private void pmnu_saveChanges_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Do you wish to include picture settings when updating the preset: " + treeView_presets.SelectedNode.Text, "Update Preset", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
                presetHandler.updatePreset(treeView_presets.SelectedNode.Text, QueryGenerator.generateTabbedComponentsQuery(this), true);
            else if (result == DialogResult.No)
                presetHandler.updatePreset(treeView_presets.SelectedNode.Text, QueryGenerator.generateTabbedComponentsQuery(this), false);
        }
        private void pmnu_delete_click(object sender, EventArgs e)
        {
            if (treeView_presets.SelectedNode != null)
            {
                presetHandler.remove(treeView_presets.SelectedNode.Text);

                // Remember each nodes expanded status so we can reload it
                List<Boolean> nodeStatus = saveTreeViewState();

                // Now reload the preset panel
                loadPresetPanel();

                // Now reload the TreeView states
                loadTreeViewStates(nodeStatus);
            }
            treeView_presets.Select();
        }
        private void presets_menu_Opening(object sender, System.ComponentModel.CancelEventArgs e)
        {
            // Make sure that the save menu is always disabled by default
            pmnu_saveChanges.Enabled = false;

            // Now enable the save menu if the selected preset is a user preset
            if (treeView_presets.SelectedNode != null)
            {
                if (presetHandler.checkIfUserPresetExists(treeView_presets.SelectedNode.Text))
                {
                    pmnu_saveChanges.Enabled = true;
                }
            }
            treeView_presets.Select();
        }

        // Presets Management
        private void btn_addPreset_Click(object sender, EventArgs e)
        {
            // Remember each nodes expanded status so we can reload it
            List<Boolean> nodeStatus = saveTreeViewState();
            nodeStatus.Add(true);

            // Now add the new preset
            Form preset = new frmAddPreset(this, QueryGenerator.generateTabbedComponentsQuery(this), presetHandler);
            preset.ShowDialog();

            // Now reload the TreeView states
            loadTreeViewStates(nodeStatus);
        }
        private void btn_removePreset_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you wish to delete the selected preset?", "Preset", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                if (treeView_presets.SelectedNode != null)
                    presetHandler.remove(treeView_presets.SelectedNode.Text);

                // Remember each nodes expanded status so we can reload it
                List<Boolean> nodeStatus = saveTreeViewState();

                // Now reload the preset panel
                loadPresetPanel();

                // Now reload the TreeView states
                loadTreeViewStates(nodeStatus);
            }
            treeView_presets.Select();
        }
        private void btn_setDefault_Click(object sender, EventArgs e)
        {
            if (treeView_presets.SelectedNode != null)
            {
                DialogResult result = MessageBox.Show("Are you sure you wish to set this preset as the default?", "Preset", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
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
                    if (groupBox_output.Text.Contains(treeView_presets.GetNodeAt(e.Location).Text))
                        selectPreset();
                }
            }

            treeView_presets.Select();
        }
        private void treeView_presets_AfterSelect(object sender, TreeViewEventArgs e)
        {
            selectPreset();
        }
        private void selectPreset()
        {
            if (treeView_presets.SelectedNode != null)
            {
                // Ok, so, we've selected a preset. Now we want to load it.
                string presetName = treeView_presets.SelectedNode.Text;
                if (presetHandler.getPreset(presetName) != null)
                {
                    string query = presetHandler.getPreset(presetName).Query;
                    Boolean loadPictureSettings = presetHandler.getPreset(presetName).PictureSettings;

                    if (query != null)
                    {
                        //Ok, Reset all the H264 widgets before changing the preset
                        x264Panel.reset2Defaults();

                        // Send the query from the file to the Query Parser class
                        QueryParser presetQuery = QueryParser.Parse(query);

                        // Now load the preset
                        PresetLoader.presetLoader(this, presetQuery, presetName, loadPictureSettings);

                        // The x264 widgets will need updated, so do this now:
                        x264Panel.X264_StandardizeOptString();
                        x264Panel.X264_SetCurrentSettingsInPanel();

                        if (maxWidth != 0 && maxHeight != 0)
                            lbl_max.Text = "Max Width / Height";
                        else if (maxWidth != 0)
                            lbl_max.Text = "Max Width";
                        else
                            lbl_max.Text = "";
                    }
                }
            }
        }
        private void treeView_presets_deleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DialogResult result = MessageBox.Show("Are you sure you wish to delete the selected preset?", "Preset", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    if (treeView_presets.SelectedNode != null)
                        presetHandler.remove(treeView_presets.SelectedNode.Text);

                    // Remember each nodes expanded status so we can reload it
                    List<Boolean> nodeStatus = new List<Boolean>();
                    foreach (TreeNode node in treeView_presets.Nodes)
                        nodeStatus.Add(node.IsExpanded);

                    // Now reload the preset panel
                    loadPresetPanel();

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
        private List<Boolean> saveTreeViewState()
        {
            // Remember each nodes expanded status so we can reload it
            List<Boolean> nodeStatus = new List<Boolean>();
            foreach (TreeNode node in treeView_presets.Nodes)
            {
                nodeStatus.Add(node.IsExpanded);
                foreach (TreeNode subNode in node.Nodes)
                    nodeStatus.Add(node.IsExpanded);
            }
            return nodeStatus;
        }
        private void loadTreeViewStates(List<Boolean> nodeStatus)
        {
            // And finally, re-expand any of the nodes if required
            int i = 0;
            foreach (TreeNode node in treeView_presets.Nodes)
            {
                if (nodeStatus[i])
                    node.Expand();

                foreach (TreeNode subNode in node.Nodes)
                {
                    if (nodeStatus[i])
                        subNode.Expand();
                }

                i++;
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
        #endregion

        #region ToolStrip
        private void btn_source_Click(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.drive_detection == "Checked")
            {
                mnu_dvd_drive.Visible = true;
                Thread driveInfoThread = new Thread(getDriveInfoThread);
                driveInfoThread.Start();
            }
            else
                mnu_dvd_drive.Visible = false;
        }
        private void btn_start_Click(object sender, EventArgs e)
        {
            if (btn_start.Text == "Stop")
            {
                DialogResult result = MessageBox.Show("Are you sure you wish to cancel the encode? Please note that this may break the encoded file. \nTo safely cancel your encode, press ctrl-c on your keyboard in the CLI window. This *may* allow you to preview your encoded content.", "Cancel Encode?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    // Pause The Queue
                    encodeQueue.pauseEncode();
                    encodeQueue.endEncode();

                    // Update the GUI
                    setEncodeFinished();
                }
            }
            else
            {
                if (encodeQueue.count() != 0 || (text_source.Text != string.Empty && text_source.Text != "Click 'Source' to continue" && text_destination.Text != string.Empty))
                {
                    // Set the last action to encode. 
                    // This is used for tracking which file to load in the activity window
                    lastAction = "encode";

                    String query = rtf_query.Text != "" ? rtf_query.Text : queryGen.GenerateTheQuery(this);

                    if (encodeQueue.count() == 0)
                    {
                        encodeQueue.add(query, text_source.Text, text_destination.Text);
                        encodeQueue.write2disk("hb_queue_recovery.xml");
                    }
                    queueWindow.setQueue();
                    if (encodeQueue.count() > 1)
                        queueWindow.Show(false);

                    setEncodeStarted(); // Encode is running, so setup the GUI appropriately
                    encodeQueue.startEncode(); // Start The Queue Encoding Process
                    this.Focus();
                }
                else if (text_source.Text == string.Empty || text_source.Text == "Click 'Source' to continue" || text_destination.Text == string.Empty)
                    MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        private void btn_add2Queue_Click(object sender, EventArgs e)
        {

            if (text_source.Text == string.Empty || text_source.Text == "Click 'Source' to continue" || text_destination.Text == string.Empty)
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                String query = queryGen.GenerateTheQuery(this);
                if (rtf_query.Text != "")
                    query = rtf_query.Text;


                if (encodeQueue.checkDestinationPath(text_destination.Text))
                {
                    DialogResult result = MessageBox.Show("There is already a queue item for this destination path. \n\n If you continue, the encode will be overwritten. Do you wish to continue?",
                  "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                    if (result == DialogResult.Yes)
                        encodeQueue.add(query, text_source.Text, text_destination.Text);

                }
                else
                    encodeQueue.add(query, text_source.Text, text_destination.Text);

                encodeQueue.write2disk("hb_queue_recovery.xml"); // Writes the queue to the recovery file, just incase the GUI crashes.
                queueWindow.Show();
            }
        }
        private void btn_showQueue_Click(object sender, EventArgs e)
        {
            queueWindow.Show();
        }
        private void tb_preview_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Source' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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
                    MessageBox.Show(qtpreview, "The preview window is already open!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        private void btn_ActivityWindow_Click(object sender, EventArgs e)
        {
            String file = lastAction == "scan" ? "last_scan_log.txt" : "last_encode_log.txt";

            frmActivityWindow ActivityWindow = new frmActivityWindow(file, encodeQueue, this);
            ActivityWindow.Show();
        }
        #endregion

        #region System Tray Icon
        private void frmMain_Resize(object sender, EventArgs e)
        {
            if (FormWindowState.Minimized == this.WindowState)
            {
                notifyIcon.Visible = true;
                if (!encodeQueue.isEncoding)
                {
                    notifyIcon.BalloonTipText = lbl_encode.Text != "" ? lbl_encode.Text : "Not Encoding";
                    notifyIcon.ShowBalloonTip(500);
                }
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

        //Source
        private void btn_dvd_source_Click(object sender, EventArgs e)
        {
            // Enable the creation of chapter markers.
            Check_ChapterMarkers.Enabled = true;

            // Set the last action to scan. 
            // This is used for tracking which file to load in the activity window
            lastAction = "scan";
            text_source.Text = "";

            if (DVD_Open.ShowDialog() == DialogResult.OK)
            {
                String filename = DVD_Open.SelectedPath;

                if (filename.StartsWith("\\"))
                    MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                else
                {
                    if (filename != "")
                    {
                        setupGUIforScan(filename);
                        startScan(filename);
                    }
                    else
                        text_source.Text = "Click 'Source' to continue";
                }
            }
            else
                text_source.Text = "Click 'Source' to continue";
        }
        private void btn_file_source_Click(object sender, EventArgs e)
        {
            // Set the last action to scan. 
            // This is used for tracking which file to load in the activity window
            lastAction = "scan";
            text_source.Text = "";

            if (ISO_Open.ShowDialog() == DialogResult.OK)
            {
                String filename = ISO_Open.FileName;
                if (filename.StartsWith("\\"))
                    MessageBox.Show(
                        "Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer",
                        "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                else
                {
                    if (filename != "")
                    {
                        setupGUIforScan(filename);
                        startScan(filename);
                    }
                    else
                        text_source.Text = "Click 'Source' to continue";
                }
            }
            else
                text_source.Text = "Click 'Source' to continue";
        }
        private void mnu_dvd_drive_Click(object sender, EventArgs e)
        {
            // Enable the creation of chapter markers.
            Check_ChapterMarkers.Enabled = true;

            // Set the last action to scan. 
            // This is used for tracking which file to load in the activity window
            lastAction = "scan";

            if (mnu_dvd_drive.Text.Contains("VIDEO_TS"))
            {
                string[] path = mnu_dvd_drive.Text.Split(' ');
                String filename = path[0];
                setupGUIforScan(filename);
                startScan(filename);
            }

            // If there are no titles in the dropdown menu then the scan has obviously failed. Display an error message explaining to the user.
            if (drp_dvdtitle.Items.Count == 0)
                MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source.\nYour Source may be copy protected, badly mastered or a format which HandBrake does not support. \nPlease refer to the Documentation and FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);

            lbl_encode.Text = "";
        }
        private void drp_dvdtitle_Click(object sender, EventArgs e)
        {
            if ((drp_dvdtitle.Items.Count == 1) && (drp_dvdtitle.Items[0].ToString() == "Automatic"))
                MessageBox.Show("There are no titles to select. Please load a source file by clicking the 'Source' button above before trying to select a title.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
        }
        private void drp_dvdtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Reset some values on the form
            lbl_Aspect.Text = "Select a Title";
            //lbl_RecomendedCrop.Text = "Select a Title";
            drop_chapterStart.Items.Clear();
            drop_chapterFinish.Items.Clear();

            // If the dropdown is set to automatic nothing else needs to be done.
            // Otheriwse if its not, title data has to be loased from parsing.
            if (drp_dvdtitle.Text != "Automatic")
            {
                selectedTitle = drp_dvdtitle.SelectedItem as Parsing.Title;

                // Set the Aspect Ratio
                lbl_Aspect.Text = selectedTitle.AspectRatio.ToString();
                lbl_src_res.Text = selectedTitle.Resolution.Width + " x " + selectedTitle.Resolution.Height;
                lbl_duration.Text = selectedTitle.Duration.ToString();

                // Set the Recommended Cropping values
                text_top.Text = selectedTitle.AutoCropDimensions[0].ToString();
                text_bottom.Text = selectedTitle.AutoCropDimensions[1].ToString();
                text_left.Text = selectedTitle.AutoCropDimensions[2].ToString();
                text_right.Text = selectedTitle.AutoCropDimensions[3].ToString();

                // Populate the Angles dropdown
                drop_angle.Items.Clear();
                if (Properties.Settings.Default.dvdnav == "Checked")
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
                drp_track1Audio.Items.Clear();
                drp_track1Audio.Items.Add("Automatic");
                drp_track1Audio.Items.Add("None");
                drp_track1Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
                drp_track1Audio.SelectedIndex = 0;

                // Populate the Subtitles dropdown
                drp_subtitle.Items.Clear();
                drp_subtitle.Items.Add("None");
                drp_subtitle.Items.Add("Autoselect");
                drp_subtitle.Items.AddRange(selectedTitle.Subtitles.ToArray());
                if (drp_subtitle.Items.Count > 0)
                    drp_subtitle.Text = drp_subtitle.Items[0].ToString();

            }

            // Run the autoName & chapterNaming functions
            if (Properties.Settings.Default.autoNaming == "Checked")
            {
                string autoPath = Main.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);
                if (autoPath != null)
                    text_destination.Text = autoPath;
                else
                    MessageBox.Show("You currently have automatic file naming enabled for the destination box, but you do not have a default direcotry set. You should set this in the program options (see Tools Menu)", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }

            data_chpt.Rows.Clear();
            if (selectedTitle.Chapters.Count != 1)
            {
                DataGridView chapterGridView = Main.chapterNaming(data_chpt, drop_chapterFinish.Text);
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
        }
        private void drop_chapterStart_SelectedIndexChanged(object sender, EventArgs e)
        {
            int c_start, c_end;

            if (drop_chapterFinish.Text == "Auto" && drop_chapterFinish.Items.Count != 0)
                drop_chapterFinish.SelectedIndex = drop_chapterFinish.Items.Count - 1;

            int.TryParse(drop_chapterStart.Text, out c_start);
            int.TryParse(drop_chapterFinish.Text, out c_end);

            if (c_end != 0)
            {
                if (c_start > c_end)
                    drop_chapterFinish.Text = c_start.ToString();
            }

            lbl_duration.Text = Main.calculateDuration(drop_chapterStart.Text, drop_chapterFinish.Text, selectedTitle).ToString();

            // Run the Autonaming function
            if (Properties.Settings.Default.autoNaming == "Checked")
                text_destination.Text = Main.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);

            // Disable chapter markers if only 1 chapter is selected.
            if (c_start == c_end)
            {
                Check_ChapterMarkers.Checked = false;
                Check_ChapterMarkers.Enabled = false;
            }
            else
                Check_ChapterMarkers.Enabled = true;
        }
        private void drop_chapterFinish_SelectedIndexChanged(object sender, EventArgs e)
        {
            int c_start, c_end;

            if (drop_chapterStart.Text == "Auto" && drop_chapterStart.Items.Count >= 1)
                drop_chapterStart.SelectedIndex = 1;

            int.TryParse(drop_chapterStart.Text, out c_start);
            int.TryParse(drop_chapterFinish.Text, out c_end);

            if (c_start != 0)
            {
                if (c_end < c_start)
                    drop_chapterFinish.Text = c_start.ToString();
            }

            lbl_duration.Text = Main.calculateDuration(drop_chapterStart.Text, drop_chapterFinish.Text, selectedTitle).ToString();

            // Run the Autonaming function
            if (Properties.Settings.Default.autoNaming == "Checked")
                text_destination.Text = Main.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);

            // Add more rows to the Chapter menu if needed.
            if (Check_ChapterMarkers.Checked)
            {
                int i = data_chpt.Rows.Count, finish = 0;

                if (drop_chapterFinish.Text != "Auto")
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

            // Disable chapter markers if only 1 chapter is selected.
            if (c_start == c_end)
            {
                Check_ChapterMarkers.Checked = false;
                Check_ChapterMarkers.Enabled = false;
            }
            else
                Check_ChapterMarkers.Enabled = true;
        }

        //Destination
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
            else if (drop_format.SelectedIndex.Equals(2))
                DVD_Save.FilterIndex = 3;

            if (DVD_Save.ShowDialog() == DialogResult.OK)
            {
                if (DVD_Save.FileName.StartsWith("\\"))
                    MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                else
                {
                    // Add a file extension manually, as FileDialog.AddExtension has issues with dots in filenames
                    switch (DVD_Save.FilterIndex)
                    {
                        case 1:
                            if (!Path.GetExtension(DVD_Save.FileName).Equals(".mp4", StringComparison.InvariantCultureIgnoreCase))
                                DVD_Save.FileName += ".mp4";
                            break;
                        case 2:
                            if (!Path.GetExtension(DVD_Save.FileName).Equals(".m4v", StringComparison.InvariantCultureIgnoreCase))
                                DVD_Save.FileName += ".m4v";
                            break;
                        case 3:
                            if (!Path.GetExtension(DVD_Save.FileName).Equals(".mkv", StringComparison.InvariantCultureIgnoreCase))
                                DVD_Save.FileName += ".mkv";
                            break;
                        default:
                            //do nothing  
                            break;
                    }
                    text_destination.Text = DVD_Save.FileName;

                    // Quicktime requires .m4v file for chapter markers to work. If checked, change the extension to .m4v (mp4 and m4v are the same thing)
                    if (Check_ChapterMarkers.Checked)
                        drop_format.SelectedIndex = 1;
                }
            }
        }
        private void text_destination_TextChanged(object sender, EventArgs e)
        {
            string path = text_destination.Text;
            if (path.EndsWith(".mp4"))
                drop_format.SelectedIndex = 0;
            else if (path.EndsWith(".m4v"))
                drop_format.SelectedIndex = 1;
            else if (path.EndsWith(".mkv"))
                drop_format.SelectedIndex = 2;
        }

        // Output Settings
        private void drop_format_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drop_format.SelectedIndex == 0)
                setExtension(".mp4");
            else if (drop_format.SelectedIndex == 1)
                setExtension(".m4v");
            else if (drop_format.SelectedIndex == 2)
                setExtension(".mkv");

            setAudioByContainer(drop_format.Text);
            setVideoByContainer(drop_format.Text);
        }
        private void setExtension(string newExtension)
        {
            text_destination.Text = text_destination.Text.Replace(".mp4", newExtension);
            text_destination.Text = text_destination.Text.Replace(".m4v", newExtension);
            text_destination.Text = text_destination.Text.Replace(".mkv", newExtension);
        }

        //Video Tab
        private void drp_videoEncoder_SelectedIndexChanged(object sender, EventArgs e)
        {
            setContainerOpts();

            //Turn off some options which are H.264 only when the user selects a non h.264 encoder
            if (drp_videoEncoder.Text.Contains("H.264"))
            {
                if (check_2PassEncode.CheckState == CheckState.Checked)
                    check_turbo.Enabled = true;

                h264Tab.Enabled = true;
                if ((drop_format.Text.Contains("MP4")) || (drop_format.Text.Contains("M4V")))
                    check_iPodAtom.Enabled = true;
                else
                    check_iPodAtom.Enabled = false;
            }
            else
            {
                check_turbo.CheckState = CheckState.Unchecked;
                check_turbo.Enabled = false;
                h264Tab.Enabled = false;
                x264Panel.x264Query = "";
                check_iPodAtom.Enabled = false;
                check_iPodAtom.Checked = false;
            }

            // Setup the CQ Slider
            switch (drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    slider_videoQuality.Minimum = 1;
                    slider_videoQuality.Maximum = 31;
                    slider_videoQuality.Value = 1;
                    SliderValue.Text = "0% QP: 31.00";
                    break;
                case "H.264 (x264)":
                    slider_videoQuality.Minimum = 0;
                    slider_videoQuality.Value = 0;
                    slider_videoQuality.TickFrequency = 1;
                    SliderValue.Text = "0% RF: 51.00";
                    String step = Properties.Settings.Default.x264cqstep;
                    switch (step)
                    {
                        case "0.20":
                            slider_videoQuality.Maximum = 255;
                            break;
                        case "0.25":
                            slider_videoQuality.Maximum = 204;
                            break;
                        case "0.33":
                            slider_videoQuality.Maximum = 155;
                            break;
                        case "0.50":
                            slider_videoQuality.Maximum = 102;
                            break;
                        case "1.0":
                            slider_videoQuality.Maximum = 51;
                            break;
                        default:
                            slider_videoQuality.Maximum = 51;
                            break;
                    }
                    break;
                case "VP3 (Theora)":
                    slider_videoQuality.Minimum = 0;
                    slider_videoQuality.Maximum = 63;
                    slider_videoQuality.Value = 0;
                    SliderValue.Text = "0% QP: 0.00";
                    break;
            }
        }
        /// <summary>
        /// Set the container format options
        /// </summary>
        public void setContainerOpts()
        {
            if ((text_destination.Text.Contains(".mp4")) || (text_destination.Text.Contains(".m4v")))
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
        private void slider_videoQuality_Scroll(object sender, EventArgs e)
        {
            switch (drp_videoEncoder.Text)
            {
                case "MPEG-4 (FFmpeg)":
                    double rfValue = 31 - (slider_videoQuality.Value - 1);
                    double max = slider_videoQuality.Maximum;
                    double min = slider_videoQuality.Minimum;
                    double val = ((max - min) - (rfValue - min)) / (max - min);
                    SliderValue.Text = Math.Round((val * 100), 2) + "% QP:" + (32 - slider_videoQuality.Value);
                    break;
                case "H.264 (x264)":
                    double divided;
                    System.Globalization.CultureInfo culture = System.Globalization.CultureInfo.CreateSpecificCulture("en-US");
                    double.TryParse(Properties.Settings.Default.x264cqstep,
                                    System.Globalization.NumberStyles.Number,
                                    culture,
                                    out divided);
                    rfValue = 51.0 - slider_videoQuality.Value * divided;
                    max = slider_videoQuality.Maximum * divided;
                    min = slider_videoQuality.Minimum;
                    val = ((max - min) - (rfValue - min)) / (max - min);
                    rfValue = Math.Round(rfValue, 2);
                    SliderValue.Text = Math.Round((val * 100), 2) + "% RF:" + rfValue;
                    break;
                case "VP3 (Theora)":
                    rfValue = slider_videoQuality.Value;
                    double value = rfValue / 63;
                    SliderValue.Text = Math.Round((value * 100), 2) + "% QP:" + slider_videoQuality.Value;
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

        //Picture Tab
        private void text_width_TextChanged(object sender, EventArgs e)
        {
            if (text_width.Text == "")
                text_width.BackColor = Color.White;

            maxWidth = 0; maxHeight = 0;  // Reset max width so that it's not using the MaxWidth -X. Quick hack to allow -X for preset usage.
            lbl_max.Text = "";

            int width;
            Boolean parsed = int.TryParse(text_width.Text, out width);
            if (parsed)
            {
                text_width.BackColor = (width % 16) != 0 ? Color.LightCoral : Color.LightGreen;

                if (lbl_Aspect.Text != "Select a Title" && maxWidth == 0 && maxHeight == 0)
                {
                    if (drp_anamorphic.Text == "None")
                    {
                        int height = Main.cacluateNonAnamorphicHeight(width, text_top.Value, text_bottom.Value, text_left.Value, text_right.Value, selectedTitle);
                        if (height != 0)
                            text_height.Text = height.ToString();
                    }
                }
            }
        }
        private void text_height_TextChanged(object sender, EventArgs e)
        {
            if (text_height.Text == "")
                text_height.BackColor = Color.White;

            maxHeight = 0;  // Reset max height so that it's not using the MaxHeight -Y. Quick hack to allow -Y for preset usage.
            if (maxWidth != 0)
                lbl_max.Text = "Max Width";
            else
                lbl_max.Text = "";

            int height;
            Boolean parsed = int.TryParse(text_height.Text, out height);
            if (parsed)
                text_height.BackColor = (height % 16) != 0 ? Color.LightCoral : Color.LightGreen;
        }
        private void check_customCrop_CheckedChanged(object sender, EventArgs e)
        {
            text_left.Enabled = true;
            text_right.Enabled = true;
            text_top.Enabled = true;
            text_bottom.Enabled = true;
            if (selectedTitle != null)
            {
                text_top.Text = selectedTitle.AutoCropDimensions[0].ToString();
                text_bottom.Text = selectedTitle.AutoCropDimensions[1].ToString();
                text_left.Text = selectedTitle.AutoCropDimensions[2].ToString();
                text_right.Text = selectedTitle.AutoCropDimensions[3].ToString();
            }
            else
            {
                text_left.Text = "0";
                text_right.Text = "0";
                text_top.Text = "0";
                text_bottom.Text = "0";
            }
        }
        private void check_autoCrop_CheckedChanged(object sender, EventArgs e)
        {
            text_left.Enabled = false;
            text_right.Enabled = false;
            text_top.Enabled = false;
            text_bottom.Enabled = false;
        }
        private void drp_anamorphic_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_anamorphic.SelectedIndex == 1)
            {
                text_height.BackColor = Color.LightGray;
                text_width.BackColor = Color.LightGray;
                text_height.Text = "";
                text_width.Text = "";
                text_height.Enabled = false;
                text_width.Enabled = false;
            }

            if (drp_anamorphic.SelectedIndex == 2)
            {
                text_height.Text = "";
                text_height.Enabled = false;
                text_height.BackColor = Color.LightGray;

                text_width.Enabled = true;
                text_width.BackColor = Color.White;
            }

            if (drp_anamorphic.SelectedIndex == 0)
            {
                text_height.BackColor = Color.White;
                text_width.BackColor = Color.White;
                text_height.Enabled = true;
                text_width.Enabled = true;
            }
        }

        // Filter Tab
        private void ctl_decomb_changed(object sender, EventArgs e)
        {
            if (ctl_decomb.getDropValue != "Off")
                if (ctl_deinterlace.getDropValue != "None")
                    ctl_deinterlace.setOption("None");
        }
        private void ctl_deinterlace_changed(object sender, EventArgs e)
        {
            if (ctl_detelecine.getDropValue != "None")
                if (ctl_decomb.getDropValue != "Off")
                    ctl_decomb.setOption("Off");
        }
        private void slider_deblock_Scroll(object sender, EventArgs e)
        {
            lbl_deblockVal.Text = slider_deblock.Value == 4 ? "Off" : slider_deblock.Value.ToString();
        }

        //Audio Tab
        private void drp_track1Audio_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].Text = drp_track1Audio.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audenc_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_audenc_1.Text.Contains("AC3") || drp_audenc_1.Text.Contains("DTS"))
            {
                drp_audmix_1.Enabled = false;
                drp_audbit_1.Enabled = false;
                drp_audsr_1.Enabled = false;

                drp_audmix_1.SelectedIndex = 0;
                drp_audbit_1.SelectedIndex = 0;
                drp_audsr_1.SelectedIndex = 0;
            }
            else
            {
                drp_audmix_1.Enabled = true;
                drp_audbit_1.Enabled = true;
                drp_audsr_1.Enabled = true;

                drp_audmix_1.Text = "Automatic";
                drp_audbit_1.Text = "160";
                drp_audsr_1.Text = "Auto";
            }

            if (drp_audenc_1.Text.Contains("AAC"))
            {
                setMixDownAllOptions(drp_audmix_1);
                setBitrateSelections160(drp_audbit_1);
            }
            else
            {
                setMixDownNotAAC(drp_audmix_1);
                setBitrateSelections320(drp_audbit_1);
            }

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text = drp_audenc_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audmix_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((drp_audenc_1.Text.Contains("AAC")) && (drp_audmix_1.Text == "6 Channel Discrete"))
                setBitrateSelections384(drp_audbit_1);
            else if ((drp_audenc_1.Text.Contains("AAC")) && (drp_audmix_1.Text != "6 Channel Discrete"))
                setBitrateSelections160(drp_audbit_1); drp_audbit_1.Text = "160";

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text = drp_audmix_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audsr_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text = drp_audsr_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audbit_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                if (drp_audenc_1.Text.Contains("AC3"))
                    drp_audbit_1.Text = "Auto";
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text = drp_audbit_1.Text;
                lv_audioList.Select();
            }
        }
        private void tb_drc_Scroll(object sender, EventArgs e)
        {
            double value;
            if (tb_drc.Value == 0) value = 0;
            else
                value = ((tb_drc.Value - 1) / 10.0) + 1;

            lbl_drc.Text = value.ToString();

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text = lbl_drc.Text;
                lv_audioList.Select();
            }
        }

        private void btn_addAudioTrack_Click(object sender, EventArgs e)
        {
            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            ListViewItem newTrack = new ListViewItem(drp_track1Audio.Text);
            newTrack.SubItems.Add(drp_audenc_1.Text);
            newTrack.SubItems.Add(drp_audmix_1.Text);
            newTrack.SubItems.Add(drp_audsr_1.Text);
            newTrack.SubItems.Add(drp_audbit_1.Text);
            newTrack.SubItems.Add(lbl_drc.Text);

            // Select the newly added track and select the control
            lv_audioList.Items.Add(newTrack);
            lv_audioList.Items[lv_audioList.Items.Count - 1].Selected = true;
            lv_audioList.Select();
        }
        private void btn_RemoveAudioTrack_Click(object sender, EventArgs e)
        {
            removeAudioTrack();
        }
        private void audioList_moveup_Click(object sender, EventArgs e)
        {
            if (lv_audioList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_audioList.SelectedItems[0];
                int index = item.Index;
                index--;

                if (lv_audioList.Items.Count > index && index >= 0)
                {
                    lv_audioList.Items.Remove(item);
                    lv_audioList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_audioList.Focus();
                }
            }
        }
        private void audioList_movedown_Click(object sender, EventArgs e)
        {
            if (lv_audioList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_audioList.SelectedItems[0];
                int index = item.Index;
                index++;

                if (index < lv_audioList.Items.Count)
                {
                    lv_audioList.Items.Remove(item);
                    lv_audioList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_audioList.Focus();
                }
            }

        }
        private void audioList_remove_Click(object sender, EventArgs e)
        {
            removeAudioTrack();
        }
        private void removeAudioTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_audioList.SelectedItems.Count != 0)
            {
                // Record the current selected index.
                int currentPosition = lv_audioList.SelectedIndices[0];

                lv_audioList.Items.RemoveAt(lv_audioList.SelectedIndices[0]);

                // Now reslect the correct item and give focus to the audio list.
                if (lv_audioList.Items.Count != 0)
                {
                    if (currentPosition <= (lv_audioList.Items.Count - 1))
                        lv_audioList.Items[currentPosition].Selected = true;
                    else if (currentPosition > (lv_audioList.Items.Count - 1))
                        lv_audioList.Items[lv_audioList.Items.Count - 1].Selected = true;

                    lv_audioList.Select();
                }
            }
        }
        private void lv_audioList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the Audio List.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                drp_track1Audio.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].Text;
                drp_audenc_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text;
                drp_audmix_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text;
                drp_audsr_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text;
                drp_audbit_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text;
                double drcValue; int drcCalculated;
                double.TryParse(lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text, out drcValue);
                if (drcValue == 0) drcCalculated = 0;
                else
                    drcValue = ((drcValue * 10) + 1) - 10;
                int.TryParse(drcValue.ToString(), out drcCalculated);
                tb_drc.Value = drcCalculated;
            }
        }

        private void drp_subtitle_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_subtitle.Text.Contains("None"))
            {
                check_forced.Enabled = false;
                check_forced.Checked = false;
            }
            else
                check_forced.Enabled = true;
        }

        // Chapter Marker Tab
        private void Check_ChapterMarkers_CheckedChanged(object sender, EventArgs e)
        {
            if (Check_ChapterMarkers.Checked)
            {
                text_destination.Text = text_destination.Text.Replace(".m4v", ".mp4");
                data_chpt.Rows.Clear();
                data_chpt.Enabled = true;
                DataGridView chapterGridView = Main.chapterNaming(data_chpt, drop_chapterFinish.Text);
                if (chapterGridView != null)
                    data_chpt = chapterGridView;
            }
            else
            {
                text_destination.Text = text_destination.Text.Replace(".m4v", ".mp4");
                data_chpt.Rows.Clear();
                data_chpt.Enabled = false;
            }
        }

        // Query Editor Tab
        private void btn_generate_Query_Click(object sender, EventArgs e)
        {
            rtf_query.Text = queryGen.GenerateTheQuery(this);
        }
        private void btn_clear_Click(object sender, EventArgs e)
        {
            rtf_query.Clear();
        }
        #endregion

        // MainWindow Components, Actions and Functions ***********************

        #region Source Scan
        public Boolean isScanning { get; set; }
        private static int scanProcessID { get; set; }
        private void setupGUIforScan(String filename)
        {
            text_source.Text = filename;

            foreach (Control ctrl in Controls)
            {
                if (!(ctrl is StatusStrip || ctrl is MenuStrip || ctrl is ToolStrip))
                    ctrl.Enabled = false;
            }
            lbl_encode.Visible = true;
            lbl_encode.Text = "Scanning ...";
            gb_source.Text = "Source: Scanning ...";
            btn_source.Enabled = false;
            btn_start.Enabled = false;
            btn_showQueue.Enabled = false;
            btn_add2Queue.Enabled = false;
            tb_preview.Enabled = false;
            mnu_killCLI.Visible = true;
        }
        private void startScan(String filename)
        {
            try
            {
                lbl_encode.Visible = true;
                lbl_encode.Text = "Scanning...";
                isScanning = true;
                ThreadPool.QueueUserWorkItem(scanProcess, filename);
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - startScan " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void scanProcess(object state)
        {
            try
            {
                string inputFile = (string)state;
                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(logDir, "last_scan_log.txt");

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                    File.Delete(dvdInfoPath);

                String dvdnav = string.Empty;
                if (Properties.Settings.Default.dvdnav == "Checked")
                    dvdnav = " --dvdnav";
                string strCmdLine = String.Format(@"cmd /c """"{0}"" -i ""{1}"" -t0 {2} -v >""{3}"" 2>&1""", handbrakeCLIPath, inputFile, dvdnav, dvdInfoPath);

                ProcessStartInfo hbParseDvd = new ProcessStartInfo("CMD.exe", strCmdLine) { WindowStyle = ProcessWindowStyle.Hidden };

                Boolean cleanExit = true;
                using (hbproc = Process.Start(hbParseDvd))
                {
                    Process[] before = Process.GetProcesses(); // Get a list of running processes before starting.
                    scanProcessID = Main.getCliProcess(before); 
                    hbproc.WaitForExit();
                    if (hbproc.ExitCode != 0)
                        cleanExit = false;
                }

                if (cleanExit) // If 0 exit code, CLI exited cleanly.
                {
                    if (!File.Exists(dvdInfoPath))
                    {
                        throw new Exception(
                            "Unable to retrieve the DVD Info. last_scan_log.txt is missing. \nExpected location of last_scan_log.txt: \n" +
                            dvdInfoPath);
                    }

                    using (StreamReader sr = new StreamReader(dvdInfoPath))
                    {
                        thisDVD = DVD.Parse(sr);
                        sr.Close();
                        sr.Dispose();
                    }

                    updateUIafterScan();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - scanProcess() " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                enableGUI();
            }
        }
        private void updateUIafterScan()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(updateUIafterScan));
                    return;
                }

                // Setup some GUI components
                drp_dvdtitle.Items.Clear();
                if (thisDVD.Titles.Count != 0)
                    drp_dvdtitle.Items.AddRange(thisDVD.Titles.ToArray());
                drp_dvdtitle.Text = "Automatic";
                drop_chapterFinish.Text = "Auto";
                drop_chapterStart.Text = "Auto";

                // Now select the longest title
                if (thisDVD.Titles.Count != 0)
                    drp_dvdtitle.SelectedItem = Main.selectLongestTitle(drp_dvdtitle);

                // Enable the creation of chapter markers if the file is an image of a dvd.
                if (text_source.Text.ToLower().Contains(".iso") || text_source.Text.ToLower().Contains("VIDEO_TS"))
                    Check_ChapterMarkers.Enabled = true;
                else
                {
                    Check_ChapterMarkers.Enabled = false;
                    Check_ChapterMarkers.Checked = false;
                    data_chpt.Rows.Clear();
                }

                // If no titles were found, Display an error message
                if (drp_dvdtitle.Items.Count == 0)
                    MessageBox.Show("No Title(s) found. \n\nYour Source may be copy protected, badly mastered or a format which HandBrake does not support. \nPlease refer to the Documentation and FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);

                // Enable the GUI components and enable any disabled components
                enableGUI();
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - updateUIafterScan " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                enableGUI();
            }
        }
        private void enableGUI()
        {
            try
            {
                if (InvokeRequired)
                    BeginInvoke(new UpdateWindowHandler(enableGUI));
                lbl_encode.Text = "Scan Completed";
                gb_source.Text = "Source";
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
                MessageBox.Show("frmMain.cs - enableGUI() " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void killScan()
        {
            try
            {
                enableGUI();
                resetGUI();

                Process[] prs = Process.GetProcesses();
                foreach (Process process in prs)
                {
                    if (process.Id == scanProcessID)
                    {
                        process.Refresh();
                        if (!process.HasExited)
                            process.Kill();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Unable to kill HandBrakeCLI.exe \nYou may need to manually kill HandBrakeCLI.exe using the Windows Task Manager if it does not close automatically within the next few minutes. \n\nError Information: \n" + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void resetGUI()
        {
            drp_dvdtitle.Items.Clear();
            drp_dvdtitle.Text = "Automatic";
            drop_chapterStart.Items.Clear();
            drop_chapterStart.Text = "Auto";
            drop_chapterFinish.Items.Clear();
            drop_chapterFinish.Text = "Auto";
            lbl_duration.Text = "Select a Title";
            lbl_src_res.Text = "Select a Title";
            lbl_Aspect.Text = "Select a Title";
            text_source.Text = "Click 'Source' to continue";
            text_destination.Text = "";
            thisDVD = null;
            selectedTitle = null;
            isScanning = false;
        }
        #endregion

        #region GUI
        /// <summary>
        /// Set the GUI to it's finished encoding state.
        /// </summary>
        private void setEncodeFinished()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(setEncodeFinished));
                    return;
                }

                lbl_encode.Text = "Encoding Finished";
                btn_start.Text = "Start";
                btn_start.ToolTipText = "Start the encoding process";
                btn_start.Image = Properties.Resources.Play;

                // If the window is minimized, display the notification in a popup.
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
        private void setEncodeStarted()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateWindowHandler(setEncodeStarted));
                    return;
                }

                lbl_encode.Visible = true;
                lbl_encode.Text = "Encoding in Progress";
                btn_start.Text = "Stop";
                btn_start.ToolTipText = "Stop the encoding process. \nWarning: This may break your file. Press ctrl-c in the CLI window if you wish it to exit cleanly.";
                btn_start.Image = Properties.Resources.stop;
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }
        #endregion

        #region DVD Drive Detection
        private delegate void ProgressUpdateHandler();
        private void getDriveInfoThread()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new ProgressUpdateHandler(getDriveInfoThread));
                    return;
                }

                Boolean foundDrive = false;
                DriveInfo[] theCollectionOfDrives = DriveInfo.GetDrives();
                foreach (DriveInfo curDrive in theCollectionOfDrives)
                {
                    if (curDrive.DriveType == DriveType.CDRom && curDrive.IsReady)
                    {
                        if (File.Exists(curDrive.RootDirectory + "VIDEO_TS\\VIDEO_TS.IFO"))
                        {
                            mnu_dvd_drive.Text = curDrive.RootDirectory + "VIDEO_TS (" + curDrive.VolumeLabel + ")";
                            foundDrive = true;
                            break;
                        }
                    }
                }

                if (foundDrive == false)
                    mnu_dvd_drive.Text = "[No DVD Drive Ready]";
            }
            catch (Exception)
            {
                mnu_dvd_drive.Text = "[No DVD Drive Ready / Found]";
            }
        }
        #endregion

        #region Audio Panel Code Helpers
        private void setAudioByContainer(String path)
        {
            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                string oldval = drp_audenc_1.Text;
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC (faac)");
                drp_audenc_1.Items.Add("AC3 Passthru");
                if ((oldval != "AAC (faac)") && (oldval != "AC3 Passthru"))
                    drp_audenc_1.SelectedIndex = 0;

            }
            else if (path.Contains("MKV"))
            {
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC (faac)");
                drp_audenc_1.Items.Add("MP3 (lame)");
                drp_audenc_1.Items.Add("AC3 Passthru");
                drp_audenc_1.Items.Add("DTS Passthru");
                drp_audenc_1.Items.Add("Vorbis (vorbis)");

                if (drp_audenc_1.Text == string.Empty)
                    drp_audenc_1.SelectedIndex = 0;
            }

            // Make sure the table is updated with new audio codecs
            foreach (ListViewItem row in lv_audioList.Items)
            {
                if (!drp_audenc_1.Items.Contains(row.SubItems[1].Text))
                    row.SubItems[1].Text = drp_audenc_1.Items[0].ToString();
            }
        }
        private void setVideoByContainer(String path)
        {
            string oldval;

            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("H.264 (x264)");
                if (oldval == "VP3 (Theora)")
                    drp_videoEncoder.SelectedIndex = 1;
                else
                    drp_videoEncoder.Text = oldval;

            }
            else if (path.Contains("MKV"))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("H.264 (x264)");
                drp_videoEncoder.Items.Add("VP3 (Theora)");
                drp_videoEncoder.Text = oldval;
            }
        }
        private static void setBitrateSelections384(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
            dropDown.Items.Add("192");
            dropDown.Items.Add("224");
            dropDown.Items.Add("256");
            dropDown.Items.Add("320");
            dropDown.Items.Add("384");
        }
        private static void setBitrateSelections320(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
            dropDown.Items.Add("192");
            dropDown.Items.Add("224");
            dropDown.Items.Add("256");
            dropDown.Items.Add("320");
        }
        private static void setBitrateSelections160(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
        }
        private static void setMixDownAllOptions(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Mono");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
            dropdown.Items.Add("6 Channel Discrete");
        }
        private static void setMixDownNotAAC(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Access the preset Handler and setup the preset panel.
        /// </summary>
        public void loadPresetPanel()
        {
            if (presetHandler.checkIfPresetsAreOutOfDate())
                if (Properties.Settings.Default.presetNotification == "Unchecked")
                    MessageBox.Show(
                    "HandBrake has determined your built-in presets are out of date... These presets will now be updated.",
                    "Preset Update", MessageBoxButtons.OK, MessageBoxIcon.Information);

            presetHandler.getPresetPanel(ref treeView_presets);
            treeView_presets.Update();
        }

        /// <summary>
        /// Either Encode or Scan was last performed.
        /// </summary>
        public string lastAction { get; set; }
        #endregion

        #region overrides
        /// <summary>
        /// If the queue is being processed, prompt the user to confirm application close.
        /// </summary>
        /// <param name="e"></param>
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            // If currently encoding, the queue isn't paused, and there are queue items to process, prompt to confirm close.
            if ((encodeQueue.isEncoding) && (!encodeQueue.isPaused) && (encodeQueue.count() > 0))
            {
                DialogResult result = MessageBox.Show("HandBrake has queue items to process. Closing HandBrake will not stop the current encoding, but will stop processing the queue.\n\nDo you want to close HandBrake?",
                    "Close HandBrake?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.No)
                    e.Cancel = true;
            }
            base.OnFormClosing(e);
        }
        #endregion

        #region In-GUI Encode Status (Experimental)
        private Process HBProcess { get; set; }

        private void encodeMonitorThread()
        {
            try
            {
                Parser encode = new Parser(HBProcess.StandardOutput.BaseStream);
                encode.OnEncodeProgress += encode_OnEncodeProgress;
                while (!encode.EndOfStream)
                {
                    encode.readEncodeStatus();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private void encode_OnEncodeProgress(object Sender, int CurrentTask, int TaskCount, float PercentComplete, float CurrentFps, float AverageFps, TimeSpan TimeRemaining)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new EncodeProgressEventHandler(encode_OnEncodeProgress),
                    new object[] { Sender, CurrentTask, TaskCount, PercentComplete, CurrentFps, AverageFps, TimeRemaining });
                return;
            }
            lbl_encode.Text = string.Format("Encode Progress: {0}%,       FPS: {1},       Avg FPS: {2},       Time Remaining: {3} ", PercentComplete, CurrentFps, AverageFps, TimeRemaining);
        }
        #endregion


        // This is the END of the road ****************************************
    }
}