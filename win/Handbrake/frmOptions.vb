Imports System.IO

Public Class frmOptions

    Private Sub btn_close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_close.Click
        My.Settings.Save()
        Me.Close()
    End Sub


    ' Set the check boxes to the correct state. Checked or unchecked
    Private Sub frmOptions_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Try
            If My.Settings.StartupUpdate = 1 Then
                check_updateCheck.CheckState = CheckState.Checked
            Else
                check_updateCheck.CheckState = CheckState.Unchecked
            End If

            If My.Settings.UseUsersDefaultSettings = 1 Then
                check_userDefaultSettings.CheckState = CheckState.Checked
            Else
                check_userDefaultSettings.CheckState = CheckState.Unchecked
            End If

            If My.Settings.ReadDVDatStartup = 1 Then
                check_readDVDWindow.CheckState = CheckState.Checked
            Else
                check_readDVDWindow.CheckState = CheckState.Unchecked
            End If

            If My.Settings.verbose = 1 Then
                check_verbose.CheckState = CheckState.Checked
            Else
                check_verbose.CheckState = CheckState.Unchecked
            End If

            drp_processors.Text = My.Settings.Processors

        Catch ex As Exception
            MessageBox.Show("ERROR:  " & ex.ToString)
        End Try
    End Sub

    ' Handle the event when a user clicks on the check box. Save the new setting.

    Private Sub check_updateCheck_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_updateCheck.CheckedChanged

        If check_updateCheck.CheckState = 1 Then
            Try
                My.Settings.StartupUpdate = 1
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        Else
            Try
                My.Settings.StartupUpdate = 0
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        End If


    End Sub

    Private Sub check_userDefaultSettings_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_userDefaultSettings.CheckedChanged
        If check_userDefaultSettings.CheckState = 1 Then
            Try
                My.Settings.UseUsersDefaultSettings = 1
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        Else
            Try
                My.Settings.UseUsersDefaultSettings = 0
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        End If
    End Sub

    Private Sub check_readDVDWindow_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_readDVDWindow.CheckedChanged
        If check_readDVDWindow.CheckState = 1 Then
            Try
                My.Settings.ReadDVDatStartup = 1
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        Else
            Try
                My.Settings.ReadDVDatStartup = 0
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        End If
    End Sub

    Private Sub check_verbose_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_verbose.CheckedChanged
        If check_verbose.CheckState = 1 Then
            Try
                My.Settings.verbose = 1
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        Else
            Try
                My.Settings.verbose = 0
            Catch Ex As Exception
                MessageBox.Show("ERROR:  " & Ex.ToString)
            End Try
        End If
    End Sub


    Private Sub drp_processors_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_processors.SelectedIndexChanged
        My.Settings.Processors = drp_processors.Text
    End Sub
End Class