// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBehaviourRule.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using HandBrake.Interop.Interop.Interfaces.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.ViewModels;

    public class SubtitleBehaviourTrack : PropertyChangedBase
    {
        private bool isExport;

        private SubtitleBurnInBehaviourModes burnPassthruMode;

        public SubtitleBehaviourTrack()
        {
            DefaultMode = IsDefaultModes.No;
            TrackSelectionMode = SubtitleBehaviourModes.FirstMatch;
            ForcedMode = ForcedModes.No;
            BurnPassthruMode = SubtitleBurnInBehaviourModes.PassthruBurnDrop;
        }

        public SubtitleBehaviourTrack(SubtitleBehaviourTrack rule)
        {
            this.Language = rule.Language;
            this.TrackSelectionMode = rule.TrackSelectionMode;
            this.DefaultMode = rule.DefaultMode;
            this.ForcedMode = rule.ForcedMode;
            this.TrackName = rule.TrackName;
        }

        public Language Language { get; set; }

        public string LanguageCode => Language?.Code;

        public SubtitleBehaviourModes TrackSelectionMode { get; set; }

        public IsDefaultModes DefaultMode { get; set; }

        public SubtitleBurnInBehaviourModes BurnPassthruMode
        {
            get => this.burnPassthruMode;
            set
            { 
                this.burnPassthruMode = value;
                // this.IsExport = (value == BurnPassthruModes.Export);
            }
        }

        public ForcedModes ForcedMode { get; set; }

        /// <summary>
        ///  This is an override.
        /// </summary>
        public string TrackName { get; set; }


        /* Calculated Attributes */

        public bool IsExport
        {
            get => this.isExport;
            set
            {
                if (value == this.isExport) return;
                this.isExport = value;
                this.NotifyOfPropertyChange(() => this.IsExport);
            }
        }
    }
}
