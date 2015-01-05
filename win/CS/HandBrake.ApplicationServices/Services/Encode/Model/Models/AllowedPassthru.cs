// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AllowedPassthru.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Allowed Passthru Options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Model.Models
{
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// Allowed Passthru Options
    /// </summary>
    public class AllowedPassthru
    {
        #region Constants and Fields

        /// <summary>
        /// The audio allow aac pass.
        /// </summary>
        private bool? audioAllowAACPass;

        /// <summary>
        /// The audio allow a c 3 pass.
        /// </summary>
        private bool? audioAllowAC3Pass;

        /// <summary>
        /// The audio allow dtshd pass.
        /// </summary>
        private bool? audioAllowDTSHDPass;

        /// <summary>
        /// The audio allow dts pass.
        /// </summary>
        private bool? audioAllowDTSPass;

        /// <summary>
        /// The audio allow m p 3 pass.
        /// </summary>
        private bool? audioAllowMP3Pass;

        #endregion

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
            this.AudioEncoderFallback = AudioEncoder.Ac3;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AllowedPassthru"/> class. 
        /// </summary>
        /// <param name="initialValue">
        /// The initial Value.
        /// </param>
        public AllowedPassthru(bool? initialValue)
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

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowAACPass.
        /// </summary>
        public bool? AudioAllowAACPass
        {
            get
            {
                return this.audioAllowAACPass ?? true;
            }
            set
            {
                this.audioAllowAACPass = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowAC3Pass.
        /// </summary>
        public bool? AudioAllowAC3Pass
        {
            get
            {
                return this.audioAllowAC3Pass ?? true;
            }
            set
            {
                this.audioAllowAC3Pass = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowDTSHDPass.
        /// </summary>
        public bool? AudioAllowDTSHDPass
        {
            get
            {
                return this.audioAllowDTSHDPass ?? true;
            }
            set
            {
                this.audioAllowDTSHDPass = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowDTSPass.
        /// </summary>
        public bool? AudioAllowDTSPass
        {
            get
            {
                return this.audioAllowDTSPass ?? true;
            }
            set
            {
                this.audioAllowDTSPass = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AudioAllowMP3Pass.
        /// </summary>
        public bool? AudioAllowMP3Pass
        {
            get
            {
                return this.audioAllowMP3Pass ?? true;
            }
            set
            {
                this.audioAllowMP3Pass = value;
            }
        }

        /// <summary>
        /// Gets or sets AudioEncoderFallback.
        /// </summary>
        public AudioEncoder AudioEncoderFallback { get; set; }

        #endregion
    }
}