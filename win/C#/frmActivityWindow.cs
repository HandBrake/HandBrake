/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace Handbrake
{
    public partial class frmActivityWindow : Form
    {

        Thread monitorFile;
        String read_file;
        frmMain mainWindow;
        frmQueue queueWindow;
        int position = 0;  // Position in the arraylist reached by the current log output in the rtf box.
        

        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during an encode.
        /// </summary>
        /// 
        public frmActivityWindow(string file, frmMain fm, frmQueue fq)
        {
            InitializeComponent();

            mainWindow = fm;
            queueWindow = fq;
            read_file = file;

            // Reset some varibles
            this.rtf_actLog.Text = string.Empty;
            position = 0;

            string logFile = Path.Combine(Path.GetTempPath(), read_file);
            if (File.Exists(logFile))
            {

                // Get the CPU Processor Name
                RegistryKey RegKey = Registry.LocalMachine;
                RegKey = RegKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
                Object cpuType = RegKey.GetValue("ProcessorNameString");

                // Get the screen resolution
                System.Windows.Forms.Screen scr = System.Windows.Forms.Screen.PrimaryScreen;

                // Physical Ram
                Functions.SystemInfo info = new Functions.SystemInfo();
                uint memory = info.TotalPhysicalMemory();

                // Add a header to the log file indicating that it's from the Windows GUI and display the windows version
                rtf_actLog.AppendText("### Windows GUI \n");
                rtf_actLog.AppendText(String.Format("### Running: {0} \n###\n", Environment.OSVersion.ToString()));
                rtf_actLog.AppendText(String.Format("### CPU: {0} \n", cpuType));
                rtf_actLog.AppendText(String.Format("### Ram: {0} MB \n", memory));
                rtf_actLog.AppendText(String.Format("### Screen: {0}x{1} \n", scr.Bounds.Width, scr.Bounds.Height));
                rtf_actLog.AppendText(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
                rtf_actLog.AppendText(String.Format("### Install Dir: {0} \n", Application.StartupPath));
                rtf_actLog.AppendText(String.Format("### Data Dir: {0} \n###\n", Application.UserAppDataPath));

                // Start a new thread to run the autoUpdate process
                monitorFile = new Thread(autoUpdate);
                monitorFile.Start();
            }
            else
                MessageBox.Show("The log file could not be found. Maybe you cleared your system's tempory folder or maybe you just havn't run an encode yet.", "Notice", MessageBoxButtons.OK, MessageBoxIcon.Warning);

            // Handle the event of the window being disposed. This is needed to make sure HandBrake exit's cleanly.
            this.Disposed += new EventHandler(forceQuit);
        }

        // Ok, so, this function is called when someone closes frmMain but didn't close frmActivitWindow first.
        // When you close frmMain, the activity window gets closed (disposed of) but, this doens't kill the threads that it started.
        // When that thread tries to access the disposed rich text box, it causes an exception.
        // Basically, this function is called when the window is disposed of, to kill the thread and close the window properly.
        // This allows HandBrake to close cleanly.
        private void forceQuit(object sender, EventArgs e)
        {
            if (monitorFile != null)
                monitorFile.Abort();

            this.Close();
        }

        // Update the Activity window every 5 seconds with the latest log data.
        private void autoUpdate(object state)
        {
            Boolean lastUpdate = false;
            updateTextFromThread();
            while (true)
            {   
                if ((mainWindow.isEncoding() == true) || (queueWindow.isEncoding() == true))
                    updateTextFromThread();
                else
                {
                    // The encode may just have stoped, so, refresh the log one more time before restarting it.
                    if (lastUpdate == false)
                        updateTextFromThread();

                    lastUpdate = true;
                    position = 0;
                }
                Thread.Sleep(5000);
            }
        }

        private delegate void UpdateUIHandler();
        private void updateTextFromThread()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new UpdateUIHandler(updateTextFromThread));
                    return;
                }
                // Initialize a pointer and get the log data arraylist
                ArrayList data = readFile();

                while (position < data.Count)
                {
                    rtf_actLog.AppendText(data[position].ToString());
                    if (data[position].ToString().Contains("has exited"))
                    {
                        rtf_actLog.AppendText("\n ############ End of Encode ############## \n");
                    }
                    position++;
                }

               // this.rtf_actLog.SelectionStart = this.rtf_actLog.Text.Length - 1;
               // this.rtf_actLog.ScrollToCaret();
            }
            catch (Exception exc)
            {
                MessageBox.Show("An error has occured in: updateTextFromThread(). \n You may have to restart HandBrake. \n  Error Information: \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private ArrayList readFile()
        {
            // Ok, the task here is to, Get an arraylist of log data.
            // And update some global varibles which are pointers to the last displayed log line.
            ArrayList logData = new ArrayList();

            try
            {
                // hb_encode_log.dat is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logFile = Path.Combine(Path.GetTempPath(), read_file);
                string logFile2 = Path.Combine(Path.GetTempPath(), "hb_encode_log_AppReadable.dat");

                // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                if (File.Exists(logFile2))
                    File.Delete(logFile2);

                // Copy the log file.
                File.Copy(logFile, logFile2);

                // Open the copied log file for reading
                StreamReader sr = new StreamReader(logFile2);
                string line = sr.ReadLine();
                while (line != null)
                {
                    if (line.Trim() != "")
                        logData.Add(line + System.Environment.NewLine);

                    line = sr.ReadLine();
                }
                sr.Close();
                sr.Dispose();

                return logData;
            }
            catch (Exception exc)
            {
                MessageBox.Show("Error in readFile() \n Unable to read the log file.\n You may have to restart HandBrake.\n  Error Information: \n\n" + exc.ToString(), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            return null;
        }


        // Ok, We need to make sure the monitor thread is dead when we close the window.
        protected override void OnClosing(CancelEventArgs e)
        {
            if (monitorFile != null)
                monitorFile.Abort();
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }

    }
}