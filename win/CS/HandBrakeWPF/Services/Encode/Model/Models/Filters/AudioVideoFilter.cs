// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents the use of a filter within HandBrake
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models.Filters
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;

    using HandBrakeWPF.ViewModelItems;
    using HandBrakeWPF.ViewModels;

    public class AudioVideoFilter : PropertyChangedBase
    {
        private readonly Action changeTrigger;

        public AudioVideoFilter(int hbFilter, bool isAudioFilter, FilterPreset preset = null, FilterTune tune = null, string custom = null, Action changeTrigger = null)
        {
            this.changeTrigger = changeTrigger;

            HBFilter filter = HandBrakeFilterHelpers.GetHandBrakeAudioFilters().FirstOrDefault(f => f.FilterId == hbFilter);
            if (filter != null)
            {
                this.HandBrakeFilterChoice = new HandBrakeFilter(filter);
            }

            this.Preset = preset;
            this.Tune = tune;
            this.CustomOptions = custom;

            if (preset == null)
            {
                this.Preset = this.AvailablePresets.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }

            if (tune == null)
            {
                this.Tune = this.AvailableTunes.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }
        }

        public AudioVideoFilter(int hbFilter, FilterPreset preset = null, FilterTune tune = null, string custom = null, Action changeTrigger = null)
        {
            this.changeTrigger = changeTrigger;

            HBFilter filter = HandBrakeFilterHelpers.GetHandBrakeFilters().FirstOrDefault(f => f.FilterId == hbFilter);
            if (filter != null)
            {
                this.HandBrakeFilterChoice = new HandBrakeFilter(filter);
            }

            this.Preset = preset;
            this.Tune = tune;
            this.CustomOptions = custom;

            if (preset == null)
            {
                this.Preset = this.AvailablePresets.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }

            if (tune == null)
            {
                this.Tune = this.AvailableTunes.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }
        }
        
        public AudioVideoFilter(HandBrakeFilter hbFilter, FilterPreset preset = null, FilterTune tune = null, string custom = null, Action changeTrigger = null)
        {
            this.changeTrigger = changeTrigger;
            this.HandBrakeFilterChoice = hbFilter;
            this.Preset = preset;
            this.Tune = tune;
            this.CustomOptions = custom;

            if (preset == null && AvailablePresets.Any())
            {
                this.Preset = this.AvailablePresets.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }

            if (tune == null && AvailableTunes.Any())
            {
                this.Tune = this.AvailableTunes.FirstOrDefault(s => s.Key != "custom" && s.Key != "off");
            }
        }

        public AudioVideoFilter(AudioVideoFilter filter, Action changeTrigger)
        {
            if (filter == null)
            {
                throw new ArgumentNullException(nameof(filter));
            }

            this.HandBrakeFilterChoice = filter.HandBrakeFilterChoice;
            this.Preset = filter.Preset != null ? new FilterPreset(filter.Preset.DisplayName, filter.Preset.Key) : null;
            this.Tune = filter.Tune != null ? new FilterTune(filter.Tune.DisplayName, filter.Tune.Key) : null;
            this.CustomOptions = filter.CustomOptions;
            this.changeTrigger = changeTrigger;
        }


        public int FilterId => HandBrakeFilterChoice?.FilterId ?? 0;
        
        public HandBrakeFilter HandBrakeFilterChoice { get; set; }

        public string DisplayName => this.HandBrakeFilterChoice?.DisplayName ?? "Unknown Filter";

        public IEnumerable<FilterPreset> AvailablePresets
        {
            get
            {
                foreach (HBPresetTune preset in HandBrakeFilterHelpers.GetFilterPresets(this.HandBrakeFilterChoice.FilterId))
                {
                    yield return new FilterPreset(preset);
                }
            }
        }

        public IEnumerable<FilterTune> AvailableTunes
        {
            get
            {
                foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterTunes(this.HandBrakeFilterChoice.FilterId))
                {
                    yield return new FilterTune(tune);
                }
            }
        }

        public bool CanSetPreset => this.AvailablePresets != null && this.AvailablePresets.Any();
        
        public bool CanSetTune => this.AvailableTunes != null && this.AvailableTunes.Any();

        public bool AllowsCustomOptions => this.Preset?.Key?.Contains("custom") ?? false;

        public FilterPreset Preset
        {
            get;
            set
            {
                if (Equals(value, field))
                {
                    return;
                }
                
                bool isCurrentlyCustom = this.Preset != null && this.Preset.Key == "custom";

                field = value;

                
                if (!isCurrentlyCustom && Preset?.Key == "custom" && this.FilterId != 0 && string.IsNullOrEmpty(this.CustomOptions))
                {
                    this.CustomOptions = HandBrakeFilterHelpers.GetDefaultCustomSettingsStr(this.FilterId);
                } 
                else if (isCurrentlyCustom && Preset?.Key != "custom")
                {
                    this.CustomOptions = string.Empty;
                }
                
                
                this.NotifyOfPropertyChange(() => this.Preset);
                this.NotifyOfPropertyChange(() => this.CanSetTune);
                this.NotifyOfPropertyChange(() => this.AllowsCustomOptions);
                this.changeTrigger?.Invoke();
            }
        }

        public FilterTune Tune
        {
            get;
            set
            {
                if (Equals(value, field))
                {
                    return;
                }

                field = value;
                this.NotifyOfPropertyChange(() => this.Tune);
                this.NotifyOfPropertyChange(() => this.CanSetPreset);
                this.NotifyOfPropertyChange(() => this.AllowsCustomOptions);
                this.changeTrigger?.Invoke();
            }
        }

        public string CustomOptions
        {
            get;
            set
            {
                if (value == field)
                {
                    return;
                }

                field = value;
                this.NotifyOfPropertyChange(() => this.CustomOptions);
                this.changeTrigger?.Invoke();
            }
        }

        protected bool Equals(AudioVideoFilter other)
        {
            return this.FilterId == other.FilterId 
                   && Equals(this.Preset, other.Preset) 
                   && Equals(this.Tune, other.Tune) 
                   && string.Equals(this.CustomOptions, other.CustomOptions);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((AudioVideoFilter)obj);
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(this.FilterId, this.Preset, this.Tune, this.CustomOptions);
        }
    }
}
