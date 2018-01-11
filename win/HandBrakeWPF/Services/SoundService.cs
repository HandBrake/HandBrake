// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISoundService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Functions related to the Playing Sound.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System;
    using System.IO;
    using System.Windows.Media;
    using HandBrake.Utilities.Interfaces;

    public class SoundService : ISoundService
    {
        public void PlayWhenDoneSound(string soundfile)
        {
            if (!string.IsNullOrEmpty(soundfile) && File.Exists(soundfile))
            {
                var uri = new Uri(soundfile, UriKind.RelativeOrAbsolute);
                var player = new MediaPlayer();
                player.Open(uri);
                player.Play();
            }
        }
    }
}