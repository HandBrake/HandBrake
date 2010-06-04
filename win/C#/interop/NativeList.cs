namespace HandBrake.Interop
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Represents a HandBrake style native list.
    /// </summary>
    public class NativeList
    {
        /// <summary>
        /// The list of native memory locations allocated for this list.
        /// </summary>
        private List<IntPtr> allocatedMemory = new List<IntPtr>();

        /// <summary>
        /// Gets or sets the pointer to the native list.
        /// </summary>
        public IntPtr ListPtr { get; set; }

        /// <summary>
        /// Gets the list of native memory locations allocated for this list.
        /// </summary>
        public List<IntPtr> AllocatedMemory
        {
            get
            {
                return allocatedMemory;
            }
        }
    }
}
