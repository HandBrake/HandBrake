/*  frmActivityWindow.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections;
using System.Collections.Generic;
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
        delegate void SetTextCallback(string text);
        String read_file;
        Thread monitor;
        frmMain mainWindow;
        frmQueue queueWindow;
        int position = 0;  // Position in the arraylist reached by the current log output in the rtf box.

        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during an encode.
        /// </summary>
        public frmActivityWindow(string file, frmMain fm, frmQueue fq)
        {
            InitializeComponent();
            this.rtf_actLog.Text = string.Empty;

            // When the window closes, we want to abort the monitor thread.
            this.Disposed += new EventHandler(forceQuit);

            mainWindow = fm;
            queueWindow = fq;
            read_file = file;
            position = 0;

            // Print the Log header in the Rich text box.
            displayLogHeader();

            if (file == "dvdinfo.dat")
                txt_log.Text = "Scan Log";
            else if (file == "hb_encode_log.dat")
                txt_log.Text = "Encode Log";

            // Start a new thread which will montior and keep the log window up to date if required/
            startLogThread(read_file);            
        }

        /// <summary>
        /// Displays the Log header
        /// </summary>
        private void displayLogHeader()
        {
            // Add a header to the log file indicating that it's from the Windows GUI and display the windows version
            rtf_actLog.AppendText(String.Format("### Windows GUI {1} {0} \n", Properties.Settings.Default.hb_build, Properties.Settings.Default.hb_version));
            rtf_actLog.AppendText(String.Format("### Running: {0} \n###\n", Environment.OSVersion.ToString()));
            rtf_actLog.AppendText(String.Format("### CPU: {0} \n", getCpuCount()));
            rtf_actLog.AppendText(String.Format("### Ram: {0} MB \n", TotalPhysicalMemory()));
            rtf_actLog.AppendText(String.Format("### Screen: {0}x{1} \n", screenBounds().Bounds.Width, screenBounds().Bounds.Height));
            rtf_actLog.AppendText(String.Format("### Temp Dir: {0} \n", Path.GetTempPath()));
            rtf_actLog.AppendText(String.Format("### Install Dir: {0} \n", Application.StartupPath));
            rtf_actLog.AppendText(String.Format("### Data Dir: {0} \n", Application.UserAppDataPath));
            rtf_actLog.AppendText("#########################################\n\n");
        }

        /// <summary>
        /// Starts a new thread which runs the autoUpdate function.
        /// </summary>
        /// <param name="file"> File which will be used to populate the Rich text box.</param>
        private void startLogThread(string file)
        {
            try
            {
                string logFile = Path.Combine(Path.GetTempPath(), file);
                if (File.Exists(logFile))
                {
                    // Start a new thread to run the autoUpdate process
                    monitor = new Thread(autoUpdate);
                    monitor.IsBackground = true;
                    monitor.Start();
                }
                else
                    rtf_actLog.AppendText("\n\n\nERROR: The log file could not be found. \nMaybe you cleared your system's tempory folder or maybe you just havn't run an encode yet. \nTried to find the log file in: " + logFile);

            }
            catch (Exception exc)
            {
                MessageBox.Show("startLogThread(): Exception: \n" + exc);
            }
        }

        /// <summary>
        /// Change the log file to be displayed to hb_encode_log.dat
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btn_scan_log_Click(object sender, EventArgs e)
        {
            if (monitor != null)
                monitor.Abort();

            rtf_actLog.Clear();
            read_file = "dvdinfo.dat";
            displayLogHeader();
            startLogThread(read_file);
            txt_log.Text = "Scan Log";
        }

        /// <summary>
        /// Change the log file to be displayed to dvdinfo.dat
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btn_encode_log_Click(object sender, EventArgs e)
        {
            if (monitor != null)
                monitor.Abort();

            rtf_actLog.Clear();
            read_file = "hb_encode_log.dat";
            position = 0;
            displayLogHeader();
            startLogThread(read_file);
            txt_log.Text = "Encode Log";
        }

        /// <summary>
        /// Copy to Clipboard
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.SelectedText != "")
                Clipboard.SetDataObject(rtf_actLog.SelectedText, true);
            else
                Clipboard.SetDataObject(rtf_actLog.Text, true);
        }

        /// <summary>
        /// Updates the log window with any new data which is in the log file.
        /// This is done every 5 seconds.
        /// </summary>
        /// <param name="state"></param>
        private void autoUpdate(object state)
        {
            try
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

                        lastUpdate = true; // Prevents the log window from being updated when there is no encode going.
                        position = 0; // There is no encoding, so reset the log position counter to 0 so it can be reused
                    }
                    Thread.Sleep(5000);
                }
            }
            catch (ThreadAbortException)
            {
                // Do Nothing. This is needed since we run thread.abort(). 
                // Should probably find a better way of making this work at some point.
            }
            catch (Exception exc)
            {
                MessageBox.Show("autoUpdate(): Exception: \n" + exc);
            }
        }

        /// <summary>
        /// Finds any new text in the log file and calls a funciton to display this new text.
        /// </summary>
        private void updateTextFromThread()
        {
            try
            {
                string text = "";
                List<string> data = readFile();
                int count = data.Count;

                while (position < count)
                {
                    text = data[position].ToString();
                    if (data[position].ToString().Contains("has exited"))
                        text = "\n ############ End of Log ############## \n";
                    position++;

                    SetText(text);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("updateTextFromThread(): Exception: \n" + exc);
            }
        }

        /// <summary>
        /// Updates the rich text box with anything in the string text.
        /// </summary>
        /// <param name="text"></param>
        private void SetText(string text)
        {
            try
            {
                // InvokeRequired required compares the thread ID of the
                // calling thread to the thread ID of the creating thread.
                // If these threads are different, it returns true.
                if (this.IsHandleCreated) // Make sure the windows has a handle before doing anything
                {
                    if (this.rtf_actLog.InvokeRequired)
                    {
                        SetTextCallback d = new SetTextCallback(SetText);
                        this.Invoke(d, new object[] { text });
                    }
                    else
                        this.rtf_actLog.AppendText(text);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("SetText(): Exception: \n" + exc);
            }
        }

        /// <summary>
        /// Read the log file, and store the data in a List.
        /// </summary>
        /// <returns></returns>
        private List<string> readFile()
        {
            // Ok, the task here is to, Get an arraylist of log data.
            // And update some global varibles which are pointers to the last displayed log line.
            List<string> logData = new List<string>();

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

        /// <summary>
        /// Kills the montior thead when the window is disposed of.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void forceQuit(object sender, EventArgs e)
        {
            if (monitor != null)
            {
                while (monitor.IsAlive)
                    monitor.Abort();
            }

            this.Close();
        }

        /// <summary>
        /// Copy Log Menu Item on the right click menu for the log rtf box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void mnu_copy_log_Click(object sender, EventArgs e)
        {
            if (rtf_actLog.SelectedText != "")
                Clipboard.SetDataObject(rtf_actLog.SelectedText, true);
            else
                Clipboard.SetDataObject(rtf_actLog.Text, true);
        }

        #region System Information
        private struct MEMORYSTATUS
        {
            public UInt32 dwLength;
            public UInt32 dwMemoryLoad;
            public UInt32 dwTotalPhys; // Used
            public UInt32 dwAvailPhys;
            public UInt32 dwTotalPageFile;
            public UInt32 dwAvailPageFile;
            public UInt32 dwTotalVirtual;
            public UInt32 dwAvailVirtual;
        }

        [DllImport("kernel32.dll")]
        private static extern void GlobalMemoryStatus
        (
            ref MEMORYSTATUS lpBuffer
        );

        /// <summary>
        /// Returns the total physical ram in a system
        /// </summary>
        /// <returns></returns>
        public uint TotalPhysicalMemory()
        {
            MEMORYSTATUS memStatus = new MEMORYSTATUS();
            GlobalMemoryStatus(ref memStatus);

            uint MemoryInfo = memStatus.dwTotalPhys;
            MemoryInfo = MemoryInfo / 1024 / 1024;

            return MemoryInfo;
        }

        /// <summary>
        /// Get the number of CPU Cores
        /// </summary>
        /// <returns>Object</returns>
        public Object getCpuCount()
        {
            RegistryKey RegKey = Registry.LocalMachine;
            RegKey = RegKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
            return RegKey.GetValue("ProcessorNameString");
        }

        /// <summary>
        /// Get the System screen size information.
        /// </summary>
        /// <returns>System.Windows.Forms.Scree</returns>
        public System.Windows.Forms.Screen screenBounds()
        {
            return System.Windows.Forms.Screen.PrimaryScreen;
        }

        #endregion

    }
}