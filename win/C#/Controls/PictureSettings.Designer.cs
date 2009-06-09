using System.Windows.Forms;

namespace Handbrake.Controls
{
    partial class PictureSettings
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
            this.text_height = new System.Windows.Forms.NumericUpDown();
            this.text_width = new System.Windows.Forms.NumericUpDown();
            this.label4 = new System.Windows.Forms.Label();
            this.lbl_max = new System.Windows.Forms.Label();
            this.lbl_src_res = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.drp_anamorphic = new System.Windows.Forms.ComboBox();
            this.label26 = new System.Windows.Forms.Label();
            this.lbl_Aspect = new System.Windows.Forms.Label();
            this.Label91 = new System.Windows.Forms.Label();
            this.Label55 = new System.Windows.Forms.Label();
            this.lbl_modulus = new System.Windows.Forms.Label();
            this.drop_modulus = new System.Windows.Forms.ComboBox();
            this.txt_displayWidth = new System.Windows.Forms.TextBox();
            this.lbl_displayWidth = new System.Windows.Forms.Label();
            this.lbl_parWidth = new System.Windows.Forms.Label();
            this.txt_parWidth = new System.Windows.Forms.TextBox();
            this.lbl_parHeight = new System.Windows.Forms.Label();
            this.txt_parHeight = new System.Windows.Forms.TextBox();
            this.check_KeepAR = new System.Windows.Forms.CheckBox();
            this.check_customCrop = new System.Windows.Forms.RadioButton();
            this.check_autoCrop = new System.Windows.Forms.RadioButton();
            this.crop_bottom = new System.Windows.Forms.NumericUpDown();
            this.crop_top = new System.Windows.Forms.NumericUpDown();
            this.crop_left = new System.Windows.Forms.NumericUpDown();
            this.crop_right = new System.Windows.Forms.NumericUpDown();
            this.label8 = new System.Windows.Forms.Label();
            this.Label53 = new System.Windows.Forms.Label();
            this.Label52 = new System.Windows.Forms.Label();
            this.Label51 = new System.Windows.Forms.Label();
            this.Label15 = new System.Windows.Forms.Label();
            this.lbl_anamorphic = new System.Windows.Forms.Label();
            this.lbl_anamprohicLbl = new System.Windows.Forms.Label();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.text_height)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_width)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_bottom)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_top)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_left)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_right)).BeginInit();
            this.SuspendLayout();
            // 
            // text_height
            // 
            this.text_height.Location = new System.Drawing.Point(206, 59);
            this.text_height.Maximum = new decimal(new int[] {
            2560,
            0,
            0,
            0});
            this.text_height.Minimum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 20);
            this.text_height.TabIndex = 50;
            this.text_height.Value = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_height.ValueChanged += new System.EventHandler(this.text_height_ValueChanged);
            // 
            // text_width
            // 
            this.text_width.Location = new System.Drawing.Point(68, 59);
            this.text_width.Maximum = new decimal(new int[] {
            2560,
            0,
            0,
            0});
            this.text_width.Minimum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 20);
            this.text_width.TabIndex = 49;
            this.text_width.Value = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_width.ValueChanged += new System.EventHandler(this.text_width_ValueChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.Color.Transparent;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.ForeColor = System.Drawing.Color.Black;
            this.label4.Location = new System.Drawing.Point(152, 63);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(48, 13);
            this.label4.TabIndex = 48;
            this.label4.Text = "Height:";
            // 
            // lbl_max
            // 
            this.lbl_max.AutoSize = true;
            this.lbl_max.Location = new System.Drawing.Point(273, 80);
            this.lbl_max.Name = "lbl_max";
            this.lbl_max.Size = new System.Drawing.Size(34, 13);
            this.lbl_max.TabIndex = 47;
            this.lbl_max.Text = "{max}";
            // 
            // lbl_src_res
            // 
            this.lbl_src_res.AutoSize = true;
            this.lbl_src_res.BackColor = System.Drawing.Color.Transparent;
            this.lbl_src_res.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_src_res.Location = new System.Drawing.Point(66, 36);
            this.lbl_src_res.Name = "lbl_src_res";
            this.lbl_src_res.Size = new System.Drawing.Size(72, 12);
            this.lbl_src_res.TabIndex = 41;
            this.lbl_src_res.Text = "Select a Title";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Location = new System.Drawing.Point(13, 35);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(44, 13);
            this.label7.TabIndex = 40;
            this.label7.Text = "Source:";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(13, 91);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(80, 13);
            this.label6.TabIndex = 45;
            this.label6.Text = "Anamorphic:";
            // 
            // drp_anamorphic
            // 
            this.drp_anamorphic.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_anamorphic.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_anamorphic.FormattingEnabled = true;
            this.drp_anamorphic.Items.AddRange(new object[] {
            "None",
            "Strict",
            "Loose",
            "Custom"});
            this.drp_anamorphic.Location = new System.Drawing.Point(109, 88);
            this.drp_anamorphic.Name = "drp_anamorphic";
            this.drp_anamorphic.Size = new System.Drawing.Size(110, 21);
            this.drp_anamorphic.TabIndex = 46;
            this.drp_anamorphic.SelectedIndexChanged += new System.EventHandler(this.drp_anamorphic_SelectedIndexChanged);
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.BackColor = System.Drawing.Color.Transparent;
            this.label26.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(13, 13);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(34, 13);
            this.label26.TabIndex = 39;
            this.label26.Text = "Size";
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AutoSize = true;
            this.lbl_Aspect.BackColor = System.Drawing.Color.Transparent;
            this.lbl_Aspect.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_Aspect.Location = new System.Drawing.Point(241, 36);
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.lbl_Aspect.Size = new System.Drawing.Size(72, 12);
            this.lbl_Aspect.TabIndex = 43;
            this.lbl_Aspect.Text = "Select a Title";
            // 
            // Label91
            // 
            this.Label91.AutoSize = true;
            this.Label91.BackColor = System.Drawing.Color.Transparent;
            this.Label91.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label91.Location = new System.Drawing.Point(152, 35);
            this.Label91.Name = "Label91";
            this.Label91.Size = new System.Drawing.Size(83, 13);
            this.Label91.TabIndex = 42;
            this.Label91.Text = "Aspect Ratio:";
            // 
            // Label55
            // 
            this.Label55.AutoSize = true;
            this.Label55.BackColor = System.Drawing.Color.Transparent;
            this.Label55.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Location = new System.Drawing.Point(13, 63);
            this.Label55.Name = "Label55";
            this.Label55.Size = new System.Drawing.Size(44, 13);
            this.Label55.TabIndex = 44;
            this.Label55.Text = "Width:";
            // 
            // lbl_modulus
            // 
            this.lbl_modulus.AutoSize = true;
            this.lbl_modulus.BackColor = System.Drawing.Color.Transparent;
            this.lbl_modulus.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_modulus.Location = new System.Drawing.Point(13, 118);
            this.lbl_modulus.Name = "lbl_modulus";
            this.lbl_modulus.Size = new System.Drawing.Size(58, 13);
            this.lbl_modulus.TabIndex = 51;
            this.lbl_modulus.Text = "Modulus:";
            // 
            // drop_modulus
            // 
            this.drop_modulus.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_modulus.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_modulus.FormattingEnabled = true;
            this.drop_modulus.Items.AddRange(new object[] {
            "16",
            "8",
            "4",
            "2"});
            this.drop_modulus.Location = new System.Drawing.Point(109, 115);
            this.drop_modulus.Name = "drop_modulus";
            this.drop_modulus.Size = new System.Drawing.Size(110, 21);
            this.drop_modulus.TabIndex = 52;
            // 
            // txt_displayWidth
            // 
            this.txt_displayWidth.Location = new System.Drawing.Point(109, 142);
            this.txt_displayWidth.Name = "txt_displayWidth";
            this.txt_displayWidth.Size = new System.Drawing.Size(100, 20);
            this.txt_displayWidth.TabIndex = 53;
            this.toolTip.SetToolTip(this.txt_displayWidth, "Display Width - Press \"Enter\" after entering a new value.");
            this.txt_displayWidth.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_displayWidth_Keyup);
            // 
            // lbl_displayWidth
            // 
            this.lbl_displayWidth.AutoSize = true;
            this.lbl_displayWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_displayWidth.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_displayWidth.Location = new System.Drawing.Point(13, 145);
            this.lbl_displayWidth.Name = "lbl_displayWidth";
            this.lbl_displayWidth.Size = new System.Drawing.Size(90, 13);
            this.lbl_displayWidth.TabIndex = 54;
            this.lbl_displayWidth.Text = "Display Width:";
            // 
            // lbl_parWidth
            // 
            this.lbl_parWidth.AutoSize = true;
            this.lbl_parWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parWidth.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_parWidth.Location = new System.Drawing.Point(13, 171);
            this.lbl_parWidth.Name = "lbl_parWidth";
            this.lbl_parWidth.Size = new System.Drawing.Size(71, 13);
            this.lbl_parWidth.TabIndex = 56;
            this.lbl_parWidth.Text = "PAR Width:";
            // 
            // txt_parWidth
            // 
            this.txt_parWidth.Location = new System.Drawing.Point(109, 168);
            this.txt_parWidth.Name = "txt_parWidth";
            this.txt_parWidth.Size = new System.Drawing.Size(100, 20);
            this.txt_parWidth.TabIndex = 55;
            this.toolTip.SetToolTip(this.txt_parWidth, "PAR Width - Press \"Enter\" after entering a new value.\r\n");
            this.txt_parWidth.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_parWidth_Keyup);
            // 
            // lbl_parHeight
            // 
            this.lbl_parHeight.AutoSize = true;
            this.lbl_parHeight.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parHeight.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_parHeight.Location = new System.Drawing.Point(13, 197);
            this.lbl_parHeight.Name = "lbl_parHeight";
            this.lbl_parHeight.Size = new System.Drawing.Size(75, 13);
            this.lbl_parHeight.TabIndex = 58;
            this.lbl_parHeight.Text = "PAR Height:";
            // 
            // txt_parHeight
            // 
            this.txt_parHeight.Location = new System.Drawing.Point(109, 194);
            this.txt_parHeight.Name = "txt_parHeight";
            this.txt_parHeight.Size = new System.Drawing.Size(100, 20);
            this.txt_parHeight.TabIndex = 57;
            this.toolTip.SetToolTip(this.txt_parHeight, "PAR Height - Press \"Enter\" after entering a new value.\r\n");
            this.txt_parHeight.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_parHeight_Keyup);
            // 
            // check_KeepAR
            // 
            this.check_KeepAR.AutoSize = true;
            this.check_KeepAR.Location = new System.Drawing.Point(276, 60);
            this.check_KeepAR.Name = "check_KeepAR";
            this.check_KeepAR.Size = new System.Drawing.Size(69, 17);
            this.check_KeepAR.TabIndex = 59;
            this.check_KeepAR.Text = "Keep AR";
            this.check_KeepAR.UseVisualStyleBackColor = true;
            this.check_KeepAR.CheckedChanged += new System.EventHandler(this.check_KeepAR_CheckedChanged);
            // 
            // check_customCrop
            // 
            this.check_customCrop.AutoSize = true;
            this.check_customCrop.Location = new System.Drawing.Point(387, 57);
            this.check_customCrop.Name = "check_customCrop";
            this.check_customCrop.Size = new System.Drawing.Size(63, 17);
            this.check_customCrop.TabIndex = 70;
            this.check_customCrop.Text = "Custom:";
            this.check_customCrop.UseVisualStyleBackColor = true;
            this.check_customCrop.CheckedChanged += new System.EventHandler(this.check_customCrop_CheckedChanged);
            // 
            // check_autoCrop
            // 
            this.check_autoCrop.AutoSize = true;
            this.check_autoCrop.Checked = true;
            this.check_autoCrop.Location = new System.Drawing.Point(387, 33);
            this.check_autoCrop.Name = "check_autoCrop";
            this.check_autoCrop.Size = new System.Drawing.Size(72, 17);
            this.check_autoCrop.TabIndex = 69;
            this.check_autoCrop.TabStop = true;
            this.check_autoCrop.Text = "Automatic";
            this.check_autoCrop.UseVisualStyleBackColor = true;
            this.check_autoCrop.CheckedChanged += new System.EventHandler(this.check_autoCrop_CheckedChanged);
            // 
            // crop_bottom
            // 
            this.crop_bottom.Enabled = false;
            this.crop_bottom.Location = new System.Drawing.Point(467, 146);
            this.crop_bottom.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.crop_bottom.Name = "crop_bottom";
            this.crop_bottom.Size = new System.Drawing.Size(44, 20);
            this.crop_bottom.TabIndex = 67;
            // 
            // crop_top
            // 
            this.crop_top.Enabled = false;
            this.crop_top.Location = new System.Drawing.Point(467, 100);
            this.crop_top.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.crop_top.Name = "crop_top";
            this.crop_top.Size = new System.Drawing.Size(44, 20);
            this.crop_top.TabIndex = 64;
            // 
            // crop_left
            // 
            this.crop_left.Enabled = false;
            this.crop_left.Location = new System.Drawing.Point(416, 122);
            this.crop_left.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.crop_left.Name = "crop_left";
            this.crop_left.Size = new System.Drawing.Size(44, 20);
            this.crop_left.TabIndex = 62;
            // 
            // crop_right
            // 
            this.crop_right.Enabled = false;
            this.crop_right.Location = new System.Drawing.Point(518, 122);
            this.crop_right.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.crop_right.Name = "crop_right";
            this.crop_right.Size = new System.Drawing.Size(44, 20);
            this.crop_right.TabIndex = 65;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.BackColor = System.Drawing.Color.Transparent;
            this.label8.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(384, 13);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(37, 13);
            this.label8.TabIndex = 60;
            this.label8.Text = "Crop";
            // 
            // Label53
            // 
            this.Label53.AutoSize = true;
            this.Label53.BackColor = System.Drawing.Color.Transparent;
            this.Label53.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label53.Location = new System.Drawing.Point(465, 170);
            this.Label53.Name = "Label53";
            this.Label53.Size = new System.Drawing.Size(48, 13);
            this.Label53.TabIndex = 68;
            this.Label53.Text = "Bottom";
            // 
            // Label52
            // 
            this.Label52.AutoSize = true;
            this.Label52.BackColor = System.Drawing.Color.Transparent;
            this.Label52.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label52.Location = new System.Drawing.Point(474, 85);
            this.Label52.Name = "Label52";
            this.Label52.Size = new System.Drawing.Size(28, 13);
            this.Label52.TabIndex = 63;
            this.Label52.Text = "Top";
            // 
            // Label51
            // 
            this.Label51.AutoSize = true;
            this.Label51.BackColor = System.Drawing.Color.Transparent;
            this.Label51.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label51.Location = new System.Drawing.Point(561, 124);
            this.Label51.Name = "Label51";
            this.Label51.Size = new System.Drawing.Size(36, 13);
            this.Label51.TabIndex = 66;
            this.Label51.Text = "Right";
            // 
            // Label15
            // 
            this.Label15.AutoSize = true;
            this.Label15.BackColor = System.Drawing.Color.Transparent;
            this.Label15.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label15.Location = new System.Drawing.Point(384, 124);
            this.Label15.Name = "Label15";
            this.Label15.Size = new System.Drawing.Size(28, 13);
            this.Label15.TabIndex = 61;
            this.Label15.Text = "Left";
            // 
            // lbl_anamorphic
            // 
            this.lbl_anamorphic.AutoSize = true;
            this.lbl_anamorphic.BackColor = System.Drawing.Color.Transparent;
            this.lbl_anamorphic.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_anamorphic.Location = new System.Drawing.Point(107, 222);
            this.lbl_anamorphic.Name = "lbl_anamorphic";
            this.lbl_anamorphic.Size = new System.Drawing.Size(72, 12);
            this.lbl_anamorphic.TabIndex = 74;
            this.lbl_anamorphic.Text = "Select a Title";
            // 
            // lbl_anamprohicLbl
            // 
            this.lbl_anamprohicLbl.AutoSize = true;
            this.lbl_anamprohicLbl.BackColor = System.Drawing.Color.Transparent;
            this.lbl_anamprohicLbl.Location = new System.Drawing.Point(13, 221);
            this.lbl_anamprohicLbl.Name = "lbl_anamprohicLbl";
            this.lbl_anamprohicLbl.Size = new System.Drawing.Size(66, 13);
            this.lbl_anamprohicLbl.TabIndex = 73;
            this.lbl_anamprohicLbl.Text = "Anamorphic:";
            // 
            // PictureSettings
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.lbl_anamorphic);
            this.Controls.Add(this.lbl_anamprohicLbl);
            this.Controls.Add(this.check_customCrop);
            this.Controls.Add(this.check_autoCrop);
            this.Controls.Add(this.crop_bottom);
            this.Controls.Add(this.crop_top);
            this.Controls.Add(this.crop_left);
            this.Controls.Add(this.crop_right);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.Label53);
            this.Controls.Add(this.Label52);
            this.Controls.Add(this.Label51);
            this.Controls.Add(this.Label15);
            this.Controls.Add(this.check_KeepAR);
            this.Controls.Add(this.lbl_parHeight);
            this.Controls.Add(this.txt_parHeight);
            this.Controls.Add(this.lbl_parWidth);
            this.Controls.Add(this.txt_parWidth);
            this.Controls.Add(this.lbl_displayWidth);
            this.Controls.Add(this.txt_displayWidth);
            this.Controls.Add(this.lbl_modulus);
            this.Controls.Add(this.drop_modulus);
            this.Controls.Add(this.text_height);
            this.Controls.Add(this.text_width);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.lbl_max);
            this.Controls.Add(this.lbl_src_res);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.drp_anamorphic);
            this.Controls.Add(this.label26);
            this.Controls.Add(this.lbl_Aspect);
            this.Controls.Add(this.Label91);
            this.Controls.Add(this.Label55);
            this.Name = "PictureSettings";
            this.Size = new System.Drawing.Size(666, 279);
            ((System.ComponentModel.ISupportInitialize)(this.text_height)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_width)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_bottom)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_top)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_left)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_right)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label lbl_max;
        internal System.Windows.Forms.Label lbl_src_res;
        internal System.Windows.Forms.Label label7;
        internal System.Windows.Forms.Label label6;
        internal System.Windows.Forms.ComboBox drp_anamorphic;
        internal System.Windows.Forms.Label label26;
        internal System.Windows.Forms.Label lbl_Aspect;
        internal System.Windows.Forms.Label Label91;
        internal System.Windows.Forms.Label Label55;
        internal System.Windows.Forms.Label lbl_modulus;
        internal System.Windows.Forms.ComboBox drop_modulus;
        internal System.Windows.Forms.Label lbl_displayWidth;
        internal System.Windows.Forms.Label lbl_parWidth;
        internal System.Windows.Forms.Label lbl_parHeight;
        internal System.Windows.Forms.RadioButton check_customCrop;
        internal System.Windows.Forms.RadioButton check_autoCrop;
        internal System.Windows.Forms.NumericUpDown crop_bottom;
        internal System.Windows.Forms.NumericUpDown crop_top;
        internal System.Windows.Forms.NumericUpDown crop_left;
        internal System.Windows.Forms.NumericUpDown crop_right;
        internal System.Windows.Forms.Label label8;
        internal System.Windows.Forms.Label Label53;
        internal System.Windows.Forms.Label Label52;
        internal System.Windows.Forms.Label Label51;
        internal System.Windows.Forms.Label Label15;
        internal System.Windows.Forms.NumericUpDown text_height;
        internal System.Windows.Forms.NumericUpDown text_width;
        internal System.Windows.Forms.TextBox txt_displayWidth;
        internal System.Windows.Forms.TextBox txt_parWidth;
        internal System.Windows.Forms.TextBox txt_parHeight;
        internal System.Windows.Forms.CheckBox check_KeepAR;
        internal System.Windows.Forms.Label lbl_anamorphic;
        internal System.Windows.Forms.Label lbl_anamprohicLbl;
        private ToolTip toolTip;

    }
}
