// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VisualTreeUtils" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Windows;
    using System.Windows.Media;

    public class VisualTreeUtils
    {
        public static T FindAncestor<T>(DependencyObject dObj) where T : DependencyObject
        {
            var uiElement = dObj;
            while (uiElement != null)
            {
                uiElement = VisualTreeHelper.GetParent(uiElement as Visual ?? new UIElement());

                if (uiElement is T) return (T)uiElement;
            }
            return null;
        }
    }
}
