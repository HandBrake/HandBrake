// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueResourceService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueResourceService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue
{
    using System;
    using System.Collections.Generic;

    using HandBrake.Interop.Interop.Model.Encoding;

    public class QueueResourceService
    {
        private readonly object lockOjb = new object();

        private HashSet<Guid> qsvInstances = new HashSet<Guid>();
        private HashSet<Guid> nvencInstances = new HashSet<Guid>();
        private HashSet<Guid> vceInstances = new HashSet<Guid>();

        private List<int> qsvGpus = new List<int>();
        
        public void Init()
        {
            // TODO Coming Soon!
            //   List<int> qsvGpus = hbFunctions.hb_qsv_adapters_list();
        }

        public Guid? GetHardwareLock(VideoEncoder encoder)
        {
            lock (this.lockOjb)
            {
                switch (encoder)
                {
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                    case VideoEncoder.QuickSyncH26510b:
                        if (this.qsvInstances.Count < 3)
                        {
                            Guid guid = Guid.NewGuid();
                            this.qsvInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty;
                        }

                    case VideoEncoder.NvencH264:
                    case VideoEncoder.NvencH265:
                        if (this.nvencInstances.Count < 3)
                        {
                            Guid guid = Guid.NewGuid();
                            this.nvencInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty;
                        }

                    case VideoEncoder.VceH264:
                    case VideoEncoder.VceH265:
                        if (this.vceInstances.Count < 1)
                        {
                            Guid guid = Guid.NewGuid();
                            this.vceInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty;
                        }
                }
            }

            return null;
        }

        public void UnlockHardware(VideoEncoder encoder, Guid? unlockKey)
        {
            if (unlockKey == null)
            {
                return;
            }

            lock (this.lockOjb)
            {
                switch (encoder)
                {
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                    case VideoEncoder.QuickSyncH26510b:
                        if (this.qsvInstances.Contains(unlockKey.Value))
                        {
                            this.qsvInstances.Remove(unlockKey.Value);
                        }

                        break;
                    case VideoEncoder.NvencH264:
                    case VideoEncoder.NvencH265:
                        if (this.nvencInstances.Contains(unlockKey.Value))
                        {
                            this.nvencInstances.Remove(unlockKey.Value);
                        }

                        break;

                    case VideoEncoder.VceH264:
                    case VideoEncoder.VceH265:
                        if (this.vceInstances.Contains(unlockKey.Value))
                        {
                            this.vceInstances.Remove(unlockKey.Value);
                        }

                        break;
                }
            }
        }
    }
}
