/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2020 NVIDIA Corporation.
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
 */

#ifndef __CTK_MANAGE_GRID_LICENSE_H__
#define __CTK_MANAGE_GRID_LICENSE_H__

G_BEGIN_DECLS

#define CTK_TYPE_MANAGE_GRID_LICENSE (ctk_manage_grid_license_get_type())

#define CTK_MANAGE_GRID_LICENSE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MANAGE_GRID_LICENSE, CtkManageGridLicense))

#define CTK_MANAGE_GRID_LICENSE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MANAGE_GRID_LICENSE, CtkManageGridLicenseClass))

#define CTK_IS_MANAGE_GRID_LICENSE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MANAGE_GRID_LICENSE))

#define CTK_IS_MANAGE_GRID_LICENSE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MANAGE_GRID_LICENSE))

#define CTK_MANAGE_GRID_LICENSE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MANAGE_GRID_LICENSE, CtkMANAGE_GRID_LICENSEClass))

#define GRID_LICENSE_INFO_MAX_LENGTH        128
#define GRID_MESSAGE_MAX_BUFFER_SIZE        512
#define GRID_VIRTUAL_APPLICATIONS           "GRID Virtual Applications"

typedef struct _CtkManageGridLicense       CtkManageGridLicense;
typedef struct _CtkManageGridLicenseClass  CtkManageGridLicenseClass;

typedef struct _DbusData DbusData;

struct _CtkManageGridLicense
{
    GtkVBox parent;

    CtkConfig *ctk_config;

    GtkWidget* txt_secondary_server_port;
    GtkWidget* txt_secondary_server_address;
    GtkWidget* txt_server_port;
    GtkWidget* txt_server_address;
    GtkWidget* label_license_state;
    GtkWidget* btn_apply;
    GtkWidget* radio_btn_vapp;
    GtkWidget* radio_btn_qdws;
    GtkWidget* radio_btn_vcompute;
    GtkWidget* btn_cancel;
    GtkWidget* box_server_info;

    DbusData *dbusData;
    CtrlTarget *target;

    gint license_edition_state;
    gint feature_type;                                              // Feature type from UI/gridd.conf.
    int gridd_feature_type;                                         // Feature type fetched from nvidia-gridd.
    char productName[GRID_LICENSE_INFO_MAX_LENGTH];                 // GRID product name fetched from nvml corresponding to the licensed/applied feature
    char productNameQvDWS[GRID_LICENSE_INFO_MAX_LENGTH];            // GRID product name fetched from nvml corresponding to the feature type '2'
    char productNamevCompute[GRID_LICENSE_INFO_MAX_LENGTH];         // GRID product name fetched from nvml corresponding to the feature type '4'

    int licenseStatus;                                              // Current license status to be displayed on UI
    gboolean isvComputeSupported;                                   // Check if 'NVIDIA Virtual Compute Server' feature is supported
    gboolean isQuadroSupported;                                     // Check if 'Quadro Virtual Data Center Workstation' feature is supported
};

/*
 * Status related to GRID licensing
 */
typedef enum
{
    NV_GRID_UNLICENSED_VGPU = 0,
    NV_GRID_UNLICENSED_VAPP,
    NV_GRID_LICENSE_STATUS_ACQUIRED,
    NV_GRID_LICENSE_STATUS_REQUESTING,
    NV_GRID_LICENSE_STATUS_FAILED,
    NV_GRID_LICENSE_STATUS_EXPIRED,
    NV_GRID_LICENSE_RESTART_REQUIRED_VAPP,
    NV_GRID_LICENSE_RESTART_REQUIRED_QDWS,
    NV_GRID_LICENSE_RESTART_REQUIRED_VCOMPUTE,
    NV_GRID_LICENSED_RESTART_REQUIRED_VAPP,
    NV_GRID_LICENSED_RESTART_REQUIRED_QDWS,
    NV_GRID_LICENSED_RESTART_REQUIRED_VCOMPUTE,
    NV_GRID_UNLICENSED_REQUEST_DETAILS_QDWS,
    NV_GRID_UNLICENSED_REQUEST_DETAILS_VCOMPUTE,
} licenseStatusList;

struct _CtkManageGridLicenseClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_manage_grid_license_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_manage_grid_license_new         (CtrlTarget *, CtkConfig *);
GtkTextBuffer* ctk_manage_grid_license_create_help (GtkTextTagTable *,
                                               CtkManageGridLicense *);

void           ctk_manage_grid_license_start_timer (GtkWidget *);
void           ctk_manage_grid_license_stop_timer  (GtkWidget *);

G_END_DECLS

#endif /* __CTK_MANAGE_GRID_LICENSE_H__ */
