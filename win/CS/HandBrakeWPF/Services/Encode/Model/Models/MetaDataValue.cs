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

        public MetaDataValue(string annotation, string value)
        {
            this.Annotation = annotation;
            this.Value = value;
            this.Enabled = true; // Default passthru
        }

        public string Annotation { get; set; }

        public string Value { get; set; }

        public bool Enabled
        {
            get => this.enabled;
            set
            {
                if (value == this.enabled)
                {
                    return;
                }

                this.enabled = value;
                this.NotifyOfPropertyChange(() => this.Enabled);
            }
        }

        public bool IsNew { get; set; }
    }
}
