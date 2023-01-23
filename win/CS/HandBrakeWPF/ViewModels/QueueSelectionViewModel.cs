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

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Queue Selection View Model
    /// </summary>
    public class QueueSelectionViewModel : ViewModelBase, IQueueSelectionViewModel
    {
        private readonly IErrorService errorService;
        private readonly IUserSettingService userSettingService;
        private bool orderedByDuration;
        private bool orderedByTitle;
        private bool orderedByName;
        private Action<IEnumerable<SelectionTitle>> addToQueue;

        private string currentPreset;

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueSelectionViewModel"/> class. 
        /// </summary>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public QueueSelectionViewModel(IErrorService errorService, IUserSettingService userSettingService)
        {
            this.errorService = errorService;
            this.userSettingService = userSettingService;
            this.Title = Resources.QueueSelectionViewModel_AddToQueue;
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
        /// Gets or sets the current preset.
        /// </summary>
        public string CurrentPreset
        {
            get
            {
                return this.currentPreset;
            }
            set
            {
                if (value == this.currentPreset)
                {
                    return;
                }
                this.currentPreset = value;
                this.NotifyOfPropertyChange(() => this.CurrentPreset);
            }
        }

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
        /// Gets or sets a value indicating whether ordered by name.
        /// </summary>
        public bool OrderedByName
        {
            get
            {
                return this.orderedByName;
            }

            set
            {
                this.orderedByName = value;
                this.NotifyOfPropertyChange(() => OrderedByName);
            }
        }

        /// <summary>
        /// Gets a value indicating whether is auto naming enabled.
        /// </summary>
        public bool IsAutoNamingEnabled
        {
            get
            {
                return this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);
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
            this.OrderedByName = false;
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
            this.OrderedByName = false;
        }

        /// <summary>
        /// The order by name.
        /// </summary>
        public void OrderByName()
        {
            TitleList = new BindingList<SelectionTitle>(TitleList.OrderBy(o => o.Title.SourceName).ToList());
            this.NotifyOfPropertyChange(() => TitleList);
            this.OrderedByTitle = false;
            this.OrderedByDuration = false;
            this.OrderedByName = true;
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
        /// <param name="addAction">
        /// The add Action.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public void Setup(Source scannedSource, Action<IEnumerable<SelectionTitle>> addAction, Preset preset)
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
                    string srcName = item.DisplaySourceName ?? scannedSource.SourceName;
                    SelectionTitle title = new SelectionTitle(item, srcName) { IsSelected = true };
                    TitleList.Add(title);
                }
            }

            if (preset != null)
            {
                this.CurrentPreset = string.Format(Resources.QueueSelection_UsingPreset, preset.Name);
            }

            this.NotifyOfPropertyChange(() => this.IsAutoNamingEnabled);
        }
    }
}
