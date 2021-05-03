// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PadFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Collections.Generic;

    using Caliburn.Micro;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Action = System.Action;

    public class PadFilter : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;
        private int top, bottom, left, right;
        private EncodeTask currentTask;
        private PaddingMode mode;
        private PadColour colour;

        private string customColour;

        public PadFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask
        {
            get => this.currentTask;
            private set => this.currentTask = value;
        }

        public IEnumerable<PaddingMode> PaddingModes => EnumHelper<PaddingMode>.GetEnumList();

        public PaddingMode Mode
        {
            get => this.mode;
            set
            {
                this.mode = value;
                this.IsCustomPaddingEnabled = false;

                switch (value)
                {
                    case PaddingMode.Custom:
                        this.currentTask.Padding.Enabled = true;
                        IsCustomPaddingEnabled = true;
                        break;
                    case PaddingMode.None:
                    default:
                        this.currentTask.Padding.Enabled = false;
                        break;
                }

                this.NotifyOfPropertyChange(() => this.IsCustomPaddingEnabled);
                this.NotifyOfPropertyChange(() => this.IsCustomColourVisible);
            }
        }

        public bool IsCustomPaddingEnabled { get; set; }

        public IEnumerable<PadColour> PaddingColours => EnumHelper<PadColour>.GetEnumList();

        public PadColour Colour
        {
            get
            {
                return colour;
            }

            set
            {
                this.colour = value;
                this.SetColour();
                this.NotifyOfPropertyChange(() => this.Colour);
                this.NotifyOfPropertyChange(() => this.IsCustomColourVisible);
                this.triggerTabChanged();
            }
        }

        public string CustomColour
        {
            get => this.customColour;
            set
            {
                this.customColour = value;
                this.SetColour();
                this.NotifyOfPropertyChange(() => this.CustomColour);
                this.triggerTabChanged();
            }
        }

        public bool IsCustomColourVisible => this.IsCustomPaddingEnabled && this.Colour == PadColour.Custom;

        public int Top
        {
            get => this.top;

            set
            {
                this.top = value;
                this.CalculatePosition();
                this.NotifyOfPropertyChange(() => this.Top);
                this.triggerTabChanged();
            }
        }

        public int Bottom
        {
            get => this.bottom;

            set
            {
                this.bottom = value;
                this.CalculatePosition();
                this.NotifyOfPropertyChange(() => this.Bottom);
                this.triggerTabChanged();
            }
        }
        
        public int Left
        {
            get => this.left;

            set
            {
                this.left = value;
                this.CalculatePosition();
                this.NotifyOfPropertyChange(() => this.Left);
                this.triggerTabChanged();
            }
        }
        
        public int Right
        {
            get => this.right;

            set
            {
                this.right = value;
                this.CalculatePosition();
                this.NotifyOfPropertyChange(() => this.Right);
                this.triggerTabChanged();
            }
        }

        public void SetRotationValues(int t, int b, int l, int r)
        {
            this.top = t;
            this.bottom = b;
            this.left = l;
            this.right = r;

            this.NotifyOfPropertyChange(() => this.Top);
            this.NotifyOfPropertyChange(() => this.Bottom);
            this.NotifyOfPropertyChange(() => this.Left);
            this.NotifyOfPropertyChange(() => this.Right);

            this.CalculatePosition();
        }
        
        private void CalculatePosition()
        {
            // Figure the X,Y coordinate 
            this.CurrentTask.Padding.X = this.Left;
            this.CurrentTask.Padding.Y = this.Top;
            
            // Calculate the total padding
            this.currentTask.Padding.W = this.Left + this.Right;
            this.currentTask.Padding.H = this.Top + this.Bottom;
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

            return true;
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }

        private void SetColour()
        {
            switch (this.Colour)
            {
                case PadColour.Black:
                    this.CurrentTask.Padding.Color = "black";
                    break;
                case PadColour.White:
                    this.CurrentTask.Padding.Color = "white";
                    break;
                case PadColour.Custom:
                    this.CurrentTask.Padding.Color = CustomColour?.Trim().ToLower();
                    break;
                default:
                    this.CurrentTask.Padding.Color = "black";
                    break;
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                return;
            }

            this.NotifyOfPropertyChange(() => this.Colour);
        }
    }
}
