// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InteropUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper utilities for native interop.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;
	using System.Linq;
	using System.Runtime.InteropServices;

	using HandBrake.Interop.HbLib;
	using Model.Encoding;

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
		public static List<T> ToList<T>(this IntPtr listPtr)
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
		/// Converts the HB list to a managed list of pointers.
		/// </summary>
		/// <param name="listPtr">The list to convert.</param>
		/// <returns>The managed list of pointers.</returns>
		public static List<IntPtr> ToIntPtrList(this IntPtr listPtr)
		{
			var returnList = new List<IntPtr>();
			hb_list_s itemList = ReadStructure<hb_list_s>(listPtr);

			for (int i = 0; i < itemList.items_count; i++)
			{
				IntPtr itemPtr = Marshal.ReadIntPtr(itemList.items, i * Marshal.SizeOf(typeof(IntPtr)));
				returnList.Add(itemPtr);
			}

			return returnList;
		}

		/// <summary>
		/// Converts the given native array to a managed collection.
		/// </summary>
		/// <typeparam name="T">The type of item in the list.</typeparam>
		/// <param name="arrayPtr">The pointer to the array.</param>
		/// <param name="count">The number of items in the array.</param>
		/// <returns>The converted collection.</returns>
		public static IEnumerable<T> ConvertArray<T>(IntPtr arrayPtr, int count)
		{
			IntPtr currentItem = arrayPtr;

			var result = new List<T>();
			for (int i = 0; i < count; i++)
			{
				T nativeEncoder = ReadStructure<T>(currentItem);
				result.Add(nativeEncoder);

				currentItem = IntPtr.Add(currentItem, Marshal.SizeOf(typeof(T)));
			}

			return result;
		}

		/// <summary>
		/// Creats a new, empty native HandBrake list.
		/// </summary>
		/// <param name="capacity">The capacity of the new list.</param>
		/// <returns>The new native list.</returns>
		public static NativeList CreateNativeList(int capacity)
		{
			NativeList returnList = new NativeList();
			int intSize = Marshal.SizeOf(typeof(IntPtr));

			IntPtr nativeListInternal = Marshal.AllocHGlobal(capacity * intSize);
			returnList.AllocatedMemory.Add(nativeListInternal);

			hb_list_s nativeListStruct = new hb_list_s { items = nativeListInternal, items_alloc = capacity, items_count = 0 };

		    IntPtr nativeListStructPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_list_s)));
			Marshal.StructureToPtr(nativeListStruct, nativeListStructPtr, false);

			returnList.ListPtr = nativeListStructPtr;
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

			hb_list_s nativeListStruct = new hb_list_s
			                                 {
			                                     items = nativeListInternal,
			                                     items_alloc = list.Count,
			                                     items_count = list.Count
			                                 };

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

			hb_list_s nativeListStruct = new hb_list_s
			                                 {
			                                     items = nativeListInternal,
			                                     items_alloc = list.Count,
			                                     items_count = list.Count
			                                 };

		    IntPtr nativeListStructPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_list_s)));
			Marshal.StructureToPtr(nativeListStruct, nativeListStructPtr, false);

			returnList.ListPtr = nativeListStructPtr;
			return returnList;
		}

		/// <summary>
		/// Reads in a list of objects given an interator and a conversion function.
		/// </summary>
		/// <typeparam name="T1">The type of the struct given by the iterator.</typeparam>
		/// <typeparam name="T2">The object type to convert to.</typeparam>
		/// <param name="iterator">The iterator to use to build the list.</param>
		/// <param name="converter">The converter to convert from the struct to the object.</param>
		/// <returns>The list of objects.</returns>
		public static List<T2> GetListFromIterator<T1, T2>(Func<IntPtr, IntPtr> iterator, Func<T1, T2> converter)
		{
			return ReadStructureListFromIterator<T1>(iterator).Select(converter).ToList();
		} 

		/// <summary>
		/// Reads in a list of structs given an iterator.
		/// </summary>
		/// <typeparam name="T">The type of the struct.</typeparam>
		/// <param name="iterator">The iterator to use to build the list.</param>
		/// <returns>The list of structs.</returns>
		public static List<T> ReadStructureListFromIterator<T>(Func<IntPtr, IntPtr> iterator)
		{
			var structureList = new List<T>();
			IntPtr current = IntPtr.Zero;

			current = iterator(current);
			while (current != IntPtr.Zero)
			{
				T encoder = ReadStructure<T>(current);
				structureList.Add(encoder);

				current = iterator(current);
			}

			return structureList;
		}

		/// <summary>
		/// Closes the given job.
		/// </summary>
		/// <param name="nativeJobPtr">The pointer to the job.</param>
		public static void CloseJob(IntPtr nativeJobPtr)
		{
			// Create a point to the job pointer first.
			IntPtr nativeJobPtrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));

			// Assign the new pointer to the job pointer and tell HB to clean the job up.
			Marshal.WriteIntPtr(nativeJobPtrPtr, nativeJobPtr);
			HBFunctions.hb_job_close(nativeJobPtrPtr);

			// Free the pointer we used.
			Marshal.FreeHGlobal(nativeJobPtrPtr);
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
