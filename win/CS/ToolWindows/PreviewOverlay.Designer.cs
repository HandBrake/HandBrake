namespace Handbrake.ToolWindows
{
    partial class PreviewOverlay
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PreviewOverlay));
            this.PlayWithQT = new System.Windows.Forms.Button();
            this.progressBar1 = new System.Windows.Forms.ProgressBar();
            this.drp_preview = new System.Windows.Forms.ComboBox();
            this.drp_duration = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // PlayWithQT
            // 
            this.PlayWithQT.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.PlayWithQT.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PlayWithQT.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.PlayWithQT.Location = new System.Drawing.Point(347, 45);
            this.PlayWithQT.Name = "PlayWithQT";
            this.PlayWithQT.Size = new System.Drawing.Size(90, 22);
            this.PlayWithQT.TabIndex = 4;
            this.PlayWithQT.TabStop = false;
            this.PlayWithQT.Text = "Play";
            this.PlayWithQT.UseVisualStyleBackColor = true;
            this.PlayWithQT.Click += new System.EventHandler(this.PlayWithQtClick);
            // 
            // progressBar1
            // 
            this.progressBar1.Location = new System.Drawing.Point(12, 12);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(425, 10);
            this.progressBar1.TabIndex = 6;
            // 
            // drp_preview
            // 
            this.drp_preview.FormattingEnabled = true;
            this.drp_preview.Location = new System.Drawing.Point(63, 47);
            this.drp_preview.Name = "drp_preview";
            this.drp_preview.Size = new System.Drawing.Size(56, 21);
            this.drp_preview.TabIndex = 7;
            // 
            // drp_duration
            // 
            this.drp_duration.FormattingEnabled = true;
            this.drp_duration.Location = new System.Drawing.Point(195, 47);
            this.drp_duration.Name = "drp_duration";
            this.drp_duration.Size = new System.Drawing.Size(73, 21);
            this.drp_duration.TabIndex = 8;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(125, 50);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(64, 13);
            this.label1.TabIndex = 9;
            this.label1.Text = "Duration (s):";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 50);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(48, 13);
            this.label2.TabIndex = 10;
            this.label2.Text = "Preview:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 25);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(140, 13);
            this.label3.TabIndex = 11;
            this.label3.Text = "Select a frame and duration:";
            // 
            // PreviewOverlay
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(453, 80);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.drp_duration);
            this.Controls.Add(this.drp_preview);
            this.Controls.Add(this.progressBar1);
            this.Controls.Add(this.PlayWithQT);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "PreviewOverlay";
            this.Opacity = 0.75D;
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Title Scan";
            this.TopMost = true;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button PlayWithQT;
        private System.Windows.Forms.ProgressBar progressBar1;
        private System.Windows.Forms.ComboBox drp_preview;
        private System.Windows.Forms.ComboBox drp_duration;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
    }
}