/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// An Audio Track for the Audio Panel
    /// </summary>
    public class AudioTrack : ModelBase
    {
        #region Private Variables
        /// <summary>
        /// The gain value
        /// </summary>
        private int gain;

        /// <summary>
        ///  The DRC Value
        /// </summary>
        private double drc;
        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioTrack"/> class. 
        /// </summary>
        public AudioTrack()
        {
            // Default Values
            this.Encoder = AudioEncoder.Faac;
            this.MixDown = Mixdown.DolbyProLogicII;
            this.SampleRate = 48;
            this.Bitrate = 160;
            this.DRC = 1;
            this.SourceTrack = "Automatic";
        }

        /// <summary>
        /// Gets the Audio Track Name
        /// </summary>
        public int? Track
        {
            get
            {
                string[] tempSub = SourceTrack.Split(' ');
                int value;
                if (int.TryParse(tempSub[0], out value))
                {
                    return value;
                }
                return null;
            }
        }

        /// <summary>
        /// Gets the The UI display value for sample rate
        /// </summary>
        public string SampleRateDisplayValue
        {
            get
            {
                return SampleRate == 0 ? "Auto" : SampleRate.ToString();
            }
        }

        /// <summary>
        /// Gets the The UI display value for bit rate
        /// </summary>
        public string BitRateDisplayValue
        {
            get
            {
                return Bitrate == 0 ? "Auto" : Bitrate.ToString();
            }
        }

        /// <summary>
        /// Gets or sets the Source Track
        /// Used for display purposes only.
        /// </summary>
        public string SourceTrack { get; set; }

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
        public double DRC
        {
            get
            {
                return this.drc;
            }
            set
            {
                if (!object.Equals(value, this.drc))
                {
                    this.drc = value;
                    this.OnPropertyChanged("DRC");
                }
            }
        }

        /// <summary>
        /// Gets or sets the Gain for the audio track
        /// </summary>
        public int Gain
        {
            get
            {
                return this.gain;
            }
            set
            {
                if (!object.Equals(value, this.gain))
                {
                    this.gain = value;
                    this.OnPropertyChanged("Gain");
                }
            }
        }
    }
}