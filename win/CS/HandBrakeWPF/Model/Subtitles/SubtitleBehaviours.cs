// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBehaviours.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class to track the behaviours of audio track selection
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using System.ComponentModel;
    using System.Linq;

    using Caliburn.Micro;

    /// <summary>
    ///  A class to track the behaviours of audio track selection
    /// </summary>
    public class SubtitleBehaviours : PropertyChangedBase
    {
        private SubtitleBehaviourModes selectedBehaviour;
        private BindingList<string> selectedLangauges;
        private bool addForeignAudioScanTrack;
        private bool addClosedCaptions;
        private SubtitleBurnInBehaviourModes selectedBurnInBehaviour;

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitleBehaviours"/> class.
        /// </summary>
        public SubtitleBehaviours()
        {
            this.SelectedBehaviour = SubtitleBehaviourModes.None;
            this.SelectedBurnInBehaviour = SubtitleBurnInBehaviourModes.None;
            this.SelectedLangauges = new BindingList<string>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitleBehaviours"/> class.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public SubtitleBehaviours(SubtitleBehaviours behaviours)
        {
            this.SelectedBehaviour = behaviours.selectedBehaviour;
            this.SelectedBurnInBehaviour = behaviours.selectedBurnInBehaviour;
            this.SelectedLangauges = new BindingList<string>(behaviours.SelectedLangauges.ToList());
            this.AddClosedCaptions = behaviours.AddClosedCaptions;
            this.AddForeignAudioScanTrack = behaviours.AddForeignAudioScanTrack;
        }

        /// <summary>
        /// Gets or sets the selected behaviour.
        /// </summary>
        public SubtitleBehaviourModes SelectedBehaviour
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
        /// Gets or sets the selected burn in behaviour.
        /// </summary>
        public SubtitleBurnInBehaviourModes SelectedBurnInBehaviour
        {
            get
            {
                return this.selectedBurnInBehaviour;
            }
            set
            {
                if (value == this.selectedBurnInBehaviour)
                {
                    return;
                }
                this.selectedBurnInBehaviour = value;
                this.NotifyOfPropertyChange(() => this.SelectedBurnInBehaviour);
            }
        }

        /// <summary>
        /// Gets or sets the selected langages.
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
        /// Gets or sets a value indicating whether add foreign audio scan track.
        /// </summary>
        public bool AddForeignAudioScanTrack
        {
            get
            {
                return this.addForeignAudioScanTrack;
            }
            set
            {
                if (value.Equals(this.addForeignAudioScanTrack))
                {
                    return;
                }
                this.addForeignAudioScanTrack = value;
                this.NotifyOfPropertyChange(() => this.AddForeignAudioScanTrack);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether add closed captions.
        /// </summary>
        public bool AddClosedCaptions
        {
            get
            {
                return this.addClosedCaptions;
            }
            set
            {
                if (value.Equals(this.addClosedCaptions))
                {
                    return;
                }
                this.addClosedCaptions = value;
                this.NotifyOfPropertyChange(() => this.AddClosedCaptions);
            }
        }

        /// <summary>
        /// Clone this object
        /// </summary>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        public SubtitleBehaviours Clone()
        {
            SubtitleBehaviours cloned = new SubtitleBehaviours
            {
                SelectedBehaviour = this.selectedBehaviour, 
                SelectedBurnInBehaviour = this.selectedBurnInBehaviour, 
                SelectedLangauges = new BindingList<string>(), 
                AddClosedCaptions = this.addClosedCaptions, 
                AddForeignAudioScanTrack = this.addForeignAudioScanTrack, 
            };

            foreach (var item in this.SelectedLangauges)
            {
                cloned.SelectedLangauges.Add(item);
            }

            return cloned;
        }
    }
}
