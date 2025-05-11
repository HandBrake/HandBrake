// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBehaviourRule.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using System.ComponentModel;

    using HandBrakeWPF.ViewModels;

    public class SubtitleBehaviourRule : PropertyChangedBase
    {
        public SubtitleBehaviourRule()
        {
            this.Tracks = new BindingList<SubtitleBehaviourTrack>();
        }

        public SubtitleBehaviourRule(SubtitleBehaviourRule rule)
        {
            this.UseSourceOrder = rule.UseSourceOrder;
            this.AutoloadExternal = AutoloadExternal;
            this.PassthruTrackNames = rule.PassthruTrackNames;
            this.Tracks = new BindingList<SubtitleBehaviourTrack>(rule.Tracks); // Shallow Copy
        }

        public BindingList<SubtitleBehaviourTrack> Tracks { get; set; }

        public bool UseSourceOrder { get; set; }

        public bool AutoloadExternal { get; set; }

        public bool PassthruTrackNames { get; set; }
    }
}
