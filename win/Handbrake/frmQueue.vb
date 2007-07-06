Imports System.IO
Imports System
Imports System.Diagnostics
Imports System.Threading
Imports System.ComponentModel
Imports System.Windows.Forms

Public Class frmQueue

    
    Delegate Sub SetTextCallback(ByVal [text] As Integer)

    '#
    '# Buttons
    '#
    Private Sub btn_delete_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_delete.Click
        list_queue.Items.Remove(list_queue.SelectedItem)
    End Sub

    Private Sub btn_Close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_Close.Click
        Me.Hide()
    End Sub

    '# STAGE 1
    Private Sub btn_q_encoder_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_q_encoder.Click

        Dim ApplicationPath As String = Application.StartupPath ' The applications start parth
        Dim encodeItems As Integer = list_queue.Items.Count ' Amount of items to encode

        encodeItems = encodeItems - 1

        'Start the encode process
        Try
            Dim params As String = list_queue.Items.Item(encodeItems)
            Dim proc As New System.Diagnostics.Process
            proc = System.Diagnostics.Process.Start("""" + ApplicationPath + "\hbcli.exe""", params)

            If My.Settings.Priority <> "Normal" Then
                Dim level As String
                level = My.Settings.Priority

                Select Case level
                    Case "Realtime"
                        proc.PriorityClass = ProcessPriorityClass.RealTime
                    Case "High"
                        proc.PriorityClass = ProcessPriorityClass.High
                    Case "Above Normal"
                        proc.PriorityClass = ProcessPriorityClass.AboveNormal
                    Case "Below Normal"
                        proc.PriorityClass = ProcessPriorityClass.BelowNormal
                    Case "Low"
                        proc.PriorityClass = ProcessPriorityClass.Idle
                End Select
            End If

        Catch ex As Exception
            MessageBox.Show("Unable to launch the encoder. Queue run failed.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
            MessageBox.Show(ex.ToString)
        End Try

        ' Lets start the process monitor
        hbcliMonitor = New ProcessMonitor()
        Dim t = New Thread(AddressOf hbcliMonitor.tmrProcCheck)
        t.Start()

        Try
            ' When this task is finished, remove it from the queue
            list_queue.Items.RemoveAt(encodeItems)
        Catch ex As Exception
            ' quietly ignore the error should the user try to encode with nothing on the queue by mistake.
        End Try
        
    End Sub



    '# STAGE 2
    Sub TheadCompletedMonitor(ByVal isRunning As Integer) Handles hbcliMonitor.ThreadComplete

        Dim ApplicationPath As String = Application.StartupPath ' The applications start parth
        Dim encodeItems As Integer = list_queue.Items.Count ' Amount of items to encode

        encodeItems = encodeItems - 1


        If encodeItems = -1 Then
            MessageBox.Show("Status: Queue completed!")
        Else
            'Start the encode process
            Try
                Dim params As String = list_queue.Items.Item(encodeItems)
                Dim proc As New System.Diagnostics.Process
                proc = System.Diagnostics.Process.Start("""" + ApplicationPath + "\hbcli.exe""", params)

                If My.Settings.Priority <> "Normal" Then
                    Dim level As String
                    level = My.Settings.Priority

                    Select Case level
                        Case "Realtime"
                            proc.PriorityClass = ProcessPriorityClass.RealTime
                        Case "High"
                            proc.PriorityClass = ProcessPriorityClass.High
                        Case "Above Normal"
                            proc.PriorityClass = ProcessPriorityClass.AboveNormal
                        Case "Below Normal"
                            proc.PriorityClass = ProcessPriorityClass.BelowNormal
                        Case "Low"
                            proc.PriorityClass = ProcessPriorityClass.Idle
                    End Select
                End If
            Catch ex As Exception
                MessageBox.Show("Unable to launch the encoder. Queue run failed.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
                MessageBox.Show(ex.ToString)
            End Try

            ' Lets start the process monitor
            hbcliMonitor = New ProcessMonitor()
            Dim t = New Thread(AddressOf hbcliMonitor.tmrProcCheck)
            t.Start()

            ' When this task is finished, remove it from the queue
            Me.SetText(encodeItems)
            'MsgBox("1 Enocde Process Finished: ", isRunning)
        End If

    End Sub


    '#
    '# Trying to safely alter stuff on the worker thread here.
    '#
    Private Sub SetText(ByVal [text] As Integer)
        If Me.list_queue.InvokeRequired Then
            Dim d As New SetTextCallback(AddressOf SetText)
            Me.Invoke(d, New Object() {[text]})
        Else
            Me.list_queue.Items.RemoveAt([text])
        End If
    End Sub

    '#
    '# Process Monitoring Stuff Here
    '#

    Dim WithEvents hbcliMonitor As ProcessMonitor

    Public Class ProcessMonitor
        Public isRunning As Integer = 1

        Public Event ThreadComplete(ByVal isRunning As Integer)

        Public Sub tmrProcCheck()
            Dim isRunning As Integer
            Dim process2 As Process = New Process
            Dim running As Boolean = True
            Dim hbProcess As Process() = Process.GetProcesses()

            While running
                Thread.Sleep(1000)
                hbProcess = Process.GetProcesses()
                running = False
                Dim processArr2 As Process() = hbProcess
                Dim i As Integer = 0
                While i < CInt(processArr2.Length)
                    Dim process1 As Process = processArr2(i)
                    If process1.ProcessName.Equals("hbcli") Then
                        running = True
                    End If
                    i = i + 1
                End While
            End While
            isRunning = 0
            RaiseEvent ThreadComplete(isRunning)
        End Sub
    End Class

    Private Sub btn_up_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_up.Click
        Dim count As Integer = list_queue.Items.Count
        Dim itemToMove As Integer = list_queue.SelectedIndex
        Dim previousItem As String = ""
        Dim previousItemint As Integer = 0

        If (itemToMove > 0) Then
            previousItemint = itemToMove - 1
            previousItem = list_queue.Items.Item(previousItemint)
            list_queue.Items.Item(previousItemint) = list_queue.Items.Item(itemToMove)
            list_queue.Items.Item(itemToMove) = previousItem
            list_queue.SelectedIndex = list_queue.SelectedIndex - 1
        End If
    End Sub

    Private Sub btn_down_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_down.Click
        Dim count As Integer = list_queue.Items.Count
        Dim itemToMove As Integer = list_queue.SelectedIndex
        Dim itemAfter As String = ""
        Dim itemAfterInt As Integer = 0


        MessageBox.Show(count)
        MessageBox.Show(itemToMove)
        If (itemToMove < (count - 1)) Then
            itemAfterInt = itemToMove + 1
            itemAfter = list_queue.Items.Item(itemAfterInt)
            list_queue.Items.Item(itemAfterInt) = list_queue.Items.Item(itemToMove)
            list_queue.Items.Item(itemToMove) = itemAfter
            list_queue.SelectedIndex = list_queue.SelectedIndex + 1
        End If
    End Sub
End Class