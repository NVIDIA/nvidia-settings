/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
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

#ifndef __CTK_SERVER_H__
#define __CTK_SERVER_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_SERVER (ctk_server_get_type())

#define CTK_SERVER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SERVER, \
                                 CtkServer))

#define CTK_SERVER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SERVER, \
                              CtkServerClass))

#define CTK_IS_SERVER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SERVER))

#define CTK_IS_SERVER_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SERVER))

#define CTK_SERVER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SERVER, \
                                CtkServerClass))


typedef struct _CtkServer
{
    GtkVBox parent;

    CtkConfig *ctk_config;
    NvCtrlAttributeHandle *handle;

} CtkServer;

typedef struct _CtkServerClass
{
    GtkVBoxClass parent_class;
} CtkServerClass;


GType       ctk_server_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_server_new       (NvCtrlAttributeHandle *, CtkConfig *);

GtkTextBuffer *ctk_server_create_help(GtkTextTagTable *,
                                      CtkServer *);

G_END_DECLS

#endif /* __CTK_SERVER_H__ */
