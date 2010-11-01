/*  frmUpdater.Designer.cs 
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Views
{
    partial class UpdateInfo
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UpdateInfo));
            this.label1 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.btn_skip = new System.Windows.Forms.Button();
            this.btn_installUpdate = new System.Windows.Forms.Button();
            this.btn_remindLater = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.lbl_update_text = new System.Windows.Forms.Label();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.wBrowser = new System.Windows.Forms.WebBrowser();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.button_container = new System.Windows.Forms.SplitContainer();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
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
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            // 
            // label6
            // 
            resources.ApplyResources(this.label6, "label6");
            this.label6.Name = "label6";
            // 
            // btn_skip
            // 
            resources.ApplyResources(this.btn_skip, "btn_skip");
            this.btn_skip.BackColor = System.Drawing.Color.Transparent;
            this.btn_skip.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_skip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_skip.Name = "btn_skip";
            this.btn_skip.UseVisualStyleBackColor = false;
            this.btn_skip.Click += new System.EventHandler(this.BtnSkipClick);
            // 
            // btn_installUpdate
            // 
            resources.ApplyResources(this.btn_installUpdate, "btn_installUpdate");
            this.btn_installUpdate.BackColor = System.Drawing.Color.Transparent;
            this.btn_installUpdate.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_installUpdate.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_installUpdate.Name = "btn_installUpdate";
            this.btn_installUpdate.UseVisualStyleBackColor = false;
            this.btn_installUpdate.Click += new System.EventHandler(this.BtnInstallUpdateClick);
            // 
            // btn_remindLater
            // 
            resources.ApplyResources(this.btn_remindLater, "btn_remindLater");
            this.btn_remindLater.BackColor = System.Drawing.Color.Transparent;
            this.btn_remindLater.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_remindLater.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_remindLater.Name = "btn_remindLater";
            this.btn_remindLater.UseVisualStyleBackColor = false;
            this.btn_remindLater.Click += new System.EventHandler(this.BtnRemindLaterClick);
            // 
            // label3
            // 
            resources.ApplyResources(this.label3, "label3");
            this.label3.Name = "label3";
            // 
            // lbl_update_text
            // 
            resources.ApplyResources(this.lbl_update_text, "lbl_update_text");
            this.lbl_update_text.Name = "lbl_update_text";
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.White;
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.PictureBox1);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.lbl_update_text);
            this.panel1.Controls.Add(this.label6);
            resources.ApplyResources(this.panel1, "panel1");
            this.panel1.Name = "panel1";
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.SystemColors.Control;
            resources.ApplyResources(this.panel2, "panel2");
            this.panel2.MaximumSize = new System.Drawing.Size(0, 10);
            this.panel2.MinimumSize = new System.Drawing.Size(0, 10);
            this.panel2.Name = "panel2";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = global::HandBrake.Framework.Properties.Resources.logo64;
            resources.ApplyResources(this.PictureBox1, "PictureBox1");
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.TabStop = false;
            // 
            // statusStrip1
            // 
            resources.ApplyResources(this.statusStrip1, "statusStrip1");
            this.statusStrip1.Name = "statusStrip1";
            // 
            // panel3
            // 
            resources.ApplyResources(this.panel3, "panel3");
            this.panel3.Name = "panel3";
            // 
            // panel5
            // 
            resources.ApplyResources(this.panel5, "panel5");
            this.panel5.Name = "panel5";
            // 
            // wBrowser
            // 
            resources.ApplyResources(this.wBrowser, "wBrowser");
            this.wBrowser.MinimumSize = new System.Drawing.Size(20, 20);
            this.wBrowser.Name = "wBrowser";
            // 
            // splitContainer1
            // 
            resources.ApplyResources(this.splitContainer1, "splitContainer1");
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.wBrowser);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.button_container);
            // 
            // button_container
            // 
            resources.ApplyResources(this.button_container, "button_container");
            this.button_container.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
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
            // 
            // UpdateInfo
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.panel1);
            this.MaximizeBox = false;
            this.Name = "UpdateInfo";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
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
        private System.Windows.Forms.Panel panel2;
    }
}