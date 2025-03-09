// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaDataViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Meta Data Tab
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.Interop.Interop.Json.Shared;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The meta data view model.
    /// </summary>
    public class MetaDataViewModel : ViewModelBase, IMetaDataViewModel
    {
        private readonly IScan scanService;
        private EncodeTask task;
        private CoverArtImage coverArtImage;
        private Title currentTitle;
        private bool isNextAvailable;
        private bool isBackAvailable;
        private bool isCovertArtPreviewEnabled = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="MetaDataViewModel"/> class. 
        /// </summary>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public MetaDataViewModel(IWindowManager windowManager, IUserSettingService userSettingService, IScan scanService)
        {
            this.scanService = scanService;
            this.task = new EncodeTask();
            this.RemoveCommand = new SimpleRelayCommand<MetaDataValue>(this.Delete);
            this.CoverArtImage = new CoverArtImage(null);
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged { add { } remove { } }

        public SimpleRelayCommand<MetaDataValue> RemoveCommand { get; set; }

        public ObservableCollection<MetaDataValue> SourceMetadata
        {
            get => this.task.MetaData;
            set
            {
                if (Equals(value, this.task.MetaData))
                {
                    return;
                }

                this.task.MetaData = value;
                this.OnPropertyChanged();
            }
        }

        public ObservableCollection<CoverArt> SourceCoverArts
        {
            get => this.task.CoverArts;
            set
            {
                if (Equals(value, this.task.CoverArts))
                {
                    return;
                }

                this.task.CoverArts = value;
                this.OnPropertyChanged();
            }
        }

        public int CoverArtIndex { get; set; }

        public CoverArtImage CoverArtImage
        {
            get => this.coverArtImage;
            set
            {
                if (Equals(value, this.coverArtImage))
                {
                    return;
                }

                this.coverArtImage = value;
                this.NotifyOfPropertyChange(() => this.CoverArtImage);
            }
        }

        public bool IsNextAvailable
        {
            get => this.isNextAvailable;
            set
            {
                if (value == this.isNextAvailable)
                {
                    return;
                }

                this.isNextAvailable = value;
                this.NotifyOfPropertyChange(() => this.IsNextAvailable);
            }
        }

        public bool IsBackAvailable
        {
            get => this.isBackAvailable;
            set
            {
                if (value == this.isBackAvailable)
                {
                    return;
                }

                this.isBackAvailable = value;
                this.NotifyOfPropertyChange(() => this.IsBackAvailable);
            }
        }

        public void NextCoverArt()
        {
            if (this.SourceCoverArts.Count > (this.CoverArtIndex + 1))
            {
                this.CoverArtIndex += 1;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts[this.CoverArtIndex], this.currentTitle);
            }

            // Update the Next Button Status
            this.IsNextAvailable = this.SourceCoverArts.Count > (this.CoverArtIndex + 1);
        }

        public void PreviousCoverArt()
        {
            if (this.SourceCoverArts.Count > (this.CoverArtIndex + 1))
            {
                this.CoverArtIndex = this.CoverArtIndex - 1;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts[this.CoverArtIndex], this.currentTitle);
            }

            this.IsBackAvailable = this.CoverArtIndex > 1;
        }

        public void Add()
        {
            MetaDataValue value = new MetaDataValue("", "");
            value.IsNew = true;

            this.SourceMetadata.Add(value);
        }

        public void Delete(MetaDataValue row)
        {
            if (SourceMetadata.Contains(row) && row.IsNew)
            {
                this.SourceMetadata.Remove(row);
            }
        }

        /// <summary>
        /// Setup the window after a scan.
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="selectedTitle">
        /// The selected title.
        /// </param>
        /// <param name="currentPreset">
        /// The Current preset
        /// </param>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title selectedTitle, Preset currentPreset, EncodeTask encodeTask)
        {
            this.task = encodeTask;
            this.currentTitle = selectedTitle;

            this.SourceMetadata.Clear();
            if (selectedTitle.Metadata != null)
            {
                foreach (var item in selectedTitle.Metadata)
                {
                    this.SourceMetadata.Add(new MetaDataValue(item.Key, item.Value));
                }
            }

            this.SourceCoverArts.Clear();
            if (selectedTitle.CoverArts != null)
            {
                foreach (var item in selectedTitle.CoverArts)
                {
                    this.SourceCoverArts.Add(item);
                }
            }

            this.CoverArtImage = new CoverArtImage(null);
            if (this.SourceCoverArts != null && this.SourceCoverArts.Count > 0)
            {
                this.CoverArtIndex = this.SourceCoverArts.First().ID;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts.First(), selectedTitle);
            }

            // Update the Next Button Status
            this.IsNextAvailable = this.SourceCoverArts.Count > (this.CoverArtIndex + 1);
            this.IsBackAvailable = this.CoverArtIndex > 1;

            this.NotifyOfPropertyChange(() => this.SourceMetadata);
            this.NotifyOfPropertyChange(() => this.SourceCoverArts);
        }

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask encodeTask)
        {
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask encodeTask)
        {
            this.task = encodeTask;
            this.NotifyOfPropertyChange(() => this.SourceMetadata);
        }

        public bool MatchesPreset(Preset preset)
        {
            return true;
        }

        private CoverArtImage CreateCoverArtImage(CoverArt covertArt, Title selectedTitle)
        {
            if (isCovertArtPreviewEnabled)
            {
                CoverArtImage newImage = new CoverArtImage(covertArt);
                newImage.Image = this.scanService.GetCoverArt(covertArt, selectedTitle.TitleNumber);
                this.CoverArtImage = newImage;

                return newImage;
            }

            return new CoverArtImage(null);
        }
    }
}
