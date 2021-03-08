// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DoubleClickFileBehaviours.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Improve the behaviour of textboxes including file paths. - Select the full filename.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Behaviours
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows;
    using System.Windows.Controls;

    using Microsoft.Xaml.Behaviors;

    public class DoubleClickFileBehaviours : Behavior<TextBox>
    {
        protected override void OnAttached()
        {
            AssociatedObject.MouseDoubleClick += AssociatedObjectMouseDoubleClick;
            base.OnAttached();
        }

        protected override void OnDetaching()
        {
            AssociatedObject.MouseDoubleClick -= AssociatedObjectMouseDoubleClick;
            base.OnDetaching();
        }

        private void AssociatedObjectMouseDoubleClick(object sender, RoutedEventArgs routedEventArgs)
        {
            TextBox tb = sender as TextBox;

            if (tb != null)
            {
                string filePath = tb.Text;
                if (!string.IsNullOrEmpty(filePath))
                {
                    try
                    {
                        string filename = Path.GetFileNameWithoutExtension(filePath);
                        string extension = Path.GetExtension(filePath);

                        long index = tb.CaretIndex;
                        int filenameIndex = filePath.IndexOf(filename, StringComparison.Ordinal);
                        int extensionIndex = filePath.IndexOf(extension, StringComparison.Ordinal);

                        if (index >= filenameIndex && index < extensionIndex)
                        {
                            tb.Select(filenameIndex, filename.Length);
                        }
                    }
                    catch (Exception e)
                    {
                        Debug.WriteLine(e);
                    }
                }
            }
        }
    }
}
