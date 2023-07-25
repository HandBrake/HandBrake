// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ListExtensions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   IList Extensions
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Extensions
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;

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

            if (firstIndex >= (list.Count - 1) || secondIndex < 0)
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
                list.Remove(item);
                list.Insert((index - 1), item);
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

            if (index >= 0 && index < (list.Count - 1))
            {
                list.Remove(item);
                list.Insert((index + 1), item);
            }
        }

        public static void MoveToTop<T>(this IList list, IList<T> items)
        {
            List<T> moveList = items.ToList(); // Decouple from upstream.

            // Remove all the existing items
            foreach (var item in moveList)
            {
                list.Remove(item);
            }

            // Insert all items in order at the top of the list.
            int index = 0;
            foreach (var item in moveList)
            {
                list.Insert(index, item);
                index = index +1;
            }
        }

        public static void MoveToBottom<T>(this IList list, IList<T> items)
        {
            List<T> moveList = items.ToList(); // Decouple from upstream.

            // Remove all the existing items
            foreach (var item in moveList)
            {
                list.Remove(item);
            }

            // Insert all items in order at the top of the list.
            foreach (var item in moveList)
            {
                list.Add(item);
            }
        }
        
        public static IEnumerable<T> TakeLast<T>(this IList<T> list, int n)
        {
            if (list.Count - n < 0)
            {
                n = list.Count;
            }

            for (var i = list.Count - n; i < list.Count; i++)
            {
                yield return list[i];
            }
        }
    }
}
