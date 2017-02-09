/*
 * Copyright (C) 2016 NVIDIA Corporation All rights reserved.
 * All information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * nv_grid_dbus.h
 */

#ifndef _NVIDIA_NV_GRID_DBUS_H_
#define _NVIDIA_NV_GRID_DBUS_H_

/*
 * Following are the details if client needs to open a connection and
 * communicate with nvidia-gridd to query the license state
 */
#define NV_GRID_DBUS_CLIENT     "nvidia.grid.client"
#define NV_GRID_DBUS_TARGET     "nvidia.grid.server"
#define NV_GRID_DBUS_OBJECT     "/nvidia/grid/license"
#define NV_GRID_DBUS_INTERFACE  "nvidia.grid.license"
#define NV_GRID_DBUS_METHOD     "GridLicenseState"

/*
 * List of grid license states
 */
typedef enum
{
    NV_GRID_UNLICENSED = 0,                 // Your system does not have a valid GPU license. Enter license server details and apply.
    NV_GRID_LICENSE_ACQUIRED_VGPU,          // Your system is licensed for GRID vGPU.
    NV_GRID_LICENSE_ACQUIRED_GVW,           // Your system is licensed for GRID Virtual Workstation Edition.
    NV_GRID_LICENSE_REQUESTING_VGPU,        // Acquiring license for GRID vGPU Edition. Your system does not have a valid GRID vGPU license.
    NV_GRID_LICENSE_REQUESTING_GVW,         // Acquiring license for GRID Virtual Workstation Edition. Your system does not have a valid GRID Virtual Workstation license.
    NV_GRID_LICENSE_FAILED_VGPU,            // Failed to acquire NVIDIA vGPU license.
    NV_GRID_LICENSE_FAILED_GVW,             // Failed to acquire NVIDIA GRID Virtual Workstation license.
    NV_GRID_LICENSE_EXPIRED_VGPU,           // Failed to renew license for GRID vGPU Edition. Your system does not have a valid GRID vGPU license.
    NV_GRID_LICENSE_EXPIRED_GVW,            // Failed to renew license for GRID Virtual Workstation Edition. Your system is currently running GRID Virtual Workstation (unlicensed).
    NV_GRID_LICENSE_RESTART_REQUIRED,       // Restart your system for Tesla Edition. Your system is currently running GRID Virtual Workstation Edition.
} gridLicenseStatus;

#endif // _NVIDIA_NV_GRID_DBUS_H_
