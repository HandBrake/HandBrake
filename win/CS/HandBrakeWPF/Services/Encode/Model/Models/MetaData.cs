// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaData.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An MetaData Class
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.Collections.Generic;

    public class MetaData
    {
        public MetaData()
        {
            this.MetaDataInfo = new Dictionary<string, string>();
        }

        public MetaData(MetaData metadata)
        {
            if (metadata != null)
            {
                this.MetaDataInfo = metadata.MetaDataInfo;
                this.PassthruMetadataEnabled = metadata.PassthruMetadataEnabled;
            }
        }

        public bool PassthruMetadataEnabled { get; set; }

        public Dictionary<string, string> MetaDataInfo { get; set; }
    }
}
