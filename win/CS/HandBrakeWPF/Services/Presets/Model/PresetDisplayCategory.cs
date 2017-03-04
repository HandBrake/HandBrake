﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetDisplayCategory.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Preset Category encoding with.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Model
{
    using System.ComponentModel;
    using Caliburn.Micro;
    using HandBrakeWPF.Services.Presets.Interfaces;

    public class PresetDisplayCategory : PropertyChangedBase, IPresetObject
    {
        private bool isSelected;
        private bool isExpanded;

        public PresetDisplayCategory(string category, BindingList<Preset> presets)
        {
            this.Category = category;
            this.Presets = presets;
        }

        public string Category { get; private set; }
        public BindingList<Preset> Presets { get; private set; }

        public string Description => this.Category;

        public bool IsExpanded
        {
            get
            {
                return this.isExpanded;
            }
            set
            {
                if (value == this.isExpanded) return;
                this.isExpanded = value;
                this.NotifyOfPropertyChange(() => this.IsExpanded);
            }
        }

        public bool IsSelected
        {
            get
            {
                return this.isSelected;
            }
            set
            {
                if (value == this.isSelected) return;
                this.isSelected = value;
                this.NotifyOfPropertyChange(() => this.IsSelected);
            }
        }

        protected bool Equals(PresetDisplayCategory other)
        {
            return string.Equals(this.Category, other.Category);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((PresetDisplayCategory)obj);
        }

        public override int GetHashCode()
        {
            return (this.Category != null ? this.Category.GetHashCode() : 0);
        }
    }
}
