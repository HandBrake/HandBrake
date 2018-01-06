// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBAudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb audio encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using HandBrake.ApplicationServices.Interop.HbLib;

    /// <summary>
    /// The hb audio encoder.
    /// </summary>
    public class HBAudioEncoder
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HBAudioEncoder"/> class.
        /// </summary>
        /// <param name="compatibleContainers">
        /// The compatible containers.
        /// </param>
        /// <param name="compressionLimits">
        /// The compression limits.
        /// </param>
        /// <param name="defaultCompression">
        /// The default compression.
        /// </param>
        /// <param name="defaultQuality">
        /// The default quality.
        /// </param>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <param name="qualityLimits">
        /// The quality limits.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public HBAudioEncoder(int compatibleContainers, RangeLimits compressionLimits, float defaultCompression, float defaultQuality, string displayName, int id, RangeLimits qualityLimits, string shortName)
        {
            this.CompatibleContainers = compatibleContainers;
            this.CompressionLimits = compressionLimits;
            this.DefaultCompression = defaultCompression;
            this.DefaultQuality = defaultQuality;
            this.DisplayName = displayName;
            this.Id = id;
            this.QualityLimits = qualityLimits;
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets the compatible containers.
        /// </summary>
        public int CompatibleContainers { get; private set; }

        /// <summary>
        /// Gets the compression limits.
        /// </summary>
        public RangeLimits CompressionLimits { get; private set; }

        /// <summary>
        /// Gets the default compression.
        /// </summary>
        public float DefaultCompression { get; private set; }

        /// <summary>
        /// Gets the default quality.
        /// </summary>
        public float DefaultQuality { get; private set; }

        /// <summary>
        /// Gets the display name.
        /// </summary>
        public string DisplayName { get; private set; }

        /// <summary>
        /// Gets the id.
        /// </summary>
        public int Id { get; private set; }

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