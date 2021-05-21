﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;

    using VideoEncoder = HandBrakeWPF.Model.Video.VideoEncoder;

    public class QueueResourceService
    {
        private readonly IUserSettingService userSettingService;

        private readonly object lockObj = new object();

        private readonly HashSet<Guid> qsvInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> nvencInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> vceInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> mfInstances = new HashSet<Guid>();
        private readonly HashSet<Guid> totalInstances = new HashSet<Guid>();

        private List<int> qsvGpus = new List<int>();
        private int intelGpuCounter = -1; // Always default to the first card when allocating. 

        private int maxAllowedInstances;
        private int totalQsvInstances;
        private int totalVceInstances;
        private int totalNvidiaInstances;
        private int totalMfInstances; 

        public QueueResourceService(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
        }

        public int TotalActiveInstances
        {
            get
            {
                lock (this.lockObj)
                {
                    return this.totalInstances.Count;
                }
            }
        }

        public void Init()
        {
            this.maxAllowedInstances = this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);

            // Allow QSV adapter scaling. 
            this.qsvGpus = HandBrakeEncoderHelpers.GetQsvAdaptorList();
            this.totalQsvInstances = this.qsvGpus.Count * 2; // Allow two instances per GPU

            // Most Nvidia cards support 3 instances.
            this.totalNvidiaInstances = 3;

            // VCE Support still TBD
            this.totalVceInstances = 3;

            this.totalMfInstances = 1;

            // Whether using hardware or not, some CPU is needed so don't allow more jobs than CPU.
            if (this.maxAllowedInstances > Utilities.SystemInfo.GetCpuCoreCount)
            {
                this.maxAllowedInstances = Utilities.SystemInfo.GetCpuCoreCount;
            }
        }

        public Guid? GetToken(EncodeTask task)
        {
            lock (this.lockObj)
            {
                switch (task.VideoEncoder)
                {
                    case VideoEncoder.QuickSync:
                    case VideoEncoder.QuickSyncH265:
                    case VideoEncoder.QuickSyncH26510b:
                        if (this.qsvInstances.Count < this.totalQsvInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                        {
                            this.AllocateIntelGPU(task);

                            Guid guid = Guid.NewGuid();
                            this.qsvInstances.Add(guid);
                            this.totalInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty; // Busy
                        }

                    case VideoEncoder.NvencH264:
                    case VideoEncoder.NvencH265:
                        if (this.nvencInstances.Count < this.totalNvidiaInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                        {
                            Guid guid = Guid.NewGuid();
                            this.nvencInstances.Add(guid);
                            this.totalInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty; // Busy
                        }

                    case VideoEncoder.VceH264:
                    case VideoEncoder.VceH265:
                        if (this.vceInstances.Count < this.totalVceInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                        {
                            Guid guid = Guid.NewGuid();
                            this.vceInstances.Add(guid);
                            this.totalInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty; // Busy
                        }

                    case VideoEncoder.MFH264:
                    case VideoEncoder.MFH265:
                        if (this.mfInstances.Count < this.totalMfInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                        {
                            Guid guid = Guid.NewGuid();
                            this.mfInstances.Add(guid);
                            this.totalInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty; // Busy
                        }


                    default:
                        if (this.TotalActiveInstances <= this.maxAllowedInstances)
                        {
                            Guid guid = Guid.NewGuid();
                            this.totalInstances.Add(guid);
                            return guid;
                        }
                        else
                        {
                            return Guid.Empty; // Busy
                        }
                }
            }
        }

        public void ReleaseToken(VideoEncoder encoder, Guid? unlockKey)
        {
            if (unlockKey == null)
            {
                return;
            }

            lock (this.lockObj)
            {
                if (this.totalInstances.Contains(unlockKey.Value))
                {
                    this.totalInstances.Remove(unlockKey.Value);
                }

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
                    case VideoEncoder.MFH264:
                    case VideoEncoder.MFH265:
                        if (this.mfInstances.Contains(unlockKey.Value))
                        {
                            this.mfInstances.Remove(unlockKey.Value);
                        }

                        break;
                }
            }
        }

        private void AllocateIntelGPU(EncodeTask task)
        {
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
