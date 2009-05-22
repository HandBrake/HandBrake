/*  x264Panel.Designer.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    partial class x264Panel
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(x264Panel));
            this.slider_psytrellis = new System.Windows.Forms.TrackBar();
            this.lbl_psytrellis = new System.Windows.Forms.Label();
            this.lbl_psyrd = new System.Windows.Forms.Label();
            this.slider_psyrd = new System.Windows.Forms.TrackBar();
            this.lbl_adaptBFrames = new System.Windows.Forms.Label();
            this.drop_adaptBFrames = new System.Windows.Forms.ComboBox();
            this.label43 = new System.Windows.Forms.Label();
            this.btn_reset = new System.Windows.Forms.Button();
            this.rtf_x264Query = new System.Windows.Forms.RichTextBox();
            this.check_Cabac = new System.Windows.Forms.CheckBox();
            this.check_noDCTDecimate = new System.Windows.Forms.CheckBox();
            this.check_noFastPSkip = new System.Windows.Forms.CheckBox();
            this.lbl_trellis = new System.Windows.Forms.Label();
            this.drop_trellis = new System.Windows.Forms.ComboBox();
            this.drop_deblockBeta = new System.Windows.Forms.ComboBox();
            this.label41 = new System.Windows.Forms.Label();
            this.drop_deblockAlpha = new System.Windows.Forms.ComboBox();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.check_8x8DCT = new System.Windows.Forms.CheckBox();
            this.label45 = new System.Windows.Forms.Label();
            this.drop_analysis = new System.Windows.Forms.ComboBox();
            this.label48 = new System.Windows.Forms.Label();
            this.drop_subpixelMotionEstimation = new System.Windows.Forms.ComboBox();
            this.lbl_merange = new System.Windows.Forms.Label();
            this.drop_MotionEstimationRange = new System.Windows.Forms.ComboBox();
            this.label54 = new System.Windows.Forms.Label();
            this.drop_MotionEstimationMethod = new System.Windows.Forms.ComboBox();
            this.check_pyrmidalBFrames = new System.Windows.Forms.CheckBox();
            this.check_weightedBFrames = new System.Windows.Forms.CheckBox();
            this.lbl_direct_prediction = new System.Windows.Forms.Label();
            this.drop_directPrediction = new System.Windows.Forms.ComboBox();
            this.label62 = new System.Windows.Forms.Label();
            this.drop_bFrames = new System.Windows.Forms.ComboBox();
            this.label64 = new System.Windows.Forms.Label();
            this.drop_refFrames = new System.Windows.Forms.ComboBox();
            this.check_mixedReferences = new System.Windows.Forms.CheckBox();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.slider_psytrellis)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_psyrd)).BeginInit();
            this.SuspendLayout();
            // 
            // slider_psytrellis
            // 
            this.slider_psytrellis.Location = new System.Drawing.Point(436, 194);
            this.slider_psytrellis.Name = "slider_psytrellis";
            this.slider_psytrellis.Size = new System.Drawing.Size(131, 45);
            this.slider_psytrellis.TabIndex = 86;
            this.ToolTip.SetToolTip(this.slider_psytrellis, "Psychovisual Trellis tries to retain more sharpness and detail, but can cause art" +
                    "ifacting. \r\nIt is considered experimental, which is why it\'s off by default. Goo" +
                    "d values are 0.1 to 0.2.");
            this.slider_psytrellis.Scroll += new System.EventHandler(this.slider_psytrellis_Scroll);
            // 
            // lbl_psytrellis
            // 
            this.lbl_psytrellis.AutoSize = true;
            this.lbl_psytrellis.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_psytrellis.Location = new System.Drawing.Point(319, 198);
            this.lbl_psytrellis.Name = "lbl_psytrellis";
            this.lbl_psytrellis.Size = new System.Drawing.Size(111, 12);
            this.lbl_psytrellis.TabIndex = 85;
            this.lbl_psytrellis.Text = "Psychovisual Trellis:";
            // 
            // lbl_psyrd
            // 
            this.lbl_psyrd.AutoSize = true;
            this.lbl_psyrd.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_psyrd.Location = new System.Drawing.Point(275, 164);
            this.lbl_psyrd.Name = "lbl_psyrd";
            this.lbl_psyrd.Size = new System.Drawing.Size(155, 12);
            this.lbl_psyrd.TabIndex = 84;
            this.lbl_psyrd.Text = "Psychovisual Rate Distortion:";
            // 
            // slider_psyrd
            // 
            this.slider_psyrd.Location = new System.Drawing.Point(436, 153);
            this.slider_psyrd.Name = "slider_psyrd";
            this.slider_psyrd.Size = new System.Drawing.Size(131, 45);
            this.slider_psyrd.TabIndex = 83;
            this.ToolTip.SetToolTip(this.slider_psyrd, resources.GetString("slider_psyrd.ToolTip"));
            this.slider_psyrd.Scroll += new System.EventHandler(this.slider_psyrd_Scroll);
            // 
            // lbl_adaptBFrames
            // 
            this.lbl_adaptBFrames.AutoSize = true;
            this.lbl_adaptBFrames.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_adaptBFrames.Location = new System.Drawing.Point(17, 124);
            this.lbl_adaptBFrames.Name = "lbl_adaptBFrames";
            this.lbl_adaptBFrames.Size = new System.Drawing.Size(106, 12);
            this.lbl_adaptBFrames.TabIndex = 81;
            this.lbl_adaptBFrames.Text = "Adaptive B-Frames:";
            // 
            // drop_adaptBFrames
            // 
            this.drop_adaptBFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_adaptBFrames.FormattingEnabled = true;
            this.drop_adaptBFrames.Items.AddRange(new object[] {
            "Default (Fast)",
            "Off",
            "Fast",
            "Optimal"});
            this.drop_adaptBFrames.Location = new System.Drawing.Point(129, 121);
            this.drop_adaptBFrames.Name = "drop_adaptBFrames";
            this.drop_adaptBFrames.Size = new System.Drawing.Size(121, 21);
            this.drop_adaptBFrames.TabIndex = 82;
            this.ToolTip.SetToolTip(this.drop_adaptBFrames, resources.GetString("drop_adaptBFrames.ToolTip"));
            this.drop_adaptBFrames.SelectedIndexChanged += new System.EventHandler(this.drop_adaptBFrames_SelectedIndexChanged);
            // 
            // label43
            // 
            this.label43.AutoSize = true;
            this.label43.BackColor = System.Drawing.Color.Transparent;
            this.label43.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label43.Location = new System.Drawing.Point(13, 13);
            this.label43.Name = "label43";
            this.label43.Size = new System.Drawing.Size(165, 13);
            this.label43.TabIndex = 49;
            this.label43.Text = "Advanced H.264 Options";
            // 
            // btn_reset
            // 
            this.btn_reset.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_reset.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_reset.Location = new System.Drawing.Point(13, 229);
            this.btn_reset.Name = "btn_reset";
            this.btn_reset.Size = new System.Drawing.Size(75, 23);
            this.btn_reset.TabIndex = 79;
            this.btn_reset.Text = "Reset All";
            this.btn_reset.UseVisualStyleBackColor = true;
            this.btn_reset.Click += new System.EventHandler(this.btn_reset_Click);
            // 
            // rtf_x264Query
            // 
            this.rtf_x264Query.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.rtf_x264Query.Location = new System.Drawing.Point(13, 258);
            this.rtf_x264Query.Name = "rtf_x264Query";
            this.rtf_x264Query.Size = new System.Drawing.Size(698, 43);
            this.rtf_x264Query.TabIndex = 80;
            this.rtf_x264Query.Text = "";
            this.rtf_x264Query.TextChanged += new System.EventHandler(this.rtf_x264Query_TextChanged);
            // 
            // check_Cabac
            // 
            this.check_Cabac.AutoSize = true;
            this.check_Cabac.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_Cabac.Checked = true;
            this.check_Cabac.CheckState = System.Windows.Forms.CheckState.Checked;
            this.check_Cabac.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_Cabac.Location = new System.Drawing.Point(273, 128);
            this.check_Cabac.Name = "check_Cabac";
            this.check_Cabac.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_Cabac.Size = new System.Drawing.Size(147, 16);
            this.check_Cabac.TabIndex = 78;
            this.check_Cabac.Text = "CABAC Entropy Coding:";
            this.check_Cabac.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_Cabac, resources.GetString("check_Cabac.ToolTip"));
            this.check_Cabac.UseVisualStyleBackColor = true;
            this.check_Cabac.CheckedChanged += new System.EventHandler(this.check_Cabac_CheckedChanged);
            // 
            // check_noDCTDecimate
            // 
            this.check_noDCTDecimate.AutoSize = true;
            this.check_noDCTDecimate.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noDCTDecimate.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_noDCTDecimate.Location = new System.Drawing.Point(592, 40);
            this.check_noDCTDecimate.Name = "check_noDCTDecimate";
            this.check_noDCTDecimate.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noDCTDecimate.Size = new System.Drawing.Size(121, 16);
            this.check_noDCTDecimate.TabIndex = 77;
            this.check_noDCTDecimate.Text = "No DCT-Decimate:";
            this.check_noDCTDecimate.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_noDCTDecimate, resources.GetString("check_noDCTDecimate.ToolTip"));
            this.check_noDCTDecimate.UseVisualStyleBackColor = true;
            this.check_noDCTDecimate.CheckedChanged += new System.EventHandler(this.check_noDCTDecimate_CheckedChanged);
            // 
            // check_noFastPSkip
            // 
            this.check_noFastPSkip.AutoSize = true;
            this.check_noFastPSkip.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noFastPSkip.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_noFastPSkip.Location = new System.Drawing.Point(608, 19);
            this.check_noFastPSkip.Name = "check_noFastPSkip";
            this.check_noFastPSkip.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noFastPSkip.Size = new System.Drawing.Size(106, 16);
            this.check_noFastPSkip.TabIndex = 76;
            this.check_noFastPSkip.Text = "No Fast-P-Skip:";
            this.check_noFastPSkip.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_noFastPSkip, "This can help with blocking on solid colors like blue skies, but it also slows do" +
                    "wn the encode.");
            this.check_noFastPSkip.UseVisualStyleBackColor = true;
            this.check_noFastPSkip.CheckedChanged += new System.EventHandler(this.check_noFastPSkip_CheckedChanged);
            // 
            // lbl_trellis
            // 
            this.lbl_trellis.AutoSize = true;
            this.lbl_trellis.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_trellis.Location = new System.Drawing.Point(421, 130);
            this.lbl_trellis.Name = "lbl_trellis";
            this.lbl_trellis.Size = new System.Drawing.Size(41, 12);
            this.lbl_trellis.TabIndex = 67;
            this.lbl_trellis.Text = "Trellis:";
            // 
            // drop_trellis
            // 
            this.drop_trellis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_trellis.FormattingEnabled = true;
            this.drop_trellis.Items.AddRange(new object[] {
            "Default (0)",
            "0",
            "1",
            "2"});
            this.drop_trellis.Location = new System.Drawing.Point(468, 127);
            this.drop_trellis.Name = "drop_trellis";
            this.drop_trellis.Size = new System.Drawing.Size(94, 21);
            this.drop_trellis.TabIndex = 75;
            this.ToolTip.SetToolTip(this.drop_trellis, resources.GetString("drop_trellis.ToolTip"));
            this.drop_trellis.SelectedIndexChanged += new System.EventHandler(this.drop_trellis_SelectedIndexChanged);
            // 
            // drop_deblockBeta
            // 
            this.drop_deblockBeta.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockBeta.FormattingEnabled = true;
            this.drop_deblockBeta.Items.AddRange(new object[] {
            "Default (0)",
            "-6",
            "-5",
            "-4",
            "-3",
            "-2",
            "-1",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6"});
            this.drop_deblockBeta.Location = new System.Drawing.Point(642, 95);
            this.drop_deblockBeta.Name = "drop_deblockBeta";
            this.drop_deblockBeta.Size = new System.Drawing.Size(69, 21);
            this.drop_deblockBeta.TabIndex = 74;
            this.ToolTip.SetToolTip(this.drop_deblockBeta, resources.GetString("drop_deblockBeta.ToolTip"));
            this.drop_deblockBeta.SelectedIndexChanged += new System.EventHandler(this.drop_deblockBeta_SelectedIndexChanged);
            // 
            // label41
            // 
            this.label41.AutoSize = true;
            this.label41.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label41.Location = new System.Drawing.Point(575, 72);
            this.label41.Name = "label41";
            this.label41.Size = new System.Drawing.Size(65, 12);
            this.label41.TabIndex = 66;
            this.label41.Text = "Deblocking:";
            // 
            // drop_deblockAlpha
            // 
            this.drop_deblockAlpha.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockAlpha.FormattingEnabled = true;
            this.drop_deblockAlpha.Items.AddRange(new object[] {
            "Default (0)",
            "-6",
            "-5",
            "-4",
            "-3",
            "-2",
            "-1",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6"});
            this.drop_deblockAlpha.Location = new System.Drawing.Point(643, 69);
            this.drop_deblockAlpha.Name = "drop_deblockAlpha";
            this.drop_deblockAlpha.Size = new System.Drawing.Size(68, 21);
            this.drop_deblockAlpha.TabIndex = 73;
            this.ToolTip.SetToolTip(this.drop_deblockAlpha, resources.GetString("drop_deblockAlpha.ToolTip"));
            this.drop_deblockAlpha.SelectedIndexChanged += new System.EventHandler(this.drop_deblockAlpha_SelectedIndexChanged);
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.Color.Black;
            this.panel3.Location = new System.Drawing.Point(277, 123);
            this.panel3.Margin = new System.Windows.Forms.Padding(0);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(290, 1);
            this.panel3.TabIndex = 65;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.Black;
            this.panel1.Location = new System.Drawing.Point(13, 89);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(247, 1);
            this.panel1.TabIndex = 51;
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.Color.Black;
            this.panel2.Location = new System.Drawing.Point(277, 95);
            this.panel2.Margin = new System.Windows.Forms.Padding(0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(290, 1);
            this.panel2.TabIndex = 63;
            // 
            // check_8x8DCT
            // 
            this.check_8x8DCT.AutoSize = true;
            this.check_8x8DCT.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_8x8DCT.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_8x8DCT.Location = new System.Drawing.Point(492, 100);
            this.check_8x8DCT.Name = "check_8x8DCT";
            this.check_8x8DCT.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_8x8DCT.Size = new System.Drawing.Size(74, 16);
            this.check_8x8DCT.TabIndex = 72;
            this.check_8x8DCT.Text = "8x8 DCT:";
            this.check_8x8DCT.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_8x8DCT, resources.GetString("check_8x8DCT.ToolTip"));
            this.check_8x8DCT.UseVisualStyleBackColor = true;
            this.check_8x8DCT.CheckedChanged += new System.EventHandler(this.check_8x8DCT_CheckedChanged);
            // 
            // label45
            // 
            this.label45.AutoSize = true;
            this.label45.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label45.Location = new System.Drawing.Point(364, 102);
            this.label45.Name = "label45";
            this.label45.Size = new System.Drawing.Size(53, 12);
            this.label45.TabIndex = 64;
            this.label45.Text = "Analysis:";
            // 
            // drop_analysis
            // 
            this.drop_analysis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_analysis.FormattingEnabled = true;
            this.drop_analysis.Items.AddRange(new object[] {
            "Default (some)",
            "None",
            "All"});
            this.drop_analysis.Location = new System.Drawing.Point(423, 99);
            this.drop_analysis.Name = "drop_analysis";
            this.drop_analysis.Size = new System.Drawing.Size(63, 21);
            this.drop_analysis.TabIndex = 71;
            this.ToolTip.SetToolTip(this.drop_analysis, resources.GetString("drop_analysis.ToolTip"));
            this.drop_analysis.SelectedIndexChanged += new System.EventHandler(this.drop_analysis_SelectedIndexChanged);
            // 
            // label48
            // 
            this.label48.AutoSize = true;
            this.label48.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label48.Location = new System.Drawing.Point(271, 45);
            this.label48.Name = "label48";
            this.label48.Size = new System.Drawing.Size(146, 12);
            this.label48.TabIndex = 62;
            this.label48.Text = "Subpixel Motion Estimation:";
            // 
            // drop_subpixelMotionEstimation
            // 
            this.drop_subpixelMotionEstimation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_subpixelMotionEstimation.FormattingEnabled = true;
            this.drop_subpixelMotionEstimation.Items.AddRange(new object[] {
            "Default (6)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.drop_subpixelMotionEstimation.Location = new System.Drawing.Point(423, 42);
            this.drop_subpixelMotionEstimation.Name = "drop_subpixelMotionEstimation";
            this.drop_subpixelMotionEstimation.Size = new System.Drawing.Size(139, 21);
            this.drop_subpixelMotionEstimation.TabIndex = 70;
            this.ToolTip.SetToolTip(this.drop_subpixelMotionEstimation, resources.GetString("drop_subpixelMotionEstimation.ToolTip"));
            this.drop_subpixelMotionEstimation.SelectedIndexChanged += new System.EventHandler(this.drop_subpixelMotionEstimation_SelectedIndexChanged);
            // 
            // lbl_merange
            // 
            this.lbl_merange.AutoSize = true;
            this.lbl_merange.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_merange.Location = new System.Drawing.Point(283, 71);
            this.lbl_merange.Name = "lbl_merange";
            this.lbl_merange.Size = new System.Drawing.Size(134, 12);
            this.lbl_merange.TabIndex = 61;
            this.lbl_merange.Text = "Motion Estimation Range:";
            // 
            // drop_MotionEstimationRange
            // 
            this.drop_MotionEstimationRange.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationRange.FormattingEnabled = true;
            this.drop_MotionEstimationRange.Items.AddRange(new object[] {
            "Default (16)",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16",
            "17",
            "18",
            "19",
            "20",
            "21",
            "22",
            "23",
            "24",
            "25",
            "26",
            "27",
            "28",
            "29",
            "30",
            "31",
            "32",
            "33",
            "34",
            "35",
            "36",
            "37",
            "38",
            "39",
            "40",
            "41",
            "42",
            "43",
            "44",
            "45",
            "46",
            "47",
            "48",
            "49",
            "50",
            "51",
            "52",
            "53",
            "54",
            "55",
            "56",
            "57",
            "58",
            "59",
            "60",
            "61",
            "62",
            "63",
            "64"});
            this.drop_MotionEstimationRange.Location = new System.Drawing.Point(423, 68);
            this.drop_MotionEstimationRange.Name = "drop_MotionEstimationRange";
            this.drop_MotionEstimationRange.Size = new System.Drawing.Size(139, 21);
            this.drop_MotionEstimationRange.TabIndex = 69;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationRange, resources.GetString("drop_MotionEstimationRange.ToolTip"));
            this.drop_MotionEstimationRange.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationRange_SelectedIndexChanged);
            // 
            // label54
            // 
            this.label54.AutoSize = true;
            this.label54.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label54.Location = new System.Drawing.Point(277, 19);
            this.label54.Name = "label54";
            this.label54.Size = new System.Drawing.Size(140, 12);
            this.label54.TabIndex = 60;
            this.label54.Text = "Motion Estimation Method:";
            // 
            // drop_MotionEstimationMethod
            // 
            this.drop_MotionEstimationMethod.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationMethod.FormattingEnabled = true;
            this.drop_MotionEstimationMethod.ItemHeight = 13;
            this.drop_MotionEstimationMethod.Items.AddRange(new object[] {
            "Default (Hexagon)",
            "Diamond",
            "Hexagon",
            "Uneven Multi-Hexagon",
            "Exhaustive",
            "Transformed Exhaustive"});
            this.drop_MotionEstimationMethod.Location = new System.Drawing.Point(423, 16);
            this.drop_MotionEstimationMethod.Name = "drop_MotionEstimationMethod";
            this.drop_MotionEstimationMethod.Size = new System.Drawing.Size(139, 21);
            this.drop_MotionEstimationMethod.TabIndex = 68;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationMethod, resources.GetString("drop_MotionEstimationMethod.ToolTip"));
            this.drop_MotionEstimationMethod.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationMethod_SelectedIndexChanged);
            // 
            // check_pyrmidalBFrames
            // 
            this.check_pyrmidalBFrames.AutoSize = true;
            this.check_pyrmidalBFrames.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_pyrmidalBFrames.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_pyrmidalBFrames.Location = new System.Drawing.Point(12, 194);
            this.check_pyrmidalBFrames.Name = "check_pyrmidalBFrames";
            this.check_pyrmidalBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_pyrmidalBFrames.Size = new System.Drawing.Size(130, 16);
            this.check_pyrmidalBFrames.TabIndex = 59;
            this.check_pyrmidalBFrames.Text = "Pyramidal B-Frames:";
            this.ToolTip.SetToolTip(this.check_pyrmidalBFrames, resources.GetString("check_pyrmidalBFrames.ToolTip"));
            this.check_pyrmidalBFrames.UseVisualStyleBackColor = true;
            this.check_pyrmidalBFrames.CheckedChanged += new System.EventHandler(this.check_pyrmidalBFrames_CheckedChanged);
            // 
            // check_weightedBFrames
            // 
            this.check_weightedBFrames.AutoSize = true;
            this.check_weightedBFrames.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_weightedBFrames.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_weightedBFrames.Location = new System.Drawing.Point(16, 173);
            this.check_weightedBFrames.Name = "check_weightedBFrames";
            this.check_weightedBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_weightedBFrames.Size = new System.Drawing.Size(126, 16);
            this.check_weightedBFrames.TabIndex = 58;
            this.check_weightedBFrames.Text = "Weighted B-Frames:";
            this.ToolTip.SetToolTip(this.check_weightedBFrames, resources.GetString("check_weightedBFrames.ToolTip"));
            this.check_weightedBFrames.UseVisualStyleBackColor = true;
            this.check_weightedBFrames.CheckedChanged += new System.EventHandler(this.check_weightedBFrames_CheckedChanged);
            // 
            // lbl_direct_prediction
            // 
            this.lbl_direct_prediction.AutoSize = true;
            this.lbl_direct_prediction.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_direct_prediction.Location = new System.Drawing.Point(29, 150);
            this.lbl_direct_prediction.Name = "lbl_direct_prediction";
            this.lbl_direct_prediction.Size = new System.Drawing.Size(94, 12);
            this.lbl_direct_prediction.TabIndex = 53;
            this.lbl_direct_prediction.Text = "Direct Prediction:";
            // 
            // drop_directPrediction
            // 
            this.drop_directPrediction.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_directPrediction.FormattingEnabled = true;
            this.drop_directPrediction.Items.AddRange(new object[] {
            "Default (Spatial)",
            "None",
            "Spatial",
            "Temporal",
            "Automatic"});
            this.drop_directPrediction.Location = new System.Drawing.Point(129, 147);
            this.drop_directPrediction.Name = "drop_directPrediction";
            this.drop_directPrediction.Size = new System.Drawing.Size(121, 21);
            this.drop_directPrediction.TabIndex = 57;
            this.ToolTip.SetToolTip(this.drop_directPrediction, resources.GetString("drop_directPrediction.ToolTip"));
            this.drop_directPrediction.SelectedIndexChanged += new System.EventHandler(this.drop_directPrediction_SelectedIndexChanged);
            // 
            // label62
            // 
            this.label62.AutoSize = true;
            this.label62.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label62.Location = new System.Drawing.Point(65, 98);
            this.label62.Name = "label62";
            this.label62.Size = new System.Drawing.Size(58, 12);
            this.label62.TabIndex = 52;
            this.label62.Text = "B-Frames:";
            // 
            // drop_bFrames
            // 
            this.drop_bFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_bFrames.FormattingEnabled = true;
            this.drop_bFrames.Items.AddRange(new object[] {
            "Default (0)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"});
            this.drop_bFrames.Location = new System.Drawing.Point(129, 95);
            this.drop_bFrames.Name = "drop_bFrames";
            this.drop_bFrames.Size = new System.Drawing.Size(121, 21);
            this.drop_bFrames.TabIndex = 56;
            this.ToolTip.SetToolTip(this.drop_bFrames, "Sane values are 1-6. B-Frames are smaller than other frames, so they let you pack" +
                    " in more quality at the same bitrate.\r\nUse more of them with animated material: " +
                    "9-16.");
            this.drop_bFrames.SelectedIndexChanged += new System.EventHandler(this.drop_bFrames_SelectedIndexChanged);
            // 
            // label64
            // 
            this.label64.AutoSize = true;
            this.label64.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label64.Location = new System.Drawing.Point(24, 45);
            this.label64.Name = "label64";
            this.label64.Size = new System.Drawing.Size(99, 12);
            this.label64.TabIndex = 50;
            this.label64.Text = "Reference Frames:";
            // 
            // drop_refFrames
            // 
            this.drop_refFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_refFrames.FormattingEnabled = true;
            this.drop_refFrames.Items.AddRange(new object[] {
            "Default (1)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"});
            this.drop_refFrames.Location = new System.Drawing.Point(129, 42);
            this.drop_refFrames.Name = "drop_refFrames";
            this.drop_refFrames.Size = new System.Drawing.Size(121, 21);
            this.drop_refFrames.TabIndex = 54;
            this.ToolTip.SetToolTip(this.drop_refFrames, "Sane values are 1-6. \r\nThe more you add, the higher the quality - but the slower " +
                    "the encode. Be careful...too many and QuickTime struggle to play the video back." +
                    "");
            this.drop_refFrames.SelectedIndexChanged += new System.EventHandler(this.drop_refFrames_SelectedIndexChanged);
            // 
            // check_mixedReferences
            // 
            this.check_mixedReferences.AutoSize = true;
            this.check_mixedReferences.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_mixedReferences.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_mixedReferences.Location = new System.Drawing.Point(25, 68);
            this.check_mixedReferences.Name = "check_mixedReferences";
            this.check_mixedReferences.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_mixedReferences.Size = new System.Drawing.Size(117, 16);
            this.check_mixedReferences.TabIndex = 55;
            this.check_mixedReferences.Text = "Mixed References:";
            this.check_mixedReferences.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_mixedReferences, "With this on, different references can be used for different parts of each 16x16 " +
                    "pixel macroblock, increasing quality.");
            this.check_mixedReferences.UseVisualStyleBackColor = true;
            this.check_mixedReferences.CheckedChanged += new System.EventHandler(this.check_mixedReferences_CheckedChanged);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            // 
            // x264Panel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.slider_psytrellis);
            this.Controls.Add(this.lbl_psytrellis);
            this.Controls.Add(this.lbl_psyrd);
            this.Controls.Add(this.slider_psyrd);
            this.Controls.Add(this.lbl_adaptBFrames);
            this.Controls.Add(this.drop_adaptBFrames);
            this.Controls.Add(this.label43);
            this.Controls.Add(this.btn_reset);
            this.Controls.Add(this.rtf_x264Query);
            this.Controls.Add(this.check_Cabac);
            this.Controls.Add(this.check_noDCTDecimate);
            this.Controls.Add(this.check_noFastPSkip);
            this.Controls.Add(this.lbl_trellis);
            this.Controls.Add(this.drop_trellis);
            this.Controls.Add(this.drop_deblockBeta);
            this.Controls.Add(this.label41);
            this.Controls.Add(this.drop_deblockAlpha);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.check_8x8DCT);
            this.Controls.Add(this.label45);
            this.Controls.Add(this.drop_analysis);
            this.Controls.Add(this.label48);
            this.Controls.Add(this.drop_subpixelMotionEstimation);
            this.Controls.Add(this.lbl_merange);
            this.Controls.Add(this.drop_MotionEstimationRange);
            this.Controls.Add(this.label54);
            this.Controls.Add(this.drop_MotionEstimationMethod);
            this.Controls.Add(this.check_pyrmidalBFrames);
            this.Controls.Add(this.check_weightedBFrames);
            this.Controls.Add(this.lbl_direct_prediction);
            this.Controls.Add(this.drop_directPrediction);
            this.Controls.Add(this.label62);
            this.Controls.Add(this.drop_bFrames);
            this.Controls.Add(this.label64);
            this.Controls.Add(this.drop_refFrames);
            this.Controls.Add(this.check_mixedReferences);
            this.Name = "x264Panel";
            this.Size = new System.Drawing.Size(720, 306);
            ((System.ComponentModel.ISupportInitialize)(this.slider_psytrellis)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_psyrd)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.TrackBar slider_psytrellis;
        internal System.Windows.Forms.Label lbl_psytrellis;
        internal System.Windows.Forms.Label lbl_psyrd;
        internal System.Windows.Forms.TrackBar slider_psyrd;
        internal System.Windows.Forms.Label lbl_adaptBFrames;
        internal System.Windows.Forms.ComboBox drop_adaptBFrames;
        internal System.Windows.Forms.Label label43;
        internal System.Windows.Forms.Button btn_reset;
        internal System.Windows.Forms.RichTextBox rtf_x264Query;
        internal System.Windows.Forms.CheckBox check_Cabac;
        internal System.Windows.Forms.CheckBox check_noDCTDecimate;
        internal System.Windows.Forms.CheckBox check_noFastPSkip;
        internal System.Windows.Forms.Label lbl_trellis;
        internal System.Windows.Forms.ComboBox drop_trellis;
        internal System.Windows.Forms.ComboBox drop_deblockBeta;
        internal System.Windows.Forms.Label label41;
        internal System.Windows.Forms.ComboBox drop_deblockAlpha;
        internal System.Windows.Forms.Panel panel3;
        internal System.Windows.Forms.Panel panel1;
        internal System.Windows.Forms.Panel panel2;
        internal System.Windows.Forms.CheckBox check_8x8DCT;
        internal System.Windows.Forms.Label label45;
        internal System.Windows.Forms.ComboBox drop_analysis;
        internal System.Windows.Forms.Label label48;
        internal System.Windows.Forms.ComboBox drop_subpixelMotionEstimation;
        internal System.Windows.Forms.Label lbl_merange;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationRange;
        internal System.Windows.Forms.Label label54;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationMethod;
        internal System.Windows.Forms.CheckBox check_pyrmidalBFrames;
        internal System.Windows.Forms.CheckBox check_weightedBFrames;
        internal System.Windows.Forms.Label lbl_direct_prediction;
        internal System.Windows.Forms.ComboBox drop_directPrediction;
        internal System.Windows.Forms.Label label62;
        internal System.Windows.Forms.ComboBox drop_bFrames;
        internal System.Windows.Forms.Label label64;
        internal System.Windows.Forms.ComboBox drop_refFrames;
        internal System.Windows.Forms.CheckBox check_mixedReferences;
        private System.Windows.Forms.ToolTip ToolTip;
    }
}
