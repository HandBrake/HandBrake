// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ListExtensions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   IList Extensions
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Extensions
{
    using System;
    using System.Collections;

    public static class ListExtensions
    {
        public static void Swap<T>(this IList list, int firstIndex, int secondIndex)
        {
            if (list == null)
            {
                throw new ArgumentNullException("List was null");
            }

            if (firstIndex > list.Count - 1)
            {
                throw new IndexOutOfRangeException("First Index: " + firstIndex);
            }

            if (firstIndex > list.Count - 1 || secondIndex < 0)
            {
                throw new IndexOutOfRangeException("Second Index: " + secondIndex);
            }

            T temp = (T)list[firstIndex];
            list[firstIndex] = list[secondIndex];
            list[secondIndex] = temp;
        }

        public static void MoveUp<T>(this IList list, T item)
        {
            if (list == null)
            {
                throw new ArgumentNullException("List was null");
            }

            if (item == null)
            {
                throw new ArgumentNullException("Item was null");
            }

            int index = list.IndexOf(item);

            if (index > 0)
            {
                list.Swap<T>(index, index - 1);
            }
        }

        public static void MoveDown<T>(this IList list, T item)
        {
            if (list == null)
            {
                throw new ArgumentNullException("List was null");
            }

            if (item == null)
            {
                throw new ArgumentNullException("Item was null");
            }

            int index = list.IndexOf(item);

            if (index >= 0)
            {
                list.Swap<T>(index, index + 1);
            }
        }
    }
}
