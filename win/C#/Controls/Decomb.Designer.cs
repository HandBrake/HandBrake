/*  Decomb.designer.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */
namespace Handbrake
{
    partial class Decomb
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
            this.drop_decomb = new System.Windows.Forms.ComboBox();
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
            this.label18.Size = new System.Drawing.Size(59, 13);
            this.label18.TabIndex = 43;
            this.label18.Text = "Decomb:";
            // 
            // drop_decomb
            // 
            this.drop_decomb.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_decomb.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_decomb.FormattingEnabled = true;
            this.drop_decomb.Items.AddRange(new object[] {
            "Off",
            "Default",
            "Custom"});
            this.drop_decomb.Location = new System.Drawing.Point(111, 4);
            this.drop_decomb.Name = "drop_decomb";
            this.drop_decomb.Size = new System.Drawing.Size(161, 21);
            this.drop_decomb.TabIndex = 42;
            this.drop_decomb.SelectedIndexChanged += new System.EventHandler(this.drop_decomb_SelectedIndexChanged);
            // 
            // text_custom
            // 
            this.text_custom.Location = new System.Drawing.Point(278, 4);
            this.text_custom.Name = "text_custom";
            this.text_custom.Size = new System.Drawing.Size(115, 20);
            this.text_custom.TabIndex = 44;
            this.text_custom.Visible = false;
            // 
            // Decomb
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.AutoSize = true;
            this.Controls.Add(this.text_custom);
            this.Controls.Add(this.label18);
            this.Controls.Add(this.drop_decomb);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.MaximumSize = new System.Drawing.Size(400, 30);
            this.Name = "Decomb";
            this.Size = new System.Drawing.Size(400, 30);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label label18;
        internal System.Windows.Forms.ComboBox drop_decomb;
        private System.Windows.Forms.TextBox text_custom;
    }
}
