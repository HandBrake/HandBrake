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
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // rtf_actLog
            // 
            this.rtf_actLog.Cursor = System.Windows.Forms.Cursors.IBeam;
            this.rtf_actLog.DetectUrls = false;
            this.rtf_actLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.rtf_actLog.Location = new System.Drawing.Point(0, 0);
            this.rtf_actLog.Name = "rtf_actLog";
            this.rtf_actLog.ReadOnly = true;
            this.rtf_actLog.Size = new System.Drawing.Size(390, 390);
            this.rtf_actLog.TabIndex = 29;
            this.rtf_actLog.Text = "";
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.rtf_actLog);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(390, 390);
            this.panel1.TabIndex = 95;
            // 
            // frmActivityWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(390, 390);
            this.Controls.Add(this.panel1);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmActivityWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Activity Window";
            this.Load += new System.EventHandler(this.frmActivityWindow_Load);
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.RichTextBox rtf_actLog;
        internal System.Windows.Forms.ToolTip ToolTip;
        private System.Windows.Forms.Panel panel1;
    }
}