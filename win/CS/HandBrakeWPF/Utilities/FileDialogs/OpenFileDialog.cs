// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OpenFileDialog.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
// OpenFileDialog Wrapper with LegacyFallback.
// A ComException can sometimes occur. This may be due to Visual Themes being turned off at the OS level.
// Fall back to the WinForms dialogs for now as a workaround.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.FileDialogs
{
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    internal class OpenFileDialog
    {
        public string Filter { get; set; }

        public bool CheckFileExists  { get; set; }

        public bool Multiselect { get; set; }

        public string InitialDirectory { get; set; }

        public string FileName { get; set; }

        public string[] FileNames { get; set; }

        public int FilterIndex { get; set; }

        public bool? ShowDialog()
        {
            Microsoft.Win32.OpenFileDialog dialog = new Microsoft.Win32.OpenFileDialog
                                                    {
                                                        Filter = this.Filter,
                                                        CheckFileExists = this.CheckFileExists,
                                                        Multiselect = this.Multiselect,
                                                        FileName = this.FileName,
                                                        InitialDirectory = this.InitialDirectory,
                                                        FilterIndex = this.FilterIndex
                                                    };

            try
            {
                bool? result = dialog.ShowDialog();
                if (result == true)
                {
                    this.FileName = dialog.FileName;
                    this.FileNames = dialog.FileNames;
                }
                else
                {
                    this.FileNames = null;
                    this.FileName = null;
                }
            
                return result;
            }
            catch (COMException)
            {
                System.Windows.Forms.OpenFileDialog dialog2 = new System.Windows.Forms.OpenFileDialog();
                dialog2.Title = dialog.Title;
                dialog2.Filter = dialog.Filter;
                dialog2.CheckFileExists = dialog.CheckFileExists;
                dialog2.Multiselect = dialog.Multiselect;
                dialog2.InitialDirectory = dialog.InitialDirectory;
                dialog2.FileName = dialog.FileName;
                dialog2.FilterIndex = dialog.FilterIndex;

                dialog2.AutoUpgradeEnabled = false;

                if (dialog2.ShowDialog() == DialogResult.OK)
                {
                    this.FileName = dialog.FileName;
                    this.FileNames = dialog.FileNames;
                    return true;
                }
                else
                {
                    this.FileNames = null;
                    this.FileName = null;
                }
            }

            return null;
        }
    }
}
