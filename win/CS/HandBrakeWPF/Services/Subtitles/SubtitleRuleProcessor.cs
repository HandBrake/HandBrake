// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleRuleProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Subtitles
{
    using System.Linq;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Services.Scan.Model;
    using System.Collections.Generic;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;

    using SubtitleTrack = Encode.Model.Models.SubtitleTrack;

    public class SubtitleRuleProcessor : ISubtitleRuleProcessor
    {
        // TODO: Add support for:  CC, Foreign Audio Scan

        private readonly ISubtitleFileHandler subtitleFileHandler;

        public SubtitleRuleProcessor(ISubtitleFileHandler fileHandler)
        {
            this.subtitleFileHandler = fileHandler;
        }

        public List<SubtitleTrack> GenerateTrackList(SubtitleBehaviourRule rules, List<Subtitle> sourceTracks, OutputFormat outputFormat, string sourcePath)
        {
            List<SubtitleTrack> outputTracks = new List<SubtitleTrack>();

            // First, get the set of tracks the rules allow for:
            List<Subtitle> soruceSubtitles = this.GetSelectedLanguagesTracks(rules, sourceTracks);

            // Now, lets run though the rules and setup the tracks
            foreach (SubtitleBehaviourTrack rule in rules.Tracks)
            {
                List<SubtitleTrack> tracks = GenerateTracksForRule(rules, rule, soruceSubtitles, outputFormat, outputTracks);
                outputTracks.AddRange(tracks);
            }

            if (rules.UseSourceOrder)
            {
                List<SubtitleTrack> orderedTrackList = outputTracks.OrderBy(s => s.SourceTrack.TrackNumber).ToList();
                outputTracks = orderedTrackList;
            }

            if (rules.AutoloadExternal)
            {
                List<SubtitleTrack> tracks = this.subtitleFileHandler.FindLocalFiles(sourcePath);
                outputTracks.AddRange(tracks);
            }

            return outputTracks;
        }

        private List<SubtitleTrack> GenerateTracksForRule(SubtitleBehaviourRule rules, SubtitleBehaviourTrack trackRule, List<Subtitle> soruceSubtitles, OutputFormat outputFormat, List<SubtitleTrack> outputTracks)
        {
            List<SubtitleTrack> tracks = new List<SubtitleTrack>();

            switch (trackRule.TrackSelectionMode)
            {
                case SubtitleBehaviourModes.FirstMatch:
                    Subtitle subtitle = soruceSubtitles.FirstOrDefault(s => s.LanguageCodeClean == trackRule.LanguageCode);
                    if (subtitle != null)
                    {
                        SubtitleTrack newTrack = GenerateTrack(rules, trackRule, subtitle, outputFormat, outputTracks);
                        if (newTrack != null)
                        {
                            tracks.Add(newTrack);
                        }
                    }
          
                    break;
                case SubtitleBehaviourModes.AllMatching:
                    List<Subtitle> subtitles = soruceSubtitles.Where(s => s.LanguageCodeClean == trackRule.LanguageCode).ToList();
                    foreach (Subtitle subtitleTrack in subtitles)
                    {
                        SubtitleTrack newTrack = GenerateTrack(rules, trackRule, subtitleTrack, outputFormat, outputTracks);
                        if (newTrack != null)
                        {
                            tracks.Add(newTrack);
                        }
                    }
                    break;
            }

            return tracks;
        }

        private SubtitleTrack GenerateTrack(SubtitleBehaviourRule rule, SubtitleBehaviourTrack trackRule, Subtitle sourceSubtitle, OutputFormat outputFormat, List<SubtitleTrack> outputTracks)
        {
            SubtitleTrack track = new SubtitleTrack();

            track.SourceTrack = sourceSubtitle;

            switch (trackRule.BurnPassthruMode)
            {
                case SubtitleBurnInBehaviourModes.BurnDrop:
                    if (sourceSubtitle.CanBurnIn && !this.ContainsBurnIn(outputTracks))
                    {
                        track.Burned = true;
                    }
                    else
                    {
                        return null; // Last Resort, Drop the track.
                    }

                    break;
                case SubtitleBurnInBehaviourModes.PassthruBurnDrop:
                    if (!sourceSubtitle.CanPassthru(outputFormat))
                    {
                        if (sourceSubtitle.CanBurnIn && !this.ContainsBurnIn(outputTracks))
                        {
                            track.Burned = true;
                        }
                        else
                        {
                            return null; // Last Resort, Drop the track.
                        }
                    }

                    break;
                case SubtitleBurnInBehaviourModes.PassthruDrop:
                    if (!sourceSubtitle.CanPassthru(outputFormat))
                    {
                        return null; // Drop Track.
                    }
                    break;
            }

            switch (trackRule.ForcedMode)
            {
                case ForcedModes.No:
                    break; // Nothing to do.
                case ForcedModes.Yes:
                    track.Forced = true;
                    break;
            }

            switch (trackRule.DefaultMode)
            {
                case IsDefaultModes.No:
                    break; // Nothing to do.
                case IsDefaultModes.Yes:
                    if (!this.ContainsDefault(outputTracks))
                    {
                        track.Default = true; // Only one track can be default.
                    }
                    break;
            }

            track.Name = trackRule.TrackName;

            if (!string.IsNullOrEmpty(sourceSubtitle.Name) && rule.PassthruTrackNames)
            {
                track.Name = sourceSubtitle.Name;
            }

            return track;
        }

        /// <summary>
        /// Gets a list of source tracks for the users selected languages.
        /// </summary>
        /// <returns>
        /// A list of source subtitle tracks.
        /// </returns>
        public List<Subtitle> GetSelectedLanguagesTracks(SubtitleBehaviourRule rules, IList<Subtitle> sourceTracks)
        {
            List<Language> languages = GetRequestedHumanLanguages(rules);

            // Translate to Iso Codes
            List<string> iso6392Codes = new List<string>();
            if (languages.Contains(HandBrakeLanguagesHelper.AnyLanguage))
            {
                iso6392Codes = HandBrakeLanguagesHelper.GetIsoCodes();
                iso6392Codes = HandBrakeLanguagesHelper.OrderIsoCodes(iso6392Codes, languages);
            }
            else
            {
                iso6392Codes = HandBrakeLanguagesHelper.GetLanguageCodes(languages);
            }


            // Get an ordered list of subtitles. 
            List<Subtitle> orderedSubtitles = new List<Subtitle>();

            if (rules.UseSourceOrder)
            {
                // Use the order of subtitles from the source.
                foreach (Subtitle subtitle in sourceTracks)
                {
                    if (iso6392Codes.Contains(subtitle.LanguageCodeClean))
                    {
                        orderedSubtitles.Add(subtitle);
                    }
                }
            }
            else
            {
                // Use the order preference chosen in the subtitle rules.
                foreach (string code in iso6392Codes)
                {
                    orderedSubtitles.AddRange(sourceTracks.Where(subtitle => subtitle.LanguageCodeClean == code));
                }
            }


            return orderedSubtitles;
        }

        public List<Language> GetRequestedHumanLanguages(SubtitleBehaviourRule rules)
        {
            List<Language> languages = new List<Language>();
            foreach (SubtitleBehaviourTrack subtitle in rules.Tracks)
            {
                if (!languages.Contains(subtitle.Language))
                {
                    languages.Add(subtitle.Language);
                }
            }
            return languages;
        }

        private bool ContainsBurnIn(List<SubtitleTrack> outputTracks)
        {
            if (outputTracks.Any(s => s.Burned))
            {
                return true;
            }

            return false;
        }

        private bool ContainsDefault(List<SubtitleTrack> outputTracks)
        {
            if (outputTracks.Any(s => s.Default))
            {
                return true;
            }

            return false;
        }
    }
}