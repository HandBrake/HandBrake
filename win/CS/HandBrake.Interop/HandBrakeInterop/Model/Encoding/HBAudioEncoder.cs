// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBAudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb audio encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using HandBrake.Interop.HbLib;

    /// <summary>
    /// The hb audio encoder.
    /// </summary>
    public class HBAudioEncoder
    {
        /// <summary>
        /// Gets or sets the compatible containers.
        /// </summary>
        public int CompatibleContainers { get; set; }

        /// <summary>
        /// Gets or sets the compression limits.
        /// </summary>
        public RangeLimits CompressionLimits { get; set; }

        /// <summary>
        /// Gets or sets the default compression.
        /// </summary>
        public float DefaultCompression { get; set; }

        /// <summary>
        /// Gets or sets the default quality.
        /// </summary>
        public float DefaultQuality { get; set; }

        /// <summary>
        /// Gets or sets the display name.
        /// </summary>
        public string DisplayName { get; set; }

        /// <summary>
        /// Gets or sets the id.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets a value indicating whether the encoder is passthrough.
        /// </summary>
        public bool IsPassthrough
        {
            get
            {
                return (this.Id & NativeConstants.HB_ACODEC_PASS_FLAG) > 0;
            }
        }

        /// <summary>
        /// Gets or sets the quality limits.
        /// </summary>
        public RangeLimits QualityLimits { get; set; }

        /// <summary>
        /// Gets or sets the short name.
        /// </summary>
        public string ShortName { get; set; }

        /// <summary>
        /// Gets a value indicating whether the encoder supports compression.
        /// </summary>
        public bool SupportsCompression
        {
            get
            {
                return this.CompressionLimits.High >= 0;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the encoder supports quality.
        /// </summary>
        public bool SupportsQuality
        {
            get
            {
                return this.QualityLimits.High >= 0;
            }
        }
    }
}