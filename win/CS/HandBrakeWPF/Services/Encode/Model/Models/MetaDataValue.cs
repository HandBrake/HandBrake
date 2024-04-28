// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaDataValue.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A UI representation of a metadata attribute. 
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    public class MetaDataValue
    {
        public MetaDataValue(string annotation, string value)
        {
            this.Annotation = annotation;
            this.Value = value;
        }

        public string Annotation { get; set; }

        public string Value { get; set; }
    }
}
