// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ListBoxHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Helper for the ListBox control to provide SelectedItems functionality.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Collections;
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// A Helper for the ListBox control to provide SelectedItems functionality.
    /// </summary>
    public class ListBoxHelper
    {
        /// <summary>
        /// SelectedItems Attached Dependency Property
        /// </summary>
        public static readonly DependencyProperty SelectedItemsProperty =
            DependencyProperty.RegisterAttached("SelectedItems", typeof(IList), typeof(ListBoxHelper), new FrameworkPropertyMetadata(null, OnSelectedItemsChanged));
       
        /// <summary>
        /// Gets the SelectedItems property.  This dependency property 
        /// indicates ....
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        /// <returns>
        /// The get selected items.
        /// </returns>
        public static IList GetSelectedItems(DependencyObject item)
        {
            return (IList)item.GetValue(SelectedItemsProperty);
        }

        /// <summary>
        /// Sets the SelectedItems property.  This dependency property 
        /// indicates ....
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        /// <param name="value">
        /// The value.
        /// </param>
        public static void SetSelectedItems(DependencyObject item, IList value)
        {
            item.SetValue(SelectedItemsProperty, value);
        }

        /// <summary>
        /// Handles changes to the SelectedItems property.
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        /// <param name="e">
        /// The DependencyPropertyChangedEventArgs.
        /// </param>
        private static void OnSelectedItemsChanged(DependencyObject item, DependencyPropertyChangedEventArgs e)
        {
            ListBox listBox = item as ListBox;
            if (listBox != null)
            {
                ResetSelectedItems(listBox);
                listBox.SelectionChanged += delegate { ResetSelectedItems(listBox); };
            }
        }

        /// <summary>
        /// Reset Selected Items
        /// </summary>
        /// <param name="listBox">
        /// The list box.
        /// </param>
        private static void ResetSelectedItems(ListBox listBox)
        {
            IList selectedItems = GetSelectedItems(listBox);
            selectedItems.Clear();
            if (listBox.SelectedItems != null)
            {
                foreach (var item in listBox.SelectedItems)
                    selectedItems.Add(item);
            }
        }
    }
}
