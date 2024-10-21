// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChaptersView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ChaptersView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Encode.Model.Models;

    using KeyEventArgs = System.Windows.Input.KeyEventArgs;
    using TextBox = System.Windows.Controls.TextBox;
    using UserControl = System.Windows.Controls.UserControl;

    /// <summary>
    /// Interaction logic for ChaptersView.xaml
    /// </summary>
    public partial class ChaptersView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ChaptersView"/> class.
        /// </summary>
        public ChaptersView()
        {
            InitializeComponent();
        }

        private void ChaptersList_OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                this.chaptersList.Focus();
                this.chaptersList.SelectedIndex += 1; // increase

                this.chaptersList.ScrollIntoView(this.chaptersList.SelectedItem);

                // Attempt to focus and select the text box text.
                ItemContainerGenerator generator = this.chaptersList.ItemContainerGenerator;
                ListViewItem selectedItem = generator.ContainerFromIndex(this.chaptersList.SelectedIndex) as ListViewItem;
                if (selectedItem != null)
                {
                    IInputElement firstFocusable = FindFirstFocusableElement(selectedItem);
                    if (firstFocusable != null)
                    {
                        firstFocusable.Focus();
                        TextBox textbox = firstFocusable as TextBox;
                        if (textbox != null)
                        {
                            textbox.SelectAll();
                        }
                    }
                }
            }
        }

        private IInputElement FindFirstFocusableElement(DependencyObject obj)
        {
            IInputElement firstFocusable = null;

            int count = VisualTreeHelper.GetChildrenCount(obj);
            for (int i = 0; i < count && null == firstFocusable; i++)
            {
                DependencyObject child = VisualTreeHelper.GetChild(obj, i);
                firstFocusable = child is IInputElement inputElement && inputElement.Focusable
                    ? inputElement
                    : this.FindFirstFocusableElement(child);
            }

            return firstFocusable;
        }

        private void ChapterName_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            TextBox tb = sender as TextBox;
            if (tb != null)
            {
                ChapterMarker c = tb.DataContext as ChapterMarker;
                ListView lv = VisualTreeUtils.FindAncestor<ListView>(tb);
                if (lv != null && c != null)
                {
                    lv.SelectedItem = c;
                }
            }
        }
    }
}
