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
        /// Initializes a new instance of the <see cref="AudioTrack"/> class. 
        /// Audio Track instance
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Track = "Automatic";
            this.MixDown = "Automatic";
            this.SampleRate = "Auto";
            this.Bitrate = "Auto";
            this.DRC = "1";
        }

        /// <summary>
        /// Gets or sets Audio Track Name
        /// </summary>
        public string Track { get; set; }

        /// <summary>
        /// Gets or sets Audio Mixdown
        /// </summary>
        public string MixDown { get; set; }

        /// <summary>
        /// Gets or sets Audio Encoder
        /// </summary>
        public string Encoder { get; set; }

        /// <summary>
        /// Gets or sets Audio Bitrate
        /// </summary>
        public string Bitrate { get; set; }

        /// <summary>
        /// Gets or sets Audio SampleRate
        /// </summary>
        public string SampleRate { get; set; }

        /// <summary>
        /// Gets or sets Dynamic Range Compression
        /// </summary>
        public string DRC { get; set; }
    }
}