// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FiltersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Filters View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;

    using DenoisePreset = HandBrakeWPF.Services.Encode.Model.Models.DenoisePreset;
    using DenoiseTune = HandBrakeWPF.Services.Encode.Model.Models.DenoiseTune;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Filters View Model
    /// </summary>
    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="FiltersViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public FiltersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
            this.DeblockValue = 4; // OFF
   
            this.SharpenFilter = new SharpenItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DenoiseFilter = new DenoiseItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DetelecineFilter = new DetelecineItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DeinterlaceFilter = new DeinterlaceFilterItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets CurrentTask.
        /// </summary>
        public EncodeTask CurrentTask { get; private set; }

        /// <summary>
        /// Gets DeblockText.
        /// </summary>
        public string DeblockText
        {
            get
            {
                return this.DeblockValue == 4 ? "Off" : this.DeblockValue.ToString(CultureInfo.InvariantCulture);
            }
        }

        /// <summary>
        /// Gets or sets DeblockValue.
        /// </summary>
        public int DeblockValue
        {
            get
            {
                return this.CurrentTask.Deblock;
            }

            set
            {
                this.CurrentTask.Deblock = value;
                this.NotifyOfPropertyChange(() => this.DeblockValue);
                this.NotifyOfPropertyChange(() => this.DeblockText);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether Grayscale.
        /// </summary>
        public bool Grayscale
        {
            get
            {
                return this.CurrentTask.Grayscale;
            }

            set
            {
                this.CurrentTask.Grayscale = value;
                this.NotifyOfPropertyChange(() => this.Grayscale);
                this.OnTabStatusChanged(null);
            }
        }


        public DenoiseItem DenoiseFilter { get; set; }

        public SharpenItem SharpenFilter { get; set; }

        public DetelecineItem DetelecineFilter { get; set; }

        public DeinterlaceFilterItem DeinterlaceFilter { get; set; }

        /// <summary>
        /// The rotation options.
        /// </summary>
        public BindingList<int> RotationOptions => new BindingList<int> { 0, 90, 180, 270 };

        /// <summary>
        /// Selected Rotation.
        /// </summary>
        public int SelectedRotation
        {
            get
            {
                return this.CurrentTask.Rotation;
            }

            set
            {
                this.CurrentTask.Rotation = value;
                this.NotifyOfPropertyChange(() => this.SelectedRotation);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Flip the Video
        /// </summary>
        public bool FlipVideo
        {
            get
            {
                return this.CurrentTask.FlipVideo;
            }

            set
            {
                this.CurrentTask.FlipVideo = value;
                this.NotifyOfPropertyChange(() => this.FlipVideo);
                this.OnTabStatusChanged(null);
            }
        }


        #endregion

        #region Implemented Interfaces

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
            this.CurrentTask = task;

            if (preset != null)
            {
                // Properties
               

                this.Grayscale = preset.Task.Grayscale;
                this.DeblockValue = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;
                
                this.SharpenFilter.SetPreset(preset, task);
                this.DenoiseFilter.SetPreset(preset, task);
                this.DetelecineFilter.SetPreset(preset, task);
                this.DeinterlaceFilter.SetPreset(preset, task);

                this.SelectedRotation = preset.Task.Rotation;
                this.FlipVideo = preset.Task.FlipVideo;
            }
            else
            {
                // Default everything to off
                this.Grayscale = false;
                this.DeblockValue = 0;

                this.SelectedRotation = 0;
                this.FlipVideo = false;
            }
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.Grayscale);
            this.NotifyOfPropertyChange(() => this.DeblockValue);
            

            this.NotifyOfPropertyChange(() => this.FlipVideo);
            this.NotifyOfPropertyChange(() => this.SelectedRotation);

            this.SharpenFilter.UpdateTask(task);
            this.DenoiseFilter.UpdateTask(task);
            this.DetelecineFilter.UpdateTask(task);
            this.DeinterlaceFilter.UpdateTask(task);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (!this.DenoiseFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.SharpenFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.DetelecineFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.DeinterlaceFilter.MatchesPreset(preset))
            {
                return false;
            }

            int presetDeblock = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;

            if (presetDeblock != this.DeblockValue)
            {
                return false;
            }

            if (preset.Task.Grayscale != this.Grayscale)
            {
                return false;
            }

            if (preset.Task.Rotation != this.SelectedRotation)
            {
                return false;
            }

            if (preset.Task.FlipVideo != this.FlipVideo)
            {
                return false;
            }

            return true;
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
            this.CurrentTask = task;
            this.SharpenFilter.SetSource(source, title, preset, task);
            this.DenoiseFilter.SetSource(source, title, preset, task);
            this.DetelecineFilter.SetSource(source, title, preset, task);
            this.DeinterlaceFilter.SetSource(source, title, preset, task);
        }

        #endregion

        #region Private Methods
        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }
        #endregion
    }
}