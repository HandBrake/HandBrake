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

    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.ViewModels;

    public class SubtitleBehaviours : PropertyChangedBase
    {
        private SubtitleBehaviourModes selectedBehaviour;
        private BindingList<Language> selectedLanguages;
        private bool addForeignAudioScanTrack;
        private bool addClosedCaptions;
        private SubtitleBurnInBehaviourModes selectedBurnInBehaviour;

        public SubtitleBehaviours()
        {
            this.SelectedBehaviour = SubtitleBehaviourModes.None;
            this.SelectedBurnInBehaviour = SubtitleBurnInBehaviourModes.None;
            this.SelectedLanguages = new BindingList<Language>();
        }

        public SubtitleBehaviours(SubtitleBehaviours behaviours)
        {
            this.SelectedBehaviour = behaviours.selectedBehaviour;
            this.SelectedBurnInBehaviour = behaviours.selectedBurnInBehaviour;
            this.SelectedLanguages = new BindingList<Language>(behaviours.SelectedLanguages.ToList());
            this.AddClosedCaptions = behaviours.AddClosedCaptions;
            this.AddForeignAudioScanTrack = behaviours.AddForeignAudioScanTrack;
        }

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

        public BindingList<Language> SelectedLanguages
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
    }
}
