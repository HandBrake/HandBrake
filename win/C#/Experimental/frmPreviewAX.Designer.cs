namespace Handbrake
{
    partial class frmPreviewAX
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmPreviewAX));
            this.toolBar = new System.Windows.Forms.ToolStrip();
            this.btn_encode = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_play = new System.Windows.Forms.ToolStripButton();
            this.btn_stop = new System.Windows.Forms.ToolStripButton();
            this.btn_step_fwd = new System.Windows.Forms.ToolStripButton();
            this.btn_step_back = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_reset = new System.Windows.Forms.ToolStripButton();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.txt_position = new System.Windows.Forms.ToolStripStatusLabel();
            this.vlc_player = new AxAXVLC.AxVLCPlugin2();
            this.toolBar.SuspendLayout();
            this.statusStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.vlc_player)).BeginInit();
            this.SuspendLayout();
            // 
            // toolBar
            // 
            this.toolBar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode,
            this.toolStripSeparator1,
            this.btn_play,
            this.btn_stop,
            this.btn_step_fwd,
            this.btn_step_back,
            this.toolStripSeparator2,
            this.btn_reset});
            this.toolBar.Location = new System.Drawing.Point(0, 0);
            this.toolBar.Name = "toolBar";
            this.toolBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolBar.Size = new System.Drawing.Size(750, 39);
            this.toolBar.TabIndex = 37;
            this.toolBar.Text = "toolStrip1";
            // 
            // btn_encode
            // 
            this.btn_encode.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_encode.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_encode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(127, 36);
            this.btn_encode.Text = "Encode Sample";
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_play
            // 
            this.btn_play.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_play.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_play.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_play.Name = "btn_play";
            this.btn_play.Size = new System.Drawing.Size(64, 36);
            this.btn_play.Text = "Play";
            this.btn_play.Click += new System.EventHandler(this.btn_play_Click);
            // 
            // btn_stop
            // 
            this.btn_stop.Image = global::Handbrake.Properties.Resources.stop;
            this.btn_stop.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_stop.ImageTransparentColor = System.Drawing.Color.MediumAquamarine;
            this.btn_stop.Name = "btn_stop";
            this.btn_stop.Size = new System.Drawing.Size(69, 36);
            this.btn_stop.Text = "Stop";
            this.btn_stop.Click += new System.EventHandler(this.btn_stop_Click);
            // 
            // btn_step_fwd
            // 
            this.btn_step_fwd.Image = ((System.Drawing.Image)(resources.GetObject("btn_step_fwd.Image")));
            this.btn_step_fwd.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_step_fwd.Name = "btn_step_fwd";
            this.btn_step_fwd.Size = new System.Drawing.Size(64, 36);
            this.btn_step_fwd.Text = "Slower";
            // 
            // btn_step_back
            // 
            this.btn_step_back.Image = ((System.Drawing.Image)(resources.GetObject("btn_step_back.Image")));
            this.btn_step_back.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_step_back.Name = "btn_step_back";
            this.btn_step_back.Size = new System.Drawing.Size(60, 36);
            this.btn_step_back.Text = "Faster";
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_reset
            // 
            this.btn_reset.Image = global::Handbrake.Properties.Resources.window;
            this.btn_reset.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_reset.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_reset.Name = "btn_reset";
            this.btn_reset.Size = new System.Drawing.Size(148, 36);
            this.btn_reset.Text = "Reset Window Size";
            this.btn_reset.Click += new System.EventHandler(this.btn_reset_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.txt_position});
            this.statusStrip.Location = new System.Drawing.Point(0, 438);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(750, 23);
            this.statusStrip.TabIndex = 38;
            this.statusStrip.Text = "statusStrip1";
            // 
            // txt_position
            // 
            this.txt_position.Font = new System.Drawing.Font("Tahoma", 11.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txt_position.Name = "txt_position";
            this.txt_position.Size = new System.Drawing.Size(704, 18);
            this.txt_position.Spring = true;
            this.txt_position.Text = "DANGER! MAY CAUSE BLUESCREEN OF DEATH!!, SEE CODE FOR COMMENTS";
            // 
            // vlc_player
            // 
            this.vlc_player.Dock = System.Windows.Forms.DockStyle.Fill;
            this.vlc_player.Enabled = true;
            this.vlc_player.Location = new System.Drawing.Point(0, 39);
            this.vlc_player.Name = "vlc_player";
            this.vlc_player.OcxState = ((System.Windows.Forms.AxHost.State)(resources.GetObject("vlc_player.OcxState")));
            this.vlc_player.Size = new System.Drawing.Size(750, 399);
            this.vlc_player.TabIndex = 39;
            // 
            // frmPreviewAX
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(750, 461);
            this.Controls.Add(this.vlc_player);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.toolBar);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmPreviewAX";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Video Preview";
            this.TopMost = true;
            this.toolBar.ResumeLayout(false);
            this.toolBar.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.vlc_player)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolStrip toolBar;
        private System.Windows.Forms.ToolStripButton btn_encode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_play;
        private System.Windows.Forms.ToolStripButton btn_step_fwd;
        private System.Windows.Forms.ToolStripButton btn_step_back;
        private System.Windows.Forms.ToolStripButton btn_stop;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel txt_position;
        private System.Windows.Forms.ToolStripButton btn_reset;
        private AxAXVLC.AxVLCPlugin2 vlc_player;
    }
}