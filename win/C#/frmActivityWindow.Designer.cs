/*  frmDvdInfo.Designer.cs 
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmActivityWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmActivityWindow));
            this.rtf_actLog = new System.Windows.Forms.RichTextBox();
            this.rightClickMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_copy_log = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_openLogFolder = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.dropDownLogSelector = new System.Windows.Forms.ToolStripDropDownButton();
            this.btn_encode_log = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_scan_log = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_copy = new System.Windows.Forms.ToolStripButton();
            this.panel1 = new System.Windows.Forms.Panel();
            this.rightClickMenu.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // rtf_actLog
            // 
            this.rtf_actLog.ContextMenuStrip = this.rightClickMenu;
            this.rtf_actLog.Cursor = System.Windows.Forms.Cursors.IBeam;
            this.rtf_actLog.DetectUrls = false;
            this.rtf_actLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.rtf_actLog.Location = new System.Drawing.Point(0, 25);
            this.rtf_actLog.Name = "rtf_actLog";
            this.rtf_actLog.ReadOnly = true;
            this.rtf_actLog.Size = new System.Drawing.Size(404, 552);
            this.rtf_actLog.TabIndex = 29;
            this.rtf_actLog.Text = "";
            // 
            // rightClickMenu
            // 
            this.rightClickMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_copy_log,
            this.mnu_openLogFolder});
            this.rightClickMenu.Name = "rightClickMenu";
            this.rightClickMenu.Size = new System.Drawing.Size(254, 48);
            // 
            // mnu_copy_log
            // 
            this.mnu_copy_log.Image = global::Handbrake.Properties.Resources.copy;
            this.mnu_copy_log.Name = "mnu_copy_log";
            this.mnu_copy_log.Size = new System.Drawing.Size(253, 22);
            this.mnu_copy_log.Text = "Copy";
            this.mnu_copy_log.Click += new System.EventHandler(this.MnuCopyLogClick);
            // 
            // mnu_openLogFolder
            // 
            this.mnu_openLogFolder.Image = global::Handbrake.Properties.Resources.folder;
            this.mnu_openLogFolder.Name = "mnu_openLogFolder";
            this.mnu_openLogFolder.Size = new System.Drawing.Size(253, 22);
            this.mnu_openLogFolder.Text = "Open Individual Log File Directory";
            this.mnu_openLogFolder.Click += new System.EventHandler(this.MnuOpenLogFolderClick);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.dropDownLogSelector,
            this.btn_copy});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.toolStrip1.Size = new System.Drawing.Size(404, 25);
            this.toolStrip1.TabIndex = 96;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // dropDownLogSelector
            // 
            this.dropDownLogSelector.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.dropDownLogSelector.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode_log,
            this.btn_scan_log});
            this.dropDownLogSelector.Image = global::Handbrake.Properties.Resources.Output_Small;
            this.dropDownLogSelector.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.dropDownLogSelector.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.dropDownLogSelector.Name = "dropDownLogSelector";
            this.dropDownLogSelector.Size = new System.Drawing.Size(90, 22);
            this.dropDownLogSelector.Text = "Select Log";
            // 
            // btn_encode_log
            // 
            this.btn_encode_log.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.btn_encode_log.Name = "btn_encode_log";
            this.btn_encode_log.Size = new System.Drawing.Size(152, 22);
            this.btn_encode_log.Text = "Encode Log";
            this.btn_encode_log.Click += new System.EventHandler(this.BtnEncodeLogClick);
            // 
            // btn_scan_log
            // 
            this.btn_scan_log.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.btn_scan_log.Name = "btn_scan_log";
            this.btn_scan_log.Size = new System.Drawing.Size(152, 22);
            this.btn_scan_log.Text = "Scan Log";
            this.btn_scan_log.Click += new System.EventHandler(this.BtnScanLogClick);
            // 
            // btn_copy
            // 
            this.btn_copy.Image = ((System.Drawing.Image)(resources.GetObject("btn_copy.Image")));
            this.btn_copy.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_copy.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.Size = new System.Drawing.Size(122, 22);
            this.btn_copy.Text = "Copy to clipboard";
            this.btn_copy.Click += new System.EventHandler(this.BtnCopyClick);
            // 
            // panel1
            // 
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 25);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(404, 552);
            this.panel1.TabIndex = 97;
            // 
            // frmActivityWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(404, 577);
            this.Controls.Add(this.rtf_actLog);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.toolStrip1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmActivityWindow";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Activity Window";
            this.Load += new System.EventHandler(this.NewActivityWindow_Load);
            this.rightClickMenu.ResumeLayout(false);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.RichTextBox rtf_actLog;
        internal System.Windows.Forms.ToolTip ToolTip;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ToolStripDropDownButton dropDownLogSelector;
        private System.Windows.Forms.ToolStripMenuItem btn_encode_log;
        private System.Windows.Forms.ToolStripMenuItem btn_scan_log;
        private System.Windows.Forms.ToolStripButton btn_copy;
        private System.Windows.Forms.ContextMenuStrip rightClickMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_copy_log;
        private System.Windows.Forms.ToolStripMenuItem mnu_openLogFolder;
    }
}