// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PopupWindowViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PopupWindowViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using HandBrake.ViewModels.Interfaces;

    /// <summary>
    /// The popup window view model.
    /// </summary>
    public class PopupWindowViewModel : ViewModelBase, IPopupWindowViewModel
    {
        private string windowTitle;
        private string subText;

        /// <summary>
        /// Initializes a new instance of the <see cref="PopupWindowViewModel"/> class.
        /// </summary>
        /// <param name="contentViewModel">
        /// The content View Model.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="subText">
        /// The sub Text.
        /// </param>
        public PopupWindowViewModel(IViewModelBase contentViewModel, string title, string subText)
        {
            this.ContentViewModel = contentViewModel;
            this.WindowTitle = title;
            this.Title = title;
            this.SubText = subText;
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
