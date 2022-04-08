// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NumaInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    public class NumaInfo
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct PROCESSORCORE
        {
            public byte Flags;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct NUMANODE
        {
            public uint NodeNumber;
        }

        public enum PROCESSOR_CACHE_TYPE
        {
            CacheUnified,
            CacheInstruction,
            CacheData,
            CacheTrace
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CACHE_DESCRIPTOR
        {
            public byte Level;
            public byte Associativity;
            public ushort LineSize;
            public uint Size;
            public PROCESSOR_CACHE_TYPE Type;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct SYSTEM_LOGICAL_PROCESSOR_INFORMATION_UNION
        {
            [FieldOffset(0)]
            public PROCESSORCORE ProcessorCore;
            [FieldOffset(0)]
            public NUMANODE NumaNode;
            [FieldOffset(0)]
            public CACHE_DESCRIPTOR Cache;
            [FieldOffset(0)]
            private UInt64 Reserved1;
            [FieldOffset(8)]
            private UInt64 Reserved2;
        }

        public enum LOGICAL_PROCESSOR_RELATIONSHIP
        {
            RelationProcessorCore,
            RelationNumaNode,
            RelationCache,
            RelationProcessorPackage,
            RelationGroup,
            RelationAll = 0xffff
        }

        public struct SYSTEM_LOGICAL_PROCESSOR_INFORMATION
        {
            public UIntPtr ProcessorMask;
            public LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
            public SYSTEM_LOGICAL_PROCESSOR_INFORMATION_UNION ProcessorInformation;
        }

        [DllImport(@"kernel32.dll", SetLastError = true)]
        public static extern bool GetLogicalProcessorInformation(IntPtr Buffer, ref uint ReturnLength);

        public static int GetNumaNodes()
        {
            int numaNodes = 0;
            uint returnLength = 0;

            GetLogicalProcessorInformation(IntPtr.Zero, ref returnLength);

            IntPtr Ptr = Marshal.AllocHGlobal((int)returnLength);
            try
            {
                if (GetLogicalProcessorInformation(Ptr, ref returnLength))
                {
                    int size = Marshal.SizeOf(typeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
                    int len = (int)returnLength / size;
                    IntPtr item = Ptr;
                    for (int i = 0; i < len; i++)
                    {
                        SYSTEM_LOGICAL_PROCESSOR_INFORMATION record =
                            (SYSTEM_LOGICAL_PROCESSOR_INFORMATION)Marshal.PtrToStructure(
                                item,
                                typeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
                        if (record.Relationship == LOGICAL_PROCESSOR_RELATIONSHIP.RelationNumaNode)
                        {
                            numaNodes += 1;
                        }

                        item += size;
                    }

                    return numaNodes;
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Failed to find NUMA nodes: " + ex);
            }
            finally
            {
                Marshal.FreeHGlobal(Ptr);
            }

            return 0;
        }
    }
}
