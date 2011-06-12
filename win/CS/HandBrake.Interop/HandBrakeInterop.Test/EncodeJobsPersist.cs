using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Linq;
using System.Xml.Serialization;
using HandBrake.Interop;

namespace HandBrakeInterop.Test
{
	public static class EncodeJobsPersist
	{
		private static XmlSerializer xmlSerializer = new XmlSerializer(typeof(EncodeJob));

		public static EncodeJob GetJob(string jobName)
		{
			XDocument doc = XDocument.Load(jobName + ".xml");
			using (XmlReader reader = doc.CreateReader())
			{
				var job = xmlSerializer.Deserialize(reader) as EncodeJob;



				return job;
			}
		}
	}
}
