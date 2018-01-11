// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISoundService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Functions related to the Playing Sound.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    /// <summary>
    /// Functions related to the Playing Sound.
    /// </summary>
    public interface ISoundService
    {
        /// <summary>
        /// Plays the Done Sound File.
        /// </summary>
        /// <param name="soundfile">Done Sound</param>
        void PlayWhenDoneSound(string soundfile);
    }
}