/*  AudioTrack.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    /// <summary>
    /// An Audio Track for the Audio Panel
    /// </summary>
    public class AudioTrack
    {
        /// <summary>
        /// Audio Track instance
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            Track = "Automatic";
            MixDown = "Automatic";
            SampleRate = "Auto";
            Bitrate = "Auto";
            DRC = "1";
        }

        /// <summary>
        /// Audio Track Name
        /// </summary>
        public string Track { get; set; }

        /// <summary>
        /// Audio Mixdown
        /// </summary>
        public string MixDown { get; set; }

        /// <summary>
        /// Audio Encoder
        /// </summary>
        public string Encoder { get; set; }

        /// <summary>
        /// Audio Bitrate
        /// </summary>
        public string Bitrate { get; set; }

        /// <summary>
        /// Audio SampleRate
        /// </summary>
        public string SampleRate { get; set; }

        /// <summary>
        /// Dynamic Range Compression
        /// </summary>
        public string DRC { get; set; }
    }
}
