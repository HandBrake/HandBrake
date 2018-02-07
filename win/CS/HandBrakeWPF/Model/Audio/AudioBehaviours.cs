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
        private BindingList<string> selectedLangauges;
        private AudioTrackDefaultsMode trackDefaultBehaviour;

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviours"/> class.
        /// </summary>
        public AudioBehaviours()
        {
            this.SelectedBehaviour = AudioBehaviourModes.None;
            this.SelectedTrackDefaultBehaviour = AudioTrackDefaultsMode.FirstTrack;
            this.SelectedLangauges = new BindingList<string>();
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
            this.SelectedLangauges = new BindingList<string>(behaviours.selectedLangauges.ToList());
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
        public BindingList<string> SelectedLangauges
        {
            get
            {
                return this.selectedLangauges;
            }
            set
            {
                if (Equals(value, this.selectedLangauges))
                {
                    return;
                }
                this.selectedLangauges = value;
                this.NotifyOfPropertyChange(() => this.SelectedLangauges);
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
                SelectedLangauges = new BindingList<string>(),
                SelectedTrackDefaultBehaviour = this.SelectedTrackDefaultBehaviour,
                BehaviourTracks = this.BehaviourTracks
            };

            foreach (var item in this.SelectedLangauges)
            {
                cloned.SelectedLangauges.Add(item);
            }

            return cloned;
        }
    }
}
