// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBVideoEncoder.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Encoders
{
    using System;
    using System.Collections.Generic;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Helpers;

    public class HBVideoEncoder
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HBVideoEncoder"/> class.
        /// </summary>
        /// <param name="compatibleContainers">
        /// The compatible containers.
        /// </param>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public HBVideoEncoder(int compatibleContainers, string displayName, int id, string shortName)
        {
            this.CompatibleContainers = compatibleContainers;
            this.DisplayName = displayName;
            this.Id = id;
            this.ShortName = shortName ?? string.Empty;
        }

        /// <summary>
        /// Gets the compatible containers.
        /// </summary>
        public int CompatibleContainers { get; private set; }

        /// <summary>
        /// Gets the display name.
        /// </summary>
        public string DisplayName { get; private set; }

        /// <summary>
        /// Gets the id.
        /// </summary>
        public int Id { get; private set; }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string ShortName { get; private set; }

        public string Codec
        {
            get
            {
                string[] codec = this.DisplayName.Split(' ');

                if (codec.Length >= 1)
                {
                    return codec[0];
                }

                return string.Empty;
            }
        }

        /// <summary>
        /// Gets the list of presets this encoder supports. (null if the encoder doesn't support presets)
        /// </summary>
        public List<string> Presets
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(HBFunctions.hb_video_encoder_get_presets(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of tunes this encoder supports. (null if the encoder doesn't support tunes)
        /// </summary>
        public List<string> Tunes
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(HBFunctions.hb_video_encoder_get_tunes(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of profiles this encoder supports. (null if the encoder doesn't support profiles)
        /// </summary>
        public List<string> Profiles
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(HBFunctions.hb_video_encoder_get_profiles(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of levels this encoder supports. (null if the encoder doesn't support levels)
        /// </summary>
        public List<string> Levels
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(HBFunctions.hb_video_encoder_get_levels(this.Id));
            }
        }

        public int BitDepth
        {
            get
            {
                if (this.DisplayName.Contains("10"))
                {
                    return 10;
                }

                if (this.DisplayName.Contains("12"))
                {
                    return 12;
                }

                return 8;
            }
        }

        public bool SupportsMP4 => (this.CompatibleContainers & NativeConstants.HB_MUX_MASK_MP4) == NativeConstants.HB_MUX_MASK_MP4
                                   || (this.CompatibleContainers & NativeConstants.HB_MUX_AV_MP4) == NativeConstants.HB_MUX_AV_MP4;

        public bool SupportsMKV => (this.CompatibleContainers & NativeConstants.HB_MUX_MASK_MKV) == NativeConstants.HB_MUX_MASK_MKV
                                   || (this.CompatibleContainers & NativeConstants.HB_MUX_AV_MKV) == NativeConstants.HB_MUX_AV_MKV;

        public bool SupportsWebM => (this.CompatibleContainers & NativeConstants.HB_MUX_MASK_WEBM) == NativeConstants.HB_MUX_MASK_WEBM
                                    || (this.CompatibleContainers & NativeConstants.HB_MUX_AV_WEBM) == NativeConstants.HB_MUX_AV_WEBM;

        public bool SupportsMultiPass() => HandBrakeEncoderHelpers.VideoEncoderSupportsMultiPass(this.ShortName, true)
                                    || HandBrakeEncoderHelpers.VideoEncoderSupportsMultiPass(this.ShortName, false);

        public bool SupportsMultiPass(bool constantQuality) => HandBrakeEncoderHelpers.VideoEncoderSupportsMultiPass(this.ShortName, constantQuality);

        public bool IsLossless => this.IsFFV1;

        public bool SupportsQuality => HandBrakeEncoderHelpers.VideoEncoderSupportsQualityMode(this.ShortName);

        public bool SupportsQualityAdjustment => SupportsQuality && !IsFFV1;

        public bool SupportsBitrate => HandBrakeEncoderHelpers.VideoEncoderSupportsBitrateMode(this.ShortName);

        // TODO check if there is a nicer way of doing this.
        public bool IsSVTAV1 => this.ShortName.Contains("svt_av1");
        public bool IsX264 => this.ShortName.Contains("x264");
        public bool IsX265 => this.ShortName.Contains("x265");
        public bool IsH264 => this.ShortName.Contains("264");
        public bool IsQuickSync => this.ShortName.Contains("qsv");
        public bool IsQuickSyncH265 => this.ShortName.Contains("qsv_h265");
        public bool IsQuickSyncAV1 => this.ShortName.Contains("qsv_av1");
        public bool IsNVEnc => this.ShortName.Contains("nvenc");
        public bool IsVCN => this.ShortName.Contains("vce") || this.ShortName.Contains("vcn");
        public bool IsMediaFoundation => this.ShortName.Contains("mf");
        public bool IsMpeg2 => this.ShortName.Contains("mpeg2");
        public bool IsVP9 => this.ShortName.Contains("VP9", StringComparison.InvariantCultureIgnoreCase);
        public bool IsHardwareEncoder => this.IsNVEnc || this.IsMediaFoundation || this.IsVCN || this.IsQuickSync;
        public bool IsFFV1 => this.ShortName.Contains("ffv1", StringComparison.InvariantCultureIgnoreCase);
    }
}