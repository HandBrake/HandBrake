/*  frmQueue.Designer.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmQueue
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmQueue));
            this.btn_down = new System.Windows.Forms.Button();
            this.btn_up = new System.Windows.Forms.Button();
            this.btn_delete = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.label4 = new System.Windows.Forms.Label();
            this.lbl_chapt = new System.Windows.Forms.Label();
            this.lbl_title = new System.Windows.Forms.Label();
            this.lbl_aEnc = new System.Windows.Forms.Label();
            this.lbl_vEnc = new System.Windows.Forms.Label();
            this.lbl_dest = new System.Windows.Forms.Label();
            this.lbl_source = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_encode = new System.Windows.Forms.ToolStripButton();
            this.btn_pause = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.drop_button_queue = new System.Windows.Forms.ToolStripDropDownButton();
            this.mnu_batch = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_import = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_export = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_readd = new System.Windows.Forms.ToolStripMenuItem();
            this.SaveFile = new System.Windows.Forms.SaveFileDialog();
            this.list_queue = new System.Windows.Forms.ListView();
            this.Title = new System.Windows.Forms.ColumnHeader();
            this.Chapters = new System.Windows.Forms.ColumnHeader();
            this.Source = new System.Windows.Forms.ColumnHeader();
            this.Destination = new System.Windows.Forms.ColumnHeader();
            this.EncoderVideo = new System.Windows.Forms.ColumnHeader();
            this.Audio = new System.Windows.Forms.ColumnHeader();
            this.mnu_queue = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_up = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_Down = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.lbl_encodesPending = new System.Windows.Forms.ToolStripStatusLabel();
            this.OpenFile = new System.Windows.Forms.OpenFileDialog();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.toolStrip1.SuspendLayout();
            this.mnu_queue.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btn_down
            // 
            this.btn_down.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_down.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_down.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_down.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_down.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_down.Location = new System.Drawing.Point(625, 52);
            this.btn_down.Name = "btn_down";
            this.tableLayoutPanel1.SetRowSpan(this.btn_down, 2);
            this.btn_down.Size = new System.Drawing.Size(64, 22);
            this.btn_down.TabIndex = 33;
            this.btn_down.TabStop = false;
            this.btn_down.Text = "Down";
            this.toolTip1.SetToolTip(this.btn_down, "Move the selected item down 1 place in the queue");
            this.btn_down.UseVisualStyleBackColor = true;
            this.btn_down.Click += new System.EventHandler(this.btn_down_Click);
            // 
            // btn_up
            // 
            this.btn_up.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_up.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_up.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_up.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_up.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_up.Location = new System.Drawing.Point(564, 52);
            this.btn_up.Name = "btn_up";
            this.tableLayoutPanel1.SetRowSpan(this.btn_up, 2);
            this.btn_up.Size = new System.Drawing.Size(55, 22);
            this.btn_up.TabIndex = 32;
            this.btn_up.TabStop = false;
            this.btn_up.Text = "Up";
            this.toolTip1.SetToolTip(this.btn_up, "Move the selected item up 1 place in the queue.");
            this.btn_up.UseVisualStyleBackColor = true;
            this.btn_up.Click += new System.EventHandler(this.btn_up_Click);
            // 
            // btn_delete
            // 
            this.btn_delete.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_delete.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_delete.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_delete.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_delete.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_delete.Location = new System.Drawing.Point(695, 52);
            this.btn_delete.Name = "btn_delete";
            this.tableLayoutPanel1.SetRowSpan(this.btn_delete, 2);
            this.btn_delete.Size = new System.Drawing.Size(64, 22);
            this.btn_delete.TabIndex = 31;
            this.btn_delete.TabStop = false;
            this.btn_delete.Text = "Delete";
            this.toolTip1.SetToolTip(this.btn_delete, "Remove the selected item from the queue");
            this.btn_delete.UseVisualStyleBackColor = true;
            this.btn_delete.Click += new System.EventHandler(this.btn_delete_Click);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(125, 49);
            this.label4.Name = "label4";
            this.tableLayoutPanel1.SetRowSpan(this.label4, 2);
            this.label4.Size = new System.Drawing.Size(48, 26);
            this.label4.TabIndex = 70;
            this.label4.Text = "Video:\r\nAudio:";
            // 
            // lbl_chapt
            // 
            this.lbl_chapt.AutoSize = true;
            this.lbl_chapt.Location = new System.Drawing.Point(108, 62);
            this.lbl_chapt.Name = "lbl_chapt";
            this.lbl_chapt.Size = new System.Drawing.Size(11, 13);
            this.lbl_chapt.TabIndex = 69;
            this.lbl_chapt.Text = "-";
            // 
            // lbl_title
            // 
            this.lbl_title.AccessibleRole = System.Windows.Forms.AccessibleRole.None;
            this.lbl_title.AutoSize = true;
            this.lbl_title.Location = new System.Drawing.Point(108, 49);
            this.lbl_title.Name = "lbl_title";
            this.lbl_title.Size = new System.Drawing.Size(11, 13);
            this.lbl_title.TabIndex = 68;
            this.lbl_title.Text = "-";
            // 
            // lbl_aEnc
            // 
            this.lbl_aEnc.AutoSize = true;
            this.lbl_aEnc.Location = new System.Drawing.Point(179, 62);
            this.lbl_aEnc.Name = "lbl_aEnc";
            this.lbl_aEnc.Size = new System.Drawing.Size(11, 13);
            this.lbl_aEnc.TabIndex = 67;
            this.lbl_aEnc.Text = "-";
            // 
            // lbl_vEnc
            // 
            this.lbl_vEnc.AutoSize = true;
            this.lbl_vEnc.Location = new System.Drawing.Point(179, 49);
            this.lbl_vEnc.Name = "lbl_vEnc";
            this.lbl_vEnc.Size = new System.Drawing.Size(11, 13);
            this.lbl_vEnc.TabIndex = 66;
            this.lbl_vEnc.Text = "-";
            // 
            // lbl_dest
            // 
            this.lbl_dest.AutoSize = true;
            this.tableLayoutPanel1.SetColumnSpan(this.lbl_dest, 6);
            this.lbl_dest.Location = new System.Drawing.Point(108, 36);
            this.lbl_dest.Name = "lbl_dest";
            this.lbl_dest.Size = new System.Drawing.Size(11, 13);
            this.lbl_dest.TabIndex = 65;
            this.lbl_dest.Text = "-";
            this.lbl_dest.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_source
            // 
            this.lbl_source.AutoSize = true;
            this.tableLayoutPanel1.SetColumnSpan(this.lbl_source, 6);
            this.lbl_source.Location = new System.Drawing.Point(108, 23);
            this.lbl_source.Name = "lbl_source";
            this.lbl_source.Size = new System.Drawing.Size(11, 13);
            this.lbl_source.TabIndex = 64;
            this.lbl_source.Text = "-";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(3, 23);
            this.label1.Name = "label1";
            this.tableLayoutPanel1.SetRowSpan(this.label1, 4);
            this.label1.Size = new System.Drawing.Size(99, 52);
            this.label1.TabIndex = 63;
            this.label1.Text = "Source:\r\nDestination:\r\nDVD Title:\r\nDVD Chapters:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(3, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(82, 13);
            this.label3.TabIndex = 62;
            this.label3.Text = "Current Job";
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode,
            this.btn_pause,
            this.toolStripSeparator1,
            this.drop_button_queue});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.toolStrip1.Size = new System.Drawing.Size(787, 39);
            this.toolStrip1.TabIndex = 71;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_encode
            // 
            this.btn_encode.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_encode.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_encode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(82, 36);
            this.btn_encode.Text = "Encode";
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // btn_pause
            // 
            this.btn_pause.Image = global::Handbrake.Properties.Resources.Pause;
            this.btn_pause.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_pause.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_pause.Name = "btn_pause";
            this.btn_pause.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.btn_pause.Size = new System.Drawing.Size(74, 36);
            this.btn_pause.Text = "Pause";
            this.btn_pause.Visible = false;
            this.btn_pause.Click += new System.EventHandler(this.btn_pause_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 39);
            // 
            // drop_button_queue
            // 
            this.drop_button_queue.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_batch,
            this.mnu_import,
            this.mnu_export,
            this.toolStripSeparator2,
            this.mnu_readd});
            this.drop_button_queue.Image = global::Handbrake.Properties.Resources.ActivityWindow;
            this.drop_button_queue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.drop_button_queue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.drop_button_queue.Name = "drop_button_queue";
            this.drop_button_queue.Size = new System.Drawing.Size(87, 36);
            this.drop_button_queue.Text = "Queue";
            // 
            // mnu_batch
            // 
            this.mnu_batch.Image = global::Handbrake.Properties.Resources.Output_Small;
            this.mnu_batch.Name = "mnu_batch";
            this.mnu_batch.Size = new System.Drawing.Size(187, 22);
            this.mnu_batch.Text = "Generate Batch Script";
            this.mnu_batch.Click += new System.EventHandler(this.mnu_batch_Click);
            // 
            // mnu_import
            // 
            this.mnu_import.Image = global::Handbrake.Properties.Resources.folder;
            this.mnu_import.Name = "mnu_import";
            this.mnu_import.Size = new System.Drawing.Size(187, 22);
            this.mnu_import.Text = "Import Queue";
            this.mnu_import.Click += new System.EventHandler(this.mnu_import_Click);
            // 
            // mnu_export
            // 
            this.mnu_export.Image = global::Handbrake.Properties.Resources.save;
            this.mnu_export.Name = "mnu_export";
            this.mnu_export.Size = new System.Drawing.Size(187, 22);
            this.mnu_export.Text = "Export Queue";
            this.mnu_export.Click += new System.EventHandler(this.mnu_export_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(184, 6);
            // 
            // mnu_readd
            // 
            this.mnu_readd.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.mnu_readd.Image = global::Handbrake.Properties.Resources.AddToQueue_small;
            this.mnu_readd.Name = "mnu_readd";
            this.mnu_readd.Size = new System.Drawing.Size(187, 22);
            this.mnu_readd.Text = "Re-Add Current Job";
            this.mnu_readd.Click += new System.EventHandler(this.mnu_readd_Click);
            // 
            // SaveFile
            // 
            this.SaveFile.Filter = "Batch|.bat";
            // 
            // list_queue
            // 
            this.list_queue.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Title,
            this.Chapters,
            this.Source,
            this.Destination,
            this.EncoderVideo,
            this.Audio});
            this.list_queue.ContextMenuStrip = this.mnu_queue;
            this.list_queue.Dock = System.Windows.Forms.DockStyle.Fill;
            this.list_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.list_queue.FullRowSelect = true;
            this.list_queue.GridLines = true;
            this.list_queue.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.list_queue.Location = new System.Drawing.Point(13, 0);
            this.list_queue.Name = "list_queue";
            this.list_queue.Size = new System.Drawing.Size(761, 219);
            this.list_queue.TabIndex = 72;
            //this.list_queue.UseCompatibleStateImageBehavior = false;
            this.list_queue.View = System.Windows.Forms.View.Details;
            this.list_queue.KeyUp += new System.Windows.Forms.KeyEventHandler(this.list_queue_deleteKey);
            // 
            // Title
            // 
            this.Title.Text = "Title";
            this.Title.Width = 39;
            // 
            // Chapters
            // 
            this.Chapters.Text = "Chapters";
            this.Chapters.Width = 71;
            // 
            // Source
            // 
            this.Source.Text = "Source";
            this.Source.Width = 219;
            // 
            // Destination
            // 
            this.Destination.Text = "Destination";
            this.Destination.Width = 210;
            // 
            // EncoderVideo
            // 
            this.EncoderVideo.Text = "Video Encoder";
            this.EncoderVideo.Width = 110;
            // 
            // Audio
            // 
            this.Audio.Text = "Audio Encoder";
            this.Audio.Width = 94;
            // 
            // mnu_queue
            // 
            this.mnu_queue.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_up,
            this.mnu_Down,
            this.toolStripSeparator3,
            this.mnu_delete});
            this.mnu_queue.Name = "mnu_queue";
            this.mnu_queue.Size = new System.Drawing.Size(139, 76);
            // 
            // mnu_up
            // 
            this.mnu_up.Name = "mnu_up";
            this.mnu_up.Size = new System.Drawing.Size(138, 22);
            this.mnu_up.Text = "Move Up";
            this.mnu_up.Click += new System.EventHandler(this.mnu_up_Click);
            // 
            // mnu_Down
            // 
            this.mnu_Down.Name = "mnu_Down";
            this.mnu_Down.Size = new System.Drawing.Size(138, 22);
            this.mnu_Down.Text = "Move Down";
            this.mnu_Down.Click += new System.EventHandler(this.mnu_Down_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(135, 6);
            // 
            // mnu_delete
            // 
            this.mnu_delete.Name = "mnu_delete";
            this.mnu_delete.Size = new System.Drawing.Size(138, 22);
            this.mnu_delete.Text = "Delete";
            this.mnu_delete.Click += new System.EventHandler(this.mnu_delete_Click);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_encodesPending});
            this.statusStrip1.Location = new System.Drawing.Point(0, 368);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Padding = new System.Windows.Forms.Padding(1, 0, 12, 0);
            this.statusStrip1.Size = new System.Drawing.Size(787, 22);
            this.statusStrip1.TabIndex = 73;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // lbl_encodesPending
            // 
            this.lbl_encodesPending.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.lbl_encodesPending.Margin = new System.Windows.Forms.Padding(0, 3, 10, 2);
            this.lbl_encodesPending.Name = "lbl_encodesPending";
            this.lbl_encodesPending.Size = new System.Drawing.Size(115, 17);
            this.lbl_encodesPending.Text = "0 encode(s) pending";
            // 
            // OpenFile
            // 
            this.OpenFile.Filter = "HandBrake Queue|*.queue";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(0, 39);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tableLayoutPanel1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.list_queue);
            this.splitContainer1.Panel2.Controls.Add(this.panel3);
            this.splitContainer1.Panel2.Controls.Add(this.panel2);
            this.splitContainer1.Panel2.Controls.Add(this.panel1);
            this.splitContainer1.Size = new System.Drawing.Size(787, 329);
            this.splitContainer1.SplitterDistance = 94;
            this.splitContainer1.SplitterWidth = 1;
            this.splitContainer1.TabIndex = 74;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.ColumnCount = 7;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.Controls.Add(this.label1, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.btn_delete, 6, 4);
            this.tableLayoutPanel1.Controls.Add(this.btn_down, 5, 4);
            this.tableLayoutPanel1.Controls.Add(this.lbl_aEnc, 3, 5);
            this.tableLayoutPanel1.Controls.Add(this.btn_up, 4, 4);
            this.tableLayoutPanel1.Controls.Add(this.lbl_source, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.lbl_vEnc, 3, 4);
            this.tableLayoutPanel1.Controls.Add(this.label4, 2, 4);
            this.tableLayoutPanel1.Controls.Add(this.lbl_dest, 1, 3);
            this.tableLayoutPanel1.Controls.Add(this.lbl_title, 1, 4);
            this.tableLayoutPanel1.Controls.Add(this.lbl_chapt, 1, 5);
            this.tableLayoutPanel1.Controls.Add(this.label3, 0, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(13, 13);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 6;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 10F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.Size = new System.Drawing.Size(762, 77);
            this.tableLayoutPanel1.TabIndex = 71;
            // 
            // panel3
            // 
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(13, 219);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(761, 15);
            this.panel3.TabIndex = 77;
            // 
            // panel2
            // 
            this.panel2.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel2.Location = new System.Drawing.Point(774, 0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(13, 234);
            this.panel2.TabIndex = 76;
            // 
            // panel1
            // 
            this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(13, 234);
            this.panel1.TabIndex = 75;
            // 
            // frmQueue
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(787, 390);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.toolStrip1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(677, 417);
            this.Name = "frmQueue";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Encode Queue";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.mnu_queue.ResumeLayout(false);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btn_down;
        internal System.Windows.Forms.Button btn_up;
        internal System.Windows.Forms.Button btn_delete;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label lbl_chapt;
        private System.Windows.Forms.Label lbl_title;
        private System.Windows.Forms.Label lbl_aEnc;
        private System.Windows.Forms.Label lbl_vEnc;
        private System.Windows.Forms.Label lbl_dest;
        private System.Windows.Forms.Label lbl_source;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_encode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_pause;
        private System.Windows.Forms.SaveFileDialog SaveFile;
        private System.Windows.Forms.ListView list_queue;
        private System.Windows.Forms.ColumnHeader Title;
        private System.Windows.Forms.ColumnHeader Chapters;
        private System.Windows.Forms.ColumnHeader Source;
        private System.Windows.Forms.ColumnHeader Destination;
        private System.Windows.Forms.ColumnHeader EncoderVideo;
        private System.Windows.Forms.ColumnHeader Audio;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripDropDownButton drop_button_queue;
        private System.Windows.Forms.ToolStripMenuItem mnu_batch;
        private System.Windows.Forms.ToolStripMenuItem mnu_import;
        private System.Windows.Forms.ToolStripMenuItem mnu_export;
        private System.Windows.Forms.OpenFileDialog OpenFile;
        private System.Windows.Forms.ToolStripStatusLabel lbl_encodesPending;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_readd;
        private System.Windows.Forms.ContextMenuStrip mnu_queue;
        private System.Windows.Forms.ToolStripMenuItem mnu_up;
        private System.Windows.Forms.ToolStripMenuItem mnu_Down;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem mnu_delete;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    }
}
