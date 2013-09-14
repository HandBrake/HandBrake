using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop.Model.Encoding
{
	public class HBContainer
	{
		public string ShortName { get; set; }

		public string DisplayName { get; set; }

		public string DefaultExtension { get; set; }

		public int Id { get; set; }
	}
}
