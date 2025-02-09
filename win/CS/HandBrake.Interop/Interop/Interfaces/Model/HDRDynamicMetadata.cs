using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using HandBrake.Interop.Attributes;
using HandBrake.Interop.Interop.HbLib;

namespace HandBrake.Interop.Interop.Interfaces.Model
{
    public enum HDRDynamicMetadata
    {
        [DisplayName("None")]
        [ShortName("none")]
        None = hb_hdr_dynamic_metadata_mode_t.HB_HDR_DYNAMIC_METADATA_NONE,
        [DisplayName("HDR10+")]
        [ShortName("hdr10plus")]
        HDR10Plus = hb_hdr_dynamic_metadata_mode_t.HB_HDR_DYNAMIC_METADATA_HDR10PLUS,
        [DisplayName("Dolby Vision")]
        [ShortName("dolbyvision")]
        DolbyVision = hb_hdr_dynamic_metadata_mode_t.HB_HDR_DYNAMIC_METADATA_DOVI,
        [DisplayName("All")]
        [ShortName("all")]
        All = hb_hdr_dynamic_metadata_mode_t.HB_HDR_DYNAMIC_METADATA_ALL,
    }
}
