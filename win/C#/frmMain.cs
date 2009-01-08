/*  frmMain.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace Handbrake
{
    public partial class frmMain : Form
    {
        // Objects which may be used by one or more other objects
        private delegate void UpdateWindowHandler();
        Functions.Main hb_common_func = new Functions.Main();
        Functions.Encode encodeHandler = new Functions.Encode();
        Queue.QueueHandler encodeQueue = new Queue.QueueHandler();
        Presets.PresetsHandler presetHandler = new Presets.PresetsHandler();
        Parsing.Title selectedTitle;
        Parsing.DVD thisDVD;

        // Objects belonging to this window only
        PresetLoader presetLoader = new PresetLoader();
        x264Panel x264PanelFunctions = new x264Panel();
        QueryGenerator queryGen = new QueryGenerator();

        // Globals: Mainly used for tracking.
        private frmQueue queueWindow;
        private frmGenPreview vlcpreview;
        private frmPreview qtpreview;
        private string lastAction = null;
        public int maxWidth = 0;
        public int maxHeight = 0;

        // Applicaiton Startup ************************************************

        #region Application Startup

        public frmMain()
        {
            // Load the splash screen in this thread
            Form splash = new frmSplashScreen();
            splash.Show();

            //Create a label that can be updated from the parent thread.
            Label lblStatus = new Label();
            lblStatus.Size = new Size(250, 20);
            lblStatus.Location = new Point(10, 280);
            splash.Controls.Add(lblStatus);
            InitializeComponent();

            // Update the users config file with the CLI version data.
            lblStatus.Text = "Setting Version Data ...";
            Application.DoEvents();
            ArrayList x = hb_common_func.getCliVersionData();
            Properties.Settings.Default.hb_build = int.Parse(x[1].ToString());
            Properties.Settings.Default.hb_version = x[0].ToString();

            // show the form, but leave disabled until preloading is complete then show the main form
            this.Enabled = false;
            this.Show();
            Application.DoEvents(); // Forces frmMain to draw

            // update the status
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
            x264PanelFunctions.reset2Defaults(this); // Initialize all the x264 widgets to their default values
            loadPresetPanel();                       // Load the Preset Panel
            treeView_presets.ExpandAll();
            lbl_encode.Text = "";
            queueWindow = new frmQueue(encodeQueue);        // Prepare the Queue
            if (Properties.Settings.Default.QueryEditorTab != "Checked")
                tabs_panel.TabPages.RemoveAt(5); // Remove the query editor tab if the user does not want it enabled.

            // Load the user's default settings or Normal Preset
            if (Properties.Settings.Default.defaultSettings == "Checked" && Properties.Settings.Default.defaultPreset != "")
            {
                // Ok, so, we've selected a preset. Now we want to load it.
                if (presetHandler.getPreset(Properties.Settings.Default.defaultPreset) != null)
                {
                    string query = presetHandler.getPreset(Properties.Settings.Default.defaultPreset).Query;
                    Boolean loadPictureSettings = presetHandler.getPreset(Properties.Settings.Default.defaultPreset).PictureSettings;

                    if (query != null)
                    {
                        //Ok, Reset all the H264 widgets before changing the preset
                        x264PanelFunctions.reset2Defaults(this);

                        // Send the query from the file to the Query Parser class, then load the preset
                        Functions.QueryParser presetQuery = Functions.QueryParser.Parse(query);
                        presetLoader.presetLoader(this, presetQuery, Properties.Settings.Default.defaultPreset, loadPictureSettings);

                        // The x264 widgets will need updated, so do this now:
                        x264PanelFunctions.X264_StandardizeOptString(this);
                        x264PanelFunctions.X264_SetCurrentSettingsInPanel(this);
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

            // Event Handlers
            events();

            // Queue Recovery
            queueRecovery();
        }

        // Startup Functions
        private delegate void updateStatusChanger();
        private void startupUpdateCheck()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new updateStatusChanger(startupUpdateCheck));
                    return;
                }

                Boolean update = hb_common_func.updateCheck(false);
                if (update == true)
                {
                    frmUpdater updateWindow = new frmUpdater();
                    updateWindow.Show();
                }
            }
            catch (Exception) { /* Do Nothing*/ }
        }
        private void queueRecovery()
        {
            if (hb_common_func.check_queue_recovery() == true)
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

            // Handle Encode Start
            encodeQueue.OnEncodeEnded += new EventHandler(encodeEnded);
            encodeQueue.OnPaused += new EventHandler(encodePaused);
            encodeQueue.OnEncodeStart += new EventHandler(encodeStarted);
        }
        private void encodeStarted(object sender, EventArgs e)
        {
            setLastAction("encode");
            setEncodeStarted();
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
        private void mnu_exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }
        #endregion

        #region Tools Menu
        private void mnu_encode_Click(object sender, EventArgs e)
        {
            queueWindow.setQueue();
            queueWindow.Show();
        }
        private void mnu_encodeLog_Click(object sender, EventArgs e)
        {
            String file = String.Empty;
            if (lastAction == "scan")
                file = "dvdinfo.dat";
            else
                file = "hb_encode_log.dat";

            frmActivityWindow dvdInfoWindow = new frmActivityWindow(file, encodeHandler);
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
            string presetsFile = Application.StartupPath.ToString() + "\\presets.xml";
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
                MessageBox.Show("An error has occured during the preset removal process.\n If you are using Windows Vista, you may need to run under Administrator Mode to complete this task. \n" + exc.ToString());
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
            Boolean update = hb_common_func.updateCheck(true);
            if (update == true)
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
                presetHandler.updatePreset(treeView_presets.SelectedNode.Text, queryGen.generateTabbedComponentsQuery(this), true);
            else if (result == DialogResult.No)
                presetHandler.updatePreset(treeView_presets.SelectedNode.Text, queryGen.generateTabbedComponentsQuery(this), false);
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
            Form preset = new frmAddPreset(this, queryGen.generateTabbedComponentsQuery(this), presetHandler);
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
                Properties.Settings.Default.defaultPreset = treeView_presets.SelectedNode.Text;
                Properties.Settings.Default.Save();
                MessageBox.Show("New default preset set.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
                MessageBox.Show("Please select a preset first.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
        private void treeview_presets_mouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
                treeView_presets.SelectedNode = treeView_presets.GetNodeAt(e.Location);
            else if (e.Button == MouseButtons.Left)
                selectPreset();

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
                        x264PanelFunctions.reset2Defaults(this);

                        // Send the query from the file to the Query Parser class
                        Functions.QueryParser presetQuery = Functions.QueryParser.Parse(query);

                        // Now load the preset
                        presetLoader.presetLoader(this, presetQuery, presetName, loadPictureSettings);

                        // The x264 widgets will need updated, so do this now:
                        x264PanelFunctions.X264_StandardizeOptString(this);
                        x264PanelFunctions.X264_SetCurrentSettingsInPanel(this);
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
                        if (nodeStatus[i] == true)
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
                if (nodeStatus[i] == true)
                    node.Expand();

                foreach (TreeNode subNode in node.Nodes)
                {
                    if (nodeStatus[i] == true)
                        subNode.Expand();
                }

                i++;
            }
        }
        private void loadNormalPreset()
        {
            treeView_presets.Nodes.Find("Normal", true);

            foreach (TreeNode treenode in treeView_presets.Nodes)
            {
                foreach (TreeNode node in treenode.Nodes)
                {
                    if (node.Text.ToString().Equals("Normal"))
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

                    // Kill the current process.
                    Process[] aProc = Process.GetProcessesByName("HandBrakeCLI");
                    Process HandBrakeCLI;
                    if (aProc.Length > 0)
                    {
                        HandBrakeCLI = aProc[0];
                        HandBrakeCLI.Kill();
                    }

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

                    String query;
                    if (rtf_query.Text != "")
                        query = rtf_query.Text;
                    else
                        query = queryGen.GenerateTheQuery(this);

                    if (encodeQueue.count() == 0)
                    {
                        encodeQueue.add(query, text_source.Text, text_destination.Text);
                        encodeQueue.write2disk("hb_queue_recovery.xml");
                    }
                    queueWindow.setQueue();
                    if (encodeQueue.count() > 1)
                        queueWindow.Show();

                    setEncodeStarted(); // Encode is running, so setup the GUI appropriately
                    encodeQueue.startEncode(); // Start The Queue Encoding Process

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

                encodeQueue.add(query, text_source.Text, text_destination.Text);
                encodeQueue.write2disk("hb_queue_recovery.xml"); // Writes the queue to the recovery file, just incase the GUI crashes.

                queueWindow.setQueue();
                queueWindow.Show();
            }
        }
        private void btn_showQueue_Click(object sender, EventArgs e)
        {
            queueWindow.setQueue();
            queueWindow.Show();
        }
        private void mnu_vlcpreview_Click(object sender, EventArgs e)
        {
            if (text_source.Text == "" || text_source.Text == "Click 'Source' to continue" || text_destination.Text == "")
                MessageBox.Show("No source OR destination selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                if (vlcpreview == null)
                {
                    vlcpreview = new frmGenPreview(this);
                    vlcpreview.Show();
                }
                else if (vlcpreview.IsDisposed)
                {
                    vlcpreview = new frmGenPreview(this);
                    vlcpreview.Show();
                }
                else
                    MessageBox.Show("The preview window is already open!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        private void mnu_qtpreview_Click(object sender, EventArgs e)
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
                    MessageBox.Show("The preview window is already open!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        private void btn_ActivityWindow_Click(object sender, EventArgs e)
        {
            String file = String.Empty;
            if (lastAction == "scan")
                file = "dvdinfo.dat";
            else
                file = "hb_encode_log.dat";

            frmActivityWindow ActivityWindow = new frmActivityWindow(file, encodeHandler);
            ActivityWindow.Show();
        }
        #endregion

        #region System Tray Icon
        private void frmMain_Resize(object sender, EventArgs e)
        {
            if (FormWindowState.Minimized == this.WindowState)
            {
                notifyIcon.Visible = true;
                if (lbl_encode.Text != "")
                    notifyIcon.BalloonTipText = lbl_encode.Text;
                else
                    notifyIcon.BalloonTipText = "Not Encoding";
                notifyIcon.ShowBalloonTip(500);
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

            String filename = "";
            text_source.Text = "";

            DVD_Open.ShowDialog();
            filename = DVD_Open.SelectedPath;

            if (filename.StartsWith("\\"))
                MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                if (filename != "")
                {
                    Form frmRD = new frmReadDVD(filename, this);
                    text_source.Text = filename;
                    lbl_encode.Text = "Scanning ...";
                    frmRD.ShowDialog();
                }
                else
                    text_source.Text = "Click 'Source' to continue";

                // If there are no titles in the dropdown menu then the scan has obviously failed. Display an error message explaining to the user.
                if (drp_dvdtitle.Items.Count == 0)
                    MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source.\nYour Source may be copy protected, badly mastered or a format which HandBrake does not support. \nPlease refer to the Documentation and FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);

                lbl_encode.Text = "";
            }
        }
        private void btn_file_source_Click(object sender, EventArgs e)
        {
            // Set the last action to scan. 
            // This is used for tracking which file to load in the activity window
            lastAction = "scan";

            String filename = "";
            text_source.Text = "";

            ISO_Open.ShowDialog();
            filename = ISO_Open.FileName;

            if (filename.StartsWith("\\"))
                MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                if (filename != "")
                {
                    Form frmRD = new frmReadDVD(filename, this);
                    text_source.Text = filename;
                    lbl_encode.Text = "Scanning ...";
                    frmRD.ShowDialog();
                }
                else
                    text_source.Text = "Click 'Source' to continue";

                // If there are no titles in the dropdown menu then the scan has obviously failed. Display an error message explaining to the user.
                if (drp_dvdtitle.Items.Count == 0)
                    MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source.\nYour Source may be copy protected, badly mastered or a format which HandBrake does not support. \nPlease refer to the Documentation and FAQ (see Help Menu).", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);

                lbl_encode.Text = "";

                // Enable the creation of chapter markers if the file is an image of a dvd.
                if (filename.ToLower().Contains(".iso"))
                    Check_ChapterMarkers.Enabled = true;
                else
                {
                    Check_ChapterMarkers.Enabled = false;
                    Check_ChapterMarkers.Checked = false;
                    data_chpt.Rows.Clear();
                }
            }
        }
        private void mnu_dvd_drive_Click(object sender, EventArgs e)
        {
            // Enable the creation of chapter markers.
            Check_ChapterMarkers.Enabled = true;

            // Set the last action to scan. 
            // This is used for tracking which file to load in the activity window
            lastAction = "scan";

            String filename = "";
            if (mnu_dvd_drive.Text.Contains("VIDEO_TS"))
            {
                string[] path = mnu_dvd_drive.Text.Split(' ');
                filename = path[0];
                lbl_encode.Text = "Scanning ...";
                Form frmRD = new frmReadDVD(filename, this);
                text_source.Text = filename;
                frmRD.ShowDialog();
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
                text_destination.Text = hb_common_func.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);

            data_chpt.Rows.Clear();
            DataGridView chapterGridView = hb_common_func.chapterNaming(data_chpt, drop_chapterStart.Text, drop_chapterFinish.Text);
            if (chapterGridView != null)
                data_chpt = chapterGridView;
        }
        private void drop_chapterStart_SelectedIndexChanged(object sender, EventArgs e)
        {
            int c_start, c_end = 1;

            if (drop_chapterFinish.Text == "Auto" && drop_chapterFinish.Items.Count != 0)
                drop_chapterFinish.SelectedIndex = drop_chapterFinish.Items.Count - 1;

            int.TryParse(drop_chapterStart.Text, out c_start);
            int.TryParse(drop_chapterFinish.Text, out c_end);

            if (c_end != 0)
            {
                if (c_start > c_end)
                    drop_chapterFinish.Text = c_start.ToString();
            }

            lbl_duration.Text = hb_common_func.calculateDuration(drop_chapterStart.Text, drop_chapterFinish.Text, selectedTitle).ToString();

            // Run the Autonaming function
            if (Properties.Settings.Default.autoNaming == "Checked")
                text_destination.Text = hb_common_func.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);
        }
        private void drop_chapterFinish_SelectedIndexChanged(object sender, EventArgs e)
        {
            int c_start, c_end = 1;

            if (drop_chapterStart.Text == "Auto" && drop_chapterStart.Items.Count >= 1)
                drop_chapterStart.SelectedIndex = 1;

            int.TryParse(drop_chapterStart.Text, out c_start);
            int.TryParse(drop_chapterFinish.Text, out c_end);

            if (c_start != 0)
            {
                if (c_end < c_start)
                    drop_chapterFinish.Text = c_start.ToString();
            }

            lbl_duration.Text = hb_common_func.calculateDuration(drop_chapterStart.Text, drop_chapterFinish.Text, selectedTitle).ToString();

            // Run the Autonaming function
            if (Properties.Settings.Default.autoNaming == "Checked")
                text_destination.Text = hb_common_func.autoName(drp_dvdtitle, drop_chapterStart.Text, drop_chapterFinish.Text, text_source.Text, text_destination.Text, drop_format.SelectedIndex);
        }

        //Destination
        private void btn_destBrowse_Click(object sender, EventArgs e)
        {
            // This removes the file extension from the filename box on the save file dialog.
            // It's daft but some users don't realise that typing an extension overrides the dropdown extension selected.
            DVD_Save.FileName = DVD_Save.FileName.Replace(".mp4", "").Replace(".m4v", "").Replace(".mkv", "").Replace(".ogm", "").Replace(".avi", "");

            // Show the dialog and set the main form file path
            if (drop_format.SelectedIndex.Equals(0))
                DVD_Save.FilterIndex = 1;
            else if (drop_format.SelectedIndex.Equals(1))
                DVD_Save.FilterIndex = 2;
            else if (drop_format.SelectedIndex.Equals(2))
                DVD_Save.FilterIndex = 3;
            else if (drop_format.SelectedIndex.Equals(3))
                DVD_Save.FilterIndex = 4;
            else if (drop_format.SelectedIndex.Equals(4))
                DVD_Save.FilterIndex = 5;

            if (DVD_Save.ShowDialog() == DialogResult.OK)
            {
                if (DVD_Save.FileName.StartsWith("\\"))
                    MessageBox.Show("Sorry, HandBrake does not support UNC file paths. \nTry mounting the share as a network drive in My Computer", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                else
                {
                    setAudioByContainer(DVD_Save.FileName);
                    text_destination.Text = DVD_Save.FileName;

                    // Quicktime requires .m4v file for chapter markers to work. If checked, change the extension to .m4v (mp4 and m4v are the same thing)
                    if (Check_ChapterMarkers.Checked)
                        text_destination.Text = text_destination.Text.Replace(".mp4", ".m4v");
                }
            }
        }
        private void text_destination_TextChanged(object sender, EventArgs e)
        {
            setAudioByContainer(text_destination.Text);
            setVideoByContainer(text_destination.Text);
            string path = text_destination.Text;
            if (path.EndsWith(".mp4"))
                drop_format.SelectedIndex = 0;
            else if (path.EndsWith(".m4v"))
                drop_format.SelectedIndex = 1;
            else if (path.EndsWith(".mkv"))
                drop_format.SelectedIndex = 2;
            else if (path.EndsWith(".avi"))
                drop_format.SelectedIndex = 3;
            else if (path.EndsWith(".ogm"))
                drop_format.SelectedIndex = 4;

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
            else if (drop_format.SelectedIndex == 3)
                setExtension(".avi");
            else if (drop_format.SelectedIndex == 4)
                setExtension(".ogm");
        }
        private void setExtension(string newExtension)
        {
            text_destination.Text = text_destination.Text.Replace(".mp4", newExtension);
            text_destination.Text = text_destination.Text.Replace(".m4v", newExtension);
            text_destination.Text = text_destination.Text.Replace(".mkv", newExtension);
            text_destination.Text = text_destination.Text.Replace(".avi", newExtension);
            text_destination.Text = text_destination.Text.Replace(".ogm", newExtension);
        }

        //Video Tab
        private void drp_videoEncoder_SelectedIndexChanged(object sender, EventArgs e)
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

            //Turn off some options which are H.264 only when the user selects a non h.264 encoder
            if (drp_videoEncoder.Text.Contains("H.264"))
            {
                if (check_2PassEncode.CheckState == CheckState.Checked)
                    check_turbo.Enabled = true;

                h264Tab.Enabled = true;
                if ((text_destination.Text.Contains(".mp4")) || (text_destination.Text.Contains(".m4v")))
                    check_iPodAtom.Enabled = true;
                else
                    check_iPodAtom.Enabled = false;
                if (!drp_anamorphic.Items.Contains("Loose"))
                    drp_anamorphic.Items.Add("Loose");
            }
            else
            {
                check_turbo.CheckState = CheckState.Unchecked;
                check_turbo.Enabled = false;
                h264Tab.Enabled = false;
                rtf_x264Query.Text = "";
                check_iPodAtom.Enabled = false;
                check_iPodAtom.Checked = false;
                if (drp_anamorphic.Items.Count == 3)
                    drp_anamorphic.Items.RemoveAt(2);
            }

        }
        private void text_bitrate_TextChanged(object sender, EventArgs e)
        {
            text_filesize.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
            check_2PassEncode.Enabled = true;
        }
        private void text_filesize_TextChanged(object sender, EventArgs e)
        {
            text_bitrate.Text = "";
            slider_videoQuality.Value = 0;
            SliderValue.Text = "0%";
            check_2PassEncode.Enabled = true;
        }
        private void slider_videoQuality_Scroll(object sender, EventArgs e)
        {
            SliderValue.Text = slider_videoQuality.Value.ToString() + "%";
            text_bitrate.Text = "";
            text_filesize.Text = "";
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

            int width;
            Boolean parsed = int.TryParse(text_width.Text, out width);
            if (parsed != false)
            {
                if ((width % 16) != 0)
                    text_width.BackColor = Color.LightCoral;
                else
                    text_width.BackColor = Color.LightGreen;


                if (lbl_Aspect.Text != "Select a Title" && maxWidth == 0 && maxHeight == 0)
                {
                    if (drp_anamorphic.Text == "None")
                    {
                        int height = hb_common_func.cacluateNonAnamorphicHeight(width, text_top.Value, text_bottom.Value, text_left.Value, text_right.Value, selectedTitle);
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

            int height;
            Boolean parsed = int.TryParse(text_height.Text, out height);
            if (parsed != false)
            {
                if ((height % 16) != 0)
                    text_height.BackColor = Color.LightCoral;
                else
                    text_height.BackColor = Color.LightGreen;
            }
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
        private void slider_deblock_Scroll(object sender, EventArgs e)
        {
            if (slider_deblock.Value == 4)
                lbl_deblockVal.Text = "Off";
            else
                lbl_deblockVal.Text = slider_deblock.Value.ToString();
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
            if (drp_audenc_1.Text == "AC3")
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

            if (drp_audenc_1.Text == "AAC")
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
            if ((drp_audenc_1.Text == "AAC") && (drp_audmix_1.Text == "6 Channel Discrete"))
                setBitrateSelections384(drp_audbit_1);
            else if ((drp_audenc_1.Text == "AAC") && (drp_audmix_1.Text != "6 Channel Discrete"))
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
                if (drp_audenc_1.Text == "AC3")
                    drp_audbit_1.Text = "Auto";
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text = drp_audbit_1.Text;
                lv_audioList.Select();
            }
        }
        private void tb_drc_Scroll(object sender, EventArgs e)
        {
            double value = (tb_drc.Value / 10.0) + 1;
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
                double drcValue = 0; int drcCalculated = 0;
                double.TryParse(lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text, out drcValue);
                drcValue = (drcValue * 10) - 10;
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
                DataGridView chapterGridView = hb_common_func.chapterNaming(data_chpt, drop_chapterStart.Text, drop_chapterFinish.Text);
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

        // Advanced Tab
        private void drop_refFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("ref", this);
        }
        private void check_mixedReferences_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("mixed-refs", this);
        }
        private void drop_bFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("bframes", this);
        }
        private void drop_directPrediction_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("direct", this);
        }
        private void check_weightedBFrames_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("weightb", this);
        }
        private void check_bFrameDistortion_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("brdo", this);
        }
        private void check_BidirectionalRefinement_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("bime", this);
        }
        private void check_pyrmidalBFrames_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("b-pyramid", this);
        }
        private void drop_MotionEstimationMethod_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("me", this);
        }
        private void drop_MotionEstimationRange_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("merange", this);
        }
        private void drop_subpixelMotionEstimation_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("subq", this);
        }
        private void drop_analysis_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("analyse", this);
        }
        private void check_8x8DCT_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("8x8dct", this);
        }
        private void drop_deblockAlpha_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("deblock", this);

        }
        private void drop_deblockBeta_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("deblock", this);

        }
        private void drop_trellis_SelectedIndexChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("trellis", this);
        }
        private void check_noFastPSkip_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("no-fast-pskip", this);
        }
        private void check_noDCTDecimate_CheckedChanged(object sender, EventArgs e)
        {
            x264PanelFunctions.on_x264_WidgetChange("no-dct-decimate", this);

        }
        private void check_Cabac_CheckedChanged(object sender, EventArgs e)
        {

            x264PanelFunctions.on_x264_WidgetChange("cabac", this);
        }

        private void rtf_x264Query_TextChanged(object sender, EventArgs e)
        {
            if (rtf_x264Query.Text.EndsWith("\n"))
            {
                rtf_x264Query.Text = rtf_x264Query.Text.Replace("\n", "");
                x264PanelFunctions.X264_StandardizeOptString(this);
                x264PanelFunctions.X264_SetCurrentSettingsInPanel(this);

                if (rtf_x264Query.Text == string.Empty)
                    x264PanelFunctions.reset2Defaults(this);
            }
        }
        private void btn_reset_Click(object sender, EventArgs e)
        {
            rtf_x264Query.Text = "";
            x264PanelFunctions.reset2Defaults(this);
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

        #region DVD Drive Detection
        // Source Button Drive Detection
        private delegate void ProgressUpdateHandler();
        private void getDriveInfoThread()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new ProgressUpdateHandler(getDriveInfoThread));
                    return;
                }

                Boolean foundDrive = false;
                DriveInfo[] theCollectionOfDrives = DriveInfo.GetDrives();
                foreach (DriveInfo curDrive in theCollectionOfDrives)
                {
                    if (curDrive.DriveType == DriveType.CDRom)
                    {
                        if (curDrive.IsReady)
                        {
                            if (File.Exists(curDrive.RootDirectory.ToString() + "VIDEO_TS\\VIDEO_TS.IFO"))
                                mnu_dvd_drive.Text = curDrive.RootDirectory.ToString() + "VIDEO_TS (" + curDrive.VolumeLabel + ")";
                            else
                                mnu_dvd_drive.Text = "[No DVD Drive Ready]";

                            foundDrive = true;

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
            string oldval = "";

            if ((path.EndsWith(".mp4")) || (path.EndsWith(".m4v")))
            {
                oldval = drp_audenc_1.Text;
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC");
                drp_audenc_1.Items.Add("AC3");
                if ((oldval != "AAC") && (oldval != "AC3"))
                    drp_audenc_1.SelectedIndex = 0;

            }
            else if (path.EndsWith(".avi"))
            {
                oldval = drp_audenc_1.Text;
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("MP3");
                drp_audenc_1.Items.Add("AC3");
                if ((oldval != "MP3") && (oldval != "AC3"))
                    drp_audenc_1.SelectedIndex = 0;

            }
            else if (path.EndsWith(".ogm"))
            {
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("Vorbis");
                drp_audenc_1.SelectedIndex = 0;

            }
            else if (path.EndsWith(".mkv"))
            {
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC");
                drp_audenc_1.Items.Add("MP3");
                drp_audenc_1.Items.Add("AC3");
                drp_audenc_1.Items.Add("Vorbis");
                if (drp_audenc_1.Text == string.Empty)
                    drp_audenc_1.SelectedIndex = 0;
            }
        }
        private void setVideoByContainer(String path)
        {
            string oldval = "";

            if ((path.EndsWith(".mp4")) || (path.EndsWith(".m4v")))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("MPEG-4 (XviD)");
                drp_videoEncoder.Items.Add("H.264 (x264)");
                if (oldval == "VP3 (Theora)")
                    drp_videoEncoder.SelectedIndex = 2;
                else
                    drp_videoEncoder.Text = oldval;

            }
            else if (path.EndsWith(".avi"))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("MPEG-4 (XviD)");
                drp_videoEncoder.Items.Add("H.264 (x264)");
                if (oldval == "VP3 (Theora)")
                    drp_videoEncoder.SelectedIndex = 2;
                else
                    drp_videoEncoder.Text = oldval;
            }
            else if (path.EndsWith(".ogm"))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("MPEG-4 (XviD)");
                drp_videoEncoder.Items.Add("VP3 (Theora)");
                if (oldval == "H.264 (x264)")
                    drp_videoEncoder.SelectedIndex = 2;
                else
                    drp_videoEncoder.Text = oldval;
            }
            else if (path.EndsWith(".mkv"))
            {
                oldval = drp_videoEncoder.Text;
                drp_videoEncoder.Items.Clear();
                drp_videoEncoder.Items.Add("MPEG-4 (FFmpeg)");
                drp_videoEncoder.Items.Add("MPEG-4 (XviD)");
                drp_videoEncoder.Items.Add("H.264 (x264)");
                drp_videoEncoder.Items.Add("VP3 (Theora)");
                drp_videoEncoder.Text = oldval;
            }
        }
        private void setBitrateSelections384(ComboBox dropDown)
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
        private void setBitrateSelections320(ComboBox dropDown)
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
        private void setBitrateSelections160(ComboBox dropDown)
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
        private void setMixDownAllOptions(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Mono");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
            dropdown.Items.Add("6 Channel Discrete");
        }
        private void setMixDownNotAAC(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
        }
        private void audioEncoderChange(ComboBox audenc, ComboBox audMix, ComboBox audbit, ComboBox audsr)
        {
            if (audenc.Text == "AC3")
            {
                audMix.Enabled = false;
                audbit.Enabled = false;
                audsr.Enabled = false;

                audMix.Text = "Automatic";
                audbit.Text = "160";
                audsr.Text = "Auto";
            }
            else
            {
                // Just make sure not to re-enable the following boxes if the track above is none
                /* if (drp_track2Audio.Text != "None")
                 {
                     audMix.Enabled = true;
                     audbit.Enabled = true;
                     audsr.Enabled = true;

                     audMix.Text = "Automatic";
                     audbit.Text = "160";
                     audsr.Text = "Auto";
                 }*/
            }
        }
        #endregion

        #region Public Methods

        /// <summary>
        /// Setup the GUI for Encoding or finished Encoding.
        /// 1 = Encode Running
        /// 0 = Encode Finished.
        /// </summary>
        /// <param name="i">Int</param>
        public void setEncodeFinished()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateWindowHandler(setEncodeFinished));
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
        public void setEncodeStarted()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateWindowHandler(setEncodeStarted));
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

        /// <summary>
        /// Action can be "encode" or "scan"
        /// Set the last action varible in the main window.
        /// This is used to control which log file is displayed when the Activity window is displayed.
        /// </summary>
        /// <param name="last">String</param>
        public void setLastAction(string last)
        {
            this.lastAction = last;
        }

        /// <summary>
        /// DVD parseing. Pass in a parsed DVD.
        /// </summary>
        /// <param name="dvd"></param>
        public void setStreamReader(Parsing.DVD dvd)
        {
            this.thisDVD = dvd;
        }

        /// <summary>
        /// Reload the preset panel display
        /// </summary>
        public void loadPresetPanel()
        {
            presetHandler.loadPresetData();

            treeView_presets.Nodes.Clear();

            List<Presets.Preset> presetNameList = new List<Presets.Preset>();
            List<string> presetNames = new List<string>();
            TreeNode preset_treeview = new TreeNode();

            TreeNode rootNode = new TreeNode();
            TreeNode rootNodeTwo = new TreeNode();
            TreeNode childNode = new TreeNode();
            int workingLevel = 0;
            string previousCategory = String.Empty;
            string currentCategory = String.Empty;

            presetNameList = presetHandler.getBuildInPresets();
            if (presetNameList.Count != 0)
            {
                foreach (Presets.Preset preset in presetNameList)
                {
                    // Handle Root Nodes

                    // First Case - No presets have been read yet so setup the root category
                    if (preset.Level == 1 && currentCategory == String.Empty)
                    {
                        rootNode = new TreeNode(preset.Category);
                        workingLevel = preset.Level;
                        currentCategory = preset.Category;
                        previousCategory = preset.Category;
                    }

                    // Second Case - This is the first sub child node.
                    if (preset.Level == 2 && workingLevel == 1 && currentCategory != preset.Category)
                    {
                        rootNodeTwo = new TreeNode(preset.Category);
                        workingLevel = preset.Level;
                        currentCategory = preset.Category;
                        rootNode.Nodes.Add(rootNodeTwo);
                    }

                    // Third Case - Any presets the sub presets detected in the above if statment.
                    if (preset.Level == 1 && workingLevel == 2)
                    {
                        workingLevel = preset.Level;
                        currentCategory = preset.Category;
                    }

                    // Fourth Case - We've finished this root node and are onto the next root node.
                    if (preset.Level == 1 && workingLevel == 1 && previousCategory != preset.Category)
                    {
                        treeView_presets.Nodes.Add(rootNode); // Add the finished node

                        rootNode = new TreeNode(preset.Category);
                        workingLevel = preset.Level;
                        currentCategory = preset.Category;
                        previousCategory = preset.Category;
                    }

                    // Handle Child Nodes
                    // Add First level child nodes to the current root node
                    if (preset.Level == 1 && workingLevel == 1 && currentCategory == preset.Category)
                    {
                        childNode = new TreeNode(preset.Name);
                        rootNode.Nodes.Add(childNode);
                    }

                    // Add Second level child nodes to the current sub root node
                    if (preset.Level == 2 && workingLevel == 2 && currentCategory == preset.Category)
                    {
                        childNode = new TreeNode(preset.Name);
                        rootNodeTwo.Nodes.Add(childNode);
                    }
                }

                // Add the final root node which does not get added above.
                treeView_presets.Nodes.Add(rootNode);
            }

            // User Presets
            presetNames = presetHandler.getUserPresetNames();
            foreach (string preset in presetNames)
            {
                preset_treeview = new TreeNode(preset);
                preset_treeview.ForeColor = Color.Black;

                // Now Fill Out List View with Items
                treeView_presets.Nodes.Add(preset_treeview);
            }
        }

        #endregion




        // This is the END of the road ------------------------------------------------------------------------------
    }
}