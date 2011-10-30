// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBAudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using HandBrake.Interop.HbLib;

	public class HBAudioEncoder
	{
		public string ShortName { get; set; }

		public string DisplayName { get; set; }

		public int Id { get; set; }

		public Container CompatibleContainers { get; set; }

		public bool SupportsQuality
		{
			get
			{
				return this.QualityLimits.High >= 0;
			}
		}

		public RangeLimits QualityLimits { get; set; }

		public float DefaultQuality { get; set; }

		public bool SupportsCompression
		{
			get
			{
				return this.CompressionLimits.High >= 0;
			}
		}

		public RangeLimits CompressionLimits { get; set; }

		public float DefaultCompression { get; set; }

		public bool IsPassthrough
		{
			get
			{
				return (this.Id & NativeConstants.HB_ACODEC_PASS_FLAG) > 0;
			}
		}
	}
}
