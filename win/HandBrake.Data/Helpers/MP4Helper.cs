// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MP4Helper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the MP4Helper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Linq;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;

    /// <summary>
    /// The MP4 Format helper class
    /// </summary>
    public class MP4Helper
    {
        /// <summary>
        /// Gets a value indicating whether M4v extension is required.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/> to indicate if this task requires m4v extension
        /// </returns>
        public static bool RequiresM4v(EncodeTask task)
        {
            if (task.OutputFormat == OutputFormat.Mp4)
            {
                bool audio =
                    task.AudioTracks.Any(
                        item =>
                        item.Encoder == AudioEncoder.Ac3Passthrough || item.Encoder == AudioEncoder.Ac3
                        || item.Encoder == AudioEncoder.DtsPassthrough || item.Encoder == AudioEncoder.Passthrough);

                bool subtitles = task.SubtitleTracks.Any(track => track.SubtitleType != SubtitleType.VobSub);

                return audio || subtitles;
            }

            return false;
        }
    }
}
