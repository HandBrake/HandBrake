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

    using Caliburn.Micro;

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
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged { add { } remove { } }

        /// <summary>
        /// Gets or sets the meta data.
        /// </summary>
        public MetaData MetaData
        {
            get
            {
                return this.task.MetaData;
            }

            set
            {
                this.task.MetaData = value;
                this.NotifyOfPropertyChange(() => this.MetaData);
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
            return; // Disabled for now.
            this.task = encodeTask;
            this.task.MetaData = new MetaData(selectedTitle.Metadata);
            this.NotifyOfPropertyChange(() => this.MetaData);
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
            this.NotifyOfPropertyChange(() => this.MetaData);
        }

        public bool MatchesPreset(Preset preset)
        {
            return true;
        }
    }
}
