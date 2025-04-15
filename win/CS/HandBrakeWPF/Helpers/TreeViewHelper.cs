// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TreeViewHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Windows.Controls;

    public static class TreeViewHelper
    {
        public static void ExpandAllNodes(TreeView treeView)
        {
            foreach (var item in treeView.Items)
            {
                if (treeView.ItemContainerGenerator.ContainerFromItem(item) is TreeViewItem treeViewItem)
                {
                    ExpandAll(treeViewItem);
                }
            }
        }

        public static void CollapseAllNodes(TreeView treeView)
        {
            foreach (var item in treeView.Items)
            {
                if (treeView.ItemContainerGenerator.ContainerFromItem(item) is TreeViewItem treeViewItem)
                {
                    CollapseAll(treeViewItem);
                }
            }
        }

        private static void ExpandAll(TreeViewItem item)
        {
            item.IsExpanded = true;

            item.ApplyTemplate();
            if (item.Items.Count > 0)
            {
                item.UpdateLayout();
                foreach (var child in item.Items)
                {
                    if (item.ItemContainerGenerator.ContainerFromItem(child) is TreeViewItem childItem)
                    {
                        ExpandAll(childItem);
                    }
                }
            }
        }

        private static void CollapseAll(TreeViewItem item)
        {
            item.IsExpanded = false;

            item.ApplyTemplate();
            if (item.Items.Count > 0)
            {
                item.UpdateLayout();
                foreach (var child in item.Items)
                {
                    if (item.ItemContainerGenerator.ContainerFromItem(child) is TreeViewItem childItem)
                    {
                        CollapseAll(childItem);
                    }
                }
            }
        }
    }
}
