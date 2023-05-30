/*
 * Copyright (C) 2017-2023 NVIDIA Corporation.
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
 * Details to communicate with vGPU licensing daemon using dbus mechanism
 */
#define NV_GRID_DBUS_CLIENT             "nvidia.grid.client"
#define NV_GRID_DBUS_TARGET             "nvidia.grid.server"
#define NV_GRID_DBUS_OBJECT             "/nvidia/grid/license"
#define NV_GRID_DBUS_INTERFACE          "nvidia.grid.license"
#define NV_GRID_DBUS_METHOD             "GridLicenseState"
#define LICENSE_DETAILS_UPDATE_SUCCESS  0
#define LICENSE_STATE_REQUEST           1
#define LICENSE_DETAILS_UPDATE_REQUEST  2
#define LICENSE_FEATURE_TYPE_REQUEST    3
#define LICENSE_SERVER_PORT_REQUEST     4
#define PRIMARY_SERVER_ADDRESS          "PrimaryServerAddress"
#define SECONDARY_SERVER_ADDRESS        "SecondaryServerAddress"
#define SERVER_DETAILS_NOT_CONFIGURED   "Not Configured"

/*
 * vGPU software license states
 */
typedef enum
{
    NV_GRID_UNLICENSED = 0,
    NV_GRID_LICENSE_REQUESTING,
    NV_GRID_LICENSE_FAILED,
    NV_GRID_LICENSED,
    NV_GRID_LICENSE_RENEWING,
    NV_GRID_LICENSE_RENEW_FAILED,
    NV_GRID_LICENSE_EXPIRED,
} gridLicenseState;

/*
 * vGPU software license feature types
 */
typedef enum
{
    NV_GRID_LICENSE_FEATURE_TYPE_VAPP = 0,
    NV_GRID_LICENSE_FEATURE_TYPE_VGPU,
    NV_GRID_LICENSE_FEATURE_TYPE_VWS,
    NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE = 4,
} gridLicenseFeatureType;

#endif // _NVIDIA_NV_GRID_DBUS_H_
