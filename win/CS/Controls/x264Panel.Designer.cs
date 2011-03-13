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
            this.lbl_trellis = new System.Windows.Forms.Label();
            this.drop_trellis = new System.Windows.Forms.ComboBox();
            this.drop_deblockBeta = new System.Windows.Forms.ComboBox();
            this.label41 = new System.Windows.Forms.Label();
            this.drop_deblockAlpha = new System.Windows.Forms.ComboBox();
            this.check_8x8DCT = new System.Windows.Forms.CheckBox();
            this.label45 = new System.Windows.Forms.Label();
            this.drop_analysis = new System.Windows.Forms.ComboBox();
            this.label48 = new System.Windows.Forms.Label();
            this.drop_subpixelMotionEstimation = new System.Windows.Forms.ComboBox();
            this.lbl_merange = new System.Windows.Forms.Label();
            this.drop_MotionEstimationRange = new System.Windows.Forms.ComboBox();
            this.label54 = new System.Windows.Forms.Label();
            this.drop_MotionEstimationMethod = new System.Windows.Forms.ComboBox();
            this.lbl_direct_prediction = new System.Windows.Forms.Label();
            this.drop_directPrediction = new System.Windows.Forms.ComboBox();
            this.label62 = new System.Windows.Forms.Label();
            this.drop_bFrames = new System.Windows.Forms.ComboBox();
            this.label64 = new System.Windows.Forms.Label();
            this.drop_refFrames = new System.Windows.Forms.ComboBox();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.check_weightp = new System.Windows.Forms.CheckBox();
            this.slider_adaptiveQuantStrength = new System.Windows.Forms.TrackBar();
            this.combo_pyrmidalBFrames = new System.Windows.Forms.ComboBox();
            this.lbl_adaptiveQuantStrength = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.lbl_prymidalBframes = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.slider_psytrellis)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_psyrd)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_adaptiveQuantStrength)).BeginInit();
            this.SuspendLayout();
            // 
            // slider_psytrellis
            // 
            this.slider_psytrellis.BackColor = System.Drawing.SystemColors.Window;
            this.slider_psytrellis.Location = new System.Drawing.Point(427, 231);
            this.slider_psytrellis.Maximum = 20;
            this.slider_psytrellis.Name = "slider_psytrellis";
            this.slider_psytrellis.Size = new System.Drawing.Size(131, 45);
            this.slider_psytrellis.TabIndex = 86;
            this.ToolTip.SetToolTip(this.slider_psytrellis, "Psychovisual Trellis tries to retain more sharpness and detail, but can cause art" +
                    "ifacting. \r\nIt is considered experimental, which is why it\'s off by default. Goo" +
                    "d values are 0.1 to 0.2.");
            this.slider_psytrellis.Scroll += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_psytrellis
            // 
            this.lbl_psytrellis.AutoSize = true;
            this.lbl_psytrellis.Location = new System.Drawing.Point(309, 231);
            this.lbl_psytrellis.Name = "lbl_psytrellis";
            this.lbl_psytrellis.Size = new System.Drawing.Size(102, 13);
            this.lbl_psytrellis.TabIndex = 85;
            this.lbl_psytrellis.Text = "Psychovisual Trellis:";
            // 
            // lbl_psyrd
            // 
            this.lbl_psyrd.AutoSize = true;
            this.lbl_psyrd.Location = new System.Drawing.Point(264, 199);
            this.lbl_psyrd.Name = "lbl_psyrd";
            this.lbl_psyrd.Size = new System.Drawing.Size(147, 13);
            this.lbl_psyrd.TabIndex = 84;
            this.lbl_psyrd.Text = "Psychovisual Rate Distortion:";
            // 
            // slider_psyrd
            // 
            this.slider_psyrd.BackColor = System.Drawing.SystemColors.Window;
            this.slider_psyrd.Location = new System.Drawing.Point(425, 199);
            this.slider_psyrd.Maximum = 20;
            this.slider_psyrd.Name = "slider_psyrd";
            this.slider_psyrd.Size = new System.Drawing.Size(131, 45);
            this.slider_psyrd.TabIndex = 83;
            this.ToolTip.SetToolTip(this.slider_psyrd, resources.GetString("slider_psyrd.ToolTip"));
            this.slider_psyrd.Value = 10;
            this.slider_psyrd.Scroll += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_adaptBFrames
            // 
            this.lbl_adaptBFrames.AutoSize = true;
            this.lbl_adaptBFrames.Location = new System.Drawing.Point(309, 28);
            this.lbl_adaptBFrames.Name = "lbl_adaptBFrames";
            this.lbl_adaptBFrames.Size = new System.Drawing.Size(102, 13);
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
            this.drop_adaptBFrames.Location = new System.Drawing.Point(417, 24);
            this.drop_adaptBFrames.Name = "drop_adaptBFrames";
            this.drop_adaptBFrames.Size = new System.Drawing.Size(106, 21);
            this.drop_adaptBFrames.TabIndex = 82;
            this.ToolTip.SetToolTip(this.drop_adaptBFrames, resources.GetString("drop_adaptBFrames.ToolTip"));
            this.drop_adaptBFrames.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label43
            // 
            this.label43.AutoSize = true;
            this.label43.BackColor = System.Drawing.Color.Transparent;
            this.label43.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label43.Location = new System.Drawing.Point(13, 13);
            this.label43.Name = "label43";
            this.label43.Size = new System.Drawing.Size(60, 13);
            this.label43.TabIndex = 49;
            this.label43.Text = "Encoding:";
            // 
            // btn_reset
            // 
            this.btn_reset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_reset.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_reset.Location = new System.Drawing.Point(634, 230);
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
            this.rtf_x264Query.Location = new System.Drawing.Point(16, 259);
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
            this.check_Cabac.Location = new System.Drawing.Point(3, 89);
            this.check_Cabac.Name = "check_Cabac";
            this.check_Cabac.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_Cabac.Size = new System.Drawing.Size(141, 17);
            this.check_Cabac.TabIndex = 78;
            this.check_Cabac.Text = "CABAC Entropy Coding:";
            this.check_Cabac.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_Cabac, resources.GetString("check_Cabac.ToolTip"));
            this.check_Cabac.UseVisualStyleBackColor = true;
            this.check_Cabac.CheckedChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // check_noDCTDecimate
            // 
            this.check_noDCTDecimate.AutoSize = true;
            this.check_noDCTDecimate.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noDCTDecimate.Location = new System.Drawing.Point(30, 212);
            this.check_noDCTDecimate.Name = "check_noDCTDecimate";
            this.check_noDCTDecimate.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noDCTDecimate.Size = new System.Drawing.Size(114, 17);
            this.check_noDCTDecimate.TabIndex = 77;
            this.check_noDCTDecimate.Text = "No DCT-Decimate:";
            this.check_noDCTDecimate.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_noDCTDecimate, resources.GetString("check_noDCTDecimate.ToolTip"));
            this.check_noDCTDecimate.UseVisualStyleBackColor = true;
            this.check_noDCTDecimate.CheckedChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_trellis
            // 
            this.lbl_trellis.AutoSize = true;
            this.lbl_trellis.Location = new System.Drawing.Point(563, 43);
            this.lbl_trellis.Name = "lbl_trellis";
            this.lbl_trellis.Size = new System.Drawing.Size(38, 13);
            this.lbl_trellis.TabIndex = 67;
            this.lbl_trellis.Text = "Trellis:";
            // 
            // drop_trellis
            // 
            this.drop_trellis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_trellis.FormattingEnabled = true;
            this.drop_trellis.Items.AddRange(new object[] {
            "Default (Encode Only)",
            "Off",
            "Encode Only",
            "Always"});
            this.drop_trellis.Location = new System.Drawing.Point(615, 40);
            this.drop_trellis.Name = "drop_trellis";
            this.drop_trellis.Size = new System.Drawing.Size(94, 21);
            this.drop_trellis.TabIndex = 75;
            this.ToolTip.SetToolTip(this.drop_trellis, resources.GetString("drop_trellis.ToolTip"));
            this.drop_trellis.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
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
            this.drop_deblockBeta.Location = new System.Drawing.Point(631, 175);
            this.drop_deblockBeta.Name = "drop_deblockBeta";
            this.drop_deblockBeta.Size = new System.Drawing.Size(78, 21);
            this.drop_deblockBeta.TabIndex = 74;
            this.ToolTip.SetToolTip(this.drop_deblockBeta, resources.GetString("drop_deblockBeta.ToolTip"));
            this.drop_deblockBeta.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label41
            // 
            this.label41.AutoSize = true;
            this.label41.Location = new System.Drawing.Point(563, 152);
            this.label41.Name = "label41";
            this.label41.Size = new System.Drawing.Size(62, 13);
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
            this.drop_deblockAlpha.Location = new System.Drawing.Point(631, 149);
            this.drop_deblockAlpha.Name = "drop_deblockAlpha";
            this.drop_deblockAlpha.Size = new System.Drawing.Size(78, 21);
            this.drop_deblockAlpha.TabIndex = 73;
            this.ToolTip.SetToolTip(this.drop_deblockAlpha, resources.GetString("drop_deblockAlpha.ToolTip"));
            this.drop_deblockAlpha.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // check_8x8DCT
            // 
            this.check_8x8DCT.AutoSize = true;
            this.check_8x8DCT.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_8x8DCT.Location = new System.Drawing.Point(44, 111);
            this.check_8x8DCT.Name = "check_8x8DCT";
            this.check_8x8DCT.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_8x8DCT.Size = new System.Drawing.Size(100, 17);
            this.check_8x8DCT.TabIndex = 72;
            this.check_8x8DCT.Text = "8x8 Transform:";
            this.check_8x8DCT.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_8x8DCT, resources.GetString("check_8x8DCT.ToolTip"));
            this.check_8x8DCT.UseVisualStyleBackColor = true;
            this.check_8x8DCT.CheckedChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label45
            // 
            this.label45.AutoSize = true;
            this.label45.Location = new System.Drawing.Point(563, 16);
            this.label45.Name = "label45";
            this.label45.Size = new System.Drawing.Size(78, 13);
            this.label45.TabIndex = 64;
            this.label45.Text = "Partition Type:";
            // 
            // drop_analysis
            // 
            this.drop_analysis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_analysis.FormattingEnabled = true;
            this.drop_analysis.Items.AddRange(new object[] {
            "Default (most)",
            "None",
            "Some",
            "All"});
            this.drop_analysis.Location = new System.Drawing.Point(646, 13);
            this.drop_analysis.Name = "drop_analysis";
            this.drop_analysis.Size = new System.Drawing.Size(63, 21);
            this.drop_analysis.TabIndex = 71;
            this.ToolTip.SetToolTip(this.drop_analysis, resources.GetString("drop_analysis.ToolTip"));
            this.drop_analysis.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label48
            // 
            this.label48.AutoSize = true;
            this.label48.Location = new System.Drawing.Point(262, 110);
            this.label48.Name = "label48";
            this.label48.Size = new System.Drawing.Size(149, 13);
            this.label48.TabIndex = 62;
            this.label48.Text = "Subpixel ME && Mode Decision:";
            // 
            // drop_subpixelMotionEstimation
            // 
            this.drop_subpixelMotionEstimation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_subpixelMotionEstimation.FormattingEnabled = true;
            this.drop_subpixelMotionEstimation.Items.AddRange(new object[] {
            "Default (7)",
            "0: SAD, no subpel (super fast!)",
            "1: SAD, qpel",
            "2: SATD, qpel",
            "3: SATD, multi-qpel",
            "4: SATD, qpel on all",
            "5: SATD, multi-qpel on all",
            "6: RD in I/P-frames",
            "7: RD in all frames",
            "8: RD refine in I/P-frames",
            "9: RD refine in all frames",
            "10: QPRD in all frames"});
            this.drop_subpixelMotionEstimation.Location = new System.Drawing.Point(417, 105);
            this.drop_subpixelMotionEstimation.Name = "drop_subpixelMotionEstimation";
            this.drop_subpixelMotionEstimation.Size = new System.Drawing.Size(139, 21);
            this.drop_subpixelMotionEstimation.TabIndex = 70;
            this.ToolTip.SetToolTip(this.drop_subpixelMotionEstimation, resources.GetString("drop_subpixelMotionEstimation.ToolTip"));
            this.drop_subpixelMotionEstimation.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_merange
            // 
            this.lbl_merange.AutoSize = true;
            this.lbl_merange.Location = new System.Drawing.Point(282, 135);
            this.lbl_merange.Name = "lbl_merange";
            this.lbl_merange.Size = new System.Drawing.Size(129, 13);
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
            this.drop_MotionEstimationRange.Location = new System.Drawing.Point(417, 132);
            this.drop_MotionEstimationRange.Name = "drop_MotionEstimationRange";
            this.drop_MotionEstimationRange.Size = new System.Drawing.Size(139, 21);
            this.drop_MotionEstimationRange.TabIndex = 69;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationRange, resources.GetString("drop_MotionEstimationRange.ToolTip"));
            this.drop_MotionEstimationRange.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label54
            // 
            this.label54.AutoSize = true;
            this.label54.Location = new System.Drawing.Point(277, 81);
            this.label54.Name = "label54";
            this.label54.Size = new System.Drawing.Size(134, 13);
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
            this.drop_MotionEstimationMethod.Location = new System.Drawing.Point(417, 78);
            this.drop_MotionEstimationMethod.Name = "drop_MotionEstimationMethod";
            this.drop_MotionEstimationMethod.Size = new System.Drawing.Size(139, 21);
            this.drop_MotionEstimationMethod.TabIndex = 68;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationMethod, resources.GetString("drop_MotionEstimationMethod.ToolTip"));
            this.drop_MotionEstimationMethod.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_direct_prediction
            // 
            this.lbl_direct_prediction.AutoSize = true;
            this.lbl_direct_prediction.Location = new System.Drawing.Point(297, 54);
            this.lbl_direct_prediction.Name = "lbl_direct_prediction";
            this.lbl_direct_prediction.Size = new System.Drawing.Size(114, 13);
            this.lbl_direct_prediction.TabIndex = 53;
            this.lbl_direct_prediction.Text = "Adaptive Direct Mode:";
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
            this.drop_directPrediction.Location = new System.Drawing.Point(417, 51);
            this.drop_directPrediction.Name = "drop_directPrediction";
            this.drop_directPrediction.Size = new System.Drawing.Size(106, 21);
            this.drop_directPrediction.TabIndex = 57;
            this.ToolTip.SetToolTip(this.drop_directPrediction, resources.GetString("drop_directPrediction.ToolTip"));
            this.drop_directPrediction.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label62
            // 
            this.label62.AutoSize = true;
            this.label62.Location = new System.Drawing.Point(21, 64);
            this.label62.Name = "label62";
            this.label62.Size = new System.Drawing.Size(103, 13);
            this.label62.TabIndex = 52;
            this.label62.Text = "Maximum B-Frames:";
            // 
            // drop_bFrames
            // 
            this.drop_bFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_bFrames.FormattingEnabled = true;
            this.drop_bFrames.Items.AddRange(new object[] {
            "Default (3)",
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
            this.drop_bFrames.Location = new System.Drawing.Point(129, 61);
            this.drop_bFrames.Name = "drop_bFrames";
            this.drop_bFrames.Size = new System.Drawing.Size(106, 21);
            this.drop_bFrames.TabIndex = 56;
            this.ToolTip.SetToolTip(this.drop_bFrames, resources.GetString("drop_bFrames.ToolTip"));
            this.drop_bFrames.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // label64
            // 
            this.label64.AutoSize = true;
            this.label64.Location = new System.Drawing.Point(25, 38);
            this.label64.Name = "label64";
            this.label64.Size = new System.Drawing.Size(99, 13);
            this.label64.TabIndex = 50;
            this.label64.Text = "Reference Frames:";
            // 
            // drop_refFrames
            // 
            this.drop_refFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_refFrames.FormattingEnabled = true;
            this.drop_refFrames.Items.AddRange(new object[] {
            "Default (3)",
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
            this.drop_refFrames.Location = new System.Drawing.Point(129, 35);
            this.drop_refFrames.Name = "drop_refFrames";
            this.drop_refFrames.Size = new System.Drawing.Size(106, 21);
            this.drop_refFrames.TabIndex = 54;
            this.ToolTip.SetToolTip(this.drop_refFrames, resources.GetString("drop_refFrames.ToolTip"));
            this.drop_refFrames.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            // 
            // check_weightp
            // 
            this.check_weightp.AutoSize = true;
            this.check_weightp.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_weightp.ForeColor = System.Drawing.SystemColors.ControlText;
            this.check_weightp.Location = new System.Drawing.Point(20, 134);
            this.check_weightp.Name = "check_weightp";
            this.check_weightp.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_weightp.Size = new System.Drawing.Size(124, 17);
            this.check_weightp.TabIndex = 87;
            this.check_weightp.Text = "Weighted P-Frames:";
            this.ToolTip.SetToolTip(this.check_weightp, resources.GetString("check_weightp.ToolTip"));
            this.check_weightp.UseVisualStyleBackColor = true;
            this.check_weightp.CheckedChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // slider_adaptiveQuantStrength
            // 
            this.slider_adaptiveQuantStrength.BackColor = System.Drawing.SystemColors.Window;
            this.slider_adaptiveQuantStrength.Location = new System.Drawing.Point(425, 167);
            this.slider_adaptiveQuantStrength.Maximum = 20;
            this.slider_adaptiveQuantStrength.Name = "slider_adaptiveQuantStrength";
            this.slider_adaptiveQuantStrength.Size = new System.Drawing.Size(131, 45);
            this.slider_adaptiveQuantStrength.TabIndex = 88;
            this.ToolTip.SetToolTip(this.slider_adaptiveQuantStrength, "Adaptive quantization controls how the encoder distributes bits across the frame." +
                    "  \r\nHigher values take more bits away from edges and complex areas to improve ar" +
                    "eas with finer detail");
            this.slider_adaptiveQuantStrength.Value = 10;
            this.slider_adaptiveQuantStrength.Scroll += new System.EventHandler(this.widgetControlChanged);
            // 
            // combo_pyrmidalBFrames
            // 
            this.combo_pyrmidalBFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.combo_pyrmidalBFrames.FormattingEnabled = true;
            this.combo_pyrmidalBFrames.Items.AddRange(new object[] {
            "Default (Normal)",
            "Off",
            "Strict"});
            this.combo_pyrmidalBFrames.Location = new System.Drawing.Point(129, 157);
            this.combo_pyrmidalBFrames.Name = "combo_pyrmidalBFrames";
            this.combo_pyrmidalBFrames.Size = new System.Drawing.Size(106, 21);
            this.combo_pyrmidalBFrames.TabIndex = 92;
            this.ToolTip.SetToolTip(this.combo_pyrmidalBFrames, resources.GetString("combo_pyrmidalBFrames.ToolTip"));
            this.combo_pyrmidalBFrames.SelectedIndexChanged += new System.EventHandler(this.widgetControlChanged);
            // 
            // lbl_adaptiveQuantStrength
            // 
            this.lbl_adaptiveQuantStrength.AutoSize = true;
            this.lbl_adaptiveQuantStrength.Location = new System.Drawing.Point(248, 171);
            this.lbl_adaptiveQuantStrength.Name = "lbl_adaptiveQuantStrength";
            this.lbl_adaptiveQuantStrength.Size = new System.Drawing.Size(163, 13);
            this.lbl_adaptiveQuantStrength.TabIndex = 89;
            this.lbl_adaptiveQuantStrength.Text = "Adaptive Quantization Strength:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(13, 193);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(83, 13);
            this.label1.TabIndex = 90;
            this.label1.Text = "Psychovisual:";
            // 
            // lbl_prymidalBframes
            // 
            this.lbl_prymidalBframes.AutoSize = true;
            this.lbl_prymidalBframes.Location = new System.Drawing.Point(20, 160);
            this.lbl_prymidalBframes.Name = "lbl_prymidalBframes";
            this.lbl_prymidalBframes.Size = new System.Drawing.Size(105, 13);
            this.lbl_prymidalBframes.TabIndex = 91;
            this.lbl_prymidalBframes.Text = "Pyramidal B-Frames:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.BackColor = System.Drawing.Color.Transparent;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(248, 13);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(57, 13);
            this.label3.TabIndex = 93;
            this.label3.Text = "Analysis:";
            // 
            // x264Panel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.label3);
            this.Controls.Add(this.rtf_x264Query);
            this.Controls.Add(this.lbl_prymidalBframes);
            this.Controls.Add(this.combo_pyrmidalBFrames);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.slider_psytrellis);
            this.Controls.Add(this.slider_psyrd);
            this.Controls.Add(this.lbl_adaptiveQuantStrength);
            this.Controls.Add(this.slider_adaptiveQuantStrength);
            this.Controls.Add(this.check_weightp);
            this.Controls.Add(this.lbl_psytrellis);
            this.Controls.Add(this.lbl_psyrd);
            this.Controls.Add(this.lbl_adaptBFrames);
            this.Controls.Add(this.drop_adaptBFrames);
            this.Controls.Add(this.label43);
            this.Controls.Add(this.btn_reset);
            this.Controls.Add(this.check_Cabac);
            this.Controls.Add(this.check_noDCTDecimate);
            this.Controls.Add(this.lbl_trellis);
            this.Controls.Add(this.drop_trellis);
            this.Controls.Add(this.drop_deblockBeta);
            this.Controls.Add(this.label41);
            this.Controls.Add(this.drop_deblockAlpha);
            this.Controls.Add(this.check_8x8DCT);
            this.Controls.Add(this.label45);
            this.Controls.Add(this.drop_analysis);
            this.Controls.Add(this.label48);
            this.Controls.Add(this.drop_subpixelMotionEstimation);
            this.Controls.Add(this.lbl_merange);
            this.Controls.Add(this.drop_MotionEstimationRange);
            this.Controls.Add(this.label54);
            this.Controls.Add(this.drop_MotionEstimationMethod);
            this.Controls.Add(this.lbl_direct_prediction);
            this.Controls.Add(this.drop_directPrediction);
            this.Controls.Add(this.label62);
            this.Controls.Add(this.drop_bFrames);
            this.Controls.Add(this.label64);
            this.Controls.Add(this.drop_refFrames);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "x264Panel";
            this.Size = new System.Drawing.Size(720, 305);
            ((System.ComponentModel.ISupportInitialize)(this.slider_psytrellis)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_psyrd)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_adaptiveQuantStrength)).EndInit();
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
        internal System.Windows.Forms.Label lbl_trellis;
        internal System.Windows.Forms.ComboBox drop_trellis;
        internal System.Windows.Forms.ComboBox drop_deblockBeta;
        internal System.Windows.Forms.Label label41;
        internal System.Windows.Forms.ComboBox drop_deblockAlpha;
        internal System.Windows.Forms.CheckBox check_8x8DCT;
        internal System.Windows.Forms.Label label45;
        internal System.Windows.Forms.ComboBox drop_analysis;
        internal System.Windows.Forms.Label label48;
        internal System.Windows.Forms.ComboBox drop_subpixelMotionEstimation;
        internal System.Windows.Forms.Label lbl_merange;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationRange;
        internal System.Windows.Forms.Label label54;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationMethod;
        internal System.Windows.Forms.Label lbl_direct_prediction;
        internal System.Windows.Forms.ComboBox drop_directPrediction;
        internal System.Windows.Forms.Label label62;
        internal System.Windows.Forms.ComboBox drop_bFrames;
        internal System.Windows.Forms.Label label64;
        internal System.Windows.Forms.ComboBox drop_refFrames;
        private System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.CheckBox check_weightp;
        internal System.Windows.Forms.Label lbl_adaptiveQuantStrength;
        internal System.Windows.Forms.TrackBar slider_adaptiveQuantStrength;
        internal System.Windows.Forms.Label label1;
        internal System.Windows.Forms.Label lbl_prymidalBframes;
        internal System.Windows.Forms.ComboBox combo_pyrmidalBFrames;
        internal System.Windows.Forms.Label label3;
    }
}
