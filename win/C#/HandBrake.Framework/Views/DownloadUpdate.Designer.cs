/*  frmDownload.Designer.cs 
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Views
{
    partial class DownloadUpdate
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DownloadUpdate));
            this.lblProgress = new System.Windows.Forms.Label();
            this.progress_download = new System.Windows.Forms.ProgressBar();
            this.btn_cancel = new System.Windows.Forms.Button();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // lblProgress
            // 
            resources.ApplyResources(this.lblProgress, "lblProgress");
            this.lblProgress.Name = "lblProgress";
            // 
            // progress_download
            // 
            resources.ApplyResources(this.progress_download, "progress_download");
            this.progress_download.Name = "progress_download";
            this.progress_download.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            // 
            // btn_cancel
            // 
            resources.ApplyResources(this.btn_cancel, "btn_cancel");
            this.btn_cancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_cancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.TabStop = false;
            this.btn_cancel.UseVisualStyleBackColor = true;
            this.btn_cancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // tableLayoutPanel1
            // 
            resources.ApplyResources(this.tableLayoutPanel1, "tableLayoutPanel1");
            this.tableLayoutPanel1.Controls.Add(this.PictureBox1, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.btn_cancel, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.progress_download, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.lblProgress, 1, 1);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = global::HandBrake.Framework.Properties.Resources.download;
            resources.ApplyResources(this.PictureBox1, "PictureBox1");
            this.PictureBox1.Name = "PictureBox1";
            this.tableLayoutPanel1.SetRowSpan(this.PictureBox1, 3);
            this.PictureBox1.TabStop = false;
            // 
            // DownloadUpdate
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tableLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DownloadUpdate";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label lblProgress;
        private System.Windows.Forms.ProgressBar progress_download;
        internal System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        internal System.Windows.Forms.PictureBox PictureBox1;
    }
}