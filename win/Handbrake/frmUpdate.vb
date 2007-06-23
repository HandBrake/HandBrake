Imports System.IO

Public Class frmUpdate

    Private Sub frmUpdate_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        If My.Settings.StartupUpdate = 1 Then
            lbl_startupStatus.Text = "On"
        Else
            lbl_startupStatus.Text = "Off"
        End If

        Version.Text = My.Settings.HandbrakeGUIVersion
        cliVersion.Text = My.Settings.HandbrakeCLIVersion
    End Sub

    Private Sub Dest_browse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Dest_browse.Click
        Dim file_path As String = Application.StartupPath
        Try
            ' Download the update file
            ' open the file for reading and read the first 2 lines for GUI and CLI versions
            Dim wc As New System.Net.WebClient()
            wc.DownloadFile("http://download.m0k.org/handbrake/windows/update.txt", file_path & "\update.txt")
            wc.Dispose()
            Dim versionStream As StreamReader = File.OpenText(file_path & "\update.txt")
            Dim windowsGUI As String = versionStream.ReadLine()
            Dim windowsCLI As String = versionStream.ReadLine()
            versionStream.Close()

            ' Set the Latest Text label to the first line of the file
            lbl_latest.Text = windowsGUI
            lbl_encoderVersion.Text = windowsCLI

            ' If the version is now the same as the one shown here, Display the update label
            If (windowsGUI <> My.Settings.HandbrakeGUIVersion Or windowsCLI <> My.Settings.HandbrakeCLIVersion) Then
                MessageBox.Show("A new version is available. Please visit the project website to download the update.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)
            End If


        Catch ex As Exception
            ' Handdle any errors that may occur
            MessageBox.Show("Unable to check for updates. The server may be unavailible at the moment. Please try again later!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
        End Try


    End Sub

    Private Sub btn_close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_close.Click
        Me.Close()
    End Sub

End Class