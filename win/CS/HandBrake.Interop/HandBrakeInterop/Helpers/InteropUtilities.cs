// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InteropUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper utilities for native interop.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    using HandBrake.Interop.HbLib;

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
		/// Reads the given native UTF-8 string.
		/// </summary>
		/// <param name="stringPtr">The pointer to the string.</param>
		/// <returns>The resulting string.</returns>
		public static string ReadUtf8Ptr(IntPtr stringPtr)
		{
			var data = new List<byte>();
			var ptr = stringPtr;
			var offset = 0;
			while (true)
			{
				byte ch = Marshal.ReadByte(ptr, offset++);
				if (ch == 0)
				{
					break;
				}

				data.Add(ch);
			}

			return Encoding.UTF8.GetString(data.ToArray());
		}

		/// <summary>
		/// Creates a pointer to a UTF-8 null-terminated string.
		/// </summary>
		/// <param name="str">
		/// The string to encode.
		/// </param>
		/// <returns>
		/// The <see cref="IntPtr"/>.
		/// </returns>
		public static IntPtr CreateUtf8Ptr(string str)
		{
			byte[] bytes = Encoding.UTF8.GetBytes(str);
			IntPtr stringPtr = Marshal.AllocHGlobal(bytes.Length + 1);
			var offset = 0;
			foreach (byte b in bytes)
			{
				Marshal.WriteByte(stringPtr, offset, b);
				offset++;
			}

			Marshal.WriteByte(stringPtr, offset, 0);
			return stringPtr;
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
			NativeList nativeList = new NativeList(listPtr);

			for (int i = 0; i < nativeList.Count; i++)
			{
				IntPtr itemPtr = nativeList[i];
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
			NativeList nativeList = new NativeList(listPtr);

			for (int i = 0; i < nativeList.Count; i++)
			{
				IntPtr itemPtr = nativeList[i];
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
		/// Creates a native HandBrake list from the given managed list of pointers.
		/// </summary>
		/// <param name="list">The managed list to convert.</param>
		/// <returns>The converted native list.</returns>
		public static NativeList CreateIntPtrList(List<IntPtr> list)
		{
			NativeList returnList = NativeList.CreateList();

			foreach (IntPtr ptr in list)
			{
				returnList.Add(ptr);
			}

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
			NativeList returnList = NativeList.CreateList();
			foreach (T item in list)
			{
				IntPtr itemPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(T)));
				returnList.AllocatedMemory.Add(itemPtr);
				Marshal.StructureToPtr(item, itemPtr, false);

				returnList.Add(itemPtr);
			}

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
