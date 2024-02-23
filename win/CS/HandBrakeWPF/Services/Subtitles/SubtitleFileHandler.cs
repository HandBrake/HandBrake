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

    public class SubtitleFileHandler : ISubtitleFileHandler
    {
        public List<SubtitleTrack> FindLocalFiles(string sourcePath)
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
                return GetInputSubtitles(foundFiles.ToArray());
            }

            return new List<SubtitleTrack>();
        }

        public List<SubtitleTrack> GetInputSubtitles(string[] filenames)
        {
            List < SubtitleTrack > foundSubtitles = new List<SubtitleTrack>();

            foreach (var srtFile in filenames)
            {
                if (!File.Exists(srtFile))
                {
                    continue;
                }

                string extension = Path.GetExtension(srtFile);

                SubtitleTrack track = new SubtitleTrack
                                      {
                                          SrtFileName = Path.GetFileNameWithoutExtension(srtFile),
                                          SrtOffset = 0,
                                          SrtCharCode = "UTF-8",
                                          SrtLang = HandBrakeLanguagesHelper.GetByName("English"),
                                          SubtitleType = extension.Contains("ass", StringComparison.InvariantCultureIgnoreCase) ? SubtitleType.IMPORTSSA : SubtitleType.IMPORTSRT,
                                          SrtPath = srtFile
                                      };
                foundSubtitles.Add(track);
            }

            return foundSubtitles;
        }
    }

}
