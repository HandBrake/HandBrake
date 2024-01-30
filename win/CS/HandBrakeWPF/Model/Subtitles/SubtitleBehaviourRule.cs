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
            if (rule == null)
            {
                return;
            }

            this.UseSourceOrder = rule.UseSourceOrder;
            this.AutoloadExternal = rule.AutoloadExternal;
            this.PassthruTrackNames = rule.PassthruTrackNames;
            this.Tracks = new BindingList<SubtitleBehaviourTrack>();
            foreach (var track in rule.Tracks) 
            {
                this.Tracks.Add(new SubtitleBehaviourTrack(track)); // Deep Copy
            }
        }

        public BindingList<SubtitleBehaviourTrack> Tracks { get; set; }

        public bool UseSourceOrder { get; set; }

        public bool AutoloadExternal { get; set; }

        public bool PassthruTrackNames { get; set; }

        protected bool Equals(SubtitleBehaviourRule other)
        {
            for (int i = 0; i < this.Tracks.Count; i++)
            {
                if ((other.Tracks.Count - 1) >= i && !this.Tracks[i].Equals(other.Tracks[i]))
                {
                    return false;
                }
            }
            
            return this.UseSourceOrder == other.UseSourceOrder && 
                   this.AutoloadExternal == other.AutoloadExternal && 
                   this.PassthruTrackNames == other.PassthruTrackNames;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return Equals((SubtitleBehaviourRule)obj);
        }
    }
}
