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
    using HandBrakeWPF.ViewModels;

    public class MetaDataValue : PropertyChangedBase
    {
        private bool enabled;
        private string value;

        public MetaDataValue(string annotation, string value)
        {
            this.Annotation = annotation;
            this.Value = value;
        }

        public string Annotation { get; set; }

        public string Value
        {
            get => this.value;
            set
            {
                if (value == this.value)
                {
                    return;
                }

                this.value = value;
                this.NotifyOfPropertyChange(() => this.Value);
            }
        }

        public bool IsNew { get; set; }
    }
}
