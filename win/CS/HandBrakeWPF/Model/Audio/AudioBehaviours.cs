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
        /// <summary>
        /// The selected behaviour.
        /// </summary>
        private AudioBehaviourModes selectedBehaviour;

        /// <summary>
        /// The selected langauges.
        /// </summary>
        private BindingList<string> selectedLangauges;

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioBehaviours"/> class.
        /// </summary>
        public AudioBehaviours()
        {
            this.SelectedBehaviour = AudioBehaviourModes.None;
            this.SelectedLangauges = new BindingList<string>();
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
            this.SelectedLangauges = new BindingList<string>(behaviours.selectedLangauges.ToList());
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
        /// Gets or sets the selected langauges.
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
                           SelectedLangauges = new BindingList<string>()
                       };

            foreach (var item in this.SelectedLangauges)
            {
                cloned.SelectedLangauges.Add(item);
            }

            return cloned;
        }
    }
}
