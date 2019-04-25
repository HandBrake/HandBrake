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

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Filters View Model
    /// </summary>
    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        public FiltersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
   
            this.SharpenFilter = new SharpenItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DenoiseFilter = new DenoiseItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DetelecineFilter = new DetelecineItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DeinterlaceFilter = new DeinterlaceFilterItem(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.DeblockFilter = new DeblockFilter(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.RotateFlipFilter = new RotateFlipFilter(this.CurrentTask, () => this.OnTabStatusChanged(null));
            this.GrayscaleFilter = new GrayscaleFilter(this.CurrentTask, () => this.OnTabStatusChanged(null));
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        public EncodeTask CurrentTask { get; private set; }

        public DenoiseItem DenoiseFilter { get; set; }

        public SharpenItem SharpenFilter { get; set; }

        public DetelecineItem DetelecineFilter { get; set; }

        public DeinterlaceFilterItem DeinterlaceFilter { get; set; }

        public DeblockFilter DeblockFilter { get; set; }

        public RotateFlipFilter RotateFlipFilter { get; set; }

        public GrayscaleFilter GrayscaleFilter { get; set; }
        
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            this.SharpenFilter.SetPreset(preset, task);
            this.DenoiseFilter.SetPreset(preset, task);
            this.DetelecineFilter.SetPreset(preset, task);
            this.DeinterlaceFilter.SetPreset(preset, task);
            this.DeblockFilter.SetPreset(preset, task);
            this.RotateFlipFilter.SetPreset(preset, task);
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;

            this.SharpenFilter.UpdateTask(task);
            this.DenoiseFilter.UpdateTask(task);
            this.DetelecineFilter.UpdateTask(task);
            this.DeinterlaceFilter.UpdateTask(task);
            this.DeblockFilter.UpdateTask(task);
            this.RotateFlipFilter.UpdateTask(task);
            this.GrayscaleFilter.UpdateTask(task);
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

            if (!this.DeblockFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.RotateFlipFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.GrayscaleFilter.MatchesPreset(preset))
            {
                return false;
            }

            return true;
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
            this.SharpenFilter.SetSource(source, title, preset, task);
            this.DenoiseFilter.SetSource(source, title, preset, task);
            this.DetelecineFilter.SetSource(source, title, preset, task);
            this.DeinterlaceFilter.SetSource(source, title, preset, task);
            this.DeblockFilter.SetSource(source, title, preset, task);
            this.RotateFlipFilter.SetSource(source, title, preset, task);
            this.GrayscaleFilter.SetSource(source, title, preset, task);
        }

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }
    }
}