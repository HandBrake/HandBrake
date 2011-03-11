/*  x264Panel.Designer.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    partial class AdvancedEncoderOpts
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.label43 = new System.Windows.Forms.Label();
            this.advancedQuery = new System.Windows.Forms.RichTextBox();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.label64 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label43
            // 
            this.label43.AutoSize = true;
            this.label43.BackColor = System.Drawing.Color.Transparent;
            this.label43.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label43.Location = new System.Drawing.Point(13, 13);
            this.label43.Name = "label43";
            this.label43.Size = new System.Drawing.Size(63, 13);
            this.label43.TabIndex = 49;
            this.label43.Text = "Advanced";
            // 
            // advancedQuery
            // 
            this.advancedQuery.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.advancedQuery.Location = new System.Drawing.Point(16, 53);
            this.advancedQuery.Name = "advancedQuery";
            this.advancedQuery.Size = new System.Drawing.Size(688, 128);
            this.advancedQuery.TabIndex = 80;
            this.advancedQuery.Text = "";
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            // 
            // label64
            // 
            this.label64.AutoSize = true;
            this.label64.Location = new System.Drawing.Point(13, 35);
            this.label64.Name = "label64";
            this.label64.Size = new System.Drawing.Size(163, 13);
            this.label64.TabIndex = 81;
            this.label64.Text = "Encoder advanced option string:";
            // 
            // x264Panel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.label64);
            this.Controls.Add(this.advancedQuery);
            this.Controls.Add(this.label43);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Size = new System.Drawing.Size(720, 305);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label label43;
        internal System.Windows.Forms.RichTextBox advancedQuery;
        private System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.Label label64;
    }
}
