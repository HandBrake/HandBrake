/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System;
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.Interop.Model.Encoding;

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

        /// <summary>
        /// The Scanned Audio Track
        /// </summary>
        [NonSerialized]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private Audio scannedTrack;
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
            this.DRC = 0;
            this.ScannedTrack = new Audio();
        }

        /// <summary>
        /// Gets the Audio Track Name
        /// </summary>
        public int? Track
        {
            get
            {
                if (this.ScannedTrack != null)
                {
                    return this.ScannedTrack.TrackNumber;
                }

                return null;
            }
        }

        /// <summary>
        /// Gets or sets the Scanned Audio Tracks
        /// </summary>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Audio ScannedTrack
        {
            get
            {
                return this.scannedTrack;
            }

            set
            {
                this.scannedTrack = value;
                this.OnPropertyChanged("ScannedTrack");
                this.OnPropertyChanged("TrackDisplay");
            }
        }

        /// <summary>
        /// Gets the Display Value for this model.
        /// </summary>
        public string TrackDisplay
        {
            get
            {
                return this.ScannedTrack == null ? string.Empty : this.ScannedTrack.ToString();
            }
        }

        /// <summary>
        /// Gets the The UI display value for sample rate
        /// </summary>
        public string SampleRateDisplayValue
        {
            get
            {
                return this.SampleRate == 0 ? "Auto" : this.SampleRate.ToString();
            }
        }

        /// <summary>
        /// Gets the The UI display value for bit rate
        /// </summary>
        public string BitRateDisplayValue
        {
            get
            {
                if (this.Encoder == AudioEncoder.Ac3Passthrough || this.Encoder == AudioEncoder.DtsPassthrough ||
                    this.Encoder == AudioEncoder.DtsHDPassthrough)
                {
                    return "Auto";
                }

                return this.Bitrate.ToString();
            }
        }

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
        /// Gets or sets TrackName.
        /// </summary>
        public string TrackName { get; set; }

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