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
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;

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
            this.userSettingService.SettingChanged += this.UserSettingService_SettingChanged;
        }

        private void UserSettingService_SettingChanged(object sender, HandBrakeWPF.EventArgs.SettingChangedEventArgs e)
        {
            this.Init();
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

            // Whether using hardware or not, some CPU is needed so don't allow more jobs than CPU.
            if (this.maxAllowedInstances > Utilities.SystemInfo.MaximumSimultaneousInstancesSupported)
            {
                this.maxAllowedInstances = Utilities.SystemInfo.MaximumSimultaneousInstancesSupported;
            }

            // Allow QSV adapter scaling. 
            this.qsvGpus = HandBrakeEncoderHelpers.GetQsvAdaptorList();
            this.totalQsvInstances = this.qsvGpus.Count * 2; // Allow two instances per GPU

            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncHyperEncode))
            {
                // When HyperEncode is supported, we encode 1 job across multiple media engines. 
                this.totalQsvInstances = 1;
            }

            if (this.maxAllowedInstances == 1)
            {
                this.totalQsvInstances = 1;
                this.totalNvidiaInstances = 1;
                this.totalVceInstances = 1;
                this.totalMfInstances = 1;
                return;
            } 

            // NVEnc Support - (Most cards now support 5 with up-to-date drivers)
            this.totalNvidiaInstances = 5;

            // VCN Support
            this.totalVceInstances = 3;

            // ARM64 Support
            this.totalMfInstances = 1;
        }

        public Guid? GetToken(EncodeTask task)
        {
            lock (this.lockObj)
            {
                if (task.VideoEncoder.IsQuickSync)
                {
                    if (this.qsvInstances.Count < this.totalQsvInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                    {
                        this.AllocateIntelGPU(task);

                        Guid guid = Guid.NewGuid();
                        this.qsvInstances.Add(guid);
                        this.totalInstances.Add(guid);
                        return guid;
                    }

                    return Guid.Empty; // Busy
                }
                else if (task.VideoEncoder.IsNVEnc)
                {
                    if (this.nvencInstances.Count < this.totalNvidiaInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                    {
                        Guid guid = Guid.NewGuid();
                        this.nvencInstances.Add(guid);
                        this.totalInstances.Add(guid);
                        return guid;
                    }

                    return Guid.Empty; // Busy
                }
                else if (task.VideoEncoder.IsVCN)
                {
                    if (this.vceInstances.Count < this.totalVceInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                    {
                        Guid guid = Guid.NewGuid();
                        this.vceInstances.Add(guid);
                        this.totalInstances.Add(guid);
                        return guid;
                    }

                    return Guid.Empty; // Busy
                }
                else if (task.VideoEncoder.IsMediaFoundation)
                {
                    if (this.mfInstances.Count < this.totalMfInstances && this.TotalActiveInstances <= this.maxAllowedInstances)
                    {
                        Guid guid = Guid.NewGuid();
                        this.mfInstances.Add(guid);
                        this.totalInstances.Add(guid);
                        return guid;
                    }

                    return Guid.Empty; // Busy
                }
                else
                {
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

        public void ClearTokens()
        {
            lock (this.lockObj)
            {
                qsvInstances.Clear();
                nvencInstances.Clear();
                vceInstances.Clear();
                mfInstances.Clear();
                this.totalInstances.Clear();
            }
        }

        public void ReleaseToken(HBVideoEncoder encoder, Guid? unlockKey)
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

                if (encoder.IsQuickSync)
                {
                    if (this.qsvInstances.Contains(unlockKey.Value))
                    {
                        this.qsvInstances.Remove(unlockKey.Value);
                    }
                }
                else if (encoder.IsNVEnc)
                {
                    if (this.nvencInstances.Contains(unlockKey.Value))
                    {
                        this.nvencInstances.Remove(unlockKey.Value);
                    }
                }
                else if (encoder.IsVCN)
                {
                    if (this.vceInstances.Contains(unlockKey.Value))
                    {
                        this.vceInstances.Remove(unlockKey.Value);
                    }
                }
                else if (encoder.IsMediaFoundation)
                {
                    if (this.mfInstances.Contains(unlockKey.Value))
                    {
                        this.mfInstances.Remove(unlockKey.Value);
                    }
                }
            }
        }

        private void AllocateIntelGPU(EncodeTask task)
        {
            if (this.qsvGpus.Count <= 1)
            {
                return; // Not a multi-Intel-GPU system.
            }

            // HyperEncode takes priority over load balancing when enabled. 
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncHyperEncode))
            {
                task.ExtraAdvancedArguments = string.IsNullOrEmpty(task.ExtraAdvancedArguments)
                                                  ? "hyperencode=adaptive" : string.Format("hyperencode=adaptive:{0}", task.ExtraAdvancedArguments);
                return;
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
            // We set GPU for the jobs  1, 3, 5, 7, 9 to index 0
            // We set the GPU for jobs 2, 4, 5, 8, 10  to index 1 
            if (modulus == 1)
            {
                // GPU List 1
                task.ExtraAdvancedArguments = string.IsNullOrEmpty(task.ExtraAdvancedArguments)
                                                  ? string.Format("gpu={0}", this.qsvGpus[1])
                                                  : string.Format("gpu={1}:{0}", task.ExtraAdvancedArguments, this.qsvGpus[1]);
            }
            else
            {
                // GPU List 0
                task.ExtraAdvancedArguments = string.IsNullOrEmpty(task.ExtraAdvancedArguments)
                    ? string.Format("gpu={0}", this.qsvGpus[0])
                    : string.Format("gpu={1}:{0}", task.ExtraAdvancedArguments, this.qsvGpus[0]);
            }
        }
    }
}
