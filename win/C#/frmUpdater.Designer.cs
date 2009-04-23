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
            this.btn_skip = new System.Windows.Forms.Button();
            this.btn_installUpdate = new System.Windows.Forms.Button();
            this.btn_remindLater = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.lbl_update_text = new System.Windows.Forms.Label();
            this.panel1 = new System.Windows.Forms.Panel();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.wBrowser = new System.Windows.Forms.WebBrowser();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.button_container = new System.Windows.Forms.SplitContainer();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.panel1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.button_container.Panel1.SuspendLayout();
            this.button_container.Panel2.SuspendLayout();
            this.button_container.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(82, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(305, 16);
            this.label1.TabIndex = 25;
            this.label1.Text = "A New Version of Handbrake is available!";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(82, 49);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(208, 13);
            this.label6.TabIndex = 30;
            this.label6.Text = "Would you like to download it now?";
            // 
            // btn_skip
            // 
            this.btn_skip.AutoSize = true;
            this.btn_skip.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btn_skip.BackColor = System.Drawing.Color.Transparent;
            this.btn_skip.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_skip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_skip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_skip.Location = new System.Drawing.Point(0, 3);
            this.btn_skip.Name = "btn_skip";
            this.btn_skip.Size = new System.Drawing.Size(129, 23);
            this.btn_skip.TabIndex = 54;
            this.btn_skip.Text = "Skip This Version";
            this.btn_skip.UseVisualStyleBackColor = false;
            this.btn_skip.Click += new System.EventHandler(this.btn_skip_Click);
            // 
            // btn_installUpdate
            // 
            this.btn_installUpdate.AutoSize = true;
            this.btn_installUpdate.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btn_installUpdate.BackColor = System.Drawing.Color.Transparent;
            this.btn_installUpdate.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_installUpdate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_installUpdate.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_installUpdate.Location = new System.Drawing.Point(153, 3);
            this.btn_installUpdate.Name = "btn_installUpdate";
            this.btn_installUpdate.Size = new System.Drawing.Size(109, 23);
            this.btn_installUpdate.TabIndex = 55;
            this.btn_installUpdate.Text = "Install Update";
            this.btn_installUpdate.UseVisualStyleBackColor = false;
            this.btn_installUpdate.Click += new System.EventHandler(this.btn_installUpdate_Click);
            // 
            // btn_remindLater
            // 
            this.btn_remindLater.AutoSize = true;
            this.btn_remindLater.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.btn_remindLater.BackColor = System.Drawing.Color.Transparent;
            this.btn_remindLater.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_remindLater.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_remindLater.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_remindLater.Location = new System.Drawing.Point(19, 3);
            this.btn_remindLater.Name = "btn_remindLater";
            this.btn_remindLater.Size = new System.Drawing.Size(127, 23);
            this.btn_remindLater.TabIndex = 56;
            this.btn_remindLater.Text = "Remind me Later";
            this.btn_remindLater.UseVisualStyleBackColor = false;
            this.btn_remindLater.Click += new System.EventHandler(this.btn_remindLater_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(82, 84);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(103, 13);
            this.label3.TabIndex = 57;
            this.label3.Text = "Release Notes:";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = global::Handbrake.Properties.Resources.logo64;
            this.PictureBox1.InitialImage = null;
            this.PictureBox1.Location = new System.Drawing.Point(12, 12);
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.Size = new System.Drawing.Size(64, 64);
            this.PictureBox1.TabIndex = 24;
            this.PictureBox1.TabStop = false;
            // 
            // lbl_update_text
            // 
            this.lbl_update_text.AutoSize = true;
            this.lbl_update_text.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_update_text.Location = new System.Drawing.Point(82, 34);
            this.lbl_update_text.Name = "lbl_update_text";
            this.lbl_update_text.Size = new System.Drawing.Size(489, 13);
            this.lbl_update_text.TabIndex = 58;
            this.lbl_update_text.Text = "HandBrake {0.0.0} (000000000) is now available. (You have: {0.0.0} (000000000))";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.PictureBox1);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.lbl_update_text);
            this.panel1.Controls.Add(this.label6);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(712, 105);
            this.panel1.TabIndex = 60;
            // 
            // statusStrip1
            // 
            this.statusStrip1.Location = new System.Drawing.Point(0, 419);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(712, 22);
            this.statusStrip1.TabIndex = 65;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // panel3
            // 
            this.panel3.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel3.Location = new System.Drawing.Point(0, 105);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(87, 314);
            this.panel3.TabIndex = 67;
            // 
            // panel5
            // 
            this.panel5.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel5.Location = new System.Drawing.Point(683, 105);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(29, 314);
            this.panel5.TabIndex = 68;
            // 
            // wBrowser
            // 
            this.wBrowser.Dock = System.Windows.Forms.DockStyle.Fill;
            this.wBrowser.Location = new System.Drawing.Point(0, 0);
            this.wBrowser.MinimumSize = new System.Drawing.Size(20, 20);
            this.wBrowser.Name = "wBrowser";
            this.wBrowser.Size = new System.Drawing.Size(596, 264);
            this.wBrowser.TabIndex = 31;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.splitContainer1.Location = new System.Drawing.Point(87, 105);
            this.splitContainer1.Margin = new System.Windows.Forms.Padding(0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.wBrowser);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.button_container);
            this.splitContainer1.Size = new System.Drawing.Size(596, 314);
            this.splitContainer1.SplitterDistance = 264;
            this.splitContainer1.TabIndex = 69;
            // 
            // button_container
            // 
            this.button_container.Dock = System.Windows.Forms.DockStyle.Fill;
            this.button_container.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.button_container.Location = new System.Drawing.Point(0, 0);
            this.button_container.Name = "button_container";
            // 
            // button_container.Panel1
            // 
            this.button_container.Panel1.Controls.Add(this.btn_skip);
            // 
            // button_container.Panel2
            // 
            this.button_container.Panel2.Controls.Add(this.btn_remindLater);
            this.button_container.Panel2.Controls.Add(this.btn_installUpdate);
            this.button_container.Size = new System.Drawing.Size(596, 46);
            this.button_container.SplitterDistance = 330;
            this.button_container.TabIndex = 0;
            // 
            // frmUpdater
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(712, 441);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.panel1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimumSize = new System.Drawing.Size(720, 470);
            this.Name = "frmUpdater";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Update";
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.button_container.Panel1.ResumeLayout(false);
            this.button_container.Panel1.PerformLayout();
            this.button_container.Panel2.ResumeLayout(false);
            this.button_container.Panel2.PerformLayout();
            this.button_container.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.PictureBox PictureBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label6;
        internal System.Windows.Forms.Button btn_skip;
        internal System.Windows.Forms.Button btn_installUpdate;
        internal System.Windows.Forms.Button btn_remindLater;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lbl_update_text;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.WebBrowser wBrowser;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.SplitContainer button_container;
    }
}