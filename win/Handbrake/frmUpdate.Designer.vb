<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmUpdate
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing AndAlso components IsNot Nothing Then
            components.Dispose()
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmUpdate))
        Me.Label1 = New System.Windows.Forms.Label
        Me.Dest_browse = New System.Windows.Forms.Button
        Me.Label2 = New System.Windows.Forms.Label
        Me.Version = New System.Windows.Forms.Label
        Me.Label4 = New System.Windows.Forms.Label
        Me.lbl_latest = New System.Windows.Forms.Label
        Me.btn_close = New System.Windows.Forms.Button
        Me.lbl_update = New System.Windows.Forms.Label
        Me.Label3 = New System.Windows.Forms.Label
        Me.Label5 = New System.Windows.Forms.Label
        Me.Label6 = New System.Windows.Forms.Label
        Me.Label7 = New System.Windows.Forms.Label
        Me.lbl_encoderVersion = New System.Windows.Forms.Label
        Me.cliVersion = New System.Windows.Forms.Label
        Me.Label8 = New System.Windows.Forms.Label
        Me.lbl_startupStatus = New System.Windows.Forms.Label
        Me.SuspendLayout()
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(12, 9)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(110, 13)
        Me.Label1.TabIndex = 37
        Me.Label1.Text = "Update Checker"
        '
        'Dest_browse
        '
        Me.Dest_browse.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.Dest_browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.Dest_browse.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Dest_browse.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.Dest_browse.Location = New System.Drawing.Point(372, 136)
        Me.Dest_browse.Name = "Dest_browse"
        Me.Dest_browse.Size = New System.Drawing.Size(107, 22)
        Me.Dest_browse.TabIndex = 38
        Me.Dest_browse.Text = "Check Now"
        Me.Dest_browse.UseVisualStyleBackColor = True
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.Location = New System.Drawing.Point(12, 81)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(107, 13)
        Me.Label2.TabIndex = 40
        Me.Label2.Text = "Current Version: "
        Me.Label2.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'Version
        '
        Me.Version.AutoSize = True
        Me.Version.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Version.Location = New System.Drawing.Point(125, 81)
        Me.Version.Name = "Version"
        Me.Version.Size = New System.Drawing.Size(64, 13)
        Me.Version.TabIndex = 41
        Me.Version.Text = "{Version}"
        Me.Version.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.Location = New System.Drawing.Point(12, 101)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(93, 13)
        Me.Label4.TabIndex = 42
        Me.Label4.Text = "Latest Version:"
        Me.Label4.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'lbl_latest
        '
        Me.lbl_latest.AutoSize = True
        Me.lbl_latest.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lbl_latest.Location = New System.Drawing.Point(125, 101)
        Me.lbl_latest.Name = "lbl_latest"
        Me.lbl_latest.Size = New System.Drawing.Size(113, 13)
        Me.lbl_latest.TabIndex = 43
        Me.lbl_latest.Text = "Click ""Check Now"""
        Me.lbl_latest.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'btn_close
        '
        Me.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_close.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_close.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_close.Location = New System.Drawing.Point(12, 136)
        Me.btn_close.Name = "btn_close"
        Me.btn_close.Size = New System.Drawing.Size(107, 22)
        Me.btn_close.TabIndex = 44
        Me.btn_close.Text = "Close"
        Me.btn_close.UseVisualStyleBackColor = True
        '
        'lbl_update
        '
        Me.lbl_update.AutoSize = True
        Me.lbl_update.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lbl_update.ForeColor = System.Drawing.Color.DarkOrange
        Me.lbl_update.Location = New System.Drawing.Point(12, 145)
        Me.lbl_update.Name = "lbl_update"
        Me.lbl_update.Size = New System.Drawing.Size(0, 13)
        Me.lbl_update.TabIndex = 45
        Me.lbl_update.Visible = False
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.Location = New System.Drawing.Point(12, 57)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(92, 13)
        Me.Label3.TabIndex = 46
        Me.Label3.Text = "Windows GUI"
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label5.Location = New System.Drawing.Point(256, 57)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(163, 13)
        Me.Label5.TabIndex = 47
        Me.Label5.Text = "Windows Command Line"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label6.Location = New System.Drawing.Point(256, 101)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(93, 13)
        Me.Label6.TabIndex = 49
        Me.Label6.Text = "Latest Version:"
        Me.Label6.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label7.Location = New System.Drawing.Point(256, 81)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(107, 13)
        Me.Label7.TabIndex = 48
        Me.Label7.Text = "Current Version: "
        Me.Label7.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'lbl_encoderVersion
        '
        Me.lbl_encoderVersion.AutoSize = True
        Me.lbl_encoderVersion.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lbl_encoderVersion.Location = New System.Drawing.Point(369, 101)
        Me.lbl_encoderVersion.Name = "lbl_encoderVersion"
        Me.lbl_encoderVersion.Size = New System.Drawing.Size(113, 13)
        Me.lbl_encoderVersion.TabIndex = 50
        Me.lbl_encoderVersion.Text = "Click ""Check Now"""
        Me.lbl_encoderVersion.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'cliVersion
        '
        Me.cliVersion.AutoSize = True
        Me.cliVersion.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.cliVersion.Location = New System.Drawing.Point(369, 81)
        Me.cliVersion.Name = "cliVersion"
        Me.cliVersion.Size = New System.Drawing.Size(64, 13)
        Me.cliVersion.TabIndex = 51
        Me.cliVersion.Text = "{Version}"
        Me.cliVersion.TextAlign = System.Drawing.ContentAlignment.TopCenter
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label8.Location = New System.Drawing.Point(12, 32)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(142, 13)
        Me.Label8.TabIndex = 52
        Me.Label8.Text = "Startup Update Check: "
        '
        'lbl_startupStatus
        '
        Me.lbl_startupStatus.AutoSize = True
        Me.lbl_startupStatus.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lbl_startupStatus.Location = New System.Drawing.Point(150, 32)
        Me.lbl_startupStatus.Name = "lbl_startupStatus"
        Me.lbl_startupStatus.Size = New System.Drawing.Size(55, 13)
        Me.lbl_startupStatus.TabIndex = 53
        Me.lbl_startupStatus.Text = "{status}"
        '
        'frmUpdate
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(502, 172)
        Me.Controls.Add(Me.lbl_startupStatus)
        Me.Controls.Add(Me.Label8)
        Me.Controls.Add(Me.cliVersion)
        Me.Controls.Add(Me.lbl_encoderVersion)
        Me.Controls.Add(Me.Label6)
        Me.Controls.Add(Me.Label7)
        Me.Controls.Add(Me.Label5)
        Me.Controls.Add(Me.Label3)
        Me.Controls.Add(Me.lbl_update)
        Me.Controls.Add(Me.btn_close)
        Me.Controls.Add(Me.lbl_latest)
        Me.Controls.Add(Me.Label4)
        Me.Controls.Add(Me.Version)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.Dest_browse)
        Me.Controls.Add(Me.Label1)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "frmUpdate"
        Me.Text = "Update Check"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents Dest_browse As System.Windows.Forms.Button
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Version As System.Windows.Forms.Label
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents lbl_latest As System.Windows.Forms.Label
    Friend WithEvents btn_close As System.Windows.Forms.Button
    Friend WithEvents lbl_update As System.Windows.Forms.Label
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents lbl_encoderVersion As System.Windows.Forms.Label
    Friend WithEvents cliVersion As System.Windows.Forms.Label
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents lbl_startupStatus As System.Windows.Forms.Label
End Class
