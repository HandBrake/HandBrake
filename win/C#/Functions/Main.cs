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
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.Framework.Services;
    using HandBrake.Framework.Services.Interfaces;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using Model;

    /// <summary>
    /// Useful functions which various screens can use.
    /// </summary>
    public static class Main
    {
        /// <summary>
        /// The Error Service
        /// </summary>
        private static readonly IErrorService errorService = new ErrorService();

        /// <summary>
        /// The XML Serializer
        /// </summary>
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<QueueTask>));

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
                    destinationFilename = destinationFilename.Replace("{source}", sourceName)
                                                             .Replace("{title}", dvdTitle)
                                                             .Replace("{chapters}", combinedChapterTag);
                }
                else
                    destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                // Add the appropriate file extension
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    destinationFilename += Properties.Settings.Default.useM4v == 0 || Properties.Settings.Default.useM4v == 2 || mainWindow.Check_ChapterMarkers.Checked ||
                                           mainWindow.AudioSettings.RequiresM4V() || mainWindow.Subtitles.RequiresM4V()
                                               ? ".m4v"
                                               : ".mp4";
                }
                else if (mainWindow.drop_format.SelectedIndex == 1)
                    destinationFilename += ".mkv";

                // Now work out the path where the file will be stored.
                // First case: If the destination box doesn't already contain a path, make one.
                if (!mainWindow.text_destination.Text.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // If there is an auto name path, use it...
                    if (Properties.Settings.Default.autoNamePath.Trim() == "{source_path}" && !string.IsNullOrEmpty(mainWindow.sourcePath))
                    {
                        autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.sourcePath), destinationFilename);
                        if (autoNamePath == mainWindow.sourcePath)
                        {
                            // Append out_ to files that already exist or is the source file
                            autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.sourcePath), "output_" + destinationFilename);
                        }
                    }
                    else if (Properties.Settings.Default.autoNamePath.Trim() != string.Empty && Properties.Settings.Default.autoNamePath.Trim() != "Click 'Browse' to set the default location")
                    {
                        autoNamePath = Path.Combine(Properties.Settings.Default.autoNamePath, destinationFilename);
                    }
                    else // ...otherwise, output to the source directory
                        autoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    autoNamePath = Path.Combine(Path.GetDirectoryName(mainWindow.text_destination.Text), destinationFilename);

                    if (Path.HasExtension(mainWindow.text_destination.Text))
                        autoNamePath = Path.ChangeExtension(autoNamePath,
                                                            Path.GetExtension(mainWindow.text_destination.Text));
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
            if (Properties.Settings.Default.CliExeHash == base64Hash)
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

                        Properties.Settings.Default.hb_build = buildValue;
                        Properties.Settings.Default.hb_version = version;
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

                Properties.Settings.Default.CliExeHash = base64Hash;

                Properties.Settings.Default.Save();
            }
            catch (Exception e)
            {
                Properties.Settings.Default.hb_build = 0;
                Properties.Settings.Default.CliExeHash = null;
                Properties.Settings.Default.Save();

                ShowExceptiowWindow("Unable to retrieve version information from the CLI.", e.ToString());
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
            catch (Exception)
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
                if (IsMultiInstance) return; // Don't tamper with the files if we are multi instance

                foreach (string file in queueFiles)
                {
                    if (File.Exists(Path.Combine(appDataPath, file)))
                        File.Delete(Path.Combine(appDataPath, file));
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether HandBrake is running in multi instance mode
        /// </summary>
        /// <returns>True if the UI has another instance running</returns>
        public static bool IsMultiInstance
        {
            get
            {
                return Process.GetProcessesByName("HandBrake").Length > 0 ? true : false;
            }
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
                if (!string.IsNullOrEmpty(s))
                {
                    sb.Append(s[0].ToString().ToUpper());
                    sb.Append(s.Substring(1).ToLower());
                    sb.Append(" ");
                }
            }

            return sb.ToString().Trim();
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
            errorService.ShowError(shortError, longError);
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