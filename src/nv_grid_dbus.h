/*
 * Copyright (C) 2017 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * nv_grid_dbus.h
 */

#ifndef _NVIDIA_NV_GRID_DBUS_H_
#define _NVIDIA_NV_GRID_DBUS_H_

/*
 * Details to communicate with nvidia-gridd using dbus mechanism
 */
#define NV_GRID_DBUS_CLIENT             "nvidia.grid.client"
#define NV_GRID_DBUS_TARGET             "nvidia.grid.server"
#define NV_GRID_DBUS_OBJECT             "/nvidia/grid/license"
#define NV_GRID_DBUS_INTERFACE          "nvidia.grid.license"
#define NV_GRID_DBUS_METHOD             "GridLicenseState"
#define LICENSE_STATE_REQUEST           1
#define LICENSE_DETAILS_UPDATE_REQUEST  2
#define LICENSE_DETAILS_UPDATE_SUCCESS  0

/*
 * List of grid license states
 */
typedef enum
{
    NV_GRID_UNLICENSED = 0,                 // Your system does not have a valid GPU license. Enter license server details and apply.
    NV_GRID_UNLICENSED_VGPU,                // Your system does not have a valid vGPU license. Enter license server details and apply.
    NV_GRID_UNLICENSED_TESLA,               // Your system is currently running on Tesla (unlicensed).
    NV_GRID_UNLICENSED_GVW_SELECTED,        // Your system is currently running on Tesla (unlicensed). Enter license server details and apply.
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
