/*  Main.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Extensions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using Handbrake.ToolWindows;

    /// <summary>
    /// Useful functions which various screens can use.
    /// </summary>
    public static class Main
    {
        /// <summary>
        /// The XML Serializer
        /// </summary>
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<QueueTask>));

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private static readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;

        /// <summary>
        /// Set's up the DataGridView on the Chapters tab (frmMain)
        /// </summary>
        /// <param name="title">
        /// The currently selected title object.
        /// This will be used to get chapter names if they exist.
        /// </param>
        /// <param name="dataChpt">
        /// The DataGridView Control
        /// </param>
        /// <param name="chapterEnd">
        /// The chapter End.
        /// </param>
        /// <returns>
        /// The chapter naming.
        /// </returns>
        public static DataGridView ChapterNaming(Title title, DataGridView dataChpt, string chapterEnd)
        {
            int i = 0, finish = 0;

            if (chapterEnd != "Auto")
                int.TryParse(chapterEnd, out finish);

            while (i < finish)
            {
                string chapterName = string.Empty;
                if (title != null)
                {
                    if (title.Chapters.Count <= i && title.Chapters[i] != null)
                    {
                        chapterName = title.Chapters[i].ChapterName;
                    }
                }

                int n = dataChpt.Rows.Add();
                dataChpt.Rows[n].Cells[0].Value = i + 1;
                dataChpt.Rows[n].Cells[1].Value = string.IsNullOrEmpty(chapterName) ? "Chapter " + (i + 1) : chapterName;
                dataChpt.Rows[n].Cells[0].ValueType = typeof(int);
                dataChpt.Rows[n].Cells[1].ValueType = typeof(string);
                i++;
            }

            return dataChpt;
        }

        /// <summary>
        /// Import a CSV file which contains Chapter Names
        /// </summary>
        /// <param name="dataChpt">
        /// The DataGridView Control
        /// </param>
        /// <param name="filename">
        /// The filepath and name
        /// </param>
        /// <returns>A Populated DataGridView</returns>
        public static DataGridView ImportChapterNames(DataGridView dataChpt, string filename)
        {
            IDictionary<int, string> chapterMap = new Dictionary<int, string>();
            try
            {
                StreamReader sr = new StreamReader(filename);
                string csv = sr.ReadLine();
                while (csv != null)
                {
                    if (csv.Trim() != string.Empty)
                    {
                        csv = csv.Replace("\\,", "<!comma!>");
                        string[] contents = csv.Split(',');
                        int chapter;
                        int.TryParse(contents[0], out chapter);
                        chapterMap.Add(chapter, contents[1].Replace("<!comma!>", ","));
                    }
                    csv = sr.ReadLine();
                }
            }
            catch (Exception)
            {
                return null;
            }

            foreach (DataGridViewRow item in dataChpt.Rows)
            {
                string name;
                chapterMap.TryGetValue((int)item.Cells[0].Value, out name);
                item.Cells[1].Value = name ?? "Chapter " + item.Cells[0].Value;
            }

            return dataChpt;
        }

        /// <summary>
        /// Create a CSV file with the data from the Main Window Chapters tab
        /// </summary>
        /// <param name="mainWindow">Main Window</param>
        /// <param name="filePathName">Path to save the csv file</param>
        /// <returns>True if successful </returns>
        public static bool SaveChapterMarkersToCsv(frmMain mainWindow, string filePathName)
        {
            try
            {
                string csv = string.Empty;

                foreach (DataGridViewRow row in mainWindow.data_chpt.Rows)
                {
                    csv += row.Cells[0].Value.ToString();
                    csv += ",";
                    csv += row.Cells[1].Value.ToString().Replace(",", "\\,");
                    csv += Environment.NewLine;
                }
                StreamWriter file = new StreamWriter(filePathName);
                file.Write(csv);
                file.Close();
                file.Dispose();
                return true;
            }
            catch (Exception exc)
            {
                ShowExceptiowWindow("Unable to save Chapter Makrers file! \nChapter marker names will NOT be saved in your encode", exc.ToString());
                return false;
            }
        }

        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        /// <param name="mainWindow">
        /// The main Window.
        /// </param>
        /// <returns>
        /// The Generated FileName
        /// </returns>
        public static string AutoName(frmMain mainWindow)
        {
            string autoNamePath = string.Empty;
            if (mainWindow.drp_dvdtitle.Text != "Automatic")
            {
                // Get the Source Name and remove any invalid characters
                string sourceName = Path.GetInvalidFileNameChars().Aggregate(Path.GetFileNameWithoutExtension(mainWindow.SourceName), (current, character) => current.Replace(character.ToString(), string.Empty));

                // Remove Underscores
                if (UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore))
                    sourceName = sourceName.Replace("_", " ");

                // Switch to "Title Case"
                if (UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase))
                    sourceName = sourceName.ToTitleCase();

                // Get the Selected Title Number
                string[] titlesplit = mainWindow.drp_dvdtitle.Text.Split(' ');
                string dvdTitle = titlesplit[0].Replace("Automatic", string.Empty);

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = mainWindow.drop_chapterStart.Text.Replace("Auto", string.Empty);
                string chapterFinish = mainWindow.drop_chapterFinish.Text.Replace("Auto", string.Empty);
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != string.Empty)
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

                /*
                 * File Name
                 */ 
                string destinationFilename;
                if (UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != string.Empty)
                {
                    destinationFilename = UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);
                    destinationFilename = destinationFilename.Replace("{source}", sourceName)
                                                             .Replace("{title}", dvdTitle)
                                                             .Replace("{chapters}", combinedChapterTag)
                                                             .Replace("{date}", DateTime.Now.Date.ToShortDateString().Replace('/', '-'));
                }
                else
                    destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                /*
                 * File Extension
                 */ 
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    switch (UserSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                    {
                        case 0: // Automatic
                            destinationFilename += mainWindow.Check_ChapterMarkers.Checked ||
                                           mainWindow.AudioSettings.RequiresM4V() || mainWindow.Subtitles.RequiresM4V()
                                               ? ".m4v"
                                               : ".mp4";
                            break;
                        case 1: // Always MP4
                            destinationFilename += ".mp4";
                            break;
                        case 2: // Always M4V
                            destinationFilename += ".m4v";
                            break;
                    }
                }
                else if (mainWindow.drop_format.SelectedIndex == 1)
                    destinationFilename += ".mkv";

                /*
                 * File Destination Path
                 */ 

                // If there is an auto name path, use it...
                if (UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().StartsWith("{source_path}") && !string.IsNullOrEmpty(mainWindow.sourcePath))
                {
                    string savedPath = UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().Replace("{source_path}\\", string.Empty).Replace("{source_path}", string.Empty);
                    string requestedPath = Path.Combine(Path.GetDirectoryName(mainWindow.sourcePath), savedPath);

                    autoNamePath = Path.Combine(requestedPath, destinationFilename);
                    if (autoNamePath == mainWindow.sourcePath)
                    {
                        // Append out_ to files that already exist or is the source file
                        autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.sourcePath), "output_" + destinationFilename);
                    }
                }
                else if (UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Contains("{source_folder_name}") && !string.IsNullOrEmpty(mainWindow.sourcePath))
                {
                    // Second Case: We have a Path, with "{source_folder}" in it, therefore we need to replace it with the folder name from the source.
                    string path = Path.GetDirectoryName(mainWindow.sourcePath);
                    if (!string.IsNullOrEmpty(path))
                    {
                        string[] filesArray = path.Split(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                        string sourceFolder = filesArray[filesArray.Length - 1];

                        autoNamePath = Path.Combine(UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Replace("{source_folder_name}", sourceFolder), destinationFilename);
                    }
                }
                else if (!mainWindow.text_destination.Text.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // Third case: If the destination box doesn't already contain a path, make one.
                    if (UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != string.Empty &&
                        UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != "Click 'Browse' to set the default location")
                    {
                        autoNamePath = Path.Combine(UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath), destinationFilename);
                    }
                    else // ...otherwise, output to the source directory
                        autoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.text_destination.Text), destinationFilename);
                }
            }

            return autoNamePath;
        }

        /// <summary>
        /// Get's HandBrakes version data from the CLI.
        /// </summary>
        public static void SetCliVersionData()
        {
            string line;

            // 0 = SVN Build / Version
            // 1 = Build Date

            // Get the SHA1 Hash of HandBrakeCLI
            byte[] hash;
            using (Stream stream = File.OpenRead(Path.Combine(Application.StartupPath, "HandBrakeCLI.exe")))
            {
                hash = SHA1.Create().ComputeHash(stream);
            }
            string base64Hash = Convert.ToBase64String(hash);

            // Compare the hash with the last known hash. If it's the same, return.
            if (UserSettingService.GetUserSetting<string>(UserSettingConstants.CliExeHash) == base64Hash)
            {
                return;
            }

            // It's not the same, so start the CLI to get it's version data.
            Process cliProcess = new Process();
            ProcessStartInfo handBrakeCli = new ProcessStartInfo("HandBrakeCLI.exe", " -u -v0")
                {
                    UseShellExecute = false,
                    RedirectStandardError = true,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                };
            cliProcess.StartInfo = handBrakeCli;

            try
            {
                cliProcess.Start();

                // Retrieve standard output and report back to parent thread until the process is complete
                TextReader stdOutput = cliProcess.StandardError;

                while (!cliProcess.HasExited)
                {
                    line = stdOutput.ReadLine() ?? string.Empty;
                    Match m = Regex.Match(line, @"HandBrake ([svnM0-9.]*) \(([0-9]*)\)");
                    Match platform = Regex.Match(line, @"- ([A-Za-z0-9\s ]*) -");

                    if (m.Success)
                    {
                        string version = m.Groups[1].Success ? m.Groups[1].Value : string.Empty;
                        string build = m.Groups[2].Success ? m.Groups[2].Value : string.Empty;

                        int buildValue;
                        int.TryParse(build, out buildValue);

                        UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, buildValue);
                        UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeVersion, version);
                    }

                    if (platform.Success)
                    {
                        UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakePlatform, platform.Value.Replace("-", string.Empty).Trim());
                    }

                    if (cliProcess.TotalProcessorTime.Seconds > 10) // Don't wait longer than 10 seconds.
                    {
                        Process cli = Process.GetProcessById(cliProcess.Id);
                        if (!cli.HasExited)
                        {
                            cli.Kill();
                        }
                    }
                }

                UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeExeHash, base64Hash);
            }
            catch (Exception e)
            {
                UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, string.Empty);
                UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakePlatform, string.Empty);
                UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeVersion, string.Empty);
                UserSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeExeHash, string.Empty);

                ShowExceptiowWindow("Unable to retrieve version information from the CLI.", e.ToString());
                Application.Exit();
            }
        }

        /// <summary>
        /// Check if the queue recovery file contains records.
        /// If it does, it means the last queue did not complete before HandBrake closed.
        /// So, return a boolean if true. 
        /// </summary>
        /// <returns>
        /// True if there is a queue to recover.
        /// </returns>
        public static List<string> CheckQueueRecovery()
        {
            try
            {
                string tempPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
                List<string> queueFiles = new List<string>();

                DirectoryInfo info = new DirectoryInfo(tempPath);
                FileInfo[] logFiles = info.GetFiles("*.xml");
                foreach (FileInfo file in logFiles)
                {
                    if (!file.Name.Contains("hb_queue_recovery"))
                        continue;

                    using (FileStream strm = new FileStream(Path.Combine(file.DirectoryName, file.Name), FileMode.Open, FileAccess.Read))
                    {
                        List<QueueTask> list = Ser.Deserialize(strm) as List<QueueTask>;
                        if (list != null)
                        {
                            if (list.Count != 0)
                            {
                                queueFiles.Add(file.Name);
                            }
                        }
                    }
                }

                return queueFiles;
            }
            catch (Exception exc)
            {
                return new List<string>(); // Keep quiet about the error.
            }
        }

        /// <summary>
        /// Recover a queue from file.
        /// </summary>
        /// <param name="encodeQueue">
        /// The encode Queue.
        /// </param>
        public static void RecoverQueue(IQueueProcessor encodeQueue)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");

            DialogResult result = DialogResult.None;
            List<string> queueFiles = CheckQueueRecovery();
            if (queueFiles.Count == 1)
            {
                result = MessageBox.Show(
                        "HandBrake has detected unfinished items on the queue from the last time the application was launched. Would you like to recover these?",
                        "Queue Recovery Possible", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            }
            else if (queueFiles.Count > 1)
            {
                result = MessageBox.Show(
                        "HandBrake has detected multiple unfinished queue files. These will be from multiple instances of HandBrake running. Would you like to recover all unfinished jobs?",
                        "Queue Recovery Possible", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            }

            if (result == DialogResult.Yes)
            {
                foreach (string file in queueFiles)
                {
                    encodeQueue.QueueManager.RestoreQueue(appDataPath + file); // Start Recovery
                }
            }
            else
            {
                if (GeneralUtilities.IsMultiInstance) return; // Don't tamper with the files if we are multi instance

                foreach (string file in queueFiles)
                {
                    if (File.Exists(Path.Combine(appDataPath, file)))
                        File.Delete(Path.Combine(appDataPath, file));
                }
            }
        }

        /// <summary>
        /// Show the Exception Window
        /// </summary>
        /// <param name="shortError">
        /// The short error.
        /// </param>
        /// <param name="longError">
        /// The long error.
        /// </param>
        public static void ShowExceptiowWindow(string shortError, string longError)
        {
            ExceptionWindow window = new ExceptionWindow();
            window.Setup(shortError, longError);
            window.ShowDialog();
        }

        /// <summary>
        /// Get The Source from the CLI Query
        /// </summary>
        /// <param name="query">Full CLI Query</param>
        /// <returns>The Source Path</returns>
        public static string GetSourceFromQuery(string query)
        {
            int startIndex = query.IndexOf("-i \"");
            if (startIndex != -1)
            {
                string input = query.Substring(startIndex).Replace("-i \"", string.Empty).Trim();

                int closeIndex = input.IndexOf('"');

                return closeIndex == -1 ? "Unknown" : input.Substring(0, closeIndex);
            }

            return "Unknown";
        }

        /// <summary>
        /// Get the Destination from the CLI Query
        /// </summary>
        /// <param name="query">Full CLI Query</param>
        /// <returns>The Destination path</returns>
        public static string GetDestinationFromQuery(string query)
        {
            int startIndex = query.IndexOf("-o \"");
            if (startIndex != -1)
            {
                string output = query.Substring(startIndex).Replace("-o \"", string.Empty).Trim();

                int closeIndex = output.IndexOf('"');

                return closeIndex == -1 ? "Unknown" : output.Substring(0, closeIndex);
            }

            return "Unknown";
        }
    }
}