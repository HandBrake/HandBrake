/*  Main.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Xml.Serialization;
using System.Threading;
using Handbrake.EncodeQueue;

namespace Handbrake.Functions
{
    static class Main
    {
        // Private Variables
        private static readonly XmlSerializer ser = new XmlSerializer(typeof(List<QueueItem>));

        /// <summary>
        /// Calculate the duration of the selected title and chapters
        /// </summary>
        public static TimeSpan calculateDuration(string chapter_start, string chapter_end, Parsing.Title selectedTitle)
        {
            TimeSpan Duration = TimeSpan.FromSeconds(0.0);

            // Get the durations between the 2 chapter points and add them together.
            if (chapter_start != "Auto" && chapter_end != "Auto")
            {
                int start_chapter, end_chapter;
                int.TryParse(chapter_start, out start_chapter);
                int.TryParse(chapter_end, out end_chapter);

                int position = start_chapter - 1;

                if (start_chapter <= end_chapter)
                {
                    if (end_chapter > selectedTitle.Chapters.Count)
                        end_chapter = selectedTitle.Chapters.Count;

                    while (position != end_chapter)
                    {
                        TimeSpan dur = selectedTitle.Chapters[position].Duration;
                        Duration = Duration + dur;
                        position++;
                    }
                }
            }
            return Duration;
        }

        /// <summary>
        /// Select the longest title in the DVD title dropdown menu on frmMain
        /// </summary>
        public static Parsing.Title selectLongestTitle(ComboBox drp_dvdtitle)
        {
            int current_largest = 0;
            Parsing.Title title2Select;

            // Check if there are titles in the DVD title dropdown menu and make sure, it's not just "Automatic"
            if (drp_dvdtitle.Items[0].ToString() != "Automatic")
                title2Select = (Parsing.Title)drp_dvdtitle.Items[0];
            else
                title2Select = null;

            // So, If there are titles in the DVD Title dropdown menu, lets select the longest.
            if (title2Select != null)
            {
                foreach (Parsing.Title x in drp_dvdtitle.Items)
                {
                    string title = x.ToString();
                    if (title != "Automatic")
                    {
                        string[] y = title.Split(' ');
                        string time = y[1].Replace("(", "").Replace(")", "");
                        string[] z = time.Split(':');

                        int hours = int.Parse(z[0]) * 60 * 60;
                        int minutes = int.Parse(z[1]) * 60;
                        int seconds = int.Parse(z[2]);
                        int total_sec = hours + minutes + seconds;

                        if (current_largest == 0)
                        {
                            current_largest = hours + minutes + seconds;
                            title2Select = x;
                        }
                        else
                        {
                            if (total_sec > current_largest)
                            {
                                current_largest = total_sec;
                                title2Select = x;
                            }
                        }
                    }
                }
            }
            return title2Select;
        }

        /// <summary>
        /// Set's up the DataGridView on the Chapters tab (frmMain)
        /// </summary>
        public static DataGridView chapterNaming(DataGridView data_chpt, string chapter_end)
        {
            int i = 0, finish = 0;

            if (chapter_end != "Auto")
                int.TryParse(chapter_end, out finish);

            while (i < finish)
            {
                int n = data_chpt.Rows.Add();
                data_chpt.Rows[n].Cells[0].Value = (i + 1);
                data_chpt.Rows[n].Cells[1].Value = "Chapter " + (i + 1);
                data_chpt.Rows[n].Cells[0].ValueType = typeof(int);
                data_chpt.Rows[n].Cells[1].ValueType = typeof(string);
                i++;
            }

            return data_chpt;
        }

        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        public static string autoName(ComboBox drp_dvdtitle, string chapter_start, string chatper_end, string source, string dest, int format)
        {
            string AutoNamePath = string.Empty;
            if (drp_dvdtitle.Text != "Automatic")
            {
                // Get the Source Name 
                string sourceName = Path.GetFileNameWithoutExtension(source);

                // Get the Selected Title Number
                string[] titlesplit = drp_dvdtitle.Text.Split(' ');
                string dvdTitle = titlesplit[0].Replace("Automatic", "");

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = chapter_start.Replace("Auto", "");
                string chapterFinish = chatper_end.Replace("Auto", "");
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != "")
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

                // Get the destination filename.
                string destination_filename;
                if (Properties.Settings.Default.autoNameFormat != "")
                {
                    destination_filename = Properties.Settings.Default.autoNameFormat;
                    destination_filename = destination_filename.Replace("{source}", sourceName).Replace("{title}", dvdTitle).Replace("{chapters}", combinedChapterTag);
                }
                else
                    destination_filename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                // Add the appropriate file extension
                if (format == 0)
                    destination_filename += ".mp4";
                else if (format == 1)
                    destination_filename += ".m4v";
                else if (format == 2)
                    destination_filename += ".mkv";

                // Now work out the path where the file will be stored.
                // First case: If the destination box doesn't already contain a path, make one.
                if (!dest.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // If there is an auto name path, use it...
                    if (Properties.Settings.Default.autoNamePath.Trim() != "" && Properties.Settings.Default.autoNamePath.Trim() != "Click 'Browse' to set the default location")
                        AutoNamePath = Path.Combine(Properties.Settings.Default.autoNamePath, destination_filename);
                    else // ...otherwise, output to the source directory
                        AutoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    AutoNamePath = Path.Combine(Path.GetDirectoryName(dest), destination_filename);
                    AutoNamePath = Path.ChangeExtension(AutoNamePath, Path.GetExtension(dest));
                }
            }

            return AutoNamePath;
        }

        /// <summary>
        /// Checks for updates and returns true if an update is available.
        /// </summary>
        /// <param name="debug">Turns on debug mode. Don't use on program startup</param>
        /// <returns>Boolean True = Update available</returns>
        public static Boolean updateCheck(Boolean debug)
        {
            try
            {
                AppcastReader rssRead = new AppcastReader();
                rssRead.getInfo(); // Initializes the class.
                string build = rssRead.build();

                int latest = int.Parse(build);
                int current = Properties.Settings.Default.hb_build;
                int skip = Properties.Settings.Default.skipversion;

                if (latest == skip)
                    return false;

                Boolean update = (latest > current);
                return update;
            }
            catch (Exception exc)
            {
                if (debug)
                    MessageBox.Show("Unable to check for updates, Please try again later. \n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }
        }

        /// <summary>
        /// Get's HandBrakes version data from the CLI.
        /// </summary>
        /// <returns>Arraylist of Version Data. 0 = hb_version 1 = hb_build</returns>
        public static void setCliVersionData()
        {
            String line;

            // 0 = SVN Build / Version
            // 1 = Build Date
            Process cliProcess = new Process();
            ProcessStartInfo handBrakeCLI = new ProcessStartInfo("HandBrakeCLI.exe", " -u")
                                                {
                                                    UseShellExecute = false,
                                                    RedirectStandardError = true,
                                                    RedirectStandardOutput = true,
                                                    CreateNoWindow = true
                                                };
            cliProcess.StartInfo = handBrakeCLI;

            try
            {
                cliProcess.Start();
                // Retrieve standard output and report back to parent thread until the process is complete
                TextReader stdOutput = cliProcess.StandardError;

                while (!cliProcess.HasExited)
                {
                    line = stdOutput.ReadLine() ?? "";
                    Match m = Regex.Match(line, @"HandBrake ([0-9.]*)(svn[0-9M]*) \([0-9]*\)");

                    if (m.Success)
                    {
                        string data = line.Replace("(", "").Replace(")", "").Replace("HandBrake ", "");
                        string[] arr = data.Split(' ');

                        Properties.Settings.Default.hb_build = int.Parse(arr[1]);
                        Properties.Settings.Default.hb_version = arr[0];
                    }
                    if (cliProcess.TotalProcessorTime.Seconds > 10) // Don't wait longer than 10 seconds.
                        killCLI(cliProcess.Id);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Unable to retrieve version information from the CLI. \nError:\n" + e);
            }
        }

        /// <summary>
        /// Search through the running processes on the system and kill HandBrakeCLI
        /// </summary>
        private static void killCLI(int id)
        {
            Process cli = Process.GetProcessById(id);
            if (!cli.HasExited)
                cli.Kill();
        }

        /// <summary>
        /// Check if the queue recovery file contains records.
        /// If it does, it means the last queue did not complete before HandBrake closed.
        /// So, return a boolean if true. 
        /// </summary>
        public static Boolean check_queue_recovery()
        {
            try
            {
                string tempPath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml");
                if (File.Exists(tempPath))
                {
                    using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                    {
                        List<QueueItem> list = ser.Deserialize(strm) as List<QueueItem>;
                        if (list != null)
                            if (list.Count != 0)
                                return true;
                    }
                }
                return false;
            }
            catch (Exception)
            {
                return false; // Keep quiet about the error.
            }
        }

        /// <summary>
        /// Get the Process ID of HandBrakeCLI for the current instance.
        /// </summary>
        /// <param name="before">List of processes before the new process was started</param>
        /// <returns>Int - Process ID</returns>
        public static int getCliProcess(Process[] before)
        {
            // This is a bit of a cludge. Maybe someone has a better idea on how to impliment this.
            // Since we used CMD to start HandBrakeCLI, we don't get the process ID from hbProc.
            // Instead we take the processes before and after, and get the ID of HandBrakeCLI.exe
            // avoiding any previous instances of HandBrakeCLI.exe in before.
            // Kill the current process.

            Process[] hbProcesses = Process.GetProcessesByName("HandBrakeCLI");

            // Another hack. We maybe need to wait a few seconds for HandBrakeCLI to launch
            if (hbProcesses.Length == 0)
            {
                Thread.Sleep(2000);
                hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
            }

            Process hbProcess = null;
            if (hbProcesses.Length > 0)
                foreach (Process process in hbProcesses)
                {
                    Boolean found = false;
                    // Check if the current CLI instance was running before we started the current one
                    foreach (Process bprocess in before)
                    {
                        if (process.Id == bprocess.Id)
                            found = true;
                    }

                    // If it wasn't running before, we found the process we want.
                    if (!found)
                    {
                        hbProcess = process;
                        break;
                    }
                }
            if (hbProcess != null)
                return hbProcess.Id;

            return -1;
        }

    }
}
