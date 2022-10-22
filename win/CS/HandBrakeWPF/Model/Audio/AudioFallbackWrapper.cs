// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioFallbackWrapper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrakeWPF.Model.Audio
{
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.ViewModels;

    public class AudioFallbackWrapper : PropertyChangedBase
    {
        private bool isEnabled;

        public AudioFallbackWrapper(HBAudioEncoder encoder)
        {
            this.Encoder = encoder;
        }

        public HBAudioEncoder Encoder { get; }

        public bool IsEnabled
        {
            get => this.isEnabled;
            set
            {
                if (value == this.isEnabled) return;
                this.isEnabled = value;
                this.NotifyOfPropertyChange(() => this.IsEnabled);
            }
        }
    }
}
