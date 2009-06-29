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
            this.toolStripDropDownButton1 = new System.Windows.Forms.ToolStripDropDownButton();
            this.btn_encode_log = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_scan_log = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_copy = new System.Windows.Forms.ToolStripButton();
            this.panel1 = new System.Windows.Forms.Panel();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.lbl_slb = new System.Windows.Forms.ToolStripStatusLabel();
            this.txt_log = new System.Windows.Forms.ToolStripStatusLabel();
            this.rightClickMenu.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.statusStrip1.SuspendLayout();
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
            this.rtf_actLog.Size = new System.Drawing.Size(471, 530);
            this.rtf_actLog.TabIndex = 29;
            this.rtf_actLog.Text = "";
            // 
            // rightClickMenu
            // 
            this.rightClickMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_copy_log,
            this.mnu_openLogFolder});
            this.rightClickMenu.Name = "rightClickMenu";
            this.rightClickMenu.Size = new System.Drawing.Size(247, 48);
            // 
            // mnu_copy_log
            // 
            this.mnu_copy_log.Image = global::Handbrake.Properties.Resources.copy;
            this.mnu_copy_log.Name = "mnu_copy_log";
            this.mnu_copy_log.Size = new System.Drawing.Size(246, 22);
            this.mnu_copy_log.Text = "Copy";
            this.mnu_copy_log.Click += new System.EventHandler(this.mnu_copy_log_Click);
            // 
            // mnu_openLogFolder
            // 
            this.mnu_openLogFolder.Image = global::Handbrake.Properties.Resources.folder;
            this.mnu_openLogFolder.Name = "mnu_openLogFolder";
            this.mnu_openLogFolder.Size = new System.Drawing.Size(246, 22);
            this.mnu_openLogFolder.Text = "Open Individual Log File Directory";
            this.mnu_openLogFolder.Click += new System.EventHandler(this.mnu_openLogFolder_Click);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripDropDownButton1,
            this.btn_copy});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.toolStrip1.Size = new System.Drawing.Size(471, 25);
            this.toolStrip1.TabIndex = 96;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolStripDropDownButton1
            // 
            this.toolStripDropDownButton1.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolStripDropDownButton1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode_log,
            this.btn_scan_log});
            this.toolStripDropDownButton1.Image = global::Handbrake.Properties.Resources.Output_Small;
            this.toolStripDropDownButton1.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.toolStripDropDownButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripDropDownButton1.Name = "toolStripDropDownButton1";
            this.toolStripDropDownButton1.Size = new System.Drawing.Size(85, 22);
            this.toolStripDropDownButton1.Text = "Select Log";
            // 
            // btn_encode_log
            // 
            this.btn_encode_log.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.btn_encode_log.Name = "btn_encode_log";
            this.btn_encode_log.Size = new System.Drawing.Size(140, 22);
            this.btn_encode_log.Text = "Encode Log";
            this.btn_encode_log.Click += new System.EventHandler(this.btn_encode_log_Click);
            // 
            // btn_scan_log
            // 
            this.btn_scan_log.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.btn_scan_log.Name = "btn_scan_log";
            this.btn_scan_log.Size = new System.Drawing.Size(140, 22);
            this.btn_scan_log.Text = "Scan Log";
            this.btn_scan_log.Click += new System.EventHandler(this.btn_scan_log_Click);
            // 
            // btn_copy
            // 
            this.btn_copy.Image = ((System.Drawing.Image)(resources.GetObject("btn_copy.Image")));
            this.btn_copy.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_copy.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.Size = new System.Drawing.Size(111, 22);
            this.btn_copy.Text = "Copy to clipboard";
            this.btn_copy.Click += new System.EventHandler(this.btn_copy_Click);
            // 
            // panel1
            // 
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 25);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(471, 552);
            this.panel1.TabIndex = 97;
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_slb,
            this.txt_log});
            this.statusStrip1.Location = new System.Drawing.Point(0, 555);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(471, 22);
            this.statusStrip1.TabIndex = 98;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // lbl_slb
            // 
            this.lbl_slb.BackColor = System.Drawing.Color.Transparent;
            this.lbl_slb.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_slb.Name = "lbl_slb";
            this.lbl_slb.Size = new System.Drawing.Size(94, 17);
            this.lbl_slb.Text = "Selected Log: ";
            // 
            // txt_log
            // 
            this.txt_log.BackColor = System.Drawing.Color.Transparent;
            this.txt_log.Name = "txt_log";
            this.txt_log.Size = new System.Drawing.Size(74, 17);
            this.txt_log.Text = "{selected log}";
            // 
            // frmActivityWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(471, 577);
            this.Controls.Add(this.rtf_actLog);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.toolStrip1);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmActivityWindow";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Activity Window";
            this.rightClickMenu.ResumeLayout(false);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.RichTextBox rtf_actLog;
        internal System.Windows.Forms.ToolTip ToolTip;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel txt_log;
        private System.Windows.Forms.ToolStripDropDownButton toolStripDropDownButton1;
        private System.Windows.Forms.ToolStripMenuItem btn_encode_log;
        private System.Windows.Forms.ToolStripMenuItem btn_scan_log;
        private System.Windows.Forms.ToolStripButton btn_copy;
        private System.Windows.Forms.ToolStripStatusLabel lbl_slb;
        private System.Windows.Forms.ContextMenuStrip rightClickMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_copy_log;
        private System.Windows.Forms.ToolStripMenuItem mnu_openLogFolder;
    }
}