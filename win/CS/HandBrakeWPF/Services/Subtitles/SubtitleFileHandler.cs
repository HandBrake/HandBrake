// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleFileHandler.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Subtitles
{
    using System;
    using System.Collections.Generic;
    using System.IO;

    using HandBrake.Interop.Interop;

    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;

    using Language = HandBrake.Interop.Interop.Interfaces.Model.Language;

    public class SubtitleFileHandler : ISubtitleFileHandler
    {

        public List<Subtitle> FindLocalFileSources(string sourcePath, bool guessLanguage)
        {
            List<string> foundFiles = new List<string>();

            string directory = Path.GetDirectoryName(sourcePath);
            string sourceFile = Path.GetFileNameWithoutExtension(sourcePath);
            if (Directory.Exists(directory))
            {
                foreach (string file in Directory.GetFiles(directory))
                {
                    // If we have an SRT/SSA/ASS file
                    if (file.EndsWith("srt", StringComparison.InvariantCultureIgnoreCase) ||
                        file.EndsWith("ssa", StringComparison.InvariantCultureIgnoreCase) ||
                        file.EndsWith("ass", StringComparison.InvariantCultureIgnoreCase))
                    {
                        // Check if it contains the source filename in it.
                        if (file.Contains(sourceFile, StringComparison.InvariantCultureIgnoreCase))
                        {
                            foundFiles.Add(file);
                        }
                    }
                }
            }

            if (foundFiles.Count > 0)
            {
                return GetInputSubtitleSources(foundFiles.ToArray(), guessLanguage);
            }

            return new List<Subtitle>();
        }

        public List<SubtitleTrack> FindLocalFiles(string sourcePath, bool guessLanguage)
        {
               List<string> foundFiles = new List<string>();

            string directory = Path.GetDirectoryName(sourcePath);
            string sourceFile = Path.GetFileNameWithoutExtension(sourcePath);
            if (Directory.Exists(directory))
            {
                foreach (string file in Directory.GetFiles(directory))
                {
                    // If we have an SRT/SSA/ASS file
                    if (file.EndsWith("srt", StringComparison.InvariantCultureIgnoreCase) ||
                        file.EndsWith("ssa", StringComparison.InvariantCultureIgnoreCase) ||
                        file.EndsWith("ass", StringComparison.InvariantCultureIgnoreCase))
                    {
                        // Check if it contains the source filename in it.
                        if (file.Contains(sourceFile, StringComparison.InvariantCultureIgnoreCase))
                        {
                            foundFiles.Add(file);
                        }
                    }
                }
            }

            if (foundFiles.Count > 0)
            {
                return GetInputSubtitles(foundFiles.ToArray(), guessLanguage);
            }

            return new List<SubtitleTrack>();
        }

        public List<SubtitleTrack> GetInputSubtitles(string[] filenames, bool guessLanguage)
        {
            List < SubtitleTrack > foundSubtitles = new List<SubtitleTrack>();

            foreach (var srtFile in filenames)
            {
                if (!File.Exists(srtFile))
                {
                    continue;
                }

                string extension = Path.GetExtension(srtFile);
                Language foundLanguage;
                foundLanguage = guessLanguage ? this.GuessLanguage(srtFile) : HandBrakeLanguagesHelper.GetByName("English");

                SubtitleTrack track = new SubtitleTrack
                                      {
                                          SrtFileName = Path.GetFileNameWithoutExtension(srtFile),
                                          SrtOffset = 0,
                                          SrtCharCode = "UTF-8",
                                          SrtLang = foundLanguage,
                                          SubtitleType = extension.Contains("ass", StringComparison.InvariantCultureIgnoreCase) ? SubtitleType.IMPORTSSA : SubtitleType.IMPORTSRT,
                                          SrtPath = srtFile
                                      };
                foundSubtitles.Add(track);
            }

            return foundSubtitles;
        }


        public List<Subtitle> GetInputSubtitleSources(string[] filenames, bool guessLanguage)
        {
            List<Subtitle> foundSubtitles = new List<Subtitle>();

            foreach (var srtFile in filenames)
            {
                if (!File.Exists(srtFile))
                {
                    continue;
                }

                string extension = Path.GetExtension(srtFile);
                Language foundLanguage = guessLanguage ? this.GuessLanguage(srtFile) : HandBrakeLanguagesHelper.GetByName("English");

                Subtitle track = new Subtitle();
                track.CanBurnIn = true;
                track.CanForce = false;
                track.IsFakeForeignAudioScanTrack = false;
                track.SubtitleType = extension.Contains("ass", StringComparison.InvariantCultureIgnoreCase)
                    ? SubtitleType.IMPORTSSA
                    : SubtitleType.IMPORTSRT;
                if (foundLanguage != null)
                {
                    track.Language = foundLanguage.DisplayNative;
                    track.LanguageCode = foundLanguage.Code;
                }

                track.ExternalSubtitleFilePath = srtFile;
                track.Name = Path.GetFileNameWithoutExtension(srtFile);
                
                foundSubtitles.Add(track);
            }

            return foundSubtitles;
        }
        private Language GuessLanguage(string filename)
        {
            if (string.IsNullOrEmpty(filename))
            {
                return HandBrakeLanguagesHelper.GetByName("English"); // Default
            }

            filename = Path.GetFileNameWithoutExtension(filename);

            foreach (Language language in HandBrakeLanguagesHelper.AllLanguages)
            {
                if (language.Code == "und" || language.Code == "any")
                {
                    continue;
                }
                
                if (!string.IsNullOrEmpty(language.EnglishName)  && filename.Contains(language.EnglishName, StringComparison.InvariantCultureIgnoreCase))
                {
                    return language;
                }

                if (!string.IsNullOrEmpty(language.NativeName) && filename.Contains(language.NativeName, StringComparison.InvariantCultureIgnoreCase))
                {
                    return language;
                }

                if (!string.IsNullOrEmpty(language.Code) && filename.Contains("." + language.Code + ".", StringComparison.InvariantCultureIgnoreCase))
                {
                    // Note, since language codes are 3 letters, we could easily hit a mis-match
                    // It's common for filenames to be something along the lines of .eng.srt  when using language codes.
                    // So for now, we'll stick with this to limit false positives. 
                    return language;
                }
            }

            return HandBrakeLanguagesHelper.GetByName("English"); // Default
        }
    }

}
