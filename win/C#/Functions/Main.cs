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
using System.Net;

namespace Handbrake.Functions
{
    static class Main
    {
        // Private Variables
        private static readonly XmlSerializer ser = new XmlSerializer(typeof(List<Job>));

        /// <summary>
        /// Calculate the duration of the selected title and chapters
        /// </summary>
        public static TimeSpan calculateDuration(int chapterStart, int chapterEnd, Parsing.Title selectedTitle)
        {
            TimeSpan duration = TimeSpan.FromSeconds(0.0);
            chapterStart++; chapterEnd++;
            if (chapterStart != 0 && chapterEnd != 0 && chapterEnd <= selectedTitle.Chapters.Count)
            {
                for (int i = chapterStart; i <= chapterEnd; i++)
                    duration += selectedTitle.Chapters[i - 1].Duration;
            }

            return duration;
        }

        /// <summary>
        /// Select the longest title in the DVD title dropdown menu on frmMain
        /// </summary>
        public static Parsing.Title selectLongestTitle(Parsing.DVD thisDvd)
        {
            TimeSpan longestDurationFound = TimeSpan.FromSeconds(0.0);
            Parsing.Title returnTitle = null;

            foreach (Parsing.Title item in thisDvd.Titles)
            {
                if (item.Duration > longestDurationFound)
                {
                    returnTitle = item;
                    longestDurationFound = item.Duration;
                }
            }
            return returnTitle;
        }

