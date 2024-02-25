// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FolderBrowserDialog.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
// FolderBrowserDialog Wrapper. This was historically because we used a 3rd party library for this. We no longer do.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.FileDialogs
{
    using System.IO;

    using Microsoft.Win32;

    public class FolderBrowserDialog
    {
        public string Description { get; set; }
        public string SelectedPath { get; set; }
        
        public bool? ShowDialog()
        {
            OpenFolderDialog dialog = new OpenFolderDialog { Title = this.Description };

            if (!string.IsNullOrEmpty(this.SelectedPath) && Directory.Exists(this.SelectedPath))
            {
                dialog.FolderName = this.SelectedPath;
            }

            bool? result = dialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
                this.SelectedPath = dialog.FolderName;
            }
            else
            {
                this.SelectedPath = null;
            }

            return result;
        }
    }
}
