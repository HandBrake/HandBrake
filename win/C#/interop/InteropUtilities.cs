namespace HandBrake.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Helper utilities for native interop.
    /// </summary>
    public static class InteropUtilities
    {
        /// <summary>
        /// Reads the given native structure pointer.
        /// </summary>
        /// <typeparam name="T">The type to convert the structure to.</typeparam>
        /// <param name="structPtr">The pointer to the native structure.</param>
        /// <returns>The converted structure.</returns>
        public static T ReadStructure<T>(IntPtr structPtr)
        {
            return (T)Marshal.PtrToStructure(structPtr, typeof(T));
        }

        /// <summary>
        /// Converts the given native HandBrake list to a managed list.
        /// </summary>
        /// <typeparam name="T">The type of structure in the list.</typeparam>
        /// <param name="listPtr">The pointer to the native list.</param>
        /// <returns>The converted managed list.</returns>
        public static List<T> ConvertList<T>(IntPtr listPtr)
        {
            List<T> returnList = new List<T>();
            hb_list_s itemList = ReadStructure<hb_list_s>(listPtr);

            for (int i = 0; i < itemList.items_count; i++)
            {
                IntPtr itemPtr = Marshal.ReadIntPtr(itemList.items, i * Marshal.SizeOf(typeof(IntPtr)));
                returnList.Add(ReadStructure<T>(itemPtr));
            }

            return returnList;
        }

        /// <summary>
        /// Creates a native HandBrake list from the given managed list of pointers.
        /// </summary>
        /// <param name="list">The managed list to convert.</param>
        /// <returns>The converted native list.</returns>
        public static NativeList CreateIntPtrList(List<IntPtr> list)
        {
            NativeList returnList = new NativeList();
            int intSize = Marshal.SizeOf(typeof(IntPtr));

            IntPtr nativeListInternal = Marshal.AllocHGlobal(list.Count * intSize);
            returnList.AllocatedMemory.Add(nativeListInternal);
            for (int i = 0; i < list.Count; i++)
            {
                Marshal.WriteIntPtr(nativeListInternal, i * intSize, list[i]);
            }

            hb_list_s nativeListStruct = new hb_list_s();
            nativeListStruct.items = nativeListInternal;
            nativeListStruct.items_alloc = list.Count;
            nativeListStruct.items_count = list.Count;

            IntPtr nativeListStructPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_list_s)));
            Marshal.StructureToPtr(nativeListStruct, nativeListStructPtr, false);

            returnList.ListPtr = nativeListStructPtr;
            return returnList;
        }

        /// <summary>
        /// Creates a native HandBrake list from the given managed list of structures.
        /// </summary>
        /// <typeparam name="T">The type of structures in the list.</typeparam>
        /// <param name="list">The managed list to convert.</param>
        /// <returns>The converted native list.</returns>
        public static NativeList ConvertListBack<T>(List<T> list)
        {
            NativeList returnList = new NativeList();
            int intSize = Marshal.SizeOf(typeof(IntPtr));

            IntPtr nativeListInternal = Marshal.AllocHGlobal(list.Count * intSize);
            returnList.AllocatedMemory.Add(nativeListInternal);
            for (int i = 0; i < list.Count; i++)
            {
                IntPtr itemPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(T)));
                returnList.AllocatedMemory.Add(itemPtr);
                Marshal.StructureToPtr(list[i], itemPtr, false);

                Marshal.WriteIntPtr(nativeListInternal, i * intSize, itemPtr);
            }

            hb_list_s nativeListStruct = new hb_list_s();
            nativeListStruct.items = nativeListInternal;
            nativeListStruct.items_alloc = list.Count;
            nativeListStruct.items_count = list.Count;

            IntPtr nativeListStructPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_list_s)));
            Marshal.StructureToPtr(nativeListStruct, nativeListStructPtr, false);

            returnList.ListPtr = nativeListStructPtr;
            return returnList;
        }

        /// <summary>
        /// Frees all the memory locations in the given list.
        /// </summary>
        /// <param name="memoryList">The list of memory locations to free.</param>
        public static void FreeMemory(List<IntPtr> memoryList)
        {
            foreach (IntPtr memoryLocation in memoryList)
            {
                Marshal.FreeHGlobal(memoryLocation);
            }
        }
    }
}
