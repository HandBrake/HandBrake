// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioBehaviours.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Audio Behaviours
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Audio
{
    using System.ComponentModel;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrakeWPF.Services.Encode.Model.Models;

    public class AudioBehaviours : PropertyChangedBase
    {
        private AudioBehaviourModes selectedBehaviour;
        private BindingList<string> selectedLanguages;
        private AudioTrackDefaultsMode trackDefaultBehaviour;
        
        public AudioBehaviours()
        {
            this.SelectedBehaviour = AudioBehaviourModes.None;
            this.SelectedTrackDefaultBehaviour = AudioTrackDefaultsMode.FirstTrack;
            this.SelectedLanguages = new BindingList<string>();
            this.BehaviourTracks = new BindingList<AudioBehaviourTrack>();
            this.AllowedPassthruOptions = new AllowedPassthru();
        }

        public AudioBehaviours(AudioBehaviours behaviours)
        {
            this.SelectedBehaviour = behaviours.SelectedBehaviour;
            this.SelectedTrackDefaultBehaviour = behaviours.SelectedTrackDefaultBehaviour;
            this.SelectedLanguages = new BindingList<string>(behaviours.selectedLanguages.ToList());
            this.BehaviourTracks = behaviours.BehaviourTracks;
            this.AllowedPassthruOptions = new AllowedPassthru(behaviours.AllowedPassthruOptions);
        }

        public AudioBehaviourModes SelectedBehaviour
        {
            get
            {
                return this.selectedBehaviour;
            }
            
            set
            {
                if (value == this.selectedBehaviour)
                {
                    return;
                }
                this.selectedBehaviour = value;
                this.NotifyOfPropertyChange(() => this.SelectedBehaviour);
            }
        }

        public AudioTrackDefaultsMode SelectedTrackDefaultBehaviour
        {
            get
            {
                return this.trackDefaultBehaviour;
            }
            set
            {
                if (value == this.trackDefaultBehaviour)
                {
                    return;
                }
                this.trackDefaultBehaviour = value;
                this.NotifyOfPropertyChange(() => this.SelectedTrackDefaultBehaviour);
            }
        }

        public BindingList<string> SelectedLanguages
        {
            get
            {
                return this.selectedLanguages;
            }
            set
            {
                if (Equals(value, this.selectedLanguages))
                {
                    return;
                }
                this.selectedLanguages = value;
                this.NotifyOfPropertyChange(() => this.SelectedLanguages);
            }
        }

        public BindingList<AudioBehaviourTrack> BehaviourTracks { get; set; }

        public AllowedPassthru AllowedPassthruOptions { get; set; }
    }
}
