// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AllowedPassthru.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Allowed Passthru Options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.Collections.Generic;

    /// <summary>
    /// Allowed Passthru Options
    /// </summary>
    public class AllowedPassthru
    {
        #region Constructors and Destructors

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
            this.AudioAllowEAC3Pass = true;
            this.AudioAllowTrueHDPass = true;
            this.AudioAllowFlacPass = true;
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
            this.AudioAllowEAC3Pass = initialValue;
            this.AudioAllowTrueHDPass = initialValue;
            this.AudioAllowFlacPass = initialValue;
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
            this.AudioAllowEAC3Pass = initialValue.AudioAllowEAC3Pass;
            this.AudioAllowTrueHDPass = initialValue.AudioAllowTrueHDPass;
            this.AudioAllowFlacPass = initialValue.AudioAllowFlacPass;
            this.AudioEncoderFallback = initialValue.AudioEncoderFallback;
        }

        #endregion

        #region Properties

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
        /// Gets or sets a value indicating whether audio allow true hd pass.
        /// </summary>
        public bool AudioAllowTrueHDPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow flac pass.
        /// </summary>
        public bool AudioAllowFlacPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow ea c 3 pass.
        /// </summary>
        public bool AudioAllowEAC3Pass { get; set; }

        /// <summary>
        /// Gets or sets AudioEncoderFallback.
        /// </summary>
        public AudioEncoder AudioEncoderFallback { get; set; }

        /// <summary>
        /// Gets the allowed passthru options.
        /// </summary>
        public IEnumerable<AudioEncoder> AllowedPassthruOptions
        {
            get
            {
                List<AudioEncoder> audioEncoders = new List<AudioEncoder>();
                if (this.AudioAllowAACPass)
                {
                    audioEncoders.Add(AudioEncoder.AacPassthru);
                }
                if (this.AudioAllowAC3Pass)
                {
                    audioEncoders.Add(AudioEncoder.Ac3Passthrough);
                }
                if (this.AudioAllowDTSHDPass)
                {
                    audioEncoders.Add(AudioEncoder.DtsHDPassthrough);
                }
                if (this.AudioAllowDTSPass)
                {
                    audioEncoders.Add(AudioEncoder.DtsPassthrough);
                }
                if (this.AudioAllowMP3Pass)
                {
                    audioEncoders.Add(AudioEncoder.Mp3Passthru);
                }
                if (this.AudioAllowTrueHDPass)
                {
                    audioEncoders.Add(AudioEncoder.TrueHDPassthrough);
                }
                if (this.AudioAllowFlacPass)
                {
                    audioEncoders.Add(AudioEncoder.FlacPassthru);
                }
                if (this.AudioAllowEAC3Pass)
                {
                    audioEncoders.Add(AudioEncoder.EAc3Passthrough);
                }

                return audioEncoders;
            }
        } 

        #endregion
    }
}