// --------------------------------------------------------------------------------------------------------------------
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
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.ViewModels;

    public class PresetDisplayCategory : PropertyChangedBase, IPresetObject
    {
        private bool isSelected;
        private bool isExpanded;

        public PresetDisplayCategory(string category, bool isBuildIn, BindingList<Preset> presets)
        {
            this.IsBuiltIn = isBuildIn;
            this.Category = category;
            this.Presets = presets;
        }

        public string Category { get; private set; }

        public BindingList<Preset> Presets { get; private set; }

        public string Description => this.Category;

        public bool IsBuiltIn { get; }

        public bool IsExpanded
        {
            get => this.isExpanded;
            set
            {
                if (value == this.isExpanded)
                {
                    return;
                }

                this.isExpanded = value;
                this.NotifyOfPropertyChange(() => this.IsExpanded);
            }
        }

        public bool IsPresetDisabled => false;

        public bool IsSelected
        {
            get => this.isSelected;

            set
            {
                if (value == this.isSelected) return;
                this.isSelected = value;
                this.NotifyOfPropertyChange(() => this.IsSelected);
            }
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
            return this.Category != null ? this.Category.GetHashCode() : 0;
        }

        protected bool Equals(PresetDisplayCategory other)
        {
            return string.Equals(this.Category, other.Category);
        }
    }
}
