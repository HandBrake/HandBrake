/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// An Audio Track for the Audio Panel
    /// </summary>
    public class AudioTrack
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioTrack"/> class. 
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Track = 1;
            this.MixDown = Mixdown.DolbyProLogicII;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 1;
        }

        /// <summary>
        /// Gets or sets Audio Track Name
        /// </summary>
        public int Track { get; set; }

        /// <summary>
        /// Gets or sets Audio Mixdown
        /// </summary>
        public Mixdown MixDown { get; set; }

        /// <summary>
        /// Gets or sets Audio Encoder
        /// </summary>
        public AudioEncoder Encoder { get; set; }

        /// <summary>
        /// Gets or sets Audio Bitrate
        /// </summary>
        public int Bitrate { get; set; }

        /// <summary>
        /// Gets or sets Audio SampleRate
        /// </summary>
        public double SampleRate { get; set; }

        /// <summary>
        /// Gets or sets Dynamic Range Compression
        /// </summary>
        public double DRC { get; set; }
    }
}