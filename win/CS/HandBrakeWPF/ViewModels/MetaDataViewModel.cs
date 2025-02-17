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
    using System.Windows.Forms;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The meta data view model.
    /// </summary>
    public class MetaDataViewModel : ViewModelBase, IMetaDataViewModel
    {
        private EncodeTask task;

        /// <summary>
        /// Initializes a new instance of the <see cref="MetaDataViewModel"/> class. 
        /// </summary>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public MetaDataViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.task = new EncodeTask();
            this.RemoveCommand = new SimpleRelayCommand<MetaDataValue>(this.Delete);
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

            this.SourceMetadata.Clear();
            if (selectedTitle.Metadata != null)
            {
                foreach (var item in selectedTitle.Metadata)
                {
                    this.SourceMetadata.Add(new MetaDataValue(item.Key, item.Value));
                }
            }

            this.NotifyOfPropertyChange(() => this.SourceMetadata);
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
    }
}
