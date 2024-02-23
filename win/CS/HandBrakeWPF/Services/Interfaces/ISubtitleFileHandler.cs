// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISubtitleFileHandler.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the interface for the subtitle file handler that manages srt/ssa files
// </summary>
// --------------------------------------------------------------------------------------------------------------------



namespace HandBrakeWPF.Services.Interfaces
{
    using HandBrakeWPF.Services.Encode.Model.Models;

    using System.Collections.Generic;

    public interface ISubtitleFileHandler
    {
        List<SubtitleTrack> GetInputSubtitles(string[] filenames);

        List<SubtitleTrack> FindLocalFiles(string sourcePath);
    }
}
