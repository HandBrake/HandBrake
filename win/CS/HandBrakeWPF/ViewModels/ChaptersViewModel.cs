// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChaptersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Chapters View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;

    using Caliburn.Micro;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities.Output;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Microsoft.VisualBasic.FileIO;

    using ChapterMarker = HandBrakeWPF.Services.Encode.Model.Models.ChapterMarker;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;

    /// <summary>
    /// The Chapters View Model
    /// </summary>
    public class ChaptersViewModel : ViewModelBase, IChaptersViewModel
    {
        /// <summary>
        /// The source chapters backing field
        /// </summary>
        private List<Chapter> sourceChaptersList;

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="ChaptersViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public ChaptersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Task = new EncodeTask();
        }

        #endregion

        #region Public Properties
        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether chapter markers are enabled.
        /// </summary>
        public bool IncludeChapterMarkers
        {
            get
            {
                return this.Task.IncludeChapterMarkers;
            }
            set
            {
                this.Task.IncludeChapterMarkers = value;
                this.NotifyOfPropertyChange(() => this.IncludeChapterMarkers);
            }
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets SourceChapterList.
        /// </summary>
        private ObservableCollection<Chapter> SourceChapterList { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Export the Chapter Markers to a CSV file
        /// </summary>
        /// <exception cref="GeneralApplicationException">
        /// Thrown when exporting fails.
        /// </exception>
        public void Export()
        {
            string fileName = null;
            using (var saveFileDialog = new SaveFileDialog()
                                        {
                                            Filter = "Csv File|*.csv",
                                            DefaultExt = "csv",
                                            CheckPathExists = true,
                                            OverwritePrompt = true
                                        })
            {
                var dialogResult = saveFileDialog.ShowDialog();
                fileName = saveFileDialog.FileName;

                // Exit early if the user cancelled or the filename is invalid
                if (dialogResult != DialogResult.OK || string.IsNullOrWhiteSpace(fileName))
                    return;
            }

            try
            {
                using (var csv = new StreamWriter(fileName))
                {
                    foreach (ChapterMarker row in this.Task.ChapterNames)
                    {
                        csv.Write("{0},{1}{2}", row.ChapterNumber, CsvHelper.Escape(row.ChapterName), Environment.NewLine);
                    }
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    Resources.ChaptersViewModel_UnableToExportChaptersWarning,
                    Resources.ChaptersViewModel_UnableToExportChaptersMsg,
                    exc);
            }
        }

        /// <summary>
        /// Import a CSV file
        /// </summary>
        public void Import()
        {
            string filename = null;
            using (var dialog = new OpenFileDialog() { Filter = "CSV files (*.csv)|*.csv", CheckFileExists = true })
            {
                var dialogResult = dialog.ShowDialog();
                filename = dialog.FileName;

                // Exit if the user didn't press the OK button or the file name is invalid
                if (dialogResult != DialogResult.OK || string.IsNullOrWhiteSpace(filename))
                    return;
            }

            var chapterMap = new Dictionary<int, string>();

            using (TextFieldParser csv = new TextFieldParser(filename)
                    { CommentTokens = new[] { "#" }, // Comment lines
                        Delimiters = new[] { ",", "\t", ";", ":" }, // Support all of these common delimeter types
                        HasFieldsEnclosedInQuotes = true, // Assume that our data will be properly escaped
                        TextFieldType = FieldType.Delimited,
                        TrimWhiteSpace = true // Remove excess whitespace from ends of imported values
                    })
            {
                while (!csv.EndOfData)
                {
                    try
                    {
                        // Only read the first two columns, anything else will be ignored but will not raise an error
                        var row = csv.ReadFields();
                        if (row == null || row.Length < 2) // null condition happens if the file is somehow corrupt during reading
                            throw new MalformedLineException(Resources.ChaptersViewModel_UnableToImportChaptersLineDoesNotHaveAtLeastTwoColumns, csv.LineNumber);

                        int chapterNumber;
                        if (!int.TryParse(row[0], out chapterNumber))
                            throw new MalformedLineException(Resources.ChaptersViewModel_UnableToImportChaptersFirstColumnMustContainOnlyIntegerNumber, csv.LineNumber);

                        // Store the chapter name at the correct index
                        chapterMap[chapterNumber] = row[1]?.Trim();
                    }
                    catch (MalformedLineException mlex)
                    {
                        throw new GeneralApplicationException(
                            Resources.ChaptersViewModel_UnableToImportChaptersWarning,
                            string.Format(Resources.ChaptersViewModel_UnableToImportChaptersMalformedLineMsg, mlex.LineNumber),
                            mlex);
                    }
                }
            }

            // Exit early if no chapter information was extracted
            if (chapterMap.Count <= 0)
                return;

            // Now iterate over each chatper we have, and set it's name
            foreach (ChapterMarker item in this.Task.ChapterNames)
            {
                string chapterName;

                // If we don't have a chapter name for this chapter then 
                // fallback to the value that is already set for the chapter
                if (!chapterMap.TryGetValue(item.ChapterNumber, out chapterName))
                    chapterName = item.ChapterName;

                // Assign the chapter name unless the name is not set or only whitespace charaters
                item.ChapterName = !string.IsNullOrWhiteSpace(chapterName) ? chapterName : string.Empty;
            }
        }

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);

            if (preset != null)
            {
                this.IncludeChapterMarkers = preset.Task.IncludeChapterMarkers;
            }

            this.sourceChaptersList = title.Chapters;
            this.SetSourceChapters(title.Chapters);
        }

        /// <summary>
        /// Setup this tab for the specified preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.Task = task;
            this.Task.IncludeChapterMarkers = preset.Task.IncludeChapterMarkers;
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {
            this.Task = task;

            this.NotifyOfPropertyChange(() => this.Task.IncludeChapterMarkers);
            this.NotifyOfPropertyChange(() => this.Task.ChapterNames);
        }

        /// <summary>
        /// Reset Chapter Names
        /// </summary>
        public void Reset()
        {
            if (this.sourceChaptersList != null)
            {
                this.SetSourceChapters(this.sourceChaptersList);
            }
        }

        /// <summary>
        /// Set the Source Chapters List
        /// </summary>
        /// <param name="sourceChapters">
        /// The source chapters.
        /// </param>
        public void SetSourceChapters(IEnumerable<Chapter> sourceChapters)
        {
            // Cache the chapters in this screen
            this.SourceChapterList = new ObservableCollection<Chapter>(sourceChapters);
            this.Task.ChapterNames.Clear();

            // Then Add new Chapter Markers.
            int counter = 1;

            foreach (Chapter chapter in this.SourceChapterList)
            {
                string chapterName = string.IsNullOrEmpty(chapter.ChapterName) ? string.Format("Chapter {0}", counter) : chapter.ChapterName;
                var marker = new ChapterMarker(chapter.ChapterNumber, chapterName, chapter.Duration);
                this.Task.ChapterNames.Add(marker);

                counter += 1;
            }
        }

        #endregion
    }
}