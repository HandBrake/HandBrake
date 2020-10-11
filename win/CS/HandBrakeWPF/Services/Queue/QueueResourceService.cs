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

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;

    public class QueueResourceService
    {
        private readonly IUserSettingService userSettingService;

        private readonly object lockOjb = new object();

        private readonly HashSet<Guid> qsvInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> nvencInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> vceInstances = new HashSet<Guid>();

        private List<int> qsvGpus = new List<int>();
        private int intelGpuCounter = -1; // Always default to the first card when allocating. 

        public QueueResourceService(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
        }

        public void Init()
        {
            this.qsvGpus = HandBrakeEncoderHelpers.GetQsvAdaptorList();
        }

        public Guid? GetHardwareLock(EncodeTask task)
        {
            lock (this.lockOjb)
            {
                switch (task.VideoEncoder)
                {
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                    case VideoEncoder.QuickSyncH26510b:
                        if (this.qsvInstances.Count < 3)
                        {
                            this.AllocateIntelGPU(task);

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

        public void AllocateIntelGPU(EncodeTask task)
        {
            // Validation checks. 
            if (task.VideoEncoder != VideoEncoder.QuickSync && task.VideoEncoder != VideoEncoder.QuickSyncH265 && task.VideoEncoder != VideoEncoder.QuickSyncH26510b)
            {
                return; // Not a QSV job.
            }

            if (this.qsvGpus.Count <= 1)
            {
                return; // Not a multi-Intel-GPU system.
            }

            if (task.ExtraAdvancedArguments.Contains("gpu"))
            {
                return; // Users choice
            }

            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled))
            {
                return; // Multi-Process encoding is disabled.
            }

            if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes) <= 1)
            {
                return; // Multi-Process encoding is disabled.
            }

            if (this.qsvInstances.Count == 0)
            {
                // Reset to -1 if we have no encoders currently in use.
                this.intelGpuCounter = -1; 
            }
            
            this.intelGpuCounter = this.intelGpuCounter + 1;
            int modulus = this.intelGpuCounter % 2;

            // For now, it's not expected we'll see users with more than 2 Intel GPUs. Typically 1 CPU, 1 Discrete will likely be the norm.
            // Use the modulus of the above counter to flip between the 2 Intel encoders. 
            // We don't set GPU for the jobs  1, 3, 5, 7, 9 .... etc  (Default to first)
            // We do set the GPU for jobs 2, 4, 5, 8, 10 .... etc 
            if (modulus == 1)
            {
                task.ExtraAdvancedArguments = string.IsNullOrEmpty(task.ExtraAdvancedArguments)
                                                  ? string.Format("gpu={0}", this.qsvGpus[1])
                                                  : string.Format("{0}:gpu={1}", task.ExtraAdvancedArguments, this.qsvGpus[1]);
            }
        }
    }
}