        /// <summary>
        /// Set's up the DataGridView on the Chapters tab (frmMain)
        /// </summary>
        public static DataGridView chapterNaming(DataGridView dataChpt, string chapterEnd)
        {
            int i = 0, finish = 0;

            if (chapterEnd != "Auto")
                int.TryParse(chapterEnd, out finish);

            while (i < finish)
            {
                int n = dataChpt.Rows.Add();
                dataChpt.Rows[n].Cells[0].Value = (i + 1);
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
        /// <param name="dataChpt"></param>
        /// <param name="filename"></param>
        /// <returns></returns>
        public static DataGridView importChapterNames(DataGridView dataChpt, string filename)
        {
            IDictionary<int, string> chapterMap = new Dictionary<int, string>();
            try
            {
                StreamReader sr = new StreamReader(filename);
                string csv = sr.ReadLine();
                while (csv != null)
                {
                    if (csv.Trim() != "")
                    {
                        string[] contents = csv.Split(',');
                        int chapter;
                        int.TryParse(contents[0], out chapter);
                        chapterMap.Add(chapter, contents[1]);
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
        public static string autoName(frmMain mainWindow ) //ComboBox drpDvdtitle, string chapter_start, string chatper_end, string source, string dest, int format, Boolean chapters)
        {
            string AutoNamePath = string.Empty;
            if (mainWindow.drp_dvdtitle.Text != "Automatic")
            {
                // Get the Source Name 
                string sourceName = Path.GetFileNameWithoutExtension(mainWindow.sourcePath);

                // Get the Selected Title Number
                string[] titlesplit = mainWindow.drp_dvdtitle.Text.Split(' ');
                string dvdTitle = titlesplit[0].Replace("Automatic", "");

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = mainWindow.drop_chapterStart.Text.Replace("Auto", "");
                string chapterFinish = mainWindow.drop_chapterFinish.Text.Replace("Auto", "");
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != "")
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

                // Get the destination filename.
                string destinationFilename;
                if (Properties.Settings.Default.autoNameFormat != "")
                {
                    destinationFilename = Properties.Settings.Default.autoNameFormat;
                    destinationFilename = destinationFilename.Replace("{source}", sourceName).Replace("{title}", dvdTitle).Replace("{chapters}", combinedChapterTag);
                }
                else
                    destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                // Add the appropriate file extension
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    if (Properties.Settings.Default.useM4v || mainWindow.Check_ChapterMarkers.Checked || mainWindow.AudioSettings.RequiresM4V() || mainWindow.Subtitles.RequiresM4V())
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
                    if (Properties.Settings.Default.autoNamePath.Trim() != "" && Properties.Settings.Default.autoNamePath.Trim() != "Click 'Browse' to set the default location")
                        AutoNamePath = Path.Combine(Properties.Settings.Default.autoNamePath, destinationFilename);
                    else // ...otherwise, output to the source directory
                        AutoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    AutoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.text_destination.Text), destinationFilename);
                    AutoNamePath = Path.ChangeExtension(AutoNamePath, Path.GetExtension(mainWindow.text_destination.Text));
                }
            }

            return AutoNamePath;
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
                    Match platform = Regex.Match(line, @"- ([A-Za-z0-9\s ]*) -");

                    if (m.Success)
                    {
                        string data = line.Replace("(", "").Replace(")", "").Replace("HandBrake ", "");
                        string[] arr = data.Split(' ');

                        Properties.Settings.Default.hb_build = int.Parse(arr[1]);
                        Properties.Settings.Default.hb_version = arr[0];
                    }
                    if (platform.Success)
                        Properties.Settings.Default.hb_platform = platform.Value.Replace("-", "").Trim();

                    if (cliProcess.TotalProcessorTime.Seconds > 10) // Don't wait longer than 10 seconds.
                    {
                        Process cli = Process.GetProcessById(cliProcess.Id);
                        if (!cli.HasExited)
                            cli.Kill();
                    }
                }
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
        public static Boolean checkQueueRecovery()
        {
            try
            {
                string tempPath = Path.Combine(Path.GetTempPath(), "hb_queue_recovery.xml");
                if (File.Exists(tempPath))
                {
                    using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                    {
                        List<Job> list = ser.Deserialize(strm) as List<Job>;
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

            DateTime startTime = DateTime.Now;
            TimeSpan duration;

            Process[] hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
            while (hbProcesses.Length == 0)
            {
                hbProcesses = Process.GetProcessesByName("HandBrakeCLI");
                duration = DateTime.Now - startTime;
                if (duration.Seconds > 5 && hbProcesses.Length == 0) // Make sure we don't wait forever if the process doesn't start
                    return -1;
            }

            Process hbProcess = null;
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

        /// <summary>
        ///  Clear all the encode log files.
        /// </summary>
        public static void clearLogs()
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            if (Directory.Exists(logDir))
            {
                DirectoryInfo info = new DirectoryInfo(logDir);
                FileInfo[] logFiles = info.GetFiles("*.txt");
                foreach (FileInfo file in logFiles)
                {
                    if (!file.Name.Contains("last_scan_log") && !file.Name.Contains("last_encode_log") && !file.Name.Contains("tmp_appReadable_log.txt"))
                        File.Delete(file.FullName);
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
                    string url = Properties.Settings.Default.hb_build.ToString().EndsWith("1") ? Properties.Settings.Default.appcast_unstable : Properties.Settings.Default.appcast;

                    // Initialize variables
                    WebRequest request = WebRequest.Create(url);
                    WebResponse response = request.GetResponse();
                    AppcastReader reader = new AppcastReader();

                    // Get the data, convert it to a string, and parse it into the AppcastReader
                    reader.getInfo(new StreamReader(response.GetResponseStream()).ReadToEnd());

                    // Further parse the information
                    string build = reader.build;

                    int latest = int.Parse(build);
                    int current = Properties.Settings.Default.hb_build;
                    int skip = Properties.Settings.Default.skipversion;

                    // If the user wanted to skip this version, don't report the update
                    if (latest == skip)
                    {
                        UpdateCheckInformation info = new UpdateCheckInformation() { NewVersionAvailable = false, BuildInformation = null };
                        callback(new UpdateCheckResult(debug, info));
                        return;
                    }

                    // Set when the last update was
                    Properties.Settings.Default.lastUpdateCheckDate = DateTime.Now;
                    Properties.Settings.Default.Save();

                    UpdateCheckInformation info2 = new UpdateCheckInformation() { NewVersionAvailable = latest > current, BuildInformation = reader };
                    callback(new UpdateCheckResult(debug, info2));
                }
                catch (Exception exc)
                {
                    callback(new UpdateCheckResult(debug, new UpdateCheckInformation() { Error = exc }));
                }
            }));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="result"></param>
        /// <returns></returns>
        public static UpdateCheckInformation EndCheckForUpdates(IAsyncResult result)
        {
            UpdateCheckResult checkResult = (UpdateCheckResult)result;
            return checkResult.Result;
        }

        /// <summary>
        /// Used in EndUpdateCheck() for update checking and the IAsyncResult design pattern.
        /// </summary>
        private class UpdateCheckResult : IAsyncResult
        {
            public UpdateCheckResult(object asyncState, UpdateCheckInformation info)
            {
                AsyncState = asyncState;
                Result = info;
            }

            /// <summary>
            /// Gets whether the check was executed in debug mode.
            /// </summary>
            public object AsyncState { get; private set; }

            /// <summary>
            /// Gets the result of the update check.
            /// </summary>
            public UpdateCheckInformation Result { get; private set; }

            public WaitHandle AsyncWaitHandle { get { throw new NotImplementedException(); } }
            public bool CompletedSynchronously { get { throw new NotImplementedException(); } }
            public bool IsCompleted { get { throw new NotImplementedException(); } }
        }

        /// <summary>
        /// Map languages and their iso639_2 value into a IDictionary
        /// </summary>
        /// <returns></returns>
        public static IDictionary<string, string> mapLanguages()
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

    }
}
