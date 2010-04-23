/*  Main.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Windows.Forms;
    using System.Xml.Serialization;
    using Model;
    using Parsing;

    /// <summary>
    /// Useful functions which various screens can use.
    /// </summary>
    public static class Main
    {
        /// <summary>
        /// The XML Serializer
        /// </summary>
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<Job>));

        /// <summary>
        /// Calculate the duration of the selected title and chapters
        /// </summary>
        /// <param name="chapterStart">
        /// The chapter Start.
        /// </param>
        /// <param name="chapterEnd">
        /// The chapter End.
        /// </param>
        /// <param name="selectedTitle">
        /// The selected Title.
        /// </param>
        /// <returns>
        /// The calculated duration.
        /// </returns>
        public static TimeSpan CalculateDuration(int chapterStart, int chapterEnd, Title selectedTitle)
        {
            TimeSpan duration = TimeSpan.FromSeconds(0.0);
            chapterStart++;
            chapterEnd++;
            if (chapterStart != 0 && chapterEnd != 0 && chapterEnd <= selectedTitle.Chapters.Count)
            {
                for (int i = chapterStart; i <= chapterEnd; i++)
                    duration += selectedTitle.Chapters[i - 1].Duration;
            }

            return duration;
        }

        /// <summary>
        /// Set's up the DataGridView on the Chapters tab (frmMain)
        /// </summary>
        /// <param name="dataChpt">
        /// The DataGridView Control
        /// </param>
        /// <param name="chapterEnd">
        /// The chapter End.
        /// </param>
        /// <returns>
        /// The chapter naming.
        /// </returns>
        public static DataGridView ChapterNaming(DataGridView dataChpt, string chapterEnd)
        {
            int i = 0, finish = 0;

            if (chapterEnd != "Auto")
                int.TryParse(chapterEnd, out finish);

            while (i < finish)
            {
                int n = dataChpt.Rows.Add();
                dataChpt.Rows[n].Cells[0].Value = i + 1;
                dataChpt.Rows[n].Cells[1].Value = "Chapter " + (i + 1);
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
                // Get the Source Name 
                string sourceName = mainWindow.SourceName;

                if (Properties.Settings.Default.AutoNameRemoveUnderscore)
                    sourceName = sourceName.Replace("_", " ");

                if (Properties.Settings.Default.AutoNameTitleCase)
                    sourceName = TitleCase(sourceName);

                // Get the Selected Title Number
                string[] titlesplit = mainWindow.drp_dvdtitle.Text.Split(' ');
                string dvdTitle = titlesplit[0].Replace("Automatic", string.Empty);

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = mainWindow.drop_chapterStart.Text.Replace("Auto", string.Empty);
                string chapterFinish = mainWindow.drop_chapterFinish.Text.Replace("Auto", string.Empty);
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != string.Empty)
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

                // Get the destination filename.
                string destinationFilename;
                if (Properties.Settings.Default.autoNameFormat != string.Empty)
                {
                    destinationFilename = Properties.Settings.Default.autoNameFormat;
                    destinationFilename =
                        destinationFilename.Replace("{source}", sourceName).Replace("{title}", dvdTitle).Replace(
                            "{chapters}", combinedChapterTag);
                }
                else
                    destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                // Add the appropriate file extension
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    if (Properties.Settings.Default.useM4v || mainWindow.Check_ChapterMarkers.Checked ||
                        mainWindow.AudioSettings.RequiresM4V() || mainWindow.Subtitles.RequiresM4V())
                        destinationFilename += ".m4v";
                    else
                        destinationFilename += ".mp4";
                }
                else if (mainWindow.drop_format.SelectedIndex == 1)
                    destinationFilename += ".mkv";

                // Now work out the path where the file will be stored.
                // First case: If the destination box doesn't already contain a path, make one.
                if (!mainWindow.text_destination.Text.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // If there is an auto name path, use it...
                    if (Properties.Settings.Default.autoNamePath.Trim() != string.Empty &&
                        Properties.Settings.Default.autoNamePath.Trim() != "Click 'Browse' to set the default location")
                        autoNamePath = Path.Combine(Properties.Settings.Default.autoNamePath, destinationFilename);
                    else // ...otherwise, output to the source directory
                        autoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.text_destination.Text), destinationFilename);

                    if (Path.HasExtension(mainWindow.text_destination.Text))
                        autoNamePath = Path.ChangeExtension(autoNamePath, Path.GetExtension(mainWindow.text_destination.Text));
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
            DateTime lastModified = File.GetLastWriteTime("HandBrakeCLI.exe");

            if (Properties.Settings.Default.cliLastModified == lastModified && Properties.Settings.Default.hb_build != 0)
                return;

            Properties.Settings.Default.cliLastModified = lastModified;

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
                    Match m = Regex.Match(line, @"HandBrake ([svnM0-9.]*) \([0-9]*\)");
                    Match platform = Regex.Match(line, @"- ([A-Za-z0-9\s ]*) -");

                    if (m.Success)
                    {
                        string data = line.Replace("(", string.Empty).Replace(")", string.Empty).Replace("HandBrake ", string.Empty);
                        string[] arr = data.Split(' ');

                        Properties.Settings.Default.hb_build = int.Parse(arr[1]);
                        Properties.Settings.Default.hb_version = arr[0];
                    }

                    if (platform.Success)
                    {
                        Properties.Settings.Default.hb_platform = platform.Value.Replace("-", string.Empty).Trim();
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

                Properties.Settings.Default.Save();
            }
            catch (Exception e)
            {
                MessageBox.Show("Unable to retrieve version information from the CLI. \nError:\n" + e);
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
        public static bool CheckQueueRecovery()
        {
            try
            {
                string tempPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\hb_queue_recovery.xml");
                if (File.Exists(tempPath))
                {
                    using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                    {
                        List<Job> list = Ser.Deserialize(strm) as List<Job>;
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
        public static int GetCliProcess(Process[] before)
        {
            // This is a bit of a cludge. Maybe someone has a better idea on how to impliment this.
            // Since we used CMD to start HandBrakeCLI, we don't get the process ID from hbProc.
            // Instead we take the processes before and after, and get the ID of HandBrakeCLI.exe
            // avoiding any previous instances of HandBrakeCLI.exe in before.
            // Kill the current process.

            DateTime startTime = DateTime.Now;
            TimeSpan duration;

            Process[] hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
            while (hbProcesses.Length == 0)
            {
                hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
                duration = DateTime.Now - startTime;
                if (duration.Seconds > 5 && hbProcesses.Length == 0)
                    // Make sure we don't wait forever if the process doesn't start
                    return -1;
            }

            Process hbProcess = null;
            foreach (Process process in hbProcesses)
            {
                bool found = false;
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

        /// <summary>
        ///  Clear all the encode log files.
        /// </summary>
        public static void ClearLogs()
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            if (Directory.Exists(logDir))
            {
                DirectoryInfo info = new DirectoryInfo(logDir);
                FileInfo[] logFiles = info.GetFiles("*.txt");
                foreach (FileInfo file in logFiles)
                {
                    if (!file.Name.Contains("last_scan_log") && !file.Name.Contains("last_encode_log") &&
                        !file.Name.Contains("tmp_appReadable_log.txt"))
                        File.Delete(file.FullName);
                }
            }
        }

        /// <summary>
        /// Clear old log files x days in the past
        /// </summary>
        public static void ClearOldLogs()
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            if (Directory.Exists(logDir))
            {
                DirectoryInfo info = new DirectoryInfo(logDir);
                FileInfo[] logFiles = info.GetFiles("*.txt");

                foreach (FileInfo file in logFiles)
                {
                    if (file.LastWriteTime < DateTime.Now.AddDays(-30))
                    {
                        if (!file.Name.Contains("last_scan_log") && !file.Name.Contains("last_encode_log") &&
                            !file.Name.Contains("tmp_appReadable_log.txt"))
                            File.Delete(file.FullName);
                    }
                }
            }
        }

        /// <summary>
        /// Begins checking for an update to HandBrake.
        /// </summary>
        /// <param name="callback">The method that will be called when the check is finished.</param>
        /// <param name="debug">Whether or not to execute this in debug mode.</param>
        public static void BeginCheckForUpdates(AsyncCallback callback, bool debug)
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(delegate
                                                              {
                                                                  try
                                                                  {
                                                                      // Is this a stable or unstable build?
                                                                      string url =
                                                                          Properties.Settings.Default.hb_build.ToString()
                                                                              .EndsWith("1")
                                                                              ? Properties.Settings.Default.
                                                                                    appcast_unstable
                                                                              : Properties.Settings.Default.appcast;

                                                                      // Initialize variables
                                                                      WebRequest request = WebRequest.Create(url);
                                                                      WebResponse response = request.GetResponse();
                                                                      AppcastReader reader = new AppcastReader();

                                                                      // Get the data, convert it to a string, and parse it into the AppcastReader
                                                                      reader.GetInfo(
                                                                          new StreamReader(response.GetResponseStream())
                                                                              .ReadToEnd());

                                                                      // Further parse the information
                                                                      string build = reader.Build;

                                                                      int latest = int.Parse(build);
                                                                      int current = Properties.Settings.Default.hb_build;
                                                                      int skip = Properties.Settings.Default.skipversion;

                                                                      // If the user wanted to skip this version, don't report the update
                                                                      if (latest == skip)
                                                                      {
                                                                          UpdateCheckInformation info =
                                                                              new UpdateCheckInformation
                                                                                  {
                                                                                      NewVersionAvailable = false,
                                                                                      BuildInformation = null
                                                                                  };
                                                                          callback(new UpdateCheckResult(debug, info));
                                                                          return;
                                                                      }

                                                                      // Set when the last update was
                                                                      Properties.Settings.Default.lastUpdateCheckDate =
                                                                          DateTime.Now;
                                                                      Properties.Settings.Default.Save();

                                                                      UpdateCheckInformation info2 =
                                                                          new UpdateCheckInformation
                                                                              {
                                                                                  NewVersionAvailable = latest > current,
                                                                                  BuildInformation = reader
                                                                              };
                                                                      callback(new UpdateCheckResult(debug, info2));
                                                                  }
                                                                  catch (Exception exc)
                                                                  {
                                                                      callback(new UpdateCheckResult(debug, new UpdateCheckInformation { Error = exc }));
                                                                  }
                                                              }));
        }

        /// <summary>
        /// End Check for Updates
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        /// <returns>
        /// Update Check information
        /// </returns>
        public static UpdateCheckInformation EndCheckForUpdates(IAsyncResult result)
        {
            UpdateCheckResult checkResult = (UpdateCheckResult)result;
            return checkResult.Result;
        }

        /// <summary>
        /// Map languages and their iso639_2 value into a IDictionary
        /// </summary>
        /// <returns>A Dictionary containing the language and iso code</returns>
        public static IDictionary<string, string> MapLanguages()
        {
            IDictionary<string, string> languageMap = new Dictionary<string, string>
                                                          {
                                                              {"Any", "und"}, 
                                                              {"Afar", "aar"}, 
                                                              {"Abkhazian", "abk"}, 
                                                              {"Afrikaans", "afr"}, 
                                                              {"Akan", "aka"}, 
                                                              {"Albanian", "sqi"}, 
                                                              {"Amharic", "amh"}, 
                                                              {"Arabic", "ara"}, 
                                                              {"Aragonese", "arg"}, 
                                                              {"Armenian", "hye"}, 
                                                              {"Assamese", "asm"}, 
                                                              {"Avaric", "ava"}, 
                                                              {"Avestan", "ave"}, 
                                                              {"Aymara", "aym"}, 
                                                              {"Azerbaijani", "aze"}, 
                                                              {"Bashkir", "bak"}, 
                                                              {"Bambara", "bam"}, 
                                                              {"Basque", "eus"}, 
                                                              {"Belarusian", "bel"}, 
                                                              {"Bengali", "ben"}, 
                                                              {"Bihari", "bih"}, 
                                                              {"Bislama", "bis"}, 
                                                              {"Bosnian", "bos"}, 
                                                              {"Breton", "bre"}, 
                                                              {"Bulgarian", "bul"}, 
                                                              {"Burmese", "mya"}, 
                                                              {"Catalan", "cat"}, 
                                                              {"Chamorro", "cha"}, 
                                                              {"Chechen", "che"}, 
                                                              {"Chinese", "zho"}, 
                                                              {"Church Slavic", "chu"}, 
                                                              {"Chuvash", "chv"}, 
                                                              {"Cornish", "cor"}, 
                                                              {"Corsican", "cos"}, 
                                                              {"Cree", "cre"}, 
                                                              {"Czech", "ces"}, 
                                                              {"Dansk", "dan"}, 
                                                              {"Divehi", "div"}, 
                                                              {"Nederlands", "nld"}, 
                                                              {"Dzongkha", "dzo"}, 
                                                              {"English", "eng"}, 
                                                              {"Esperanto", "epo"}, 
                                                              {"Estonian", "est"}, 
                                                              {"Ewe", "ewe"}, 
                                                              {"Faroese", "fao"}, 
                                                              {"Fijian", "fij"}, 
                                                              {"Suomi", "fin"}, 
                                                              {"Francais", "fra"}, 
                                                              {"Western Frisian", "fry"}, 
                                                              {"Fulah", "ful"}, 
                                                              {"Georgian", "kat"}, 
                                                              {"Deutsch", "deu"}, 
                                                              {"Gaelic (Scots)", "gla"}, 
                                                              {"Irish", "gle"}, 
                                                              {"Galician", "glg"}, 
                                                              {"Manx", "glv"}, 
                                                              {"Greek Modern", "ell"}, 
                                                              {"Guarani", "grn"}, 
                                                              {"Gujarati", "guj"}, 
                                                              {"Haitian", "hat"}, 
                                                              {"Hausa", "hau"}, 
                                                              {"Hebrew", "heb"}, 
                                                              {"Herero", "her"}, 
                                                              {"Hindi", "hin"}, 
                                                              {"Hiri Motu", "hmo"}, 
                                                              {"Magyar", "hun"}, 
                                                              {"Igbo", "ibo"}, 
                                                              {"Islenska", "isl"}, 
                                                              {"Ido", "ido"}, 
                                                              {"Sichuan Yi", "iii"}, 
                                                              {"Inuktitut", "iku"}, 
                                                              {"Interlingue", "ile"}, 
                                                              {"Interlingua", "ina"}, 
                                                              {"Indonesian", "ind"}, 
                                                              {"Inupiaq", "ipk"}, 
                                                              {"Italiano", "ita"}, 
                                                              {"Javanese", "jav"}, 
                                                              {"Japanese", "jpn"}, 
                                                              {"Kalaallisut", "kal"}, 
                                                              {"Kannada", "kan"}, 
                                                              {"Kashmiri", "kas"}, 
                                                              {"Kanuri", "kau"}, 
                                                              {"Kazakh", "kaz"}, 
                                                              {"Central Khmer", "khm"}, 
                                                              {"Kikuyu", "kik"}, 
                                                              {"Kinyarwanda", "kin"}, 
                                                              {"Kirghiz", "kir"}, 
                                                              {"Komi", "kom"}, 
                                                              {"Kongo", "kon"}, 
                                                              {"Korean", "kor"}, 
                                                              {"Kuanyama", "kua"}, 
                                                              {"Kurdish", "kur"}, 
                                                              {"Lao", "lao"}, 
                                                              {"Latin", "lat"}, 
                                                              {"Latvian", "lav"}, 
                                                              {"Limburgan", "lim"}, 
                                                              {"Lingala", "lin"}, 
                                                              {"Lithuanian", "lit"}, 
                                                              {"Luxembourgish", "ltz"}, 
                                                              {"Luba-Katanga", "lub"}, 
                                                              {"Ganda", "lug"}, 
                                                              {"Macedonian", "mkd"}, 
                                                              {"Marshallese", "mah"}, 
                                                              {"Malayalam", "mal"}, 
                                                              {"Maori", "mri"}, 
                                                              {"Marathi", "mar"}, 
                                                              {"Malay", "msa"}, 
                                                              {"Malagasy", "mlg"}, 
                                                              {"Maltese", "mlt"}, 
                                                              {"Moldavian", "mol"}, 
                                                              {"Mongolian", "mon"}, 
                                                              {"Nauru", "nau"}, 
                                                              {"Navajo", "nav"}, 
                                                              {"Ndebele, South", "nbl"}, 
                                                              {"Ndebele, North", "nde"}, 
                                                              {"Ndonga", "ndo"}, 
                                                              {"Nepali", "nep"}, 
                                                              {"Norwegian Nynorsk", "nno"}, 
                                                              {"Norwegian Bokmål", "nob"}, 
                                                              {"Norsk", "nor"}, 
                                                              {"Chichewa; Nyanja", "nya"}, 
                                                              {"Occitan", "oci"}, 
                                                              {"Ojibwa", "oji"}, 
                                                              {"Oriya", "ori"}, 
                                                              {"Oromo", "orm"}, 
                                                              {"Ossetian", "oss"}, 
                                                              {"Panjabi", "pan"}, 
                                                              {"Persian", "fas"}, 
                                                              {"Pali", "pli"}, 
                                                              {"Polish", "pol"}, 
                                                              {"Portugues", "por"}, 
                                                              {"Pushto", "pus"}, 
                                                              {"Quechua", "que"}, 
                                                              {"Romansh", "roh"}, 
                                                              {"Romanian", "ron"}, 
                                                              {"Rundi", "run"}, 
                                                              {"Russian", "rus"}, 
                                                              {"Sango", "sag"}, 
                                                              {"Sanskrit", "san"}, 
                                                              {"Serbian", "srp"}, 
                                                              {"Hrvatski", "hrv"}, 
                                                              {"Sinhala", "sin"}, 
                                                              {"Slovak", "slk"}, 
                                                              {"Slovenian", "slv"}, 
                                                              {"Northern Sami", "sme"}, 
                                                              {"Samoan", "smo"}, 
                                                              {"Shona", "sna"}, 
                                                              {"Sindhi", "snd"}, 
                                                              {"Somali", "som"}, 
                                                              {"Sotho Southern", "sot"}, 
                                                              {"Espanol", "spa"}, 
                                                              {"Sardinian", "srd"}, 
                                                              {"Swati", "ssw"}, 
                                                              {"Sundanese", "sun"}, 
                                                              {"Swahili", "swa"}, 
                                                              {"Svenska", "swe"}, 
                                                              {"Tahitian", "tah"}, 
                                                              {"Tamil", "tam"}, 
                                                              {"Tatar", "tat"}, 
                                                              {"Telugu", "tel"}, 
                                                              {"Tajik", "tgk"}, 
                                                              {"Tagalog", "tgl"}, 
                                                              {"Thai", "tha"}, 
                                                              {"Tibetan", "bod"}, 
                                                              {"Tigrinya", "tir"}, 
                                                              {"Tonga", "ton"}, 
                                                              {"Tswana", "tsn"}, 
                                                              {"Tsonga", "tso"}, 
                                                              {"Turkmen", "tuk"}, 
                                                              {"Turkish", "tur"}, 
                                                              {"Twi", "twi"}, 
                                                              {"Uighur", "uig"}, 
                                                              {"Ukrainian", "ukr"}, 
                                                              {"Urdu", "urd"}, 
                                                              {"Uzbek", "uzb"}, 
                                                              {"Venda", "ven"}, 
                                                              {"Vietnamese", "vie"}, 
                                                              {"Volapük", "vol"}, 
                                                              {"Welsh", "cym"}, 
                                                              {"Walloon", "wln"}, 
                                                              {"Wolof", "wol"}, 
                                                              {"Xhosa", "xho"}, 
                                                              {"Yiddish", "yid"}, 
                                                              {"Yoruba", "yor"}, 
                                                              {"Zhuang", "zha"}, 
                                                              {"Zulu", "zul"}
                                                          };
            return languageMap;
        }

        /// <summary>
        /// Get a list of available DVD drives which are ready and contain DVD content.
        /// </summary>
        /// <returns>A List of Drives with their details</returns>
        public static List<DriveInformation> GetDrives()
        {
            List<DriveInformation> drives = new List<DriveInformation>();
            DriveInfo[] theCollectionOfDrives = DriveInfo.GetDrives();
            int id = 0;
            foreach (DriveInfo curDrive in theCollectionOfDrives)
            {
                if (curDrive.DriveType == DriveType.CDRom && curDrive.IsReady &&
                    File.Exists(curDrive.RootDirectory + "VIDEO_TS\\VIDEO_TS.IFO"))
                {
                    drives.Add(new DriveInformation
                                   {
                                       Id = id,
                                       VolumeLabel = curDrive.VolumeLabel,
                                       RootDirectory = curDrive.RootDirectory + "VIDEO_TS"
                                   });
                    id++;
                }
            }
            return drives;
        }

        /// <summary>
        /// Change a string to Title Case/
        /// </summary>
        /// <param name="input">
        /// The input.
        /// </param>
        /// <returns>
        /// A string in title case.
        /// </returns>
        public static string TitleCase(string input)
        {
            string[] tokens = input.Split(' ');
            StringBuilder sb = new StringBuilder(input.Length);
            foreach (string s in tokens)
            {
                sb.Append(s[0].ToString().ToUpper());
                sb.Append(s.Substring(1).ToLower());
                sb.Append(" ");
            }

            return sb.ToString().Trim();
        }
    }
}