// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBAudioEncoder.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb audio encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Encoders
{
    using HandBrake.Interop.Interop.HbLib;

    public class HBAudioEncoder
    {
        public const string AvAac = "av_aac";
        public const string Vorbis = "vorbis";
        public const string Passthru = "copy";

        // TODO Passthru options code to allow removal of these.
        public const string Ac3Passthrough = "copy:ac3";
        public const string EAc3Passthrough = "copy:eac3";
        public const string DtsPassthrough = "copy:dts";
        public const string DtsHDPassthrough = "copy:dtshd";
        public const string TrueHDPassthrough = "copy:truehd";
        public const string AacPassthru = "copy:aac";
        public const string Mp2Passthru = "copy:mp2";
        public const string Mp3Passthru = "copy:mp3";
        public const string OpusPassthru = "copy:opus";
        public const string FlacPassthru = "copy:flac";

        public static HBAudioEncoder None = new HBAudioEncoder(-1, null, 1, 1, "None", -1, null, "none");

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

        public bool SupportsMP4
        {
            get
            {
                return (this.CompatibleContainers & NativeConstants.HB_MUX_MASK_MP4) > 0 || this.CompatibleContainers == -1;
            }
        }

        public bool SupportsWebM
        {
            get
            {
                return (this.CompatibleContainers & NativeConstants.HB_MUX_MASK_WEBM) > 0 || this.CompatibleContainers == -1;
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
                return this.CompressionLimits != null && this.CompressionLimits.High >= 0;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the encoder supports quality.
        /// </summary>
        public bool SupportsQuality
        {
            get
            {
                return this.QualityLimits != null && this.QualityLimits.High >= 0;
            }
        }

        public bool IsLosslessEncoder
        {
            get
            {
                return this.ShortName.Contains("flac"); // TODO Find a better way to do this. 
            }
        }
    }
}