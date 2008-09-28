/*  frmUpdater.Designer.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmUpdater
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmUpdater));
            this.label1 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.wBrowser = new System.Windows.Forms.WebBrowser();
            this.btn_skip = new System.Windows.Forms.Button();
            this.btn_installUpdate = new System.Windows.Forms.Button();
            this.btn_remindLater = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.lbl_update_text = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(91, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(305, 16);
            this.label1.TabIndex = 25;
            this.label1.Text = "A New Version of Handbrake is available!";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(91, 46);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(208, 13);
            this.label6.TabIndex = 30;
            this.label6.Text = "Would you like to download it now?";
            // 
            // wBrowser
            // 
            this.wBrowser.Location = new System.Drawing.Point(94, 88);
            this.wBrowser.MinimumSize = new System.Drawing.Size(20, 20);
            this.wBrowser.Name = "wBrowser";
            this.wBrowser.Size = new System.Drawing.Size(503, 155);
            this.wBrowser.TabIndex = 31;
            // 
            // btn_skip
            // 
            this.btn_skip.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_skip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_skip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_skip.Location = new System.Drawing.Point(94, 250);
            this.btn_skip.Name = "btn_skip";
            this.btn_skip.Size = new System.Drawing.Size(133, 22);
            this.btn_skip.TabIndex = 54;
            this.btn_skip.Text = "Skip This Version";
            this.btn_skip.UseVisualStyleBackColor = false;
            this.btn_skip.Click += new System.EventHandler(this.btn_skip_Click);
            // 
            // btn_installUpdate
            // 
            this.btn_installUpdate.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_installUpdate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_installUpdate.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_installUpdate.Location = new System.Drawing.Point(464, 250);
            this.btn_installUpdate.Name = "btn_installUpdate";
            this.btn_installUpdate.Size = new System.Drawing.Size(133, 22);
            this.btn_installUpdate.TabIndex = 55;
            this.btn_installUpdate.Text = "Install Update";
            this.btn_installUpdate.UseVisualStyleBackColor = false;
            this.btn_installUpdate.Click += new System.EventHandler(this.btn_installUpdate_Click);
            // 
            // btn_remindLater
            // 
            this.btn_remindLater.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_remindLater.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_remindLater.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_remindLater.Location = new System.Drawing.Point(325, 250);
            this.btn_remindLater.Name = "btn_remindLater";
            this.btn_remindLater.Size = new System.Drawing.Size(133, 22);
            this.btn_remindLater.TabIndex = 56;
            this.btn_remindLater.Text = "Remind me Later";
            this.btn_remindLater.UseVisualStyleBackColor = false;
            this.btn_remindLater.Click += new System.EventHandler(this.btn_remindLater_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(91, 72);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(103, 13);
            this.label3.TabIndex = 57;
            this.label3.Text = "Release Notes:";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = global::Handbrake.Properties.Resources.logo64;
            this.PictureBox1.InitialImage = null;
            this.PictureBox1.Location = new System.Drawing.Point(12, 5);
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.Size = new System.Drawing.Size(64, 64);
            this.PictureBox1.TabIndex = 24;
            this.PictureBox1.TabStop = false;
            // 
            // lbl_update_text
            // 
            this.lbl_update_text.AutoSize = true;
            this.lbl_update_text.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_update_text.Location = new System.Drawing.Point(91, 31);
            this.lbl_update_text.Name = "lbl_update_text";
            this.lbl_update_text.Size = new System.Drawing.Size(489, 13);
            this.lbl_update_text.TabIndex = 58;
            this.lbl_update_text.Text = "HandBrake {0.0.0} (000000000) is now available. (You have: {0.0.0} (000000000))";
            // 
            // frmUpdater
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(609, 282);
            this.Controls.Add(this.lbl_update_text);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.btn_remindLater);
            this.Controls.Add(this.btn_installUpdate);
            this.Controls.Add(this.btn_skip);
            this.Controls.Add(this.wBrowser);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.PictureBox1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmUpdater";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Update";
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.PictureBox PictureBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.WebBrowser wBrowser;
        internal System.Windows.Forms.Button btn_skip;
        internal System.Windows.Forms.Button btn_installUpdate;
        internal System.Windows.Forms.Button btn_remindLater;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lbl_update_text;
    }
}