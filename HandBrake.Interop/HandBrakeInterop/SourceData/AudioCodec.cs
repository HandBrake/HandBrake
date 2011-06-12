using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.SourceData
{
	// Only contains 2 real codecs at the moment as those are what we care about. More will be added later.
	public enum AudioCodec
	{
		Ac3,

		Dts,

		Other
	}
}
