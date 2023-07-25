// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FolderBrowserDialog.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
// FolderBrowserDialog Wrapper with LegacyFallback.
// A ComException can sometimes occur. This may be due to Visual Themes being turned off at the OS level.
// Fall back to the WinForms dialogs for now as a workaround.
// </summary>
// --------------------------------------------------------------------------------------------------------------------


namespace HandBrakeWPF.Utilities.FileDialogs
{
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    using Ookii.Dialogs.Wpf;

    public class FolderBrowserDialog
    {
        public string Description { get; set; }
        public bool UseDescriptionForTitle { get; set; }
        public string SelectedPath { get; set; }
        
        public bool? ShowDialog()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = this.Description, UseDescriptionForTitle = this.UseDescriptionForTitle };
            if (!string.IsNullOrEmpty(this.SelectedPath) && Directory.Exists(this.SelectedPath))
            {
                dialog.SelectedPath = this.SelectedPath;
            }

            try
            {
                bool? result = dialog.ShowDialog();
                if (result.HasValue && result.Value)
                {
                    this.SelectedPath = dialog.SelectedPath;
                }
                else
                {
                    this.SelectedPath = null;
                }

                return result;
            }
            catch (COMException)
            {
                System.Windows.Forms.FolderBrowserDialog dialog2 = new System.Windows.Forms.FolderBrowserDialog();
                dialog2.Description = dialog.Description;
                dialog2.UseDescriptionForTitle = dialog.UseDescriptionForTitle;
                dialog2.SelectedPath = dialog.SelectedPath;
                dialog2.AutoUpgradeEnabled = false;
                DialogResult result = dialog2.ShowDialog();

                if (result == DialogResult.OK)
                {
                    this.SelectedPath = dialog.SelectedPath;
                    return true;
                }
                else
                {
                    this.SelectedPath = null;
                }
            }

            return null;
        }
    }
}
