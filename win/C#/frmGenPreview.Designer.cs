namespace Handbrake
{
    partial class frmGenPreview
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmGenPreview));
            this.toolBar = new System.Windows.Forms.ToolStrip();
            this.btn_encode = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_play = new System.Windows.Forms.ToolStripButton();
            this.label1 = new System.Windows.Forms.Label();
            this.lbl_status = new System.Windows.Forms.Label();
            this.cb_duration = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.cb_preview = new System.Windows.Forms.ComboBox();
            this.toolBar.SuspendLayout();
            this.SuspendLayout();
            // 
            // toolBar
            // 
            this.toolBar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode,
            this.toolStripSeparator1,
            this.btn_play});
            this.toolBar.Location = new System.Drawing.Point(0, 0);
            this.toolBar.Name = "toolBar";
            this.toolBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolBar.Size = new System.Drawing.Size(377, 39);
            this.toolBar.TabIndex = 38;
            this.toolBar.Text = "toolStrip1";
            // 
            // btn_encode
            // 
            this.btn_encode.Image = global::Handbrake.Properties.Resources.picture;
            this.btn_encode.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_encode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(115, 36);
            this.btn_encode.Text = "Encode Sample";
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 39);
            this.toolStripSeparator1.Visible = false;
            // 
            // btn_play
            // 
            this.btn_play.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_play.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_play.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_play.Name = "btn_play";
            this.btn_play.Size = new System.Drawing.Size(63, 36);
            this.btn_play.Text = "Play";
            this.btn_play.Visible = false;
            this.btn_play.Click += new System.EventHandler(this.btn_play_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(12, 79);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(47, 13);
            this.label1.TabIndex = 39;
            this.label1.Text = "Status:";
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(117, 79);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(111, 13);
            this.lbl_status.TabIndex = 40;
            this.lbl_status.Text = "Click \"Encode Sample\"";
            // 
            // cb_duration
            // 
            this.cb_duration.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_duration.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cb_duration.FormattingEnabled = true;
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
            this.cb_duration.Location = new System.Drawing.Point(304, 47);
            this.cb_duration.MaxLength = 2;
            this.cb_duration.Name = "cb_duration";
            this.cb_duration.Size = new System.Drawing.Size(46, 21);
            this.cb_duration.TabIndex = 41;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(180, 50);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(118, 13);
            this.label2.TabIndex = 42;
            this.label2.Text = "Duration (seconds):";
            this.label2.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(12, 50);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(102, 13);
            this.label3.TabIndex = 44;
            this.label3.Text = "Start at preview:";
            this.label3.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // cb_preview
            // 
            this.cb_preview.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_preview.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cb_preview.FormattingEnabled = true;
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
            this.cb_preview.Location = new System.Drawing.Point(120, 47);
            this.cb_preview.MaxLength = 2;
            this.cb_preview.Name = "cb_preview";
            this.cb_preview.Size = new System.Drawing.Size(46, 21);
            this.cb_preview.TabIndex = 43;
            // 
            // frmGenPreview
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(377, 104);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.cb_preview);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.cb_duration);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.toolBar);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmGenPreview";
            this.Text = "Live Preview";
            this.toolBar.ResumeLayout(false);
            this.toolBar.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolStrip toolBar;
        private System.Windows.Forms.ToolStripButton btn_encode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_play;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lbl_status;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cb_duration;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox cb_preview;
    }
}