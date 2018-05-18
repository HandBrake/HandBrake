// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TreeViewHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper functions for handling <see cref="TimeSpan" /> structures
//   Based of https://social.technet.microsoft.com/wiki/contents/articles/18188.wpf-treeview-selecteditem-twoway-mvvm-plus-expand-to-selected-and-close-all-others.aspx
// </summary>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrakeWPF.Helpers
{
    using System.Windows;
    using System.Windows.Controls;

    public class TreeViewHelper
    {
        public static object GetTreeViewSelectedItem(DependencyObject obj)
        {
            return (object)obj.GetValue(TreeViewSelectedItemProperty);
        }

        public static void SetTreeViewSelectedItem(DependencyObject obj, object value)
        {
            obj.SetValue(TreeViewSelectedItemProperty, value);
        }

        public static readonly DependencyProperty TreeViewSelectedItemProperty =
            DependencyProperty.RegisterAttached("TreeViewSelectedItem", typeof(object), typeof(TreeViewHelper), new PropertyMetadata(new object(), TreeViewSelectedItemChanged));

        private static void TreeViewSelectedItemChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            TreeView treeView = sender as TreeView;
            if (treeView == null)
            {
                return;
            }

            treeView.SelectedItemChanged -= TreeView_SelectedItemChanged;
            treeView.SelectedItemChanged += TreeView_SelectedItemChanged;

            TreeViewItem thisItem = treeView.ItemContainerGenerator.ContainerFromItem(e.NewValue) as TreeViewItem;
            if (thisItem != null)
            {
                thisItem.IsSelected = true;
                return;
            }

            for (int i = 0; i < treeView.Items.Count; i++)
            {
                if (SelectItem(e.NewValue, treeView.ItemContainerGenerator.ContainerFromIndex(i) as TreeViewItem))
                {
                    return; // Break out the loop. We've found the item and expanded it's parent if necessary.
                }
            }
        }

        private static void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            TreeView treeView = sender as TreeView;
            SetTreeViewSelectedItem(treeView, e.NewValue);
        }

        private static bool SelectItem(object o, TreeViewItem parentItem)
        {
            if (parentItem == null)
            {
                return false;
            }

            bool found = false;
            foreach (var item in parentItem.Items)
            {
                if (item.Equals(o))
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                bool isExpanded = parentItem.IsExpanded;
                if (!isExpanded)
                {
                    parentItem.IsExpanded = true;
                    parentItem.UpdateLayout();
                }
            }

            return found;
        }
    }
}