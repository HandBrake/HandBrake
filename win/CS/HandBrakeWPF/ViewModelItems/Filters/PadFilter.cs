// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PadFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.ComponentModel;
    using System.Linq;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

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

        public BindingList<PaddingMode> PaddingModes => new BindingList<PaddingMode>(EnumHelper<PaddingMode>.GetEnumList().ToList());

        public PaddingMode Mode
        {
            get => this.mode;
            set
            {
                this.mode = value;
                this.IsCustomPaddingEnabled = false;
                this.CurrentTask.Padding.Mode = value;  

                switch (value)
                {
                    case PaddingMode.Custom:
                        IsCustomPaddingEnabled = true;
                        break;

                    case PaddingMode.Fill:
                    case PaddingMode.FillHeight:
                    case PaddingMode.FillWidth:
                        IsCustomPaddingEnabled = false;
                        break;

                    case PaddingMode.None:
                    default:
                        this.Reset();
                        break;
                }

                ApplyPad(value);

                this.NotifyOfPropertyChange(() => this.Mode);
                this.NotifyOfPropertyChange(() => this.IsCustomPaddingEnabled);
                this.NotifyOfPropertyChange(() => this.IsCustomColourVisible);
                this.NotifyOfPropertyChange(() => this.IsPaddingEnabled);
            }
        }

        public bool IsCustomPaddingEnabled { get; set; }

        public bool IsPaddingEnabled => this.Mode != PaddingMode.None;

        public BindingList<PadColour> PaddingColours => new BindingList<PadColour>(EnumHelper<PadColour>.GetEnumList().ToList());

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

        public bool IsCustomColourVisible => this.IsPaddingEnabled && this.Colour == PadColour.Custom;

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

            if (this.CurrentTask.Padding == null)
            {
                this.Reset();
                return;
            }

            this.Mode = task.Padding.Mode;

            int padTop = task.Padding.Y;
            int padLeft = task.Padding.X;
            int padBottom = task.Padding.H - padTop;
            int padRight = task.Padding.W - padLeft;

            this.SetRotationValues(padTop, padBottom, padLeft, padRight);

            PadColour padColour = EnumHelper<PadColour>.GetValue(task.Padding.Color);
            if (padColour == PadColour.Black && task.Padding.Color != EnumHelper<PadColour>.GetShortName(PadColour.Black))
            {
                this.Colour = PadColour.Custom;
                this.CustomColour = task.Padding.Color;
            }
            else
            {
                this.Colour = PadColour.Black;
            }
        }

        public bool MatchesPreset(Preset preset)
        {
            if (!this.CurrentTask.Padding.Mode.Equals(preset.Task.Padding.Mode))
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
            if (!string.IsNullOrEmpty(this.customColour?.Trim()))
            {
                this.CurrentTask.Padding.Color = CustomColour?.Trim().ToLower();
            }
            else
            {
                this.CurrentTask.Padding.Color = EnumHelper<PadColour>.GetShortName(this.Colour);
            }
        }

        private void Reset()
        {
            this.mode = PaddingMode.None;
            this.Top = 0;
            this.Bottom = 0;
            this.Left = 0;
            this.Right = 0;
            this.Colour = PadColour.Black;

            this.NotifyOfPropertyChange(() => this.Mode);
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                return;
            }

            this.Reset();
            this.Mode = preset.Task.Padding.Mode;

            if (this.Mode == PaddingMode.Custom)
            {
                int padTop = preset.Task.Padding.Y;
                int padLeft = preset.Task.Padding.X;
                int padBottom = preset.Task.Padding.H - padTop;
                int padRight = preset.Task.Padding.W - padLeft;

                this.SetRotationValues(padTop, padBottom, padLeft, padRight);
            }
            
            PadColour padColour = EnumHelper<PadColour>.GetValue(preset.Task.Padding.Color);
            if (padColour == PadColour.Black && preset.Task.Padding.Color != EnumHelper<PadColour>.GetShortName(PadColour.Black))
            {
                this.Colour = PadColour.Custom;
                this.CustomColour = preset.Task.Padding.Color;
            }
            else
            {
                this.Colour = PadColour.Black;
            }
        }

        private void ApplyPad(PaddingMode mode)
        {
            bool fillwidth, fillheight;
            int[] pad = { 0, 0, 0, 0 };

            fillwidth = fillheight = mode == PaddingMode.Fill;
            fillheight = fillheight || (mode == PaddingMode.FillHeight);
            fillwidth = fillwidth || (mode == PaddingMode.FillWidth);

            if (mode == PaddingMode.Custom)
            {
                pad[0] = this.Top;
                pad[1] = this.Bottom;
                pad[2] = this.Left;
                pad[3] = this.Right;
            }

            if (fillheight && this.currentTask.MaxHeight > 0)
            {
                pad[0] = (this.currentTask.MaxHeight.Value - this.currentTask.Height.Value) / 2;
                pad[1] = this.currentTask.MaxHeight.Value - this.currentTask.Height.Value - pad[0];
            }
            if (fillwidth && this.currentTask.MaxWidth > 0)
            {
                pad[2] = (this.currentTask.MaxWidth.Value - this.currentTask.Width.Value) / 2;
                pad[3] = this.currentTask.MaxWidth.Value - this.currentTask.Width.Value - pad[2];
            }

            this.Top = pad[0];
            this.Bottom = pad[1];
            this.Left = pad[2];
            this.Right = pad[3];
        }
    }
}
