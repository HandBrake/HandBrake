/*  Denoise.designer.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */
namespace Handbrake
{
    partial class Denoise
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
            this.label18 = new System.Windows.Forms.Label();
            this.drop_denoise = new System.Windows.Forms.ComboBox();
            this.text_custom = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.BackColor = System.Drawing.Color.Transparent;
            this.label18.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label18.Location = new System.Drawing.Point(3, 7);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(58, 13);
            this.label18.TabIndex = 43;
            this.label18.Text = "Denoise:";
            // 
            // drop_denoise
            // 
            this.drop_denoise.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_denoise.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_denoise.FormattingEnabled = true;
            this.drop_denoise.Items.AddRange(new object[] {
            "None",
            "Weak",
            "Medium",
            "Strong",
            "Custom"});
            this.drop_denoise.Location = new System.Drawing.Point(111, 4);
            this.drop_denoise.Name = "drop_denoise";
            this.drop_denoise.Size = new System.Drawing.Size(161, 21);
            this.drop_denoise.TabIndex = 42;
            this.drop_denoise.SelectedIndexChanged += new System.EventHandler(this.drop_decomb_SelectedIndexChanged);
            // 
            // text_custom
            // 
            this.text_custom.Location = new System.Drawing.Point(278, 4);
            this.text_custom.Name = "text_custom";
            this.text_custom.Size = new System.Drawing.Size(115, 20);
            this.text_custom.TabIndex = 44;
            this.text_custom.Visible = false;
            // 
            // Denoise
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.AutoSize = true;
            this.Controls.Add(this.text_custom);
            this.Controls.Add(this.label18);
            this.Controls.Add(this.drop_denoise);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.MaximumSize = new System.Drawing.Size(400, 30);
            this.Name = "Denoise";
            this.Size = new System.Drawing.Size(400, 30);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label label18;
        internal System.Windows.Forms.ComboBox drop_denoise;
        private System.Windows.Forms.TextBox text_custom;
    }
}
