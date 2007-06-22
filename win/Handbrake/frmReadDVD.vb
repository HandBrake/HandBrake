Imports System.IO
Imports System
Imports System.Diagnostics
Imports System.Threading

Public Class frmReadDVD

    '#
    '# Start Reading the DVD as soon as the window launches
    '# Write the output to dvdinfo.dat
    '#
    Private Sub frmStatus_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Me.Show()
        Dim ApplicationPath As String = Application.StartupPath

        Try
            Shell("cmd /c """"" + ApplicationPath + "\hbcli.exe"" -i """ + frmMain.text_source.Text + """" & " -t0 >" + """" + ApplicationPath + "\dvdinfo.dat""" + " 2>&1""")
        Catch ex As Exception
            MessageBox.Show("ERROR: There was a problem launching the encoder. Code: frmS-1")
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    '#
    '#
    '# Ok Button Handler
    '# Stage 1 - Wait til hbcli.exe has finished writing data out to file dvdinfo.dat
    '# Stage 2 - Parse the dvdinfo.dat file
    '# Stage 3 - output the parsed version to dvd.dat or handle errors if any
    '#
    '#

    Private Sub btn_ok_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_ok.Click
        '# Stage 1 - Wait til the hbcli exe has finished
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


        '# Stage 2 - Parse the dvdinfo.dat file
        '# This involves creating a string for each title and putting it into an array.
        '# This array is called dvdData()


        ' Lets clean any previous contents out of the DVD Title dropdown.
        frmMain.drp_dvdtitle.Items.Clear()

        Dim file_path As String = Application.StartupPath
        Dim errStatus As Integer = 0
        Dim dvdData(150) As String
        Dim break As Boolean = False
        Dim titleError As Boolean = False

        Try
            ' Parse the Data into a Single String with ~ Sepeartor
            Dim RlineFile As StreamReader = File.OpenText(file_path & "\dvdinfo.dat")
            Dim RLine As String
            Dim titleData As String = ""
            Dim ChaptStatus As Integer = 0
            Dim AudioTrackStatus As Integer = 0
            Dim SubtitleStatus As Integer = 0
            Dim counter As Integer = 0
            Dim counter2 As String = 0
            Dim ErrorCounter As Integer = 0
            RLine = "---"
            dvdData(counter2) = "---Start---"

            While RLine <> Nothing
                If (RLine.Contains("+ title")) Then
                    If (titleData <> "") Then
                        dvdData(counter2) = titleData
                        add(titleData)
                        counter2 = counter2 + 1
                    End If
                    ChaptStatus = 0
                    AudioTrackStatus = 0
                    SubtitleStatus = 0
                    titleData = RLine.Trim
                ElseIf (RLine.Contains("exited.")) Then
                    add(titleData)
                    dvdData(counter2) = titleData
                    counter2 = counter2 + 1
                    ChaptStatus = 0
                    AudioTrackStatus = 0
                    SubtitleStatus = 0
                    break = True
                ElseIf (RLine.Contains("***")) Then
                    errStatus = 1
                ElseIf (RLine.Contains("No title")) Then
                    titleError = True
                    break = True
                ElseIf (RLine.Contains("+ duration")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                ElseIf (RLine.Contains("+ size")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                ElseIf (RLine.Contains("+ autocrop")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                ElseIf (RLine.Contains("+ chapters")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                    ChaptStatus = 1
                    AudioTrackStatus = 0
                    SubtitleStatus = 0
                ElseIf (RLine.Contains("+ audio")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                    ChaptStatus = 0
                    AudioTrackStatus = 1
                    SubtitleStatus = 0
                ElseIf (RLine.Contains("+ subtitle tracks")) Then
                    titleData = titleData & " ~ " & RLine.Trim
                    ChaptStatus = 0
                    AudioTrackStatus = 0
                    SubtitleStatus = 1

                ElseIf (ChaptStatus = 1) Then
                    ' This IF statment is here incase no chapters appear.
                    If (RLine.Contains("+ audio")) Then
                        ChaptStatus = 0
                        AudioTrackStatus = 1
                        SubtitleStatus = 0
                    End If
                    titleData = titleData & " & " & RLine.Trim

                ElseIf (AudioTrackStatus = 1) Then
                    'This if statment is here incase there was no audio tracks
                    If (RLine.Contains("+ subtitle tracks")) Then
                        ChaptStatus = 0
                        AudioTrackStatus = 1
                        SubtitleStatus = 0
                    End If
                    titleData = titleData & " & " & RLine.Trim

                ElseIf (SubtitleStatus = 1) Then
                    If (RLine.Contains("+ title")) Then
                        If (titleData <> "") Then
                            dvdData(counter2) = titleData
                            counter2 = counter2 + 1
                        End If
                        ChaptStatus = 0
                        AudioTrackStatus = 0
                        SubtitleStatus = 0
                        titleData = RLine.Trim
                    Else
                        titleData = titleData & " & " & RLine.Trim
                    End If
                End If
                RLine = RlineFile.ReadLine()

                If break = True Then
                    RLine = Nothing
                ElseIf RLine = "" Then
                    RLine = " "
                    ErrorCounter = ErrorCounter + 1
                    If ErrorCounter = 50 Then
                        RLine = Nothing
                    End If
                End If

                counter = counter + 1
            End While
            '# Close the file. Its no longer needed here.
            RlineFile.Close()

        Catch ex As Exception
            MessageBox.Show(ex.ToString) ' Debug
        End Try

        '# Stage 4 - Write the parsed data out into a file. 
        '# But, if theres a problem, display an error message instead
        If errStatus = 1 Then
            MessageBox.Show("ERROR: Error Reading the DVD. Some Title information may not be available.")
        End If

        If titleError = True Then
            MessageBox.Show("ERROR: No Title(s) found.")
        End If

        Try
            Dim DataWriter As StreamWriter = New StreamWriter(file_path & "\dvd.dat")
            Dim dvdCount As Integer = dvdData.Length
            Dim counter As String = 0

            While counter <> dvdCount
                DataWriter.WriteLine(dvdData(counter))
                counter = counter + 1
            End While
            DataWriter.Close()

        Catch ex As Exception
            MessageBox.Show("ERROR: Unable to write Data file. Please make sure the application has admin privileges.")
        End Try

        Me.Close()
    End Sub

    '# A function to Add data to frmSelects Select Title Tab and also populate frmMains dvdTitle Tab
    Function add(ByVal titleData)
        Dim titleInfo() As String
        Dim str(7) As String
        Dim data As String = ""

        titleInfo = titleData.Split("~")
        Try
            str(0) = titleInfo(0).Trim.Substring(8).Replace(":", "") 'Title
            str(1) = titleInfo(1).Trim.Substring(12) ' Duration
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
            MessageBox.Show("ERROR: Incomplete DVD data found. Please copy the data on the View DVD Information tab and report this error.")
        End Try

        'Now lets add the info to the main form dropdowns
        frmMain.drp_dvdtitle.Items.Add(str(0) & " (" & str(1) & ")")

        Return 0
    End Function

End Class