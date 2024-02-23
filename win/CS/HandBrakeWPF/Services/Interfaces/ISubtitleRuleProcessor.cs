// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISubtitleRuleProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

using HandBrakeWPF.Model.Subtitles;
using HandBrakeWPF.Services.Scan.Model;
using System.Collections.Generic;

namespace HandBrakeWPF.Services.Interfaces
{
    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Services.Encode.Model.Models;

    public interface ISubtitleRuleProcessor
    {
        List<SubtitleTrack> GenerateTrackList(SubtitleBehaviourRule rules, List<Subtitle> sourceTracks, OutputFormat outputFormat, string sourcePath);

        List<Subtitle> GetSelectedLanguagesTracks(SubtitleBehaviourRule rules, IList<Subtitle> sourceTracks);

        List<Language> GetRequestedHumanLanguages(SubtitleBehaviourRule rules);
    }
}
