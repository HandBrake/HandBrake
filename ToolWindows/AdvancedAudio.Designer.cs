namespace Handbrake.ToolWindows
{
    partial class AdvancedAudio
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AdvancedAudio));
            this.label1 = new System.Windows.Forms.Label();
            this.gainTrackBar = new System.Windows.Forms.TrackBar();
            this.lbl_GainValue = new System.Windows.Forms.Label();
            this.btn_close = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.gainTrackBar)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(54, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Gain (dB):";
            // 
            // gainTrackBar
            // 
            this.gainTrackBar.Location = new System.Drawing.Point(72, 12);
            this.gainTrackBar.Maximum = 41;
            this.gainTrackBar.Name = "gainTrackBar";
            this.gainTrackBar.Size = new System.Drawing.Size(171, 45);
            this.gainTrackBar.TabIndex = 2;
            this.gainTrackBar.Value = 21;
            this.gainTrackBar.Scroll += new System.EventHandler(this.gainTrackBar_Scroll);
            // 
            // lbl_GainValue
            // 
            this.lbl_GainValue.AutoSize = true;
            this.lbl_GainValue.Location = new System.Drawing.Point(249, 22);
            this.lbl_GainValue.Name = "lbl_GainValue";
            this.lbl_GainValue.Size = new System.Drawing.Size(29, 13);
            this.lbl_GainValue.TabIndex = 3;
            this.lbl_GainValue.Text = "0 dB";
            // 
            // btn_close
            // 
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(207, 52);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(71, 22);
            this.btn_close.TabIndex = 5;
            this.btn_close.TabStop = false;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = true;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // AdvancedAudio
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(295, 83);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.lbl_GainValue);
            this.Controls.Add(this.gainTrackBar);
            this.Controls.Add(this.label1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AdvancedAudio";
            this.ShowIcon = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Advanced Audio";
            this.TopMost = true;
            ((System.ComponentModel.ISupportInitialize)(this.gainTrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TrackBar gainTrackBar;
        private System.Windows.Forms.Label lbl_GainValue;
        internal System.Windows.Forms.Button btn_close;
    }
}