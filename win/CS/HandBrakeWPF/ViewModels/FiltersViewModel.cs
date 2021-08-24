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

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = Services.Encode.Model.EncodeTask;

    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        public FiltersViewModel(IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
   
            this.SharpenFilter = new SharpenItem(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.DenoiseFilter = new DenoiseItem(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.DetelecineFilter = new DetelecineItem(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.DeinterlaceFilter = new DeinterlaceFilterItem(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.DeblockFilter = new DeblockFilter(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.GrayscaleFilter = new GrayscaleFilter(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.ColourSpaceFilter = new ColourSpaceFilter(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
            this.ChromaSmoothFilter = new ChromaSmoothFilter(this.CurrentTask, () => this.OnTabStatusChanged(new TabStatusEventArgs(TabStatusEventType.FilterType)));
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        public EncodeTask CurrentTask { get; private set; }

        public DenoiseItem DenoiseFilter { get; set; }

        public SharpenItem SharpenFilter { get; set; }

        public DetelecineItem DetelecineFilter { get; set; }

        public DeinterlaceFilterItem DeinterlaceFilter { get; set; }

        public DeblockFilter DeblockFilter { get; set; }

        public ColourSpaceFilter ColourSpaceFilter { get; set; }

        public GrayscaleFilter GrayscaleFilter { get; set; }

        public ChromaSmoothFilter ChromaSmoothFilter { get; set; }
        
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            this.SharpenFilter.SetPreset(preset, task);
            this.DenoiseFilter.SetPreset(preset, task);
            this.DetelecineFilter.SetPreset(preset, task);
            this.DeinterlaceFilter.SetPreset(preset, task);
            this.DeblockFilter.SetPreset(preset, task);
            this.ColourSpaceFilter.SetPreset(preset, task);
            this.ChromaSmoothFilter.SetPreset(preset, task);
            this.GrayscaleFilter.SetPreset(preset, task);
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;

            this.SharpenFilter.UpdateTask(task);
            this.DenoiseFilter.UpdateTask(task);
            this.DetelecineFilter.UpdateTask(task);
            this.DeinterlaceFilter.UpdateTask(task);
            this.DeblockFilter.UpdateTask(task);
            this.GrayscaleFilter.UpdateTask(task);
            this.ColourSpaceFilter.UpdateTask(task);
            this.ChromaSmoothFilter.UpdateTask(task);
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

            if (!this.GrayscaleFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.ColourSpaceFilter.MatchesPreset(preset))
            {
                return false;
            }

            if (!this.ChromaSmoothFilter.MatchesPreset(preset))
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
            this.GrayscaleFilter.SetSource(source, title, preset, task);
            this.ColourSpaceFilter.SetSource(source, title, preset, task);
            this.ChromaSmoothFilter.SetSource(source, title, preset, task);
        }

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }
    }
}