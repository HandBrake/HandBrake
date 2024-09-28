// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueAddRangeLimit.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

using HandBrakeWPF.Services.Encode.Model.Models;

namespace HandBrakeWPF.Model.Queue
{
    using HandBrakeWPF.ViewModels;

    public class QueueAddRangeLimit : PropertyChangedBase
    {
        public QueueAddRangeLimit()
        {
            this.SelectedStartPoint = 1;
            this.SelectedEndPoint = 99;
        }

        private PointToPointMode selectedPointToPoint;

        private bool isEnabled;

        private long selectedStartPoint;

        private long selectedEndPoint;

        public bool IsEnabled
        {
            get => this.isEnabled;
            set
            {
                if (value == this.isEnabled)
                {
                    return;
                }

                this.isEnabled = value;
                this.NotifyOfPropertyChange(() => this.IsEnabled);
            }
        }

        public long SelectedStartPoint
        {
            get => this.selectedStartPoint;
            set
            {
                if (value == this.selectedStartPoint)
                {
                    return;
                }

                this.selectedStartPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedStartPoint);

                if (this.SelectedStartPoint > this.SelectedEndPoint)
                {
                    this.SelectedEndPoint = this.SelectedStartPoint;
                }
            }
        }

        public long SelectedEndPoint
        {
            get => this.selectedEndPoint;
            set
            {
                if (value == this.selectedEndPoint)
                {
                    return;
                }

                this.selectedEndPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedEndPoint);

                if (this.SelectedStartPoint > this.SelectedEndPoint && this.SelectedPointToPoint == PointToPointMode.Chapters)
                {
                    this.SelectedStartPoint = this.SelectedEndPoint;
                }
            }
        }

        public PointToPointMode SelectedPointToPoint
        {
            get => this.selectedPointToPoint;
            set
            {
                if (value == this.selectedPointToPoint)
                {
                    return;
                }

                this.selectedPointToPoint = value;
                this.NotifyOfPropertyChange(() => this.SelectedPointToPoint);
                this.NotifyOfPropertyChange(() => this.ShowTextEntryForPointToPointMode);
                this.NotifyOfPropertyChange(() => this.IsTimespanRange);

                UpdateRange(this.selectedPointToPoint);
            }
        }

        public bool ShowTextEntryForPointToPointMode => this.SelectedPointToPoint != PointToPointMode.Chapters;

        public bool IsTimespanRange => this.SelectedPointToPoint == PointToPointMode.Seconds;

        private void UpdateRange(PointToPointMode mode)
        {
            switch (mode)
            {
                case PointToPointMode.Frames:
                    this.SelectedStartPoint = 0;
                    this.SelectedEndPoint = 864000; // 4 hours at 60fps, 8 hours at 30fps
                    break;
                case PointToPointMode.Seconds:
                    this.SelectedStartPoint = 0;
                    this.SelectedEndPoint = 14400;
                    break;
                case PointToPointMode.Chapters:
                    this.SelectedStartPoint = 0;
                    this.SelectedEndPoint = 98;
                    break;
            }
        }
    }
}
