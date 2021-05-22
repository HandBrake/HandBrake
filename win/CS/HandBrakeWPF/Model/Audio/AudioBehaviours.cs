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

    /// <summary>
    /// Audio Behaviours
    /// </summary>
    public class AudioBehaviours : PropertyChangedBase
    {
        private AudioBehaviourModes selectedBehaviour;
        private BindingList<string> selectedLanguages;
        private AudioTrackDefaultsMode trackDefaultBehaviour;

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviours"/> class.
        /// </summary>
        public AudioBehaviours()
        {
            this.SelectedBehaviour = AudioBehaviourModes.None;
            this.SelectedTrackDefaultBehaviour = AudioTrackDefaultsMode.FirstTrack;
            this.SelectedLanguages = new BindingList<string>();
            this.BehaviourTracks = new BindingList<AudioBehaviourTrack>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviours"/> class.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public AudioBehaviours(AudioBehaviours behaviours)
        {
            this.SelectedBehaviour = behaviours.SelectedBehaviour;
            this.SelectedTrackDefaultBehaviour = behaviours.SelectedTrackDefaultBehaviour;
            this.SelectedLanguages = new BindingList<string>(behaviours.selectedLanguages.ToList());
            this.BehaviourTracks = behaviours.BehaviourTracks;
        }

        /// <summary>
        /// Gets or sets the selected behaviour.
        /// </summary>
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

        /// <summary>
        /// Gets or sets the track default behaviour.
        /// </summary>
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

        /// <summary>
        /// Gets or sets the selected languages.
        /// </summary>
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

        /// <summary>
        /// The list of track templates we are going to use to generate audio tracks for a source.
        /// </summary>
        public BindingList<AudioBehaviourTrack> BehaviourTracks { get; set; }

        /// <summary>
        /// Clone this object
        /// </summary>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        public AudioBehaviours Clone()
        {
            AudioBehaviours cloned = new AudioBehaviours
            {
                SelectedBehaviour = this.selectedBehaviour,
                SelectedLanguages = new BindingList<string>(),
                SelectedTrackDefaultBehaviour = this.SelectedTrackDefaultBehaviour,
                BehaviourTracks = this.BehaviourTracks
            };

            foreach (var item in this.SelectedLanguages)
            {
                cloned.SelectedLanguages.Add(item);
            }

            return cloned;
        }
    }
}
