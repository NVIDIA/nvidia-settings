/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
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
 */

#ifndef __CTK_MANAGE_GRID_LICENSE_H__
#define __CTK_MANAGE_GRID_LICENSE_H__

// Licensed feature types
#define GRID_LICENSED_FEATURE_TYPE_TESLA                                 0
#define GRID_LICENSED_FEATURE_TYPE_VGPU                                  1
#define GRID_LICENSED_FEATURE_TYPE_GVW                                   2

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
    GtkWidget* btn_cancel;
    GtkWidget* box_server_info;

    DbusData *dbusData;
    gint license_edition_state;
    gint feature_type;
    int license_status;
    int license_feature_type;
};

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
