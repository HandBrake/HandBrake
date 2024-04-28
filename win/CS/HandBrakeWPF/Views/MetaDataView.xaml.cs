// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaDataView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for MetaDataView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    /// <summary>
    /// Interaction logic for MetaDataView.xaml
    /// </summary>
    public partial class MetaDataView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MetaDataView"/> class.
        /// </summary>
        public MetaDataView()
        {
            this.InitializeComponent();
        }

        private void MetadataValue_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            TextBox tb = sender as TextBox;
            if (tb != null)
            {
                MetaDataValue c = tb.DataContext as MetaDataValue;
                ListView lv = VisualTreeUtils.FindAncestor<ListView>(tb);
                if (lv != null && c != null)
                {
                    lv.SelectedItem = c;
                }
            }
        }

        private void MetadataList_OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                this.metadataList.Focus();
                this.metadataList.SelectedIndex += 1; // increase

                this.metadataList.ScrollIntoView(this.metadataList.SelectedItem);

                // Attempt to focus and select the text box text.
                ItemContainerGenerator generator = this.metadataList.ItemContainerGenerator;
                ListViewItem selectedItem = generator.ContainerFromIndex(this.metadataList.SelectedIndex) as ListViewItem;
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
    }
}
