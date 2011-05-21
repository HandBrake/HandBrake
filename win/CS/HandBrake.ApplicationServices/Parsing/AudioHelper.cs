/*  AudioHelper.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
{
    using System.Collections.Generic;
    using System.IO;
    using System.Text.RegularExpressions;

    /// <summary>
    /// An Audio Helper Class
    /// </summary>
    public class AudioHelper
    {
        /// <summary>
        /// Gets A Dummy Not Found Track
        /// </summary>
        public static Audio NoneFound
        {
            get
            {
                return new Audio { Description = "None Found" };
            }
        }

        /// <summary>
        /// Parse the CLI input to an Audio Track object
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An Audio Track obkect
        /// </returns>
        public static Audio Parse(StringReader output)
        {
            string audioTrack = output.ReadLine();
            Match m = Regex.Match(audioTrack, @"^    \+ ([0-9]*), ([A-Za-z0-9,\s]*) \((.*)\) \((.*)\)");
            Match track = Regex.Match(audioTrack, @"^    \+ ([0-9]*), ([A-Za-z0-9,\s]*) \((.*)\)"); // ID and Language
            Match iso639_2 = Regex.Match(audioTrack, @"iso639-2: ([a-zA-Z]*)\)");
            Match samplerate = Regex.Match(audioTrack, @"([0-9]*)Hz");
            Match bitrate = Regex.Match(audioTrack, @"([0-9]*)bps");

            string subformat = m.Groups[4].Value.Trim().Contains("iso639") ? null : m.Groups[4].Value;
            string samplerateVal = samplerate.Success ? samplerate.Groups[0].Value.Replace("Hz", string.Empty).Trim() : "0";
            string bitrateVal = bitrate.Success ? bitrate.Groups[0].Value.Replace("bps", string.Empty).Trim() : "0";

            if (track.Success)
            {
                Audio thisTrack = new Audio
                {
                    TrackNumber = int.Parse(track.Groups[1].Value.Trim()),
                    Language = track.Groups[2].Value,
                    Format = m.Groups[3].Value,
                    Description = subformat,
                    SampleRate = int.Parse(samplerateVal),
                    Bitrate = int.Parse(bitrateVal),
                    LanguageCode = iso639_2.Value.Replace("iso639-2: ", string.Empty).Replace(")", string.Empty)
                };
                return thisTrack;
            }

            return null;
        }

        /// <summary>
        /// Pase a list of audio tracks
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An array of audio tracks
        /// </returns>
        public static Audio[] ParseList(StringReader output)
        {
            var tracks = new List<Audio>();
            while (true)
            {
                Audio thisTrack = Parse(output);
                if (thisTrack != null)
                    tracks.Add(thisTrack);
                else
                    break;
            }
            return tracks.ToArray();
        }
    }
}
