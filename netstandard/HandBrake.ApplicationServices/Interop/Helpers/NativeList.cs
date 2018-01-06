// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NativeList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a HandBrake style native list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop.HbLib;

    /// <summary>
    /// Represents a HandBrake style native list.
    /// </summary>
    internal class NativeList : IDisposable
    {
        /// <summary>
        /// Initializes a new instance of the NativeList class.
        /// </summary>
        /// <param name="listPtr">The pointer to use for the list.</param>
        public NativeList(IntPtr listPtr)
        {
            this.Ptr = listPtr;
        }

        /// <summary>
        /// The list of native memory locations allocated for this list.
        /// </summary>
        private readonly List<IntPtr> allocatedMemory = new List<IntPtr>();

        /// <summary>
        /// Gets the pointer to the native list.
        /// </summary>
        public IntPtr Ptr { get; private set; }

        /// <summary>
        /// Gets the number of items in the list.
        /// </summary>
        public int Count
        {
            get
            {
                Debug.WriteLine("Got a Zero Pointer in the NativeList");
                return this.Ptr == IntPtr.Zero ? 0 : HBFunctions.hb_list_count(this.Ptr);
            }
        }

        /// <summary>
        /// Gets the list of native memory locations allocated for this list.
        /// </summary>
        public List<IntPtr> AllocatedMemory
        {
            get
            {
                return this.allocatedMemory;
            }
        }

        /// <summary>
        /// Adds an item to the end of the list.
        /// </summary>
        /// <param name="item">The item to add.</param>
        public void Add(IntPtr item)
        {
            HBFunctions.hb_list_add(this.Ptr, item);
        }

        /// <summary>
        /// Inserts an item into the list.
        /// </summary>
        /// <param name="position">The index to insert the item at.</param>
        /// <param name="item">The item to insert.</param>
        public void Insert(int position, IntPtr item)
        {
            HBFunctions.hb_list_insert(this.Ptr, position, item);
        }

        /// <summary>
        /// Removes an item from the list.
        /// </summary>
        /// <param name="item">The item to remove.</param>
        public void Remove(IntPtr item)
        {
            HBFunctions.hb_list_rem(this.Ptr, item);
        }

        /// <summary>
        /// Gets an item out of the list.
        /// </summary>
        /// <param name="i">Index in the list.</param>
        /// <returns>The item at that index in the list.</returns>
        public IntPtr this[int i]
        {
            get
            {
                return HBFunctions.hb_list_item(this.Ptr, i);
            }
        }

        /// <summary>
        /// Disposes resources associated with this object.
        /// </summary>
        public void Dispose()
        {
            IntPtr listPtrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            Marshal.WriteIntPtr(listPtrPtr, this.Ptr);
            HBFunctions.hb_list_close(listPtrPtr);
            Marshal.FreeHGlobal(listPtrPtr);
        }

        /// <summary>
        /// Creates a new list in unmanaged memory.
        /// </summary>
        /// <returns>The created list.</returns>
        public static NativeList CreateList()
        {
            return new NativeList(HBFunctions.hb_list_init());
        }
    }
}
