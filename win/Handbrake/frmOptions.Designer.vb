<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmOptions
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
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmOptions))
        Me.Label1 = New System.Windows.Forms.Label
        Me.check_updateCheck = New System.Windows.Forms.CheckBox
        Me.btn_close = New System.Windows.Forms.Button
        Me.check_userDefaultSettings = New System.Windows.Forms.CheckBox
        Me.Label2 = New System.Windows.Forms.Label
        Me.check_readDVDWindow = New System.Windows.Forms.CheckBox
        Me.Label3 = New System.Windows.Forms.Label
        Me.SuspendLayout()
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(9, 11)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(168, 13)
        Me.Label1.TabIndex = 0
        Me.Label1.Text = "Update Check on Startup"
        '
        'check_updateCheck
        '
        Me.check_updateCheck.AutoSize = True
        Me.check_updateCheck.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.check_updateCheck.Location = New System.Drawing.Point(12, 32)
        Me.check_updateCheck.Name = "check_updateCheck"
        Me.check_updateCheck.Size = New System.Drawing.Size(71, 17)
        Me.check_updateCheck.TabIndex = 1
        Me.check_updateCheck.Text = "Enabled"
        Me.check_updateCheck.UseVisualStyleBackColor = True
        '
        'btn_close
        '
        Me.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_close.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_close.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_close.Location = New System.Drawing.Point(257, 171)
        Me.btn_close.Name = "btn_close"
        Me.btn_close.Size = New System.Drawing.Size(107, 22)
        Me.btn_close.TabIndex = 45
        Me.btn_close.Text = "Close"
        Me.btn_close.UseVisualStyleBackColor = True
        '
        'check_userDefaultSettings
        '
        Me.check_userDefaultSettings.AutoSize = True
        Me.check_userDefaultSettings.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.check_userDefaultSettings.Location = New System.Drawing.Point(12, 82)
        Me.check_userDefaultSettings.Name = "check_userDefaultSettings"
        Me.check_userDefaultSettings.Size = New System.Drawing.Size(71, 17)
        Me.check_userDefaultSettings.TabIndex = 47
        Me.check_userDefaultSettings.Text = "Enabled"
        Me.check_userDefaultSettings.UseVisualStyleBackColor = True
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.Location = New System.Drawing.Point(9, 61)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(259, 13)
        Me.Label2.TabIndex = 46
        Me.Label2.Text = "Load Users Default Settings on Startup"
        '
        'check_readDVDWindow
        '
        Me.check_readDVDWindow.AutoSize = True
        Me.check_readDVDWindow.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.check_readDVDWindow.Location = New System.Drawing.Point(12, 135)
        Me.check_readDVDWindow.Name = "check_readDVDWindow"
        Me.check_readDVDWindow.Size = New System.Drawing.Size(71, 17)
        Me.check_readDVDWindow.TabIndex = 49
        Me.check_readDVDWindow.Text = "Enabled"
        Me.check_readDVDWindow.UseVisualStyleBackColor = True
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.Location = New System.Drawing.Point(9, 113)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(238, 13)
        Me.Label3.TabIndex = 48
        Me.Label3.Text = "Show Select DVD window on startup"
        '
        'frmOptions
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(7.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(382, 202)
        Me.Controls.Add(Me.check_readDVDWindow)
        Me.Controls.Add(Me.Label3)
        Me.Controls.Add(Me.check_userDefaultSettings)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.btn_close)
        Me.Controls.Add(Me.check_updateCheck)
        Me.Controls.Add(Me.Label1)
        Me.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "frmOptions"
        Me.Text = "Options"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents check_updateCheck As System.Windows.Forms.CheckBox
    Friend WithEvents btn_close As System.Windows.Forms.Button
    Friend WithEvents check_userDefaultSettings As System.Windows.Forms.CheckBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents check_readDVDWindow As System.Windows.Forms.CheckBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
End Class
