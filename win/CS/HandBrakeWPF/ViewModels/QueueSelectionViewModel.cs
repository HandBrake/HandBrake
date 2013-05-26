// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueSelectionViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Queue Selection View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Linq;

    using HandBrake.ApplicationServices.Parsing;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Queue Selection View Model
    /// </summary>
    public class QueueSelectionViewModel : ViewModelBase, IQueueSelectionViewModel
    {
        /// <summary>
        /// The error service.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The ordered by duration.
        /// </summary>
        private bool orderedByDuration;

        /// <summary>
        /// The ordered by title.
        /// </summary>
        private bool orderedByTitle;

        /// <summary>
        /// The add to queue.
        /// </summary>
        private Action<IEnumerable<SelectionTitle>> addToQueue;

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueSelectionViewModel"/> class. 
        /// </summary>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        public QueueSelectionViewModel(IErrorService errorService)
        {
            this.errorService = errorService;
            this.Title = "Add to Queue";
            this.TitleList = new BindingList<SelectionTitle>();
            this.OrderedByTitle = true;
        }

        /// <summary>
        /// Gets or sets the source.
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets the selected titles.
        /// </summary>
        public BindingList<SelectionTitle> TitleList { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ordered by title.
        /// </summary>
        public bool OrderedByTitle
        {
            get
            {
                return this.orderedByTitle;
            }

            set
            {
                this.orderedByTitle = value;
                this.NotifyOfPropertyChange(() => OrderedByTitle);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ordered by duration.
        /// </summary>
        public bool OrderedByDuration
        {
            get
            {
                return this.orderedByDuration;
            }

            set
            {
                this.orderedByDuration = value;
                this.NotifyOfPropertyChange(() => OrderedByDuration);
            }
        }

        /// <summary>
        /// Gets a value indicating whether is auto naming enabled.
        /// </summary>
        public bool IsAutoNamingEnabled
        {
            get
            {
                return this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);
            }
        }

        /// <summary>
        /// Gets a value indicating whether is automatic track selection enabled.
        /// </summary>
        public bool IsAutomaticTrackSelectionEnabled
        {
            get
            {
                // TODO decide what is the minimal requirement to hide the warning message.
                if (this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage) != Constants.Any ||
                    this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles) !=
                    Constants.Any)
                {
                    return true;
                }

                return false;
            }
        }

        /// <summary>
        /// The order by title.
        /// </summary>
        public void OrderByTitle()
        {
            TitleList = new BindingList<SelectionTitle>(TitleList.OrderBy(o => o.Title.TitleNumber).ToList());
            this.NotifyOfPropertyChange(() => TitleList);
            this.OrderedByTitle = true;
            this.OrderedByDuration = false;
        }

        /// <summary>
        /// The order by duration.
        /// </summary>
        public void OrderByDuration()
        {
            TitleList = new BindingList<SelectionTitle>(TitleList.OrderByDescending(o => o.Title.Duration).ToList());
            this.NotifyOfPropertyChange(() => TitleList);
            this.OrderedByTitle = false;
            this.OrderedByDuration = true;
        }

        /// <summary>
        /// The select all.
        /// </summary>
        public void SelectAll()
        {
            foreach (var item in TitleList)
            {
                item.IsSelected = true;
            }
        }

        /// <summary>
        /// The select all.
        /// </summary>
        public void UnSelectAll()
        {
            foreach (var item in TitleList)
            {
                item.IsSelected = false;
            }
        }

        /// <summary>
        /// Add a Preset
        /// </summary>
        public void Add()
        {
            this.addToQueue(this.TitleList.Where(c => c.IsSelected));
            this.Close();
        }

        /// <summary>
        /// Cancel adding a preset
        /// </summary>
        public void Cancel()
        {
            this.TitleList.Clear();
            this.Close();
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }

        /// <summary>
        /// The setup.
        /// </summary>
        /// <param name="scannedSource">
        /// The scanned source.
        /// </param>
        /// <param name="srcName">
        /// The src Name.
        /// </param>
        /// <param name="addAction">
        /// The add Action.
        /// </param>
        public void Setup(Source scannedSource, string srcName, Action<IEnumerable<SelectionTitle>> addAction)
        {
            this.TitleList.Clear();
            this.addToQueue = addAction;

            if (scannedSource != null)
            {
                IEnumerable<Title> titles = orderedByTitle
                                         ? scannedSource.Titles
                                         : scannedSource.Titles.OrderByDescending(o => o.Duration).ToList();

                foreach (Title item in titles)
                {
                    SelectionTitle title = new SelectionTitle(item, srcName) { IsSelected = true };
                    TitleList.Add(title);
                }
            }

            this.NotifyOfPropertyChange(() => this.IsAutoNamingEnabled);
            this.NotifyOfPropertyChange(() => this.IsAutomaticTrackSelectionEnabled);
        }
    }
}
