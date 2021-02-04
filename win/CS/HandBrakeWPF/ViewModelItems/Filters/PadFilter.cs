// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DeblockFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DeblockFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Action = System.Action;

    public class PadFilter : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public PadFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask { get; private set; }

        public IEnumerable<PaddingMode> PaddingModes => EnumHelper<PaddingMode>.GetEnumList();

        public PaddingMode Mode { get; set; }

        public IEnumerable<string> PaddingColours { get; } = new List<string>() { "Black", "White", "Custom" };

        public int X
        {
            get
            {
                return this.CurrentTask.Padding.X;
            }

            set
            {
                this.CurrentTask.Padding.X = value;
                this.NotifyOfPropertyChange(() => this.X);
                this.triggerTabChanged();
            }
        }

        public int Y
        {
            get
            {
                return this.CurrentTask.Padding.Y;
            }

            set
            {
                this.CurrentTask.Padding.Y = value;
                this.NotifyOfPropertyChange(() => this.Y);
                this.triggerTabChanged();
            }
        }

        public string Colour
        {
            get
            {
                return this.CurrentTask.Padding.Color;
            }

            set
            {
                this.CurrentTask.Padding.Color = value;
                this.NotifyOfPropertyChange(() => this.Colour);
                this.triggerTabChanged();
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                return;
            }

            this.NotifyOfPropertyChange(() => this.X);
            this.NotifyOfPropertyChange(() => this.Y);
            this.NotifyOfPropertyChange(() => this.Colour);
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Padding.Enabled && Mode != PaddingMode.None)
            {
                return false;
            }

            if (preset.Task.Padding.X != this.X)
            {
                return false;
            }

            if (preset.Task.Padding.Y != this.Y)
            {
                return false;
            }

            if (preset.Task.Padding.Color != this.Colour)
            {
                return false;
            }

            return true;
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }
    }
}
