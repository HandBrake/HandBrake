// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RotateFlipFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the RotateFlipFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System;
    using System.ComponentModel;

    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

    public class RotateFlipFilter : PropertyChangedBase
    {
        private readonly Action<FlipRotationCommand> triggerTabChanged;

        public RotateFlipFilter(EncodeTask currentTask, Action<FlipRotationCommand> triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask { get; private set; }

        public BindingList<int> RotationOptions => new BindingList<int> { 0, 90, 180, 270 };

        public int SelectedRotation
        {
            get
            {
                return this.CurrentTask.Rotation;
            }

            set
            {
                int previousFlipValue = this.CurrentTask.FlipVideo ? 1 : 0;
                int previousRotation = this.CurrentTask.Rotation;
                this.CurrentTask.Rotation = value;
                this.NotifyOfPropertyChange(() => this.SelectedRotation);


                this.triggerTabChanged(new FlipRotationCommand(ChangedPictureField.Rotate, previousRotation, previousFlipValue));
            }
        }

        public bool FlipVideo
        {
            get
            {
                return this.CurrentTask.FlipVideo;
            }

            set
            {
                int previousFlipValue = this.CurrentTask.FlipVideo ? 1 : 0;
                int previousRotation = this.CurrentTask.Rotation;
                this.CurrentTask.FlipVideo = value;
                this.NotifyOfPropertyChange(() => this.FlipVideo);
                this.triggerTabChanged(new FlipRotationCommand(ChangedPictureField.Flip, previousRotation, previousFlipValue));
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedRotation = 0;
                this.FlipVideo = false;
                return;
            }

            this.SelectedRotation = preset.Task.Rotation;
            this.FlipVideo = preset.Task.FlipVideo;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.FlipVideo);
            this.NotifyOfPropertyChange(() => this.SelectedRotation);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Rotation != this.SelectedRotation)
            {
                return false;
            }

            if (preset.Task.FlipVideo != this.FlipVideo)
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
