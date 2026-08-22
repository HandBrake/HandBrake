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
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Encode.Model.Models.Filters;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModelItems;
    using HandBrakeWPF.ViewModels.Interfaces;
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Linq;

    using HandBrakeWPF.Commands;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    
    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        public FiltersViewModel(IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
            this.AvailableFilters = new BindingList<HandBrakeFilter>();
            this.AvailableFiltersMenu = new ObservableCollection<FilterMenuItem>();
            this.RemoveCommand = new SimpleRelayCommand<AudioVideoFilter>(this.Remove);


            foreach (HBFilter filter in HandBrakeFilterHelpers.GetHandBrakeFilters())
            {
                this.AvailableFilters.Add(new HandBrakeFilter(filter));
            }

            this.BuildAvailableFiltersMenu();
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        public SimpleRelayCommand<AudioVideoFilter> RemoveCommand { get; set; }

        public ListboxDeleteCommand DeleteCommand => new ListboxDeleteCommand();

        public EncodeTask CurrentTask { get; private set; }

        public BindingList<HandBrakeFilter> AvailableFilters { get; private set; }

        public ObservableCollection<FilterMenuItem> AvailableFiltersMenu { get; private set; }

        public ObservableCollection<AudioVideoFilter> VideoFilters
        {
            get
            {

                return this.CurrentTask.VideoFilters;
            }
        }

        public void AddTrack(HandBrakeFilter hbFilter)
        {
            bool alreadyExists = this.VideoFilters.Any(f => f.HandBrakeFilterChoice?.FilterId == hbFilter.FilterId);
            if (alreadyExists)
            {
                return;
            }

            if (!string.IsNullOrEmpty(hbFilter.Category))
            {
                var filtersToRemove = this.VideoFilters
                    .Where(f => f.HandBrakeFilterChoice?.Category == hbFilter.Category)
                    .ToList();

                foreach (var filter in filtersToRemove)
                {
                    this.VideoFilters.Remove(filter);
                }
            }

            AudioVideoFilter newFilter  = new AudioVideoFilter(hbFilter, null, null, null, ChangeTrigger);
            this.VideoFilters.Add(newFilter);

            ChangeTrigger();
        }

        private void ChangeTrigger()
        {
            this.TabStatusChanged?.Invoke(this, new TabStatusEventArgs(null));
        }

        public void Clear()
        {
            this.VideoFilters.Clear();
        }
        
        private void Remove(AudioVideoFilter obj)
        {
            this.VideoFilters.Remove(obj);
        }
        

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
            this.VideoFilters.Clear();

            foreach (AudioVideoFilter filter in preset.Task.VideoFilters)
            {
                this.VideoFilters.Add(new AudioVideoFilter(filter, ChangeTrigger)); // Decouple Copy from preset.
            }

            this.NotifyOfPropertyChange(() => this.VideoFilters);
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.VideoFilters);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (this.VideoFilters.Count != preset.Task.VideoFilters.Count)
            {
                return false;
            }

            return this.VideoFilters.All(f => preset.Task.VideoFilters.Contains(f)) 
                   && preset.Task.VideoFilters.All(f => this.VideoFilters.Contains(f));
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.VideoFilters);
        }

        private void BuildAvailableFiltersMenu()
        {
            this.AvailableFiltersMenu.Clear();

            // Track category nodes by category name
            Dictionary<string, FilterMenuItem> categoryNodes = new Dictionary<string, FilterMenuItem>();

            // Process all filters in original order
            foreach (HandBrakeFilter filter in this.AvailableFilters)
            {
                if (string.IsNullOrEmpty(filter.Category))
                {
                    // Add uncategorized filter directly to top level
                    this.AvailableFiltersMenu.Add(new FilterMenuItem
                    {
                        Header = filter.DisplayName,
                        Filter = filter
                    });
                }
                else
                {
                    // Add categorized filter under its category node
                    if (!categoryNodes.ContainsKey(filter.Category))
                    {
                        // Create new category node at current position
                        var categoryNode = new FilterMenuItem
                        {
                            Header = filter.Category,
                            Filter = null
                        };
                        categoryNodes[filter.Category] = categoryNode;
                        this.AvailableFiltersMenu.Add(categoryNode);
                    }

                    // Add filter as child of category node
                    categoryNodes[filter.Category].Children.Add(new FilterMenuItem
                    {
                        Header = filter.DisplayName,
                        Filter = filter
                    });
                }
            }
        }
    }
}