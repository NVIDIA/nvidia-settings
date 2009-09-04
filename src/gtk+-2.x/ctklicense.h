/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

#ifndef __CTK_LICENSE_DIALOG_H__
#define __CTK_LICENSE_DIALOG_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_LICENSE_DIALOG (ctk_license_dialog_get_type())

#define CTK_LICENSE_DIALOG(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LICENSE_DIALOG, \
                                 CtkLicenseDialog))

#define CTK_LICENSE_DIALOG_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LICENSE_DIALOG, \
                              CtkLicenseDialogClass))

#define CTK_IS_LICENSE_DIALOG(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LICENSE_DIALOG))

#define CTK_IS_LICENSE_DIALOG_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LICENSE_DIALOG))

#define CTK_LICENSE_DIALOG_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LICENSE_DIALOG, \
                                CtkLicenseDialogClass))

typedef struct _CtkLicenseDialog       CtkLicenseDialog;
typedef struct _CtkLicenseDialogClass  CtkLicenseDialogClass;

struct _CtkLicenseDialog
{
    GtkVBox parent;
    
    GtkWidget *window;
    GtkWidget *dialog;
};

struct _CtkLicenseDialogClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_license_dialog_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_license_dialog_new       (GtkWidget *object, gchar *panel_name);
gint ctk_license_run_dialog(CtkLicenseDialog *ctk_license_dialog);

G_END_DECLS

#endif /* __CTK_LICENSE_DIALOG_H__ */
