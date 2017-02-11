// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MenuItemExtensions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the MenuItemExtensions type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.AttachedProperties
{
    using System.Collections.Generic;
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// The menu item extensions.
    /// </summary>
    public class MenuItemExtensions : DependencyObject
    {
        #region Constants and Fields

        /// <summary>
        /// The group name property.
        /// </summary>
        public static readonly DependencyProperty GroupNameProperty = DependencyProperty.RegisterAttached(
            "GroupName", 
            typeof(string), 
            typeof(MenuItemExtensions), 
            new PropertyMetadata(string.Empty, OnGroupNameChanged));

        /// <summary>
        /// The element to group names.
        /// </summary>
        public static Dictionary<MenuItem, string> ElementToGroupNames = new Dictionary<MenuItem, string>();

        #endregion

        #region Public Methods

        /// <summary>
        /// The get group name.
        /// </summary>
        /// <param name="element">
        /// The element.
        /// </param>
        /// <returns>
        /// The group name as a string.
        /// </returns>
        public static string GetGroupName(MenuItem element)
        {
            return element.GetValue(GroupNameProperty).ToString();
        }

        /// <summary>
        /// The set group name.
        /// </summary>
        /// <param name="element">
        /// The element.
        /// </param>
        /// <param name="value">
        /// The value.
        /// </param>
        public static void SetGroupName(MenuItem element, string value)
        {
            element.SetValue(GroupNameProperty, value);
        }

        #endregion

        #region Methods

        /// <summary>
        /// The menu item checked.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void MenuItemChecked(object sender, RoutedEventArgs e)
        {
            var menuItem = e.OriginalSource as MenuItem;
            foreach (var item in ElementToGroupNames)
            {
                if (item.Key != menuItem && item.Value == GetGroupName(menuItem))
                {
                    item.Key.IsChecked = false;
                }
            }
        }

        /// <summary>
        /// The on group name changed.
        /// </summary>
        /// <param name="d">
        /// The d.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void OnGroupNameChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            MenuItem menuItem = d as MenuItem;

            if (menuItem != null)
            {
                string newGroupName = e.NewValue.ToString();
                string oldGroupName = e.OldValue.ToString();
                if (string.IsNullOrEmpty(newGroupName))
                {
                    RemoveCheckboxFromGrouping(menuItem);
                }
                else
                {
                    if (newGroupName != oldGroupName)
                    {
                        if (!string.IsNullOrEmpty(oldGroupName))
                        {
                            RemoveCheckboxFromGrouping(menuItem);
                        }
                        ElementToGroupNames.Add(menuItem, e.NewValue.ToString());
                        menuItem.Checked += MenuItemChecked;
                    }
                }
            }
        }

        /// <summary>
        /// The remove checkbox from grouping.
        /// </summary>
        /// <param name="checkBox">
        /// The check box.
        /// </param>
        private static void RemoveCheckboxFromGrouping(MenuItem checkBox)
        {
            ElementToGroupNames.Remove(checkBox);
            checkBox.Checked -= MenuItemChecked;
        }

        #endregion
    }
}