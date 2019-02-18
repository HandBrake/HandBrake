// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PopupWindowViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PopupWindowViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The popup window view model.
    /// </summary>
    public class PopupWindowViewModel : ViewModelBase, IPopupWindowViewModel
    {
        private string windowTitle;
        private string subText;

        private string saveButtonText;

        public PopupWindowViewModel(IViewModelBase contentViewModel, string title, string subText, string saveBtnText)
        {
            this.ContentViewModel = contentViewModel;
            this.WindowTitle = title;
            this.Title = title;
            this.SubText = subText;
            this.saveButtonText = saveBtnText;
        }

        /// <summary>
        /// Gets or sets the content view model.
        /// </summary>
        public IViewModelBase ContentViewModel { get; set; }

        /// <summary>
        /// Gets or sets the title.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return this.windowTitle;
            }
            set
            {
                if (value == this.windowTitle)
                {
                    return;
                }
                this.windowTitle = value;
                this.NotifyOfPropertyChange(() => this.Title);
            }
        }

        /// <summary>
        /// Gets or sets the sub text.
        /// </summary>
        public string SubText
        {
            get
            {
                return this.subText;
            }
            set
            {
                if (value == this.subText)
                {
                    return;
                }
                this.subText = value;

                this.SubTextVisible = !string.IsNullOrEmpty(value);

                this.NotifyOfPropertyChange(() => this.SubText);
                this.NotifyOfPropertyChange(() => this.SubTextVisible);
            }
        }

        public string SaveButtonText
        {
            get => this.saveButtonText  ?? Resources.Generic_Save;
            set
            {
                if (value == this.saveButtonText) return;
                this.saveButtonText = value;
                this.NotifyOfPropertyChange(() => this.SaveButtonText);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether sub text visible.
        /// </summary>
        public bool SubTextVisible { get; set; }

        /// <summary>
        /// The save.
        /// </summary>
        public void Save()
        {
            this.TryClose(true);
        }

        /// <summary>
        /// The cancel.
        /// </summary>
        public void Cancel()
        {
            this.TryClose(false);
        }
    }
}
