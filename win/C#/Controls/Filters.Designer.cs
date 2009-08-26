namespace Handbrake.Controls
{
    partial class Filters
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
            this.text_customDT = new System.Windows.Forms.TextBox();
            this.label18 = new System.Windows.Forms.Label();
            this.drop_detelecine = new System.Windows.Forms.ComboBox();
            this.text_customDC = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.drop_decomb = new System.Windows.Forms.ComboBox();
            this.text_customDI = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.drop_deinterlace = new System.Windows.Forms.ComboBox();
            this.text_customDN = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.drop_denoise = new System.Windows.Forms.ComboBox();
            this.slider_deblock = new System.Windows.Forms.TrackBar();
            this.check_grayscale = new System.Windows.Forms.CheckBox();
            this.label8 = new System.Windows.Forms.Label();
            this.lbl_deblockVal = new System.Windows.Forms.Label();
            this.label68 = new System.Windows.Forms.Label();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            ((System.ComponentModel.ISupportInitialize)(this.slider_deblock)).BeginInit();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // text_customDT
            // 
            this.text_customDT.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_customDT.Location = new System.Drawing.Point(251, 3);
            this.text_customDT.Name = "text_customDT";
            this.text_customDT.Size = new System.Drawing.Size(115, 21);
            this.text_customDT.TabIndex = 47;
            this.text_customDT.Visible = false;
            // 
            // label18
            // 
            this.label18.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label18.AutoSize = true;
            this.label18.BackColor = System.Drawing.Color.Transparent;
            this.label18.Location = new System.Drawing.Point(0, 7);
            this.label18.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(61, 13);
            this.label18.TabIndex = 46;
            this.label18.Text = "Detelecine:";
            // 
            // drop_detelecine
            // 
            this.drop_detelecine.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.drop_detelecine.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_detelecine.FormattingEnabled = true;
            this.drop_detelecine.Items.AddRange(new object[] {
            "Off",
            "Custom",
            "Default"});
            this.drop_detelecine.Location = new System.Drawing.Point(71, 3);
            this.drop_detelecine.Name = "drop_detelecine";
            this.drop_detelecine.Size = new System.Drawing.Size(174, 21);
            this.drop_detelecine.TabIndex = 45;
            this.drop_detelecine.SelectedIndexChanged += new System.EventHandler(this.drop_detelecine_SelectedIndexChanged);
            // 
            // text_customDC
            // 
            this.text_customDC.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_customDC.Location = new System.Drawing.Point(251, 30);
            this.text_customDC.Name = "text_customDC";
            this.text_customDC.Size = new System.Drawing.Size(115, 21);
            this.text_customDC.TabIndex = 50;
            this.text_customDC.Visible = false;
            // 
            // label1
            // 
            this.label1.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Location = new System.Drawing.Point(0, 34);
            this.label1.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(49, 13);
            this.label1.TabIndex = 49;
            this.label1.Text = "Decomb:";
            // 
            // drop_decomb
            // 
            this.drop_decomb.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.drop_decomb.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_decomb.FormattingEnabled = true;
            this.drop_decomb.Items.AddRange(new object[] {
            "Off",
            "Custom",
            "Default"});
            this.drop_decomb.Location = new System.Drawing.Point(71, 30);
            this.drop_decomb.Name = "drop_decomb";
            this.drop_decomb.Size = new System.Drawing.Size(174, 21);
            this.drop_decomb.TabIndex = 48;
            this.drop_decomb.SelectedIndexChanged += new System.EventHandler(this.drop_decomb_SelectedIndexChanged);
            // 
            // text_customDI
            // 
            this.text_customDI.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_customDI.Location = new System.Drawing.Point(251, 57);
            this.text_customDI.Name = "text_customDI";
            this.text_customDI.Size = new System.Drawing.Size(115, 21);
            this.text_customDI.TabIndex = 53;
            this.text_customDI.Visible = false;
            // 
            // label2
            // 
            this.label2.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label2.AutoSize = true;
            this.label2.BackColor = System.Drawing.Color.Transparent;
            this.label2.Location = new System.Drawing.Point(0, 61);
            this.label2.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(65, 13);
            this.label2.TabIndex = 52;
            this.label2.Text = "Deinterlace:";
            // 
            // drop_deinterlace
            // 
            this.drop_deinterlace.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.drop_deinterlace.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deinterlace.FormattingEnabled = true;
            this.drop_deinterlace.Items.AddRange(new object[] {
            "Off",
            "Custom",
            "Fast",
            "Slow",
            "Slower"});
            this.drop_deinterlace.Location = new System.Drawing.Point(71, 57);
            this.drop_deinterlace.Name = "drop_deinterlace";
            this.drop_deinterlace.Size = new System.Drawing.Size(174, 21);
            this.drop_deinterlace.TabIndex = 51;
            this.drop_deinterlace.SelectedIndexChanged += new System.EventHandler(this.drop_deinterlace_SelectedIndexChanged);
            // 
            // text_customDN
            // 
            this.text_customDN.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_customDN.Location = new System.Drawing.Point(251, 84);
            this.text_customDN.Name = "text_customDN";
            this.text_customDN.Size = new System.Drawing.Size(115, 21);
            this.text_customDN.TabIndex = 56;
            this.text_customDN.Visible = false;
            // 
            // label3
            // 
            this.label3.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label3.AutoSize = true;
            this.label3.BackColor = System.Drawing.Color.Transparent;
            this.label3.Location = new System.Drawing.Point(0, 88);
            this.label3.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(49, 13);
            this.label3.TabIndex = 55;
            this.label3.Text = "Denoise:";
            // 
            // drop_denoise
            // 
            this.drop_denoise.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.drop_denoise.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_denoise.FormattingEnabled = true;
            this.drop_denoise.Items.AddRange(new object[] {
            "Off",
            "Custom",
            "Weak",
            "Medium",
            "Strong"});
            this.drop_denoise.Location = new System.Drawing.Point(71, 84);
            this.drop_denoise.Name = "drop_denoise";
            this.drop_denoise.Size = new System.Drawing.Size(174, 21);
            this.drop_denoise.TabIndex = 54;
            this.drop_denoise.SelectedIndexChanged += new System.EventHandler(this.drop_denoise_SelectedIndexChanged);
            // 
            // slider_deblock
            // 
            this.slider_deblock.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.slider_deblock.Location = new System.Drawing.Point(71, 111);
            this.slider_deblock.Maximum = 15;
            this.slider_deblock.Minimum = 4;
            this.slider_deblock.Name = "slider_deblock";
            this.slider_deblock.Size = new System.Drawing.Size(174, 45);
            this.slider_deblock.TabIndex = 58;
            this.slider_deblock.Value = 4;
            this.slider_deblock.Scroll += new System.EventHandler(this.slider_deblock_Scroll);
            // 
            // check_grayscale
            // 
            this.check_grayscale.AutoSize = true;
            this.check_grayscale.BackColor = System.Drawing.Color.Transparent;
            this.check_grayscale.Location = new System.Drawing.Point(71, 162);
            this.check_grayscale.Name = "check_grayscale";
            this.check_grayscale.Size = new System.Drawing.Size(119, 17);
            this.check_grayscale.TabIndex = 57;
            this.check_grayscale.Text = "Grayscale Encoding";
            this.check_grayscale.UseVisualStyleBackColor = false;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.BackColor = System.Drawing.Color.Transparent;
            this.label8.Location = new System.Drawing.Point(0, 115);
            this.label8.Margin = new System.Windows.Forms.Padding(0, 7, 3, 3);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(48, 13);
            this.label8.TabIndex = 60;
            this.label8.Text = "Deblock:";
            // 
            // lbl_deblockVal
            // 
            this.lbl_deblockVal.AutoSize = true;
            this.lbl_deblockVal.BackColor = System.Drawing.Color.Transparent;
            this.lbl_deblockVal.Location = new System.Drawing.Point(251, 115);
            this.lbl_deblockVal.Margin = new System.Windows.Forms.Padding(3, 7, 3, 3);
            this.lbl_deblockVal.Name = "lbl_deblockVal";
            this.lbl_deblockVal.Size = new System.Drawing.Size(23, 13);
            this.lbl_deblockVal.TabIndex = 59;
            this.lbl_deblockVal.Text = "Off";
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(42, 13);
            this.label68.TabIndex = 61;
            this.label68.Text = "Filters";
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.ColumnCount = 3;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.Controls.Add(this.label18, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.label1, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.lbl_deblockVal, 2, 4);
            this.tableLayoutPanel1.Controls.Add(this.check_grayscale, 1, 5);
            this.tableLayoutPanel1.Controls.Add(this.text_customDN, 2, 3);
            this.tableLayoutPanel1.Controls.Add(this.slider_deblock, 1, 4);
            this.tableLayoutPanel1.Controls.Add(this.text_customDI, 2, 2);
            this.tableLayoutPanel1.Controls.Add(this.label2, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.text_customDC, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.label3, 0, 3);
            this.tableLayoutPanel1.Controls.Add(this.text_customDT, 2, 0);
            this.tableLayoutPanel1.Controls.Add(this.label8, 0, 4);
            this.tableLayoutPanel1.Controls.Add(this.drop_detelecine, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.drop_denoise, 1, 3);
            this.tableLayoutPanel1.Controls.Add(this.drop_decomb, 1, 1);
            this.tableLayoutPanel1.Controls.Add(this.drop_deinterlace, 1, 2);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(19, 29);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 6;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.Size = new System.Drawing.Size(369, 182);
            this.tableLayoutPanel1.TabIndex = 62;
            // 
            // Filters
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.label68);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "Filters";
            this.Size = new System.Drawing.Size(592, 270);
            ((System.ComponentModel.ISupportInitialize)(this.slider_deblock)).EndInit();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox text_customDT;
        private System.Windows.Forms.Label label18;
        internal System.Windows.Forms.ComboBox drop_detelecine;
        private System.Windows.Forms.TextBox text_customDC;
        internal System.Windows.Forms.Label label1;
        internal System.Windows.Forms.ComboBox drop_decomb;
        private System.Windows.Forms.TextBox text_customDI;
        internal System.Windows.Forms.Label label2;
        internal System.Windows.Forms.ComboBox drop_deinterlace;
        private System.Windows.Forms.TextBox text_customDN;
        internal System.Windows.Forms.Label label3;
        internal System.Windows.Forms.ComboBox drop_denoise;
        internal System.Windows.Forms.TrackBar slider_deblock;
        internal System.Windows.Forms.CheckBox check_grayscale;
        internal System.Windows.Forms.Label label8;
        internal System.Windows.Forms.Label lbl_deblockVal;
        internal System.Windows.Forms.Label label68;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    }
}
