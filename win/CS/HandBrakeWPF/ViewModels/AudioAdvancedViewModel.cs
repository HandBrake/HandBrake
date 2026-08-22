// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioAdvancedViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
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
    using HandBrakeWPF.ViewModelItems;
    using HandBrakeWPF.ViewModels.Interfaces;
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Linq;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Views;

    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;

    public class AudioAdvancedViewModel : ViewModelBase, IAudioAdvancedViewModel
    {
        private readonly IWindowManager windowManager;

        public AudioAdvancedViewModel(IUserSettingService userSettingService, IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.AvailableFilters = new BindingList<HandBrakeFilter>();
            this.AvailableFiltersMenu = new ObservableCollection<FilterMenuItem>();
            this.RemoveCommand = new SimpleRelayCommand<AudioVideoFilter>(this.Remove);
            this.Title = Resources.AudioAdvancedView_Title;


            foreach (HBFilter filter in HandBrakeFilterHelpers.GetHandBrakeAudioFilters())
            {
                this.AvailableFilters.Add(new HandBrakeFilter(filter));
            }

            this.BuildAvailableFiltersMenu();
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        public SimpleRelayCommand<AudioVideoFilter> RemoveCommand { get; set; }


        public ListboxDeleteCommand DeleteCommand => new ListboxDeleteCommand();

        public AudioTrack AudioTrack { get; private set; }

        public BindingList<HandBrakeFilter> AvailableFilters { get; private set; }

        public ObservableCollection<FilterMenuItem> AvailableFiltersMenu { get; private set; }

        public ObservableCollection<AudioVideoFilter> AudioFilters
        {
            get
            {

                return this.AudioTrack.AudioFilters;
            }
        }

        public bool? ShowDialog()
        {
            return this.windowManager.ShowDialog<AudioAdvancedView>(this);
        }

        public void AddTrack(HandBrakeFilter hbFilter)
        {
            bool alreadyExists = this.AudioFilters.Any(f => f.HandBrakeFilterChoice?.FilterId == hbFilter.FilterId);
            if (alreadyExists)
            {
                return;
            }
            
            if (!string.IsNullOrEmpty(hbFilter.Category))
            {
                var filtersToRemove = this.AudioFilters
                    .Where(f => f.HandBrakeFilterChoice?.Category == hbFilter.Category)
                    .ToList();

                foreach (var filter in filtersToRemove)
                {
                    this.AudioFilters.Remove(filter);
                }
            }

            AudioVideoFilter newFilter  = new AudioVideoFilter(hbFilter, null, null, null, ChangeTrigger);
            this.AudioFilters.Add(newFilter);

            ChangeTrigger();
        }

        private void ChangeTrigger()
        {
            this.TabStatusChanged?.Invoke(this, new TabStatusEventArgs(null));
        }

        public void Clear()
        {
            this.AudioFilters.Clear();
        }
        
        private void Remove(AudioVideoFilter obj)
        {
            this.AudioFilters.Remove(obj);
        }
        

        //public void SetPreset(Preset preset, AudioTrack task)
        //{
        //    this.AudioTrack = task;
        //    this.AudioFilters.Clear();

        //    foreach (AudioVideoFilter filter in preset.Task.VideoFilters)
        //    {
        //        this.AudioFilters.Add(new AudioVideoFilter(filter, ChangeTrigger)); // Decouple Copy from preset.
        //    }

        //    this.NotifyOfPropertyChange(() => this.AudioFilters);
        //}

        public void UpdateTask(AudioTrack task)
        {
            this.AudioTrack = task;
            this.NotifyOfPropertyChange(() => this.AudioFilters);
        }

        //public bool MatchesPreset(Preset preset)
        //{
        //    if (this.AudioFilters.Count != preset.Task.VideoFilters.Count)
        //    {
        //        return false;
        //    }

        //    return this.AudioFilters.All(f => preset.Task.VideoFilters.Contains(f)) 
        //           && preset.Task.VideoFilters.All(f => this.AudioFilters.Contains(f));
        //}

        //public void SetSource(Source source, Title title, Preset preset, AudioTrack task)
        //{
        //    this.AudioTrack = task;
        //    this.NotifyOfPropertyChange(() => this.AudioFilters);
        //}

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