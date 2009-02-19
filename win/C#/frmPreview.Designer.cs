namespace Handbrake
{
    partial class frmPreview
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
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmPreview));
            this.toolBar = new System.Windows.Forms.ToolStrip();
            this.lbl_preview = new System.Windows.Forms.ToolStripLabel();
            this.cb_preview = new System.Windows.Forms.ToolStripComboBox();
            this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
            this.cb_duration = new System.Windows.Forms.ToolStripComboBox();
            this.btn_playQT = new System.Windows.Forms.ToolStripButton();
            this.btn_playVLC = new System.Windows.Forms.ToolStripButton();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.lbl_status = new System.Windows.Forms.ToolStripStatusLabel();
            this.QTControl = new AxQTOControlLib.AxQTControl();
            this.toolBar.SuspendLayout();
            this.statusStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.QTControl)).BeginInit();
            this.SuspendLayout();
            // 
            // toolBar
            // 
            this.toolBar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_preview,
            this.cb_preview,
            this.toolStripLabel2,
            this.cb_duration,
            this.btn_playQT,
            this.btn_playVLC});
            this.toolBar.Location = new System.Drawing.Point(0, 0);
            this.toolBar.Name = "toolBar";
            this.toolBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolBar.Size = new System.Drawing.Size(774, 39);
            this.toolBar.TabIndex = 37;
            this.toolBar.Text = "toolStrip1";
            // 
            // lbl_preview
            // 
            this.lbl_preview.BackColor = System.Drawing.Color.Transparent;
            this.lbl_preview.Name = "lbl_preview";
            this.lbl_preview.Size = new System.Drawing.Size(100, 36);
            this.lbl_preview.Text = "Start at Preview:";
            // 
            // cb_preview
            // 
            this.cb_preview.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_preview.DropDownWidth = 75;
            this.cb_preview.FlatStyle = System.Windows.Forms.FlatStyle.Standard;
            this.cb_preview.Items.AddRange(new object[] {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10"});
            this.cb_preview.Name = "cb_preview";
            this.cb_preview.Size = new System.Drawing.Size(75, 39);
            // 
            // toolStripLabel2
            // 
            this.toolStripLabel2.BackColor = System.Drawing.Color.Transparent;
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new System.Drawing.Size(111, 36);
            this.toolStripLabel2.Text = "Duration (seconds)";
            // 
            // cb_duration
            // 
            this.cb_duration.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_duration.DropDownWidth = 75;
            this.cb_duration.FlatStyle = System.Windows.Forms.FlatStyle.Standard;
            this.cb_duration.Items.AddRange(new object[] {
            "5",
            "10",
            "15",
            "20",
            "25",
            "30",
            "35",
            "40",
            "45",
            "50",
            "55",
            "60"});
            this.cb_duration.Margin = new System.Windows.Forms.Padding(0);
            this.cb_duration.Name = "cb_duration";
            this.cb_duration.Size = new System.Drawing.Size(75, 39);
            // 
            // btn_playQT
            // 
            this.btn_playQT.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_playQT.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_playQT.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_playQT.Name = "btn_playQT";
            this.btn_playQT.Size = new System.Drawing.Size(113, 36);
            this.btn_playQT.Text = "Play with QT";
            this.btn_playQT.Click += new System.EventHandler(this.btn_playQT_Click);
            // 
            // btn_playVLC
            // 
            this.btn_playVLC.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_playVLC.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_playVLC.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_playVLC.Name = "btn_playVLC";
            this.btn_playVLC.Size = new System.Drawing.Size(117, 36);
            this.btn_playVLC.Text = "Play with VLC";
            this.btn_playVLC.Click += new System.EventHandler(this.btn_playVLC_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_status});
            this.statusStrip.Location = new System.Drawing.Point(0, 486);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(774, 22);
            this.statusStrip.TabIndex = 38;
            this.statusStrip.Text = "statusStrip1";
            // 
            // lbl_status
            // 
            this.lbl_status.BackColor = System.Drawing.Color.Transparent;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(31, 17);
            this.lbl_status.Text = "{0}";
            // 
            // QTControl
            // 
            this.QTControl.Enabled = true;
            this.QTControl.Location = new System.Drawing.Point(0, 42);
            this.QTControl.Name = "QTControl";
            this.QTControl.OcxState = ((System.Windows.Forms.AxHost.State)(resources.GetObject("QTControl.OcxState")));
            this.QTControl.Size = new System.Drawing.Size(64, 64);
            this.QTControl.TabIndex = 39;
            this.QTControl.Visible = false;
            // 
            // frmPreview
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Black;
            this.ClientSize = new System.Drawing.Size(774, 508);
            this.Controls.Add(this.QTControl);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.toolBar);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmPreview";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Video Preview";
            this.TopMost = true;
            this.toolBar.ResumeLayout(false);
            this.toolBar.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.QTControl)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolStrip toolBar;
        private System.Windows.Forms.ToolStripButton btn_playQT;
        private System.Windows.Forms.StatusStrip statusStrip;
        private AxQTOControlLib.AxQTControl QTControl;
        private System.Windows.Forms.ToolStripComboBox cb_preview;
        private System.Windows.Forms.ToolStripLabel lbl_preview;
        private System.Windows.Forms.ToolStripLabel toolStripLabel2;
        private System.Windows.Forms.ToolStripComboBox cb_duration;
        private System.Windows.Forms.ToolStripStatusLabel lbl_status;
        private System.Windows.Forms.ToolStripButton btn_playVLC;
    }
}