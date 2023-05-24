// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SaveFileDialog.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
// SaveFileDialog Wrapper with LegacyFallback.
// A ComException can sometimes occur. This may be due to Visual Themes being turned off at the OS level.
// Fall back to the WinForms dialogs for now as a workaround.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.FileDialogs
{
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    public class SaveFileDialog
    {
        public string Title { get; set; }
        public string Filter { get; set; }
        public bool CheckPathExists { get; set; }
        public bool AddExtension { get; set; }
        public string DefaultExt { get; set; }
        public bool OverwritePrompt { get; set; }
        public int FilterIndex { get; set; }
        public string FileName { get; set; }
        public string InitialDirectory { get; set; }
        public bool RestoreDirectory { get; set; }

        public bool? ShowDialog()
        {
            Microsoft.Win32.SaveFileDialog saveDialog = new Microsoft.Win32.SaveFileDialog
                                                        {
                                                            Title = this.Title,
                                                            Filter = this.Filter,
                                                            CheckPathExists = this.CheckPathExists,
                                                            AddExtension = this.AddExtension,
                                                            DefaultExt = this.DefaultExt,
                                                            OverwritePrompt = this.OverwritePrompt,
                                                            FilterIndex = this.FilterIndex,
                                                            InitialDirectory = this.InitialDirectory,
                                                            RestoreDirectory = this.RestoreDirectory,
                                                            FileName = this.FileName
            };

            try
            {
                bool? result = saveDialog.ShowDialog();

                this.FileName = result == true ? saveDialog.FileName : null;

                return result;
            }
            catch (COMException)
            {
                System.Windows.Forms.SaveFileDialog dialog = new System.Windows.Forms.SaveFileDialog();
                dialog.Title = saveDialog.Title;
                dialog.Filter = saveDialog.Filter;
                dialog.CheckPathExists = saveDialog.CheckFileExists;
                dialog.AddExtension = saveDialog.AddExtension;
                dialog.DefaultExt = saveDialog.DefaultExt;
                dialog.OverwritePrompt = saveDialog.OverwritePrompt;
                dialog.FilterIndex = saveDialog.FilterIndex;
                dialog.InitialDirectory = saveDialog.InitialDirectory;
                dialog.RestoreDirectory = saveDialog.RestoreDirectory;
                dialog.FileName = saveDialog.FileName;

                dialog.AutoUpgradeEnabled = false;

                DialogResult result = dialog.ShowDialog();

                if (result != DialogResult.Cancel)
                {
                    this.FileName = dialog.FileName;
                    return true;
                }
                else
                {
                    this.FileName = null;
                }
            }

            return null;
        }
    }
}
