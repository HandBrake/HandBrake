/*  AllowedPassthru.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using Interop.Model.Encoding;

    /// <summary>
    /// Allowed Passthru Options
    /// </summary>
    public class AllowedPassthru
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AllowedPassthru"/> class. 
        /// </summary>
        public AllowedPassthru()
        {
            this.AudioAllowAACPass = true;
            this.AudioAllowAC3Pass = true;
            this.AudioAllowDTSHDPass = true;
            this.AudioAllowDTSPass = true;
            this.AudioAllowMP3Pass = true;
            this.AudioEncoderFallback = AudioEncoder.Ac3;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AllowedPassthru"/> class. 
        /// </summary>
        /// <param name="initialValue">
        /// The initial Value.
        /// </param>
        public AllowedPassthru(bool initialValue)
        {
            this.AudioAllowAACPass = initialValue;
            this.AudioAllowAC3Pass = initialValue;
            this.AudioAllowDTSHDPass = initialValue;
            this.AudioAllowDTSPass = initialValue;
            this.AudioAllowMP3Pass = initialValue;
            this.AudioEncoderFallback = AudioEncoder.Ac3;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AllowedPassthru"/> class.
        /// Copy Constructor
        /// </summary>
        /// <param name="initialValue">
        /// The initial value.
        /// </param>
        public AllowedPassthru(AllowedPassthru initialValue)
        {
            this.AudioAllowAACPass = initialValue.AudioAllowAACPass;
            this.AudioAllowAC3Pass = initialValue.AudioAllowAC3Pass;
            this.AudioAllowDTSHDPass = initialValue.AudioAllowDTSHDPass;
            this.AudioAllowDTSPass = initialValue.AudioAllowDTSPass;
            this.AudioAllowMP3Pass = initialValue.AudioAllowMP3Pass;
            this.AudioEncoderFallback = initialValue.AudioEncoderFallback;
        }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowAACPass.
        /// </summary>
        public bool AudioAllowAACPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowAC3Pass.
        /// </summary>
        public bool AudioAllowAC3Pass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowDTSHDPass.
        /// </summary>
        public bool AudioAllowDTSHDPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowDTSPass.
        /// </summary>
        public bool AudioAllowDTSPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowMP3Pass.
        /// </summary>
        public bool AudioAllowMP3Pass { get; set; }

        /// <summary>
        /// Gets or sets AudioEncoderFallback.
        /// </summary>
        public AudioEncoder AudioEncoderFallback { get; set; }
    }
}
