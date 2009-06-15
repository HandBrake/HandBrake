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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PictureSettings));
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
            this.text_height.AccessibleDescription = null;
            this.text_height.AccessibleName = null;
            resources.ApplyResources(this.text_height, "text_height");
            this.text_height.Font = null;
            this.text_height.Maximum = new decimal(new int[] {
            2560,
            0,
            0,
            0});
            this.text_height.Name = "text_height";
            this.toolTip.SetToolTip(this.text_height, resources.GetString("text_height.ToolTip"));
            this.text_height.Value = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_height.ValueChanged += new System.EventHandler(this.text_height_ValueChanged);
            // 
            // text_width
            // 
            this.text_width.AccessibleDescription = null;
            this.text_width.AccessibleName = null;
            resources.ApplyResources(this.text_width, "text_width");
            this.text_width.Font = null;
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
            this.toolTip.SetToolTip(this.text_width, resources.GetString("text_width.ToolTip"));
            this.text_width.Value = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.text_width.ValueChanged += new System.EventHandler(this.text_width_ValueChanged);
            // 
            // label4
            // 
            this.label4.AccessibleDescription = null;
            this.label4.AccessibleName = null;
            resources.ApplyResources(this.label4, "label4");
            this.label4.BackColor = System.Drawing.Color.Transparent;
            this.label4.ForeColor = System.Drawing.Color.Black;
            this.label4.Name = "label4";
            this.toolTip.SetToolTip(this.label4, resources.GetString("label4.ToolTip"));
            // 
            // lbl_max
            // 
            this.lbl_max.AccessibleDescription = null;
            this.lbl_max.AccessibleName = null;
            resources.ApplyResources(this.lbl_max, "lbl_max");
            this.lbl_max.Font = null;
            this.lbl_max.Name = "lbl_max";
            this.toolTip.SetToolTip(this.lbl_max, resources.GetString("lbl_max.ToolTip"));
            // 
            // lbl_src_res
            // 
            this.lbl_src_res.AccessibleDescription = null;
            this.lbl_src_res.AccessibleName = null;
            resources.ApplyResources(this.lbl_src_res, "lbl_src_res");
            this.lbl_src_res.BackColor = System.Drawing.Color.Transparent;
            this.lbl_src_res.Name = "lbl_src_res";
            this.toolTip.SetToolTip(this.lbl_src_res, resources.GetString("lbl_src_res.ToolTip"));
            // 
            // label7
            // 
            this.label7.AccessibleDescription = null;
            this.label7.AccessibleName = null;
            resources.ApplyResources(this.label7, "label7");
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Font = null;
            this.label7.Name = "label7";
            this.toolTip.SetToolTip(this.label7, resources.GetString("label7.ToolTip"));
            // 
            // label6
            // 
            this.label6.AccessibleDescription = null;
            this.label6.AccessibleName = null;
            resources.ApplyResources(this.label6, "label6");
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Name = "label6";
            this.toolTip.SetToolTip(this.label6, resources.GetString("label6.ToolTip"));
            // 
            // drp_anamorphic
            // 
            this.drp_anamorphic.AccessibleDescription = null;
            this.drp_anamorphic.AccessibleName = null;
            resources.ApplyResources(this.drp_anamorphic, "drp_anamorphic");
            this.drp_anamorphic.BackgroundImage = null;
            this.drp_anamorphic.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_anamorphic.FormattingEnabled = true;
            this.drp_anamorphic.Items.AddRange(new object[] {
            resources.GetString("drp_anamorphic.Items"),
            resources.GetString("drp_anamorphic.Items1"),
            resources.GetString("drp_anamorphic.Items2"),
            resources.GetString("drp_anamorphic.Items3")});
            this.drp_anamorphic.Name = "drp_anamorphic";
            this.toolTip.SetToolTip(this.drp_anamorphic, resources.GetString("drp_anamorphic.ToolTip"));
            this.drp_anamorphic.SelectedIndexChanged += new System.EventHandler(this.drp_anamorphic_SelectedIndexChanged);
            // 
            // label26
            // 
            this.label26.AccessibleDescription = null;
            this.label26.AccessibleName = null;
            resources.ApplyResources(this.label26, "label26");
            this.label26.BackColor = System.Drawing.Color.Transparent;
            this.label26.Name = "label26";
            this.toolTip.SetToolTip(this.label26, resources.GetString("label26.ToolTip"));
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AccessibleDescription = null;
            this.lbl_Aspect.AccessibleName = null;
            resources.ApplyResources(this.lbl_Aspect, "lbl_Aspect");
            this.lbl_Aspect.BackColor = System.Drawing.Color.Transparent;
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.toolTip.SetToolTip(this.lbl_Aspect, resources.GetString("lbl_Aspect.ToolTip"));
            // 
            // Label91
            // 
            this.Label91.AccessibleDescription = null;
            this.Label91.AccessibleName = null;
            resources.ApplyResources(this.Label91, "Label91");
            this.Label91.BackColor = System.Drawing.Color.Transparent;
            this.Label91.Name = "Label91";
            this.toolTip.SetToolTip(this.Label91, resources.GetString("Label91.ToolTip"));
            // 
            // Label55
            // 
            this.Label55.AccessibleDescription = null;
            this.Label55.AccessibleName = null;
            resources.ApplyResources(this.Label55, "Label55");
            this.Label55.BackColor = System.Drawing.Color.Transparent;
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Name = "Label55";
            this.toolTip.SetToolTip(this.Label55, resources.GetString("Label55.ToolTip"));
            // 
            // lbl_modulus
            // 
            this.lbl_modulus.AccessibleDescription = null;
            this.lbl_modulus.AccessibleName = null;
            resources.ApplyResources(this.lbl_modulus, "lbl_modulus");
            this.lbl_modulus.BackColor = System.Drawing.Color.Transparent;
            this.lbl_modulus.Name = "lbl_modulus";
            this.toolTip.SetToolTip(this.lbl_modulus, resources.GetString("lbl_modulus.ToolTip"));
            // 
            // drop_modulus
            // 
            this.drop_modulus.AccessibleDescription = null;
            this.drop_modulus.AccessibleName = null;
            resources.ApplyResources(this.drop_modulus, "drop_modulus");
            this.drop_modulus.BackgroundImage = null;
            this.drop_modulus.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_modulus.FormattingEnabled = true;
            this.drop_modulus.Items.AddRange(new object[] {
            resources.GetString("drop_modulus.Items"),
            resources.GetString("drop_modulus.Items1"),
            resources.GetString("drop_modulus.Items2"),
            resources.GetString("drop_modulus.Items3")});
            this.drop_modulus.Name = "drop_modulus";
            this.toolTip.SetToolTip(this.drop_modulus, resources.GetString("drop_modulus.ToolTip"));
            // 
            // txt_displayWidth
            // 
            this.txt_displayWidth.AccessibleDescription = null;
            this.txt_displayWidth.AccessibleName = null;
            resources.ApplyResources(this.txt_displayWidth, "txt_displayWidth");
            this.txt_displayWidth.BackgroundImage = null;
            this.txt_displayWidth.Font = null;
            this.txt_displayWidth.Name = "txt_displayWidth";
            this.toolTip.SetToolTip(this.txt_displayWidth, resources.GetString("txt_displayWidth.ToolTip"));
            this.txt_displayWidth.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_displayWidth_Keyup);
            // 
            // lbl_displayWidth
            // 
            this.lbl_displayWidth.AccessibleDescription = null;
            this.lbl_displayWidth.AccessibleName = null;
            resources.ApplyResources(this.lbl_displayWidth, "lbl_displayWidth");
            this.lbl_displayWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_displayWidth.Name = "lbl_displayWidth";
            this.toolTip.SetToolTip(this.lbl_displayWidth, resources.GetString("lbl_displayWidth.ToolTip"));
            // 
            // lbl_parWidth
            // 
            this.lbl_parWidth.AccessibleDescription = null;
            this.lbl_parWidth.AccessibleName = null;
            resources.ApplyResources(this.lbl_parWidth, "lbl_parWidth");
            this.lbl_parWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parWidth.Name = "lbl_parWidth";
            this.toolTip.SetToolTip(this.lbl_parWidth, resources.GetString("lbl_parWidth.ToolTip"));
            // 
            // txt_parWidth
            // 
            this.txt_parWidth.AccessibleDescription = null;
            this.txt_parWidth.AccessibleName = null;
            resources.ApplyResources(this.txt_parWidth, "txt_parWidth");
            this.txt_parWidth.BackgroundImage = null;
            this.txt_parWidth.Font = null;
            this.txt_parWidth.Name = "txt_parWidth";
            this.toolTip.SetToolTip(this.txt_parWidth, resources.GetString("txt_parWidth.ToolTip"));
            this.txt_parWidth.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_parWidth_Keyup);
            // 
            // lbl_parHeight
            // 
            this.lbl_parHeight.AccessibleDescription = null;
            this.lbl_parHeight.AccessibleName = null;
            resources.ApplyResources(this.lbl_parHeight, "lbl_parHeight");
            this.lbl_parHeight.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parHeight.Name = "lbl_parHeight";
            this.toolTip.SetToolTip(this.lbl_parHeight, resources.GetString("lbl_parHeight.ToolTip"));
            // 
            // txt_parHeight
            // 
            this.txt_parHeight.AccessibleDescription = null;
            this.txt_parHeight.AccessibleName = null;
            resources.ApplyResources(this.txt_parHeight, "txt_parHeight");
            this.txt_parHeight.BackgroundImage = null;
            this.txt_parHeight.Font = null;
            this.txt_parHeight.Name = "txt_parHeight";
            this.toolTip.SetToolTip(this.txt_parHeight, resources.GetString("txt_parHeight.ToolTip"));
            this.txt_parHeight.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txt_parHeight_Keyup);
            // 
            // check_KeepAR
            // 
            this.check_KeepAR.AccessibleDescription = null;
            this.check_KeepAR.AccessibleName = null;
            resources.ApplyResources(this.check_KeepAR, "check_KeepAR");
            this.check_KeepAR.BackgroundImage = null;
            this.check_KeepAR.Font = null;
            this.check_KeepAR.Name = "check_KeepAR";
            this.toolTip.SetToolTip(this.check_KeepAR, resources.GetString("check_KeepAR.ToolTip"));
            this.check_KeepAR.UseVisualStyleBackColor = true;
            this.check_KeepAR.CheckedChanged += new System.EventHandler(this.check_KeepAR_CheckedChanged);
            // 
            // check_customCrop
            // 
            this.check_customCrop.AccessibleDescription = null;
            this.check_customCrop.AccessibleName = null;
            resources.ApplyResources(this.check_customCrop, "check_customCrop");
            this.check_customCrop.BackgroundImage = null;
            this.check_customCrop.Font = null;
            this.check_customCrop.Name = "check_customCrop";
            this.toolTip.SetToolTip(this.check_customCrop, resources.GetString("check_customCrop.ToolTip"));
            this.check_customCrop.UseVisualStyleBackColor = true;
            this.check_customCrop.CheckedChanged += new System.EventHandler(this.check_customCrop_CheckedChanged);
            // 
            // check_autoCrop
            // 
            this.check_autoCrop.AccessibleDescription = null;
            this.check_autoCrop.AccessibleName = null;
            resources.ApplyResources(this.check_autoCrop, "check_autoCrop");
            this.check_autoCrop.BackgroundImage = null;
            this.check_autoCrop.Checked = true;
            this.check_autoCrop.Font = null;
            this.check_autoCrop.Name = "check_autoCrop";
            this.check_autoCrop.TabStop = true;
            this.toolTip.SetToolTip(this.check_autoCrop, resources.GetString("check_autoCrop.ToolTip"));
            this.check_autoCrop.UseVisualStyleBackColor = true;
            this.check_autoCrop.CheckedChanged += new System.EventHandler(this.check_autoCrop_CheckedChanged);
            // 
            // crop_bottom
            // 
            this.crop_bottom.AccessibleDescription = null;
            this.crop_bottom.AccessibleName = null;
            resources.ApplyResources(this.crop_bottom, "crop_bottom");
            this.crop_bottom.Font = null;
            this.crop_bottom.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_bottom.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.crop_bottom.Name = "crop_bottom";
            this.toolTip.SetToolTip(this.crop_bottom, resources.GetString("crop_bottom.ToolTip"));
            this.crop_bottom.ValueChanged += new System.EventHandler(this.crop_bottom_ValueChanged);
            // 
            // crop_top
            // 
            this.crop_top.AccessibleDescription = null;
            this.crop_top.AccessibleName = null;
            resources.ApplyResources(this.crop_top, "crop_top");
            this.crop_top.Font = null;
            this.crop_top.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_top.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.crop_top.Name = "crop_top";
            this.toolTip.SetToolTip(this.crop_top, resources.GetString("crop_top.ToolTip"));
            this.crop_top.ValueChanged += new System.EventHandler(this.crop_top_ValueChanged);
            // 
            // crop_left
            // 
            this.crop_left.AccessibleDescription = null;
            this.crop_left.AccessibleName = null;
            resources.ApplyResources(this.crop_left, "crop_left");
            this.crop_left.Font = null;
            this.crop_left.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_left.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.crop_left.Name = "crop_left";
            this.toolTip.SetToolTip(this.crop_left, resources.GetString("crop_left.ToolTip"));
            this.crop_left.ValueChanged += new System.EventHandler(this.crop_left_ValueChanged);
            // 
            // crop_right
            // 
            this.crop_right.AccessibleDescription = null;
            this.crop_right.AccessibleName = null;
            resources.ApplyResources(this.crop_right, "crop_right");
            this.crop_right.Font = null;
            this.crop_right.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_right.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.crop_right.Name = "crop_right";
            this.toolTip.SetToolTip(this.crop_right, resources.GetString("crop_right.ToolTip"));
            this.crop_right.ValueChanged += new System.EventHandler(this.crop_right_ValueChanged);
            // 
            // label8
            // 
            this.label8.AccessibleDescription = null;
            this.label8.AccessibleName = null;
            resources.ApplyResources(this.label8, "label8");
            this.label8.BackColor = System.Drawing.Color.Transparent;
            this.label8.Name = "label8";
            this.toolTip.SetToolTip(this.label8, resources.GetString("label8.ToolTip"));
            // 
            // Label53
            // 
            this.Label53.AccessibleDescription = null;
            this.Label53.AccessibleName = null;
            resources.ApplyResources(this.Label53, "Label53");
            this.Label53.BackColor = System.Drawing.Color.Transparent;
            this.Label53.Name = "Label53";
            this.toolTip.SetToolTip(this.Label53, resources.GetString("Label53.ToolTip"));
            // 
            // Label52
            // 
            this.Label52.AccessibleDescription = null;
            this.Label52.AccessibleName = null;
            resources.ApplyResources(this.Label52, "Label52");
            this.Label52.BackColor = System.Drawing.Color.Transparent;
            this.Label52.Name = "Label52";
            this.toolTip.SetToolTip(this.Label52, resources.GetString("Label52.ToolTip"));
            // 
            // Label51
            // 
            this.Label51.AccessibleDescription = null;
            this.Label51.AccessibleName = null;
            resources.ApplyResources(this.Label51, "Label51");
            this.Label51.BackColor = System.Drawing.Color.Transparent;
            this.Label51.Name = "Label51";
            this.toolTip.SetToolTip(this.Label51, resources.GetString("Label51.ToolTip"));
            // 
            // Label15
            // 
            this.Label15.AccessibleDescription = null;
            this.Label15.AccessibleName = null;
            resources.ApplyResources(this.Label15, "Label15");
            this.Label15.BackColor = System.Drawing.Color.Transparent;
            this.Label15.Name = "Label15";
            this.toolTip.SetToolTip(this.Label15, resources.GetString("Label15.ToolTip"));
            // 
            // lbl_anamorphic
            // 
            this.lbl_anamorphic.AccessibleDescription = null;
            this.lbl_anamorphic.AccessibleName = null;
            resources.ApplyResources(this.lbl_anamorphic, "lbl_anamorphic");
            this.lbl_anamorphic.BackColor = System.Drawing.Color.Transparent;
            this.lbl_anamorphic.Name = "lbl_anamorphic";
            this.toolTip.SetToolTip(this.lbl_anamorphic, resources.GetString("lbl_anamorphic.ToolTip"));
            // 
            // lbl_anamprohicLbl
            // 
            this.lbl_anamprohicLbl.AccessibleDescription = null;
            this.lbl_anamprohicLbl.AccessibleName = null;
            resources.ApplyResources(this.lbl_anamprohicLbl, "lbl_anamprohicLbl");
            this.lbl_anamprohicLbl.BackColor = System.Drawing.Color.Transparent;
            this.lbl_anamprohicLbl.Font = null;
            this.lbl_anamprohicLbl.Name = "lbl_anamprohicLbl";
            this.toolTip.SetToolTip(this.lbl_anamprohicLbl, resources.GetString("lbl_anamprohicLbl.ToolTip"));
            // 
            // PictureSettings
            // 
            this.AccessibleDescription = null;
            this.AccessibleName = null;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            resources.ApplyResources(this, "$this");
            this.BackgroundImage = null;
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
            this.Font = null;
            this.Name = "PictureSettings";
            this.toolTip.SetToolTip(this, resources.GetString("$this.ToolTip"));
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
