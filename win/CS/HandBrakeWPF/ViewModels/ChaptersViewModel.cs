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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

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
        /// Export a CSV file.
        /// </summary>
        public void Export()
        {
            var saveFileDialog = new VistaSaveFileDialog
                {
                    Filter = "Csv File|*.csv",
                    DefaultExt = "csv",
                    CheckPathExists = true
                };
            saveFileDialog.ShowDialog();
            if (!string.IsNullOrEmpty(saveFileDialog.FileName))
            {
                this.ExportChaptersToCSV(saveFileDialog.FileName);
            }
        }

        /// <summary>
        /// Export the Chapter Markers to a CSV file
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <exception cref="GeneralApplicationException">
        /// Thrown when exporting fails.
        /// </exception>
        public void ExportChaptersToCSV(string filename)
        {
            try
            {
                string csv = string.Empty;

                foreach (ChapterMarker row in this.Task.ChapterNames)
                {
                    csv += row.ChapterNumber.ToString();
                    csv += ",";
                    csv += row.ChapterName.Replace(",", "\\,");
                    csv += Environment.NewLine;
                }
                var file = new StreamWriter(filename);
                file.Write(csv);
                file.Close();
                file.Dispose();
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    "Unable to save Chapter Makrers file! ",
                    "Chapter marker names will NOT be saved in your encode.",
                    exc);
            }
        }

        /// <summary>
        /// Import a CSV file
        /// </summary>
        public void Import()
        {
            var dialog = new VistaOpenFileDialog { Filter = "CSV files (*.csv)|*.csv", CheckFileExists = true };
            dialog.ShowDialog();
            string filename = dialog.FileName;

            if (string.IsNullOrEmpty(filename))
            {
                return;
            }

            IDictionary<int, string> chapterMap = new Dictionary<int, string>();
            try
            {
                var sr = new StreamReader(filename);
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
                // Do Nothing
            }

            // Now iterate over each chatper we have, and set it's name
            foreach (ChapterMarker item in this.Task.ChapterNames)
            {
                string chapterName;
                chapterMap.TryGetValue(item.ChapterNumber, out chapterName);
                item.ChapterName = chapterName;
            }
        }

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Title title, Preset preset, EncodeTask task)
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