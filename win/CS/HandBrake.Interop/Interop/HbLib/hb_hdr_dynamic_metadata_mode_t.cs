using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HandBrake.Interop.Interop.HbLib
{
    internal enum hb_hdr_dynamic_metadata_mode_t
    {
        HB_HDR_DYNAMIC_METADATA_NONE      = 0,
        HB_HDR_DYNAMIC_METADATA_HDR10PLUS = 1 << 1,
        HB_HDR_DYNAMIC_METADATA_DOVI      = 1 << 2,
        HB_HDR_DYNAMIC_METADATA_ALL       = HB_HDR_DYNAMIC_METADATA_HDR10PLUS | HB_HDR_DYNAMIC_METADATA_DOVI
    };
}
